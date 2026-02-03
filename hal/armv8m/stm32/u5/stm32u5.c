/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32U5 basic peripherals control functions
 *
 * Copyright 2017, 2019, 2020, 2021, 2025 Phoenix Systems
 * Copyright 2026 Apator Metrix
 * Author: Aleksander Kaminski, Jan Sikorski, Hubert Buczynski, Jacek Maksymowicz, Mateusz Karcz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <board_config.h>
#include "stm32u5.h"


#define GPIOA_BASE ((void *)0x42020000)
#define GPIOB_BASE ((void *)0x42020400)
#define GPIOC_BASE ((void *)0x42020800)
#define GPIOD_BASE ((void *)0x42020c00)
#define GPIOE_BASE ((void *)0x42021000)
#define GPIOF_BASE ((void *)0x42021400)
#define GPIOG_BASE ((void *)0x42021800)
#define GPIOH_BASE ((void *)0x42021c00)
#define GPIOI_BASE ((void *)0x42022000)

#define IWDG_BASE   ((void *)0x40003000)
#define PWR_BASE    ((void *)0x46020800)
#define RCC_BASE    ((void *)0x46020c00)
#define SYSCFG_BASE ((void *)0x46000400)
#define RAMCFG_BASE ((void *)0x40026000)
#define ICB_BASE    ((void *)0xe000e000)

static struct {
	volatile u32 *rcc;
	volatile u32 *gpio[9];
	volatile u32 *icb;
	volatile u32 *pwr;
	volatile u32 *syscfg;
	volatile u32 *iwdg;
	volatile u32 *ramcfg;

	u32 cpuclk;

	u32 resetFlags;
} stm32_common;


/* Systick registers */
enum {
	icb_systick_ctrl = 0x4,
	icb_systick_load,
	icb_systick_val,
	icb_systick_calib,
};


unsigned int hal_getBootReason(void)
{
	return stm32_common.resetFlags;
}


/* Clock system on STM32U5:
 *            ┌─┐
 *       HSI ─┤M├─ PLL1 → pll1_p_ck / pll1_q_ck / pll1_r_ck
 *      MSIS ─┤U├─ PLL2 → pll2_p_ck / pll2_q_ck / pll2_r_ck
 *       HSE ─┤X├─ PLL3 → pll3_p_ck / pll3_q_ck / pll3_r_ck
 *            └─┘
 *
 * HSI (16 MHz) and MSIS (up to 48 MHz, 4 MHz on reset) are internal RC oscillators.
 * HSE is external crystal oscillator.
 * 
 * The allowed SYSCLK sources are HSI, MSIS, HSE, and PLL1.
 */
static void _stm32_configureClocks(void)
{
	u32 v, clock_source;

	/* Disable HSE */
	*(stm32_common.rcc + rcc_cr) &= ~(1 << 16); /* HSEON off */

	/* Select the system clock source */
	clock_source = 0; /* MSI */
	v = *(stm32_common.rcc + rcc_cfgr1);
	v |= (clock_source << 0); /* Set SW */
	*(stm32_common.rcc + rcc_cfgr1) = v;

	while (((*(stm32_common.rcc + rcc_cfgr1) >> 2) & 0x3) != clock_source) {
		/* Wait for SWS */
	}
	stm32_common.cpuclk = 4 * 1000 * 1000;

	/* Set peripheral bus clock prescalers */
	v = *(stm32_common.rcc + rcc_cfgr2);
	v &= ~((0x7 << 8) | (0x7 << 4) | (0xF << 0)); /* Clear PPRE2, PPRE1, HPRE */
	v |= (0 << 8) | (0 << 4) | (0x8 << 0); /* Set PPRE1 ~ 2 to 1, HPRE to 2 */
	*(stm32_common.rcc + rcc_cfgr2) = v;

	v = *(stm32_common.rcc + rcc_cfgr3);
	v &= ~(0x7 << 0); /* Clear PPRE3 */
	v |= (0 << 4); /* Set PPRE3 to 1 */
	*(stm32_common.rcc + rcc_cfgr3) = v;
}


static int _stm32_getDevClockRegShift(unsigned int dev, unsigned int *shift_out)
{
	unsigned int reg = dev / 32;
	if (reg > (rcc_apb3enr - rcc_ahb1enr)) {
		return -1;
	}

	*shift_out = dev % 32;
	return reg;
}


int _stm32_rccSetDevClock(unsigned int dev, u32 status)
{
	u32 shift;
	int reg;

	reg = _stm32_getDevClockRegShift(dev, &shift);
	if (reg < 0) {
		return -1;
	}

	if (status != 0) {
		*(stm32_common.rcc + reg + rcc_ahb1enr) |= (1 << shift);
		*(stm32_common.rcc + reg + rcc_ahb1smenr) |= (1 << shift);
	} else {
		*(stm32_common.rcc + reg + rcc_ahb1enr) &= ~(1 << shift);
		*(stm32_common.rcc + reg + rcc_ahb1smenr) &= ~(1 << shift);
	}
	hal_cpuDataSyncBarrier();

	return 0;
}


u32 _stm32_rccGetCPUClock(void)
{
	return stm32_common.cpuclk;
}


void _stm32_rccClearResetFlags(void)
{
	*(stm32_common.rcc + rcc_csr) |= (1 << 23); /* RMVF */
}


int _stm32_rccSetIPClk(unsigned int ipclk, u8 setting)
{
	static const struct {
		u16 reg_offs;
		u8 mask;
		u8 shift;
	} ipclk_lookup[ipclks_count] = {
		[ipclk_usart1sel] = { rcc_ccipr1, 0x3, 0 },
		[ipclk_usart2sel] = { rcc_ccipr1, 0x3, 2 },
		[ipclk_usart3sel] = { rcc_ccipr1, 0x3, 4 },
		[ipclk_uart4sel] = { rcc_ccipr1, 0x3, 6 },
		[ipclk_uart5sel] = { rcc_ccipr1, 0x3, 8 },
		[ipclk_i2c1sel] = { rcc_ccipr1, 0x3, 10 },
		[ipclk_i2c2sel] = { rcc_ccipr1, 0x3, 12 },
		[ipclk_i2c4sel] = { rcc_ccipr1, 0x3, 14 },
		[ipclk_spi2sel] = { rcc_ccipr1, 0x3, 16 },
		[ipclk_lptim2sel] = { rcc_ccipr1, 0x3, 18 },
		[ipclk_spi1sel] = { rcc_ccipr1, 0x3, 20 },
		[ipclk_systicksel] = { rcc_ccipr1, 0x3, 22 },
		[ipclk_fdcansel] = { rcc_ccipr1, 0x3, 24 },
		[ipclk_iclksel] = { rcc_ccipr1, 0x3, 26 },
		[ipclk_timicsel] = { rcc_ccipr1, 0x7, 29 },
		[ipclk_mdf1sel] = { rcc_ccipr2, 0x7, 0 },
		[ipclk_sai1sel] = { rcc_ccipr2, 0x7, 5 },
		[ipclk_sai2sel] = { rcc_ccipr2, 0x7, 8 },
		[ipclk_saessel] = { rcc_ccipr2, 0x1, 11 },
		[ipclk_rngsel] = { rcc_ccipr2, 0x3, 12 },
		[ipclk_sdmmcsel] = { rcc_ccipr2, 0x1, 14 },
		[ipclk_octospisel] = { rcc_ccipr2, 0x3, 20 },
		[ipclk_lpuart1sel] = { rcc_ccipr3, 0x7, 0 },
		[ipclk_spi3sel] = { rcc_ccipr3, 0x3, 3 },
		[ipclk_i2c3sel] = { rcc_ccipr3, 0x3, 6 },
		[ipclk_lptim34sel] = { rcc_ccipr3, 0x3, 8 },
		[ipclk_lptim1sel] = { rcc_ccipr3, 0x3, 10 },
		[ipclk_adcdacsel] = { rcc_ccipr3, 0x7, 12 },
		[ipclk_dac1sel] = { rcc_ccipr3, 0x1, 15 },
		[ipclk_adf1sel] = { rcc_ccipr3, 0x7, 16 },
	};

	u32 v;
	if (ipclk >= ipclks_count) {
		return -1;
	}

	v = *(stm32_common.rcc + ipclk_lookup[ipclk].reg_offs);
	v &= ~((u32)ipclk_lookup[ipclk].mask << ipclk_lookup[ipclk].shift);
	setting &= ipclk_lookup[ipclk].mask;
	v |= (u32)setting << ipclk_lookup[ipclk].shift;
	*(stm32_common.rcc + ipclk_lookup[ipclk].reg_offs) = v;
	return 0;
}


/* PWR */


int _stm32_pwrSupplyOperation(unsigned int supply, unsigned int operation, int status)
{
	static const struct {
		u16 reg_offs;
		s8 bits[pwr_supplies_count];
	} supply_lookup[] = {
		[pwr_supply_vddio2] = {
			.reg_offs = pwr_svmcr,
			.bits = {
				[pwr_supply_op_monitor_enable] = 25,
				[pwr_supply_op_valid] = 29,
			},
		},
		[pwr_supply_vusb] = {
			.reg_offs = pwr_svmcr,
			.bits = {
				[pwr_supply_op_monitor_enable] = 24,
				[pwr_supply_op_valid] = 28,
			},
		},
		[pwr_supply_vdda] = {
			.reg_offs = pwr_svmcr,
			.bits = {
				[pwr_supply_op_monitor_enable] = 26,
				[pwr_supply_op_valid] = 30,
			},
		},
	};

	volatile u32 *reg;
	s16 bit_offs;
	if ((supply >= pwr_supplies_count) || (operation >= pwr_supply_ops_count)) {
		return -1;
	}

	reg = stm32_common.pwr + supply_lookup[supply].reg_offs;
	bit_offs = supply_lookup[supply].bits[operation];
	if (bit_offs < 0) {
		return -1;
	}

	if (operation == pwr_supply_op_valid) {
		return (*reg >> bit_offs) & 1;
	}

	if (status != 0) {
		*reg |= 1 << bit_offs;
	}
	else {
		*reg &= ~(1 << bit_offs);
	}

	return 0;
}


static void _stm32_initPowerSupply(unsigned int supply)
{
	volatile int i;
	if (supply >= pwr_supplies_count) {
		return;
	}

	_stm32_pwrSupplyOperation(supply, pwr_supply_op_monitor_enable, 1);
	for (i = 0; i < 1000; i++) {
		/* TODO: implement wait in a better way */
	}

	_stm32_pwrSupplyOperation(supply, pwr_supply_op_valid, 1);
}


/* SysTick */


int _stm32_systickInit(u32 interval)
{
	u32 load = (stm32_common.cpuclk / interval) - 1;
	if (load > 0x00ffffff) {
		return -1;
	}

	*(stm32_common.icb + icb_systick_load) = load;
	*(stm32_common.icb + icb_systick_val) = 0;

	*(stm32_common.icb + icb_systick_ctrl) |= (1 << 2) | (1 << 1) | (1 << 0); /* CLKSOURCE | TICKINT | ENABLE */

	return 0;
}


void _stm32_systickDone(void)
{
	*(stm32_common.icb + icb_systick_ctrl) = 0;
}


/* GPIO */


static volatile u32 *_stm32_gpioGetBase(unsigned int d, u8 pin)
{
	if ((d < dev_gpioa) || (d > dev_gpioi) || (pin > 15)) {
		return NULL;
	}

	return stm32_common.gpio[d - dev_gpioa];
}


int _stm32_gpioConfig(unsigned int d, u8 pin, u8 mode, u8 af, u8 otype, u8 ospeed, u8 pupd)
{
	volatile u32 *base;
	u32 t;

	base = _stm32_gpioGetBase(d, pin);
	if (base == NULL) {
		return -1;
	}

	t = *(base + gpio_moder) & ~(0x3 << (pin << 1));
	*(base + gpio_moder) = t | (mode & 0x3) << (pin << 1);

	t = *(base + gpio_otyper) & ~(1 << pin);
	*(base + gpio_otyper) = t | (otype & 1) << pin;

	t = *(base + gpio_ospeedr) & ~(0x3 << (pin << 1));
	*(base + gpio_ospeedr) = t | (ospeed & 0x3) << (pin << 1);

	t = *(base + gpio_pupdr) & ~(0x03 << (pin << 1));
	*(base + gpio_pupdr) = t | (pupd & 0x3) << (pin << 1);

	if (pin < 8) {
		t = *(base + gpio_afrl) & ~(0xf << (pin << 2));
		*(base + gpio_afrl) = t | (af & 0xf) << (pin << 2);
	}
	else {
		t = *(base + gpio_afrh) & ~(0xf << ((pin - 8) << 2));
		*(base + gpio_afrh) = t | (af & 0xf) << ((pin - 8) << 2);
	}

	return 0;
}


/* Watchdog */


static void _stm32_wdgInit(void)
{
#if defined(WATCHDOG)
	/* Enable write access to IWDG */
	*(stm32_common.iwdg + iwdg_kr) = 0x5555;

	/* 32 kHz independent clock */
	*(stm32_common.iwdg + iwdg_pr) = 6; /* prescaler divider / 256 */
	*(stm32_common.iwdg + iwdg_rlr) = 30 /* s */ * 32000 /* Hz */ / 256;

	_stm32_wdgReload();

	/* Enable watchdog */
	*(stm32_common.iwdg + iwdg_kr) = 0xcccc;
#endif
}


void _stm32_wdgReload(void)
{
#if defined(WATCHDOG)
	*(stm32_common.iwdg + iwdg_kr) = 0xaaaa;
#endif
}


static void _stm32_initSRAM(void)
{
	/* Enable built-in memories that aren't turned on through reset:
	 * SRAM3 ~ 4, BKPSRAM */
	static const struct {
		int dev;
		int crOffs;
		int erkeyrOffs;
	} srams[] = {
		{ .dev = dev_sram3, .crOffs = ramcfg_sram3cr, .erkeyrOffs = ramcfg_sram3erkeyr },
		{ .dev = dev_sram4, .crOffs = ramcfg_sram4cr, .erkeyrOffs = ramcfg_sram4erkeyr },
		{ .dev = dev_bkpsram, .crOffs = ramcfg_bkpsramcr, .erkeyrOffs = -1 }, /* Don't erase backup RAM on startup */
	};
	unsigned int i;

	/* Enable all memories in RCC */
	_stm32_rccSetDevClock(dev_ramcfg, 1);
	for (i = 0; i < sizeof(srams) / sizeof(srams[0]); i++) {
		_stm32_rccSetDevClock(srams[i].dev, 1);

		*(stm32_common.ramcfg + srams[i].crOffs) &= ~(1 << 20);
		if (srams[i].erkeyrOffs >= 0) {
			*(stm32_common.ramcfg + srams[i].erkeyrOffs) = 0xca;
			*(stm32_common.ramcfg + srams[i].erkeyrOffs) = 0x53;
			*(stm32_common.ramcfg + srams[i].crOffs) |= (1 << 8);
			while ((*(stm32_common.ramcfg + srams[i].crOffs) & (1 << 8)) != 0) {
				/* Wait for end of erase */
			}
		}
	}
}


void _stm32_init(void)
{
	u32 i;

	/* Base addresses init */
	stm32_common.rcc = RCC_BASE;
	stm32_common.pwr = PWR_BASE;
	stm32_common.icb = ICB_BASE;
	stm32_common.iwdg = IWDG_BASE;
	stm32_common.syscfg = SYSCFG_BASE;
	stm32_common.ramcfg = RAMCFG_BASE;
	stm32_common.gpio[0] = GPIOA_BASE;
	stm32_common.gpio[1] = GPIOB_BASE;
	stm32_common.gpio[2] = GPIOC_BASE;
	stm32_common.gpio[3] = GPIOD_BASE;
	stm32_common.gpio[4] = GPIOE_BASE;
	stm32_common.gpio[5] = GPIOF_BASE;
	stm32_common.gpio[6] = GPIOG_BASE;
	stm32_common.gpio[7] = GPIOH_BASE;
	stm32_common.gpio[8] = GPIOI_BASE;

	/* Store reset flags and then clear them */
	stm32_common.resetFlags = (*(stm32_common.rcc + rcc_csr) >> 25); /* LPWRRSTF ~ OBLRSTF */
	_stm32_rccClearResetFlags();

	/* Enable System configuration controller */
	_stm32_rccSetDevClock(dev_syscfg, 1);
	_stm32_rccSetDevClock(dev_vref, 1);

	/* Enable power module */
	_stm32_rccSetDevClock(dev_pwr, 1);

	_stm32_initSRAM();

	/* Enable independent power supplies for GPIOs. */
	_stm32_initPowerSupply(pwr_supply_vddio2);
	/* Enable independent power supply for ADCs */
	_stm32_initPowerSupply(pwr_supply_vdda);
	/* Enable independent power supply for USB */
	_stm32_initPowerSupply(pwr_supply_vusb);

	/* Disable all RCC interrupts */
	*(stm32_common.rcc + rcc_cier) = 0;

	_stm32_configureClocks();

	hal_cpuDataMemoryBarrier();

	/* GPIO init */
	for (i = dev_gpioa; i <= dev_gpioi; i++) {
		_stm32_rccSetDevClock(i, 1);
	}

	hal_cpuDataMemoryBarrier();

	_stm32_wdgInit();
}
