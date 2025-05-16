/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32N6 basic peripherals control functions
 *
 * Copyright 2017, 2019, 2020, 2021 Phoenix Systems
 * Author: Aleksander Kaminski, Jan Sikorski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include "stm32n6.h"
#include "stm32n6_regs.h"


#define GPIOA_BASE ((void *)0x56020000)
#define GPIOB_BASE ((void *)0x56020400)
#define GPIOC_BASE ((void *)0x56020800)
#define GPIOD_BASE ((void *)0x56020c00)
#define GPIOE_BASE ((void *)0x56021000)
#define GPIOF_BASE ((void *)0x56021400)
#define GPIOG_BASE ((void *)0x56021800)
#define GPIOH_BASE ((void *)0x56021c00)
#define GPION_BASE ((void *)0x56023400)
#define GPIOO_BASE ((void *)0x56023800)
#define GPIOP_BASE ((void *)0x56023c00)
#define GPIOQ_BASE ((void *)0x56024000)

#define IWDG_BASE   ((void *)0x56004800)
#define PWR_BASE    ((void *)0x56024800)
#define RCC_BASE    ((void *)0x56028000)
#define RTC_BASE    ((void *)0x56004000)
#define SYSCFG_BASE ((void *)0x56008000)


static struct {
	volatile u32 *rcc;
	volatile u32 *gpio[17];
	volatile u32 *scb;
	volatile u32 *pwr;
	volatile u32 *rtc;
	volatile u32 *syscfg;
	volatile u32 *iwdg;
	volatile u32 *flash;

	u32 cpuclk;

	u32 resetFlags;
} stm32_common;

enum {
	dev_clk_end_ahb1enr = 0x20,
	dev_clk_end_ahb2enr = 0x40,
	dev_clk_end_ahb3enr = 0x60,
	dev_clk_end_ahb4enr = 0x80,
	dev_clk_end_ahb5enr = 0xa0,
	dev_clk_end_apb1lenr = 0xc0,
	dev_clk_end_apb1henr = 0xe0,
	dev_clk_end_apb2enr = 0x100,
	dev_clk_end_apb3enr = 0x120,
	dev_clk_end_apb4lenr = 0x140,
	dev_clk_end_apb4henr = 0x160,
	dev_clk_end_apb5enr = 0x180,
};


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
	syst_csr = 4,
	syst_rvr,
	syst_cvr,
	syst_calib,
};


unsigned int hal_getBootReason(void)
{
	return stm32_common.resetFlags;
}


/* RCC (Reset and Clock Controller) */


static int _stm32_getDevClockRegShift(unsigned int dev, unsigned int *shift_out)
{
	unsigned int reg = dev / 32;
	if (reg > (rcc_apb5enr - rcc_ahb1enr)) {
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

	reg += (status == 0) ? rcc_ahb1encr : rcc_ahb1ensr;
	*(stm32_common.rcc + reg) = status << (shift);
	hal_cpuDataMemoryBarrier();

	return 0;
}


int _stm32_rccGetDevClock(unsigned int dev, u32 *status)
{
	u32 shift;
	int reg;

	reg = _stm32_getDevClockRegShift(dev, &shift);
	if (reg < 0) {
		return -1;
	}

	reg += rcc_ahb1enr;
	*status = (*(stm32_common.rcc + reg) >> shift) & 1;
	return 0;
}


u32 _stm32_rccGetCPUClock(void)
{
	return stm32_common.cpuclk;
}


void _stm32_rccClearResetFlags(void)
{
	*(stm32_common.rcc + rcc_rsr) = 1 << 16;
}


int _stm32_rccSetClockIC(u32 ic, u32 pll_src, u32 divider)
{
	const u32 offset = rcc_ic1cfgr + ic - 1;
	if ((ic == 0) || (ic > 20) || (pll_src == 0) || (pll_src > 4) || (divider == 0) || (divider > 256)) {
		return -1;
	}

	*(stm32_common.rcc + offset) = ((pll_src - 1) << 28) | ((divider - 1) << 16);
	return 0;
}

static const struct {
	u16 reg_offs;
	u8 mask;
	u8 shift;
} stm32_rccIPRLookup[] = {
	[ipclk_adf1sel] = { rcc_ccipr1, 0x7, 0 },
	[ipclk_adc12sel] = { rcc_ccipr1, 0x7, 4 },
	[ipclk_adcpre] = { rcc_ccipr1, 0xff, 8 },
	[ipclk_dcmippsel] = { rcc_ccipr1, 0x3, 20 },
	[ipclk_eth1ptpsel] = { rcc_ccipr2, 0x3, 0 },
	[ipclk_eth1ptpdiv] = { rcc_ccipr2, 0xf, 4 },
	[ipclk_eth1pwrdownack] = { rcc_ccipr2, 0x1, 8 },
	[ipclk_eth1clksel] = { rcc_ccipr2, 0x3, 12 },
	[ipclk_eth1sel] = { rcc_ccipr2, 0x7, 16 },
	[ipclk_eth1refclksel] = { rcc_ccipr2, 0x1, 20 },
	[ipclk_eth1gtxclksel] = { rcc_ccipr2, 0x1, 24 },
	[ipclk_fdcansel] = { rcc_ccipr3, 0x3, 0 },
	[ipclk_fmcsel] = { rcc_ccipr3, 0x3, 4 },
	[ipclk_dftsel] = { rcc_ccipr3, 0x1, 8 },
	[ipclk_i2c1sel] = { rcc_ccipr4, 0x7, 0 },
	[ipclk_i2c2sel] = { rcc_ccipr4, 0x7, 4 },
	[ipclk_i2c3sel] = { rcc_ccipr4, 0x7, 8 },
	[ipclk_i2c4sel] = { rcc_ccipr4, 0x7, 12 },
	[ipclk_i3c1sel] = { rcc_ccipr4, 0x7, 16 },
	[ipclk_i3c2sel] = { rcc_ccipr4, 0x7, 20 },
	[ipclk_ltdcsel] = { rcc_ccipr4, 0x3, 24 },
	[ipclk_mco1sel] = { rcc_ccipr5, 0x7, 0 },
	[ipclk_mco1pre] = { rcc_ccipr5, 0xf, 4 },
	[ipclk_mco2sel] = { rcc_ccipr5, 0x7, 8 },
	[ipclk_mco2pre] = { rcc_ccipr5, 0xf, 12 },
	[ipclk_mdf1sel] = { rcc_ccipr5, 0x7, 16 },
	[ipclk_xspi1sel] = { rcc_ccipr6, 0x3, 0 },
	[ipclk_xspi2sel] = { rcc_ccipr6, 0x3, 4 },
	[ipclk_xspi3sel] = { rcc_ccipr6, 0x3, 8 },
	[ipclk_otgphy1sel] = { rcc_ccipr6, 0x3, 12 },
	[ipclk_otgphy1ckrefsel] = { rcc_ccipr6, 0x1, 16 },
	[ipclk_otgphy2sel] = { rcc_ccipr6, 0x3, 20 },
	[ipclk_otgphy2ckrefsel] = { rcc_ccipr6, 0x1, 24 },
	[ipclk_persel] = { rcc_ccipr7, 0x7, 0 },
	[ipclk_pssisel] = { rcc_ccipr7, 0x3, 4 },
	[ipclk_rtcsel] = { rcc_ccipr7, 0x3, 8 },
	[ipclk_rtcpre] = { rcc_ccipr7, 0x3f, 12 },
	[ipclk_sai1sel] = { rcc_ccipr7, 0x7, 20 },
	[ipclk_sai2sel] = { rcc_ccipr7, 0x7, 24 },
	[ipclk_sdmmc1sel] = { rcc_ccipr8, 0x3, 0 },
	[ipclk_sdmmc2sel] = { rcc_ccipr8, 0x3, 4 },
	[ipclk_spdifrx1sel] = { rcc_ccipr9, 0x7, 0 },
	[ipclk_spi1sel] = { rcc_ccipr9, 0x7, 4 },
	[ipclk_spi2sel] = { rcc_ccipr9, 0x7, 8 },
	[ipclk_spi3sel] = { rcc_ccipr9, 0x7, 12 },
	[ipclk_spi4sel] = { rcc_ccipr9, 0x7, 16 },
	[ipclk_spi5sel] = { rcc_ccipr9, 0x7, 20 },
	[ipclk_spi6sel] = { rcc_ccipr9, 0x7, 24 },
	[ipclk_lptim1sel] = { rcc_ccipr12, 0x7, 8 },
	[ipclk_lptim2sel] = { rcc_ccipr12, 0x7, 12 },
	[ipclk_lptim3sel] = { rcc_ccipr12, 0x7, 16 },
	[ipclk_lptim4sel] = { rcc_ccipr12, 0x7, 20 },
	[ipclk_lptim5sel] = { rcc_ccipr12, 0x7, 24 },
	[ipclk_usart1sel] = { rcc_ccipr13, 0x7, 0 },
	[ipclk_usart2sel] = { rcc_ccipr13, 0x7, 4 },
	[ipclk_usart3sel] = { rcc_ccipr13, 0x7, 8 },
	[ipclk_uart4sel] = { rcc_ccipr13, 0x7, 12 },
	[ipclk_uart5sel] = { rcc_ccipr13, 0x7, 16 },
	[ipclk_usart6sel] = { rcc_ccipr13, 0x7, 20 },
	[ipclk_uart7sel] = { rcc_ccipr13, 0x7, 24 },
	[ipclk_uart8sel] = { rcc_ccipr13, 0x7, 28 },
	[ipclk_uart9sel] = { rcc_ccipr14, 0x7, 0 },
	[ipclk_usart10sel] = { rcc_ccipr14, 0x7, 4 },
	[ipclk_lpuart1sel] = { rcc_ccipr14, 0x7, 8 },
};


int _stm32_rccSetIPR(u32 ipclk, u8 setting)
{
	u32 v;
	if (ipclk >= ipclks_count) {
		return -1;
	}

	v = *(stm32_common.rcc + stm32_rccIPRLookup[ipclk].reg_offs);
	v &= ~((u32)stm32_rccIPRLookup[ipclk].mask << stm32_rccIPRLookup[ipclk].shift);
	setting &= stm32_rccIPRLookup[ipclk].mask;
	v |= (u32)setting << stm32_rccIPRLookup[ipclk].shift;
	*(stm32_common.rcc + stm32_rccIPRLookup[ipclk].reg_offs) = v;
	return 0;
}


/* RTC */


void _stm32_rtcUnlockRegs(void)
{
	/* Set DBP bit */
	*(stm32_common.pwr + pwr_dbpcr) |= 1;

	/* Unlock RTC */
	*(stm32_common.rtc + rtc_wpr) = 0x000000ca;
	*(stm32_common.rtc + rtc_wpr) = 0x00000053;
}


void _stm32_rtcLockRegs(void)
{
	/* Lock RTC */
	*(stm32_common.rtc + rtc_wpr) = 0x000000ff;

	/* Reset DBP bit */
	*(stm32_common.pwr + pwr_dbpcr) &= ~1;
}


/* PWR */


void _stm32_pwrSetCPUVolt(u8 range)
{
	u32 t;

	if (range != 0 && range != 1) {
		return;
	}

	t = *(stm32_common.pwr + pwr_voscr) & ~1;
	*(stm32_common.pwr + pwr_voscr) = t | range;

	while (*(stm32_common.pwr + pwr_voscr) & (1 << 1)) {
		/* Wait for VOSRDY flag */
	}
}


/* SysTick */


int _stm32_systickInit(u32 interval)
{
	u32 load = stm32_common.cpuclk / interval - 1;
	if (load > 0x00ffffff) {
		return -1;
	}

	*(stm32_common.scb + syst_rvr) = load;
	*(stm32_common.scb + syst_cvr) = 0;

	/* Enable counter, systick interrupt and set source to processor clock */
	*(stm32_common.scb + syst_csr) |= 0x7;

	return 0;
}


void _stm32_systickDone(void)
{
	*(stm32_common.scb + syst_csr) = 0;
}


/* GPIO */


int _stm32_gpioConfig(unsigned int d, u8 pin, u8 mode, u8 af, u8 otype, u8 ospeed, u8 pupd)
{
	volatile u32 *base;
	u32 t;

	if ((d < dev_gpioa) || (d > dev_gpioq) || (pin > 15)) {
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

	return 0;
}


int _stm32_gpioSet(unsigned int d, u8 pin, u8 val)
{
	volatile u32 *base;

	if ((d < dev_gpioa) || (d > dev_gpioq) || (pin > 15)) {
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

	if ((d < dev_gpioa) || (d > dev_gpioq)) {
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

	if ((d < dev_gpioa) || (d > dev_gpioq) || (pin > 15)) {
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

	if ((d < dev_gpioa) || (d > dev_gpioq)) {
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


void _stm32_init(void)
{
	u32 i;
	static const int gpioDevs[] = { dev_gpioa, dev_gpiob, dev_gpioc, dev_gpiod, dev_gpioe, dev_gpiof, dev_gpiog,
		dev_gpioh, dev_gpion, dev_gpioo, dev_gpiop, dev_gpioq };

	/* Base addresses init */
	stm32_common.iwdg = IWDG_BASE;
	stm32_common.pwr = PWR_BASE;
	stm32_common.rcc = RCC_BASE;
	stm32_common.rtc = RTC_BASE;
	stm32_common.syscfg = SYSCFG_BASE;
	stm32_common.scb = (void *)0xe000e000;
	stm32_common.gpio[0] = GPIOA_BASE;
	stm32_common.gpio[1] = GPIOB_BASE;
	stm32_common.gpio[2] = GPIOC_BASE;
	stm32_common.gpio[3] = GPIOD_BASE;
	stm32_common.gpio[4] = GPIOE_BASE;
	stm32_common.gpio[5] = GPIOF_BASE;
	stm32_common.gpio[6] = GPIOG_BASE;
	stm32_common.gpio[7] = GPIOH_BASE;
	stm32_common.gpio[8] = NULL;
	stm32_common.gpio[9] = NULL;
	stm32_common.gpio[10] = NULL;
	stm32_common.gpio[11] = NULL;
	stm32_common.gpio[12] = NULL;
	stm32_common.gpio[13] = GPION_BASE;
	stm32_common.gpio[14] = GPIOO_BASE;
	stm32_common.gpio[15] = GPIOP_BASE;
	stm32_common.gpio[16] = GPIOQ_BASE;

	/* Store reset flags and then clean them */
	stm32_common.resetFlags = (*(stm32_common.rcc + rcc_rsr) >> 21);
	*(stm32_common.rcc + rcc_rsr) = 1 << 16;

	/* Enable System configuration controller */
	_stm32_rccSetDevClock(dev_syscfg, 1);

	/* Enable power module */
	_stm32_rccSetDevClock(dev_pwr, 1);

	/* Default settings after reset - clock source is HSI at 64 MHz */
	stm32_common.cpuclk = 64 * 1000 * 1000;

	/* Disable all RCC interrupts */
	*(stm32_common.rcc + rcc_cier) = 0;

	hal_cpuDataMemoryBarrier();

	/* GPIO init */
	for (i = 0; i < sizeof(gpioDevs) / sizeof(gpioDevs[0]); ++i) {
		_stm32_rccSetDevClock(gpioDevs[i], 1);
	}

	hal_cpuDataMemoryBarrier();

#if WATCHDOG
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

#ifdef NDEBUG
	*(u32 *)0xE0042004 = 0;
#endif
}
