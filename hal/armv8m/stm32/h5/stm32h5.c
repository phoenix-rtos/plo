/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32H5 basic peripherals control functions
 *
 * Copyright 2017, 2019, 2020, 2021, 2025, 2026 Phoenix Systems
 * Author: Aleksander Kaminski, Jan Sikorski, Hubert Buczynski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <board_config.h>
#include "stm32h5.h"
#include "stm32h5_regs.h"

#define GPIOA_BASE ((void *)0x52020000)
#define GPIOB_BASE ((void *)0x52020400)
#define GPIOC_BASE ((void *)0x52020800)
#define GPIOD_BASE ((void *)0x52020c00)
#define GPIOE_BASE ((void *)0x52021000)
#define GPIOF_BASE ((void *)0x52021400)
#define GPIOG_BASE ((void *)0x52021800)
#define GPIOH_BASE ((void *)0x52021c00)
#define GPIOI_BASE ((void *)0x52022000)

#define IWDG_BASE  ((void *)0x50003000)
#define PWR_BASE   ((void *)0x54020800)
#define RCC_BASE   ((void *)0x54020c00)
#define RTC_BASE   ((void *)0x54007800)
#define ICB_BASE   ((void *)0xe000e000)
#define FLASH_BASE ((void *)0x50022000)

static struct {
	volatile u32 *rcc;
	volatile u32 *gpio[9];
	volatile u32 *icb;
	volatile u32 *pwr;
	volatile u32 *rtc;
	volatile u32 *iwdg;
	volatile u32 *flash;

	u32 cpuclk;
	u32 perclk;

	u32 resetFlags;
} stm32_common;


enum {
	icb_ictr = 1,
	icb_actlr,
	icb_cppwr,
	scb_cpuid = 832,
	scb_icsr,
	scb_vtor,
	scb_aircr,
	scb_scr,
	scb_ccr,
	scb_shp1,
	scb_shp2,
	scb_shp3,
	scb_shcsr,
	scb_cfsr,
	scb_mmsr,
	scb_bfsr,
	scb_ufsr,
	scb_hfsr,
	scb_mmar,
	scb_bfar,
	scb_afsr,
	scb_cpacr = 866,
	scb_fpccr = 973,
	scb_fpcar,
	scb_fpdscr,
};


/* Systick registers */
enum {
	icb_systick_csr = 4,
	icb_systick_rvr,
	icb_systick_cvr,
	icb_systick_calib,
};


enum pllSrc {
	pll_src_none = 0,
	pll_src_hsi,
	pll_src_csi,
	pll_src_hse,
};


enum pllRange {
	pll_range_1_2 = 0,
	pll_range_2_4,
	pll_range_4_8,
	pll_range_8_16
};


struct pllConfig {
	int pll;
	enum pllSrc src;
	enum pllRange range;
	u8 pre_div;
	u16 mul;
	u16 mul_frac;
	u8 divr;
	u8 divq;
	u8 divp;
};


unsigned int hal_getBootReason(void)
{
	return stm32_common.resetFlags;
}


/* RCC (Reset and Clock Controller) */


static void _stm32_configurePLL(const struct pllConfig *config)
{
	u32 v;

	static const struct {
		unsigned cfgr;
		unsigned divr;
		unsigned fracr;
		u32 bitOn;
		u32 bitRdy;
	} pllRegs[3] = {
		{
			.cfgr = rcc_pll1cfgr,
			.divr = rcc_pll1divr,
			.fracr = rcc_pll1fracr,
			.bitOn = 1UL << 24U,
			.bitRdy = 1UL << 25U
		},
		{
			.cfgr = rcc_pll2cfgr,
			.divr = rcc_pll2divr,
			.fracr = rcc_pll2fracr,
			.bitOn = 1UL << 26U,
			.bitRdy = 1UL << 27U
		},
		{
			.cfgr = rcc_pll3cfgr,
			.divr = rcc_pll3divr,
			.fracr = rcc_pll3fracr,
			.bitOn = 1UL << 28U,
			.bitRdy = 1UL << 29U
		}
	};

	/* Disable PLL */
	*(stm32_common.rcc + rcc_cr) &= ~(pllRegs[config->pll].bitOn);
	hal_cpuDataSyncBarrier();

	/* Disable clock input into PLL */
	*(stm32_common.rcc + pllRegs[config->pll].cfgr) &= ~0x3;
	hal_cpuDataSyncBarrier();

	/* Set divider and fractional divider */
	*(stm32_common.rcc + pllRegs[config->pll].divr) =
		((config->divr & 0x7fU) << 24U) |
		((config->divq & 0x7fU) << 16U) |
		((config->divp & 0x7fU) << 9U) |
		(config->mul & 0x1ffU);
	*(stm32_common.rcc + pllRegs[config->pll].fracr) = (config->mul_frac & 0x1fffU) << 3U;

	/* Latch the constent of FRACN */
	hal_cpuDataSyncBarrier();
	*(stm32_common.rcc + pllRegs[config->pll].cfgr) &= ~(1UL << 4U);
	hal_cpuDataSyncBarrier();
	*(stm32_common.rcc + pllRegs[config->pll].cfgr) |= 1UL << 4U;
	hal_cpuDataSyncBarrier();

	/* Enter the configuration */
	v = *(stm32_common.rcc + pllRegs[config->pll].cfgr) & 0xfff8cfdfUL;
	*(stm32_common.rcc + pllRegs[config->pll].cfgr) = v |
		(0x7UL << 16U) | /* Enable all outputs */
		((config->pre_div & 0x3fU) << 8U) |
		((config->range & 0x3U) << 2U) |
		(config->src & 0x3U);
	hal_cpuDataSyncBarrier();

	/* Enable PLL and wait for lock */
	*(stm32_common.rcc + rcc_cr) |= pllRegs[config->pll].bitOn;
	hal_cpuDataSyncBarrier();
	while ((*(stm32_common.rcc + rcc_cr) & pllRegs[config->pll].bitRdy) == 0) {
		/* Wait for PLLxRDY */
	}
}


volatile u32 *_stm32_rccClkGetReg(unsigned int dev)
{
	switch (dev / 32) {
		case 0:
			return stm32_common.rcc + rcc_ahb1enr;
		case 1:
			return stm32_common.rcc + rcc_ahb2enr;
		case 2:
			return stm32_common.rcc + rcc_ahb4enr;
		case 3:
			return stm32_common.rcc + rcc_apb1lenr;
		case 4:
			return stm32_common.rcc + rcc_apb1henr;
		case 5:
			return stm32_common.rcc + rcc_apb2enr;
		case 6:
			return stm32_common.rcc + rcc_apb3enr;
		default:
			return NULL;
	}
}


int _stm32_rccSetDevClock(unsigned int dev, u32 status)
{
	volatile u32 *reg = _stm32_rccClkGetReg(dev);
	if (reg == NULL) {
		return -1;
	}

	if (status != 0) {
		hal_cpuDataMemoryBarrier();
		*reg |= (1UL << (dev % 32));
	}
	else {
		*reg &= ~(1UL << (dev % 32));
		hal_cpuDataMemoryBarrier();
	}

	return 0;
}


int _stm32_rccGetDevClock(unsigned int dev, u32 *status)
{
	volatile u32 *reg = _stm32_rccClkGetReg(dev);
	if (reg == NULL) {
		return -1;
	}

	hal_cpuDataMemoryBarrier();
	*status = ((*reg & (1UL << (dev % 32))) == 0) ? 0 : 1;
	return 0;
}


int _stm32_rccDevReset(unsigned int dev, u32 status)
{
	volatile u32 *reg;

	switch (dev / 32) {
		case 0:
			reg = stm32_common.rcc + rcc_ahb1rstr;
			break;
		case 1:
			reg = stm32_common.rcc + rcc_ahb2rstr;
			break;
		case 2:
			reg = stm32_common.rcc + rcc_ahb4rstr;
			break;
		case 3:
			reg = stm32_common.rcc + rcc_apb1lrstr;
			break;
		case 4:
			reg = stm32_common.rcc + rcc_apb1hrstr;
			break;
		case 5:
			reg = stm32_common.rcc + rcc_apb2rstr;
			break;
		case 6:
			reg = stm32_common.rcc + rcc_apb3rstr;
			break;
		default:
			return -1;
	}

	hal_cpuDataMemoryBarrier();
	*reg |= 1UL << (dev % 32UL);
	hal_cpuDataMemoryBarrier();
	*reg &= ~(1UL << (dev % 32UL));
	hal_cpuDataMemoryBarrier();

	return 0;
}


int _stm32_rccSetIPClk(unsigned int ipclk, u8 setting)
{
	static const struct {
		u16 reg_offs;
		u8 mask;
		u8 shift;
	} ipclk_lookup[] = {
		[ipclk_usart1sel] = { rcc_ccipr1, 0x7, 0 },
		[ipclk_usart2sel] = { rcc_ccipr1, 0x7, 3 },
		[ipclk_usart3sel] = { rcc_ccipr1, 0x7, 6 },
		[ipclk_uart4sel] = { rcc_ccipr1, 0x7, 9 },
		[ipclk_uart5sel] = { rcc_ccipr1, 0x7, 12 },
		[ipclk_usart6sel] = { rcc_ccipr1, 0x7, 15 },
		[ipclk_uart7sel] = { rcc_ccipr1, 0x7, 18 },
		[ipclk_uart8sel] = { rcc_ccipr1, 0x7, 21 },
		[ipclk_uart9sel] = { rcc_ccipr1, 0x7, 24 },
		[ipclk_usart10sel] = { rcc_ccipr1, 0x7, 27 },
		[ipclk_timicsel] = { rcc_ccipr1, 0x1, 31 },
		[ipclk_usart11sel] = { rcc_ccipr2, 0x7, 0 },
		[ipclk_uart12sel] = { rcc_ccipr2, 0x7, 4 },
		[ipclk_lptim1sel] = { rcc_ccipr2, 0x7, 8 },
		[ipclk_lptim2sel] = { rcc_ccipr2, 0x7, 12 },
		[ipclk_lptim3sel] = { rcc_ccipr2, 0x7, 16 },
		[ipclk_lptim4sel] = { rcc_ccipr2, 0x7, 20 },
		[ipclk_lptim5sel] = { rcc_ccipr2, 0x7, 24 },
		[ipclk_lptim6sel] = { rcc_ccipr2, 0x7, 28 },
		[ipclk_spi1sel] = { rcc_ccipr3, 0x7, 0 },
		[ipclk_spi2sel] = { rcc_ccipr3, 0x7, 3 },
		[ipclk_spi3sel] = { rcc_ccipr3, 0x7, 6 },
		[ipclk_spi4sel] = { rcc_ccipr3, 0x7, 9 },
		[ipclk_spi5sel] = { rcc_ccipr3, 0x7, 12 },
		[ipclk_spi6sel] = { rcc_ccipr3, 0x7, 15 },
		[ipclk_lpuart1sel] = { rcc_ccipr3, 0x7, 24 },
		[ipclk_octospi1sel] = { rcc_ccipr4, 0x3, 0 },
		[ipclk_systicksel] = { rcc_ccipr4, 0x3, 2 },
		[ipclk_usbsel] = { rcc_ccipr4, 0x3, 4 },
		[ipclk_sdmmc1sel] = { rcc_ccipr4, 0x1, 6 },
		[ipclk_sdmmc2sel] = { rcc_ccipr4, 0x1, 7 },
		[ipclk_i2c1sel] = { rcc_ccipr4, 0x3, 16 },
		[ipclk_i2c2sel] = { rcc_ccipr4, 0x3, 18 },
		[ipclk_i2c3sel] = { rcc_ccipr4, 0x3, 20 },
		[ipclk_i2c4sel] = { rcc_ccipr4, 0x3, 22 },
		[ipclk_i3c1sel] = { rcc_ccipr4, 0x3, 24 },
		[ipclk_i3c2sel] = { rcc_ccipr4, 0x3, 26 },
		[ipclk_adcdacsel] = { rcc_ccipr5, 0x7, 0 },
		[ipclk_dacsel] = { rcc_ccipr5, 0x1, 3 },
		[ipclk_rngsel] = { rcc_ccipr5, 0x3, 4 },
		[ipclk_cecsel] = { rcc_ccipr5, 0x3, 6 },
		[ipclk_fdcansel] = { rcc_ccipr5, 0x3, 8 },
		[ipclk_sai1sel] = { rcc_ccipr5, 0x7, 16 },
		[ipclk_sai2sel] = { rcc_ccipr5, 0x7, 19 },
		[ipclk_ckpersel] = { rcc_ccipr5, 0x3, 30 }
	};

	u32 v;
	if (ipclk >= (sizeof(ipclk_lookup) / sizeof(*ipclk_lookup))) {
		return -1;
	}

	hal_cpuDataMemoryBarrier();
	v = *(stm32_common.rcc + ipclk_lookup[ipclk].reg_offs);
	v &= ~((u32)ipclk_lookup[ipclk].mask << ipclk_lookup[ipclk].shift);
	setting &= ipclk_lookup[ipclk].mask;
	v |= (u32)setting << ipclk_lookup[ipclk].shift;
	*(stm32_common.rcc + ipclk_lookup[ipclk].reg_offs) = v;
	hal_cpuDataMemoryBarrier();

	return 0;
}


u32 _stm32_rccGetCPUClock(void)
{
	return stm32_common.cpuclk;
}


u32 _stm32_rccGetPerClock(void)
{
	return stm32_common.perclk;
}


void _stm32_rccClearResetFlags(void)
{
	*(stm32_common.rcc + rcc_rsr) = 1 << 16;
}


/* RTC */


void _stm32_rtcUnlockRegs(void)
{
	/* Set DBP bit */
	*(stm32_common.pwr + pwr_dbpcr) |= 1;

	/* Unlock RTC */
	*(stm32_common.rtc + rtc_wpr) = 0x000000ca;
	*(stm32_common.rtc + rtc_wpr) = 0x00000053;
	hal_cpuDataMemoryBarrier();
}


void _stm32_rtcLockRegs(void)
{
	hal_cpuDataMemoryBarrier();
	/* Lock RTC */
	*(stm32_common.rtc + rtc_wpr) = 0x000000ff;

	/* Reset DBP bit */
	*(stm32_common.pwr + pwr_dbpcr) &= ~1;
}


/* PWR */


void _stm32_pwrSetCPUVolt(u8 range)
{
	u32 t;
	u8 rangeCurr;

	hal_cpuDataMemoryBarrier();
	rangeCurr = (*(stm32_common.pwr + pwr_vossr) >> 14U) & 0x3;

	range &= 0x3U;

	/* range 3 is 0b00, range 2 is 0b01 etc., logical */
	range = 0x3U - range;

	/* Range has be adjusted gradually, we have to reach intermediate steps
	   on radical changes */
	while (range != rangeCurr) {
		if (range < rangeCurr) {
			--rangeCurr;
		}
		else {
			++rangeCurr;
		}

		t = *(stm32_common.pwr + pwr_voscr) & ~(0x3U << 4U);
		*(stm32_common.pwr + pwr_voscr) = t | (rangeCurr << 4U);
		hal_cpuDataMemoryBarrier();

		while ((*(stm32_common.pwr + pwr_vossr) & (1UL << 3U)) == 0) {
		}
	}
}


/* SysTick */


int _stm32_systickInit(u32 interval)
{
	u32 load = (stm32_common.cpuclk / interval) - 1;
	if (load > 0x00ffffff) {
		return -1;
	}

	*(stm32_common.icb + icb_systick_rvr) = load;
	*(stm32_common.icb + icb_systick_cvr) = 0;

	/* Enable counter, systick interrupt and set source to processor clock */
	*(stm32_common.icb + icb_systick_csr) |= 0x7;
	hal_cpuDataMemoryBarrier();

	return 0;
}


void _stm32_systickDone(void)
{
	hal_cpuDataMemoryBarrier();
	*(stm32_common.icb + icb_systick_csr) = 0;
}


/* GPIO */


int _stm32_gpioConfig(unsigned int d, u8 pin, u8 mode, u8 af, u8 otype, u8 ospeed, u8 pupd)
{
	volatile u32 *base;
	u32 t;

	if ((d < dev_gpioa) || (d > dev_gpioi) || (pin > 15)) {
		return -1;
	}

	base = stm32_common.gpio[d - dev_gpioa];

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

	hal_cpuDataMemoryBarrier();

	return 0;
}


int _stm32_gpioSet(unsigned int d, u8 pin, u8 val)
{
	volatile u32 *base;

	if ((d < dev_gpioa) || (d > dev_gpioi) || (pin > 15)) {
		return -1;
	}

	base = stm32_common.gpio[d - dev_gpioa];
	if (base == NULL) {
		return -1;
	}

	*(base + gpio_bsrr) = 1 << ((val == 0) ? (pin + 16) : pin);
	return 0;
}


int _stm32_gpioSetPort(unsigned int d, u16 val)
{
	volatile u32 *base;

	if ((d < dev_gpioa) || (d > dev_gpioi)) {
		return -1;
	}

	base = stm32_common.gpio[d - dev_gpioa];
	if (base == NULL) {
		return -1;
	}

	*(base + gpio_odr) = val;

	return 0;
}


int _stm32_gpioGet(unsigned int d, u8 pin, u8 *val)
{
	volatile u32 *base;

	if ((d < dev_gpioa) || (d > dev_gpioi) || (pin > 15)) {
		return -1;
	}

	base = stm32_common.gpio[d - dev_gpioa];
	if (base == NULL) {
		return -1;
	}

	*val = (*(base + gpio_idr) >> pin) & 1;

	return 0;
}


int _stm32_gpioGetPort(unsigned int d, u16 *val)
{
	volatile u32 *base;

	if ((d < dev_gpioa) || (d > dev_gpioi)) {
		return -1;
	}

	base = stm32_common.gpio[d - dev_gpioa];
	if (base == NULL) {
		return -1;
	}

	*val = *(base + gpio_idr);

	return 0;
}


/* Watchdog */


void _stm32_wdgReload(void)
{
#if defined(WATCHDOG)
	*(stm32_common.iwdg + iwdg_kr) = 0xaaaa;
#endif
}


void _stm32_initHalOnly(void)
{
	stm32_common.iwdg = IWDG_BASE;
	stm32_common.pwr = PWR_BASE;
	stm32_common.rcc = RCC_BASE;
	stm32_common.rtc = RTC_BASE;
	stm32_common.icb = ICB_BASE;
	stm32_common.flash = FLASH_BASE;
	stm32_common.gpio[0] = GPIOA_BASE;
	stm32_common.gpio[1] = GPIOB_BASE;
	stm32_common.gpio[2] = GPIOC_BASE;
	stm32_common.gpio[3] = GPIOD_BASE;
	stm32_common.gpio[4] = GPIOE_BASE;
	stm32_common.gpio[5] = GPIOF_BASE;
	stm32_common.gpio[6] = GPIOG_BASE;
	stm32_common.gpio[7] = GPIOH_BASE;
	stm32_common.gpio[8] = GPIOI_BASE;
}


void _stm32_init(void)
{
	u32 v, rb;
	static const struct pllConfig pll1Config = {
		.src = pll_src_hse,
		.range = pll_range_1_2,
		.pre_div = 24,
		.mul = 500,
		.mul_frac = 0,
		.divr = 1, /* 250 MHz output */
		.divq = 1, /* 250 MHz output */
		.divp = 1, /* 250 MHz output */
	};

	_stm32_initHalOnly();

	/* Store reset flags and then clear them */
	stm32_common.resetFlags = (*(stm32_common.rcc + rcc_rsr) >> 21);
	*(stm32_common.rcc + rcc_rsr) = 1 << 16;

	/* Enable LSI */
	_stm32_rtcUnlockRegs();
	*(stm32_common.rcc + rcc_bdcr) |= (1UL << 26U);
	while ((*(stm32_common.rcc + rcc_bdcr) & (1UL << 27)) == 0) {
		/* Wait for LSIRDY */
	}
	_stm32_rtcLockRegs();

	/* Set Vcore range for max performance */
	_stm32_pwrSetCPUVolt(0);

	/* Enable HSE (external XTAL @24 MHz */
	*(stm32_common.rcc + rcc_cr) |= (1UL << 16U);
	while ((*(stm32_common.rcc + rcc_cr) & (1UL << 17)) == 0) {
		/* Wait for HSERDY */
	}

	_stm32_configurePLL(&pll1Config);

	/* Adjust Flash wait states to allow max CPU clk */
	v = *(stm32_common.flash + 0) & ~0x1c0;
	/* Enable prefetch and 6 WS cycles */
	v |= (1UL << 8) | (0x2UL << 4) | 5;

	/* Per doc - we need to make sure that the value has been
	 * indeed stored in the register */
	do {
		*(stm32_common.flash + 0) = v;
		rb = *(stm32_common.flash + 0);
	} while (rb != v);

	/* Set PLL1P as the sys_clk (CPU clock) */
	v = *(stm32_common.rcc + rcc_cfgr1) & ~0x3UL;
	*(stm32_common.rcc + rcc_cfgr1) = v | 0x3UL;

	stm32_common.cpuclk = 250UL * 1000UL * 1000UL;

#if defined(WATCHDOG)
	/* Init watchdog */
	/* Enable write access to IWDG */
	*(stm32_common.iwdg + iwdg_kr) = 0x5555;

	/* Set prescaler to 256, ~30s interval */
	*(stm32_common.iwdg + iwdg_pr) = 0x06;
	*(stm32_common.iwdg + iwdg_rlr) = 0xfff;

	_stm32_wdgReload();

	/* Enable watchdog */
	*(stm32_common.iwdg + iwdg_kr) = 0xcccc;
#endif
}
