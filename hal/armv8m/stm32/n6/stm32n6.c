/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32N6 basic peripherals control functions
 *
 * Copyright 2017, 2019, 2020, 2021, 2025 Phoenix Systems
 * Author: Aleksander Kaminski, Jan Sikorski, Hubert Buczynski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <board_config.h>
#include "stm32n6.h"
#include "stm32n6_regs.h"
#include "otp.h"


#ifndef USE_HSE_CLOCK_SOURCE
#define USE_HSE_CLOCK_SOURCE 1
#endif

/* By default chose the higher voltage range (3.3 V), which is the safer option.
 * If there is a mismatch the performance will be degraded (slower GPIO pin performance),
 * but no damage to hardware will occur. */
#ifndef VDDIO2_RANGE_MV
#define VDDIO2_RANGE_MV 3300
#else
#if (VDDIO2_RANGE_MV != 1800) && (VDDIO2_RANGE_MV != 3300)
#error "Invalid voltage range selected"
#endif
#endif

#ifndef VDDIO3_RANGE_MV
#define VDDIO3_RANGE_MV 3300
#else
#if (VDDIO3_RANGE_MV != 1800) && (VDDIO3_RANGE_MV != 3300)
#error "Invalid voltage range selected"
#endif
#endif

#ifndef VDDIO4_RANGE_MV
#define VDDIO4_RANGE_MV 3300
#else
#if (VDDIO4_RANGE_MV != 1800) && (VDDIO4_RANGE_MV != 3300)
#error "Invalid voltage range selected"
#endif
#endif

#ifndef VDDIO5_RANGE_MV
#define VDDIO5_RANGE_MV 3300
#else
#if (VDDIO5_RANGE_MV != 1800) && (VDDIO5_RANGE_MV != 3300)
#error "Invalid voltage range selected"
#endif
#endif

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
#define RAMCFG_BASE ((void *)0x52023000)
#define ICB_BASE    ((void *)0xe000e000)
#define BSEC_BASE   ((void *)0x56009000)

static struct {
	volatile u32 *rcc;
	volatile u32 *gpio[17];
	volatile u32 *icb;
	volatile u32 *pwr;
	volatile u32 *rtc;
	volatile u32 *syscfg;
	volatile u32 *iwdg;
	volatile u32 *ramcfg;
	volatile u32 *bsec;

	u32 cpuclk;
	u32 perclk;

	u32 resetFlags;
} stm32_common;


enum {
	ipclk_persel_hsi_ck = 0,
	ipclk_persel_msi_ck,
	ipclk_persel_hse_ck,
	ipclk_persel_ic19_ck,
	ipclk_persel_ic5_ck,
	ipclk_persel_ic10_ck,
	ipclk_persel_ic15_ck,
	ipclk_persel_ic20_ck,
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
	icb_systick_csr = 4,
	icb_systick_rvr,
	icb_systick_cvr,
	icb_systick_calib,
};


typedef struct {
	u8 src;
	u8 pre_div;
	u16 mul;
	u32 mul_frac;
	u8 post_div1;
	u8 post_div2;
	char bypass;
} pll_config_t;


enum pll_srcs {
	pll_src_hsi_ck = 0,
	pll_src_msi_ck,
	pll_src_hse_ck,
	pll_src_i2s_ckin,
};


/* 0 - default voltage range requested, 1 - lower voltage range requested */
static const int stm32_voltageRangeConfig[pwr_supplies_count] = {
	[pwr_supply_vddio4] = (VDDIO4_RANGE_MV == 1800) ? 1 : 0,
	[pwr_supply_vddio5] = (VDDIO5_RANGE_MV == 1800) ? 1 : 0,
	[pwr_supply_vddio2] = (VDDIO2_RANGE_MV == 1800) ? 1 : 0,
	[pwr_supply_vddio3] = (VDDIO3_RANGE_MV == 1800) ? 1 : 0,
};

#define NUM_PLLS  4
#define NUM_ICLKS 20


unsigned int hal_getBootReason(void)
{
	return stm32_common.resetFlags;
}


/* RCC (Reset and Clock Controller) */


static int _stm32_configurePLL(unsigned int pll, const pll_config_t *config)
{
	volatile u32 *reg;
	u32 v;
	u32 on_bit;
	switch (pll) {
		case 1:
			reg = (stm32_common.rcc + rcc_pll1cfgr1);
			break;

		case 2:
			reg = (stm32_common.rcc + rcc_pll2cfgr1);
			break;

		case 3:
			reg = (stm32_common.rcc + rcc_pll3cfgr1);
			break;

		case 4:
			reg = (stm32_common.rcc + rcc_pll4cfgr1);
			break;

		default:
			return -1;
	}

	on_bit = 1 << (7 + pll);

	v = *(reg + 0);
	v |= (1 << 27); /* Bypass PLL */
	*(reg + 0) = v;
	hal_cpuDataSyncBarrier();

	*(stm32_common.rcc + rcc_ccr) = on_bit; /* Clear PLLxON */
	hal_cpuDataSyncBarrier();

	if (config->bypass != 0) {
		/* User wants to bypass PLL - nothing else to do */
		return 0;
	}

	v &= 0x8C0000FF; /* Keep only reserved bits and bypass bit */
	v |= (config->src & 0x7) << 28;
	v |= (config->pre_div & 0x3f) << 20;
	v |= (config->mul & 0xfff) << 8;
	*(reg + 0) = v;

	v = *(reg + 1);
	v &= 0xff000000;
	v |= config->mul_frac & 0xffffff;
	*(reg + 1) = v;

	v = *(reg + 2);
	v |= (1 << 30);     /* Set PDIVEN */
	v &= ~(0x3f << 24); /* Clear PDIV1 and PDIV2 */
	v |= (config->post_div1 & 0x7) << 27;
	v |= (config->post_div2 & 0x7) << 24;
	v &= ~0xf; /* Clear spread spectrum/fractional PLL bits */
	if (config->mul_frac != 0) {
		v |= 0xf; /* MODDSEN = 1, MODSSDIS = 1, DACEN = 1, MODSSRST = 1 */
	}
	else {
		v |= 0x5; /* MODDSEN = 0, MODSSDIS = 1, DACEN = 0, MODSSRST = 1 */
	}

	*(reg + 2) = v;
	hal_cpuDataSyncBarrier();

	*(stm32_common.rcc + rcc_csr) = on_bit; /* Set PLLxON */
	while ((*(stm32_common.rcc + rcc_sr) & on_bit) == 0) {
		/* Wait for PLL lock */
	}

	hal_cpuDataSyncBarrier();
	*(reg + 0) &= ~(1 << 27); /* Remove PLL bypass */
	return 0;
}


static int _stm32_configureICLK(unsigned int ic, unsigned int src_pll, unsigned int divider)
{
	volatile u32 *reg;
	u32 v;
	if ((ic < 1) || (ic > 20) || (src_pll < 1) || (src_pll > 4) || (divider < 1) || (divider > 256)) {
		return -1;
	}

	reg = (stm32_common.rcc + rcc_ic1cfgr + ic - 1);
	v = *reg;
	v &= 0xcf00ffff;
	v |= (src_pll - 1) << 28;
	v |= (divider - 1) << 16;
	*reg = v;
	hal_cpuDataSyncBarrier();
	*(stm32_common.rcc + rcc_divensr) = 1 << (ic - 1);
	hal_cpuDataSyncBarrier();
	return 0;
}


/* Clock system on STM32N6:
 *           ┌─┐        ┌─┐
 *      HSI ─┤M├─ PLL1 ─┤ ├─ IC1  → SYSA_CLK (for CPU)
 *      MSI ─┤U├─ PLL2 ─┤M├─ IC2  → SYSB_CLK (for internal buses)
 *      HSE ─┤X├─ PLL3 ─┤U├─ IC6  → SYSC_CLK (for NPU)
 * I2S_CKIN ─┤ ├─ PLL4 ─┤X├─ IC11 → SYSD_CLK (for AXISRAM)
 *           └─┘        │ ├─ ICx  → other peripherals (ipclk)
 *                      └─┘
 * HSI and MSI are internal RC oscillators (64 MHz and 16 MHz).
 * HSE is external crystal oscillator, 48 MHz on NUCLEO board.
 * I2S_CKIN is external input, intended for use with I2S peripheral.
 * IC1 ~ IC20 are additional dividers coming out of the PLLs.
 */
static void _stm32_configureClocks(void)
{
#if USE_HSE_CLOCK_SOURCE
	/* On NUCLEO board HSE is 48 MHz */
	static const pll_config_t plls[NUM_PLLS] = {
		{
			/* PLL1: (48 MHz * 50) / 2 => 1200 MHz */
			.src = pll_src_hse_ck,
			.pre_div = 1,
			.mul = 50,
			.mul_frac = 0,
			.post_div1 = 2,
			.post_div2 = 1,
			.bypass = 0,
		},
		{
			/* PLL2: (48 MHz / 3) * 100  => 1600 MHz */
			.src = pll_src_hse_ck,
			.pre_div = 3,
			.mul = 100,
			.mul_frac = 0,
			.post_div1 = 1,
			.post_div2 = 1,
			.bypass = 0,
		},
		{
			/* PLL3: 48 MHz * 25 => 1200 MHz */
			.src = pll_src_hse_ck,
			.pre_div = 1,
			.mul = 25,
			.mul_frac = 0,
			.post_div1 = 1,
			.post_div2 = 1,
			.bypass = 0,
		},
		{
			/* PLL4: (48 MHz * 50) / 3 => 800 MHz */
			.src = pll_src_hse_ck,
			.pre_div = 1,
			.mul = 50,
			.mul_frac = 0,
			.post_div1 = 3,
			.post_div2 = 1,
			.bypass = 0,
		},
	};
#else
	static const pll_config_t plls[NUM_PLLS] = {
		{
			/* PLL1: (64 MHz / 4) * 75 => 1200 MHz */
			.src = pll_src_hsi_ck,
			.pre_div = 4,
			.mul = 75,
			.mul_frac = 0,
			.post_div1 = 1,
			.post_div2 = 1,
			.bypass = 0,
		},
		{
			/* PLL2: 64 MHz * 25  => 1600 MHz */
			.src = pll_src_hsi_ck,
			.pre_div = 1,
			.mul = 25,
			.mul_frac = 0,
			.post_div1 = 1,
			.post_div2 = 1,
			.bypass = 0,
		},
		{
			/* PLL3: (64 MHz / 4) * 75 => 1200 MHz */
			.src = pll_src_hsi_ck,
			.pre_div = 4,
			.mul = 75,
			.mul_frac = 0,
			.post_div1 = 1,
			.post_div2 = 1,
			.bypass = 0,
		},
		{
			/* PLL4: (64 MHz * 25) / 2 => 800 MHz */
			.src = pll_src_hsi_ck,
			.pre_div = 1,
			.mul = 25,
			.mul_frac = 0,
			.post_div1 = 2,
			.post_div2 = 1,
			.bypass = 0,
		},
	};
#endif

	static const struct {
		u8 iclk;
		u8 pll;
		u16 div;
	} iclks[] = {
		{ .iclk = 1, .pll = 1, .div = 2 },  /* 1200 / 2 => 600 MHz (CPU) */
		{ .iclk = 2, .pll = 1, .div = 3 },  /* 1200 / 3 => 400 MHz (buses) */
		{ .iclk = 3, .pll = 1, .div = 3 },  /* 1200 / 3 => 400 MHz (XSPI) */
		{ .iclk = 6, .pll = 2, .div = 2 },  /* 1600 / 2 => 800 MHz (NPU) */
		{ .iclk = 11, .pll = 1, .div = 3 }, /* 1200 / 3 => 400 MHz (AXISRAM) */
	};

	int i;
	u32 v;

	*(stm32_common.rcc + rcc_ccr) = (1 << 4); /* Turn off HSE */

	/* Configure HSE */
	v = *(stm32_common.rcc + rcc_hsecfgr);
	v &= ~(1 << 16); /* HSEEXT off */
	v &= ~(1 << 15); /* HSEBYP off */
	v &= ~(0xf << 11);
	v |= (1 << 11); /* Use HSI div 2 in case of oscillator failure */
	v |= (1 << 6);  /* Set hse_div2_osc_ck to hse_osc_ck/2 - this clock may be used for USB */
	*(stm32_common.rcc + rcc_hsecfgr) = v;

	*(stm32_common.rcc + rcc_csr) = (1 << 4); /* Turn on HSE */
	while ((*(stm32_common.rcc + rcc_sr) & (1 << 4)) == 0) {
		/* Wait for HSE ready */
	}

	/* Activate CSS (clock security system) */
	v = *(stm32_common.rcc + rcc_hsecfgr);
	v |= (1 << 7);  /* Activate CSS for HSE */
	v |= (1 << 10); /* Allow CSS to bypass HSE */
	*(stm32_common.rcc + rcc_hsecfgr) = v;

	for (i = 0; i < NUM_PLLS; i++) {
		_stm32_configurePLL(i + 1, &plls[i]);
	}

	for (i = 0; i < (sizeof(iclks) / sizeof(iclks[0])); i++) {
		_stm32_configureICLK(iclks[i].iclk, iclks[i].pll, iclks[i].div);
	}

	v = *(stm32_common.rcc + rcc_cfgr2);
	/* Set all PPREx to 1, HPRE to 2, TIMPRE to 1 */
	v &= ~((0x7 << 0) | (0x7 << 4) | (0x7 << 12) | (0x7 << 16) | (0x7 << 20) | (0x3 << 24));
	v |= (0 << 0) | (0 << 4) | (0 << 12) | (0 << 16) | (1 << 20) | (0 << 24);
	*(stm32_common.rcc + rcc_cfgr2) = v;

	v = *(stm32_common.rcc + rcc_cfgr1);
	v |= (3 << 24); /* Set ic2_ck -> sysb_ck, ic6_ck -> sysc_ck, ic11_ck -> sysd_ck */
	v |= (3 << 16); /* Set ic1_ck -> sysa_ck */
	*(stm32_common.rcc + rcc_cfgr1) = v;

	stm32_common.cpuclk = 600 * 1000 * 1000;

	/* Set HSE or HSI as source for per_ck.
	 * NOTE: DO NOT set this to any clock that depends on a PLL.
	 * Doing so will cause a hang in BootROM after reset.
	 * TODO: investigate why this happens, it shouldn't be like this */
#if USE_HSE_CLOCK_SOURCE
	_stm32_rccSetIPClk(ipclk_persel, ipclk_persel_hse_ck);
	stm32_common.perclk = 48 * 1000 * 1000;
#else
	_stm32_rccSetIPClk(ipclk_persel, ipclk_persel_hsi_ck);
	stm32_common.perclk = 64 * 1000 * 1000;
#endif
	_stm32_rccSetDevClock(dev_per, 1);
}


static int _stm32_getDevClockRegShift(unsigned int dev, unsigned int *shift_out)
{
	unsigned int reg = dev / 32;
	if (reg > (rcc_apb5enr - rcc_busenr)) {
		return -1;
	}

	*shift_out = dev % 32;
	return reg;
}


int _stm32_rccSetDevClock(unsigned int dev, u32 status)
{
	u32 shift;
	int reg, set_clear, lowpower;

	reg = _stm32_getDevClockRegShift(dev, &shift);
	if (reg < 0) {
		return -1;
	}

	set_clear = (status == 0) ? rcc_busencr : rcc_busensr;
	*(stm32_common.rcc + reg + set_clear) = 1 << shift;
	hal_cpuDataSyncBarrier();
	(void)*(stm32_common.rcc + reg + rcc_busenr);
	/* Enable selected peripheral in Sleep mode
	 * TODO: we may want to have separate configuration for it in the future */
	lowpower = (status == 0) ? rcc_buslpencr : rcc_buslpensr;
	*(stm32_common.rcc + reg + lowpower) = 1 << shift;

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


int _stm32_rccDevReset(unsigned int dev, u32 status)
{
	u32 reg = dev / 32, shift = dev % 32;
	int set_clear;

	if (reg > (rcc_apb5rstr - rcc_busrstr)) {
		return -1;
	}

	set_clear = (status == 0) ? rcc_busrstcr : rcc_busrstsr;
	*(stm32_common.rcc + reg + set_clear) = 1 << shift;
	hal_cpuDataSyncBarrier();
	(void)*(stm32_common.rcc + reg + rcc_busrstr);

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


int _stm32_rccSetClockIC(u32 ic, u32 pll_src, u32 divider)
{
	const u32 offset = rcc_ic1cfgr + ic - 1;
	if ((ic == 0) || (ic > 20) || (pll_src == 0) || (pll_src > 4) || (divider == 0) || (divider > 256)) {
		return -1;
	}

	*(stm32_common.rcc + offset) = ((pll_src - 1) << 28) | ((divider - 1) << 16);
	return 0;
}


int _stm32_rccSetIPClk(unsigned int ipclk, u8 setting)
{
	static const struct {
		u16 reg_offs;
		u8 mask;
		u8 shift;
	} ipclk_lookup[] = {
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

	if (range != 0 && range != 1) {
		return;
	}

	t = *(stm32_common.pwr + pwr_voscr) & ~1;
	*(stm32_common.pwr + pwr_voscr) = t | range;

	while ((*(stm32_common.pwr + pwr_voscr) & (1 << 1)) == 0) {
		/* Wait for VOSRDY flag */
	}
}


int _stm32_pwrSupplyOperation(unsigned int supply, unsigned int operation, int status)
{
	static const struct {
		u16 reg_offs;
		s8 bits[pwr_supplies_count];
	} supply_lookup[] = {
		[pwr_supply_vddio4] = {
			.reg_offs = pwr_svmcr1,
			.bits = {
				[pwr_supply_op_monitor_enable] = 0,
				[pwr_supply_op_valid] = 8,
				[pwr_supply_op_ready] = 16,
				[pwr_supply_op_range_sel] = 24,
				[pwr_supply_op_range_sel_standby] = 25,
				[pwr_supply_op_get_range_sel] = 24,
			},
		},
		[pwr_supply_vddio5] = {
			.reg_offs = pwr_svmcr2,
			.bits = {
				[pwr_supply_op_monitor_enable] = 0,
				[pwr_supply_op_valid] = 8,
				[pwr_supply_op_ready] = 16,
				[pwr_supply_op_range_sel] = 24,
				[pwr_supply_op_range_sel_standby] = 25,
				[pwr_supply_op_get_range_sel] = 24,
			},
		},
		[pwr_supply_vddio2] = {
			.reg_offs = pwr_svmcr3,
			.bits = {
				[pwr_supply_op_monitor_enable] = 0,
				[pwr_supply_op_valid] = 8,
				[pwr_supply_op_ready] = 16,
				[pwr_supply_op_range_sel] = 25,
				[pwr_supply_op_range_sel_standby] = -1,
				[pwr_supply_op_get_range_sel] = 25,
			},
		},
		[pwr_supply_vddio3] = {
			.reg_offs = pwr_svmcr3,
			.bits = {
				[pwr_supply_op_monitor_enable] = 1,
				[pwr_supply_op_valid] = 9,
				[pwr_supply_op_ready] = 17,
				[pwr_supply_op_range_sel] = 26,
				[pwr_supply_op_range_sel_standby] = -1,
				[pwr_supply_op_get_range_sel] = 26,
			},
		},
		[pwr_supply_vusb33] = {
			.reg_offs = pwr_svmcr3,
			.bits = {
				[pwr_supply_op_monitor_enable] = 2,
				[pwr_supply_op_valid] = 10,
				[pwr_supply_op_ready] = 18,
				[pwr_supply_op_range_sel] = -1,
				[pwr_supply_op_range_sel_standby] = -1,
				[pwr_supply_op_get_range_sel] = -1,
			},
		},
		[pwr_supply_vdda] = {
			.reg_offs = pwr_svmcr3,
			.bits = {
				[pwr_supply_op_monitor_enable] = 4,
				[pwr_supply_op_valid] = 12,
				[pwr_supply_op_ready] = 20,
				[pwr_supply_op_range_sel] = -1,
				[pwr_supply_op_range_sel_standby] = -1,
				[pwr_supply_op_get_range_sel] = -1,
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

	if ((operation == pwr_supply_op_ready) || (operation == pwr_supply_op_get_range_sel)) {
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


int _stm32_pwrSupplyValidateRange(unsigned int supply)
{
	int ret;
	if (supply >= pwr_supplies_count) {
		return -1;
	}

	ret = _stm32_pwrSupplyOperation(supply, pwr_supply_op_get_range_sel, 1);
	if (ret < 0) {
		/* Error here means that the selected power supply does not have configurable range;
		 * so if it's not configurable, it is always configured correctly. */
		return 0;
	}

	return (ret == stm32_voltageRangeConfig[supply]) ? 0 : -1;
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

	while (_stm32_pwrSupplyOperation(supply, pwr_supply_op_ready, 0) == 0) {
		/* Wait */
	}

	if (stm32_voltageRangeConfig[supply] != 0) {
		/* IMPORTANT NOTE: Here we may configure a power supply to be in the lower voltage range.
		 * This will only take effect if the appropriate bits in OTP word 124 (HCONF1) are set to 1. */
		_stm32_pwrSupplyOperation(supply, pwr_supply_op_range_sel, 1);
		_stm32_pwrSupplyOperation(supply, pwr_supply_op_range_sel_standby, 1);
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

	*(stm32_common.icb + icb_systick_rvr) = load;
	*(stm32_common.icb + icb_systick_cvr) = 0;

	/* Enable counter, systick interrupt and set source to processor clock */
	*(stm32_common.icb + icb_systick_csr) |= 0x7;

	return 0;
}


void _stm32_systickDone(void)
{
	*(stm32_common.icb + icb_systick_csr) = 0;
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


static void _stm32_initSRAM(void)
{
	/* Enable built-in memories that aren't turned on through reset:
	 * AXISRAM3 ~ 6, AHBSRAM1 ~ 2, VENCRAM, BKPSRAM */
	static const struct {
		int crOffs;
		int erkeyrOffs;
	} srams[] = {
		{ .crOffs = ramcfg_axisram3cr, .erkeyrOffs = ramcfg_axisram3erkeyr },
		{ .crOffs = ramcfg_axisram4cr, .erkeyrOffs = ramcfg_axisram4erkeyr },
		{ .crOffs = ramcfg_axisram5cr, .erkeyrOffs = ramcfg_axisram5erkeyr },
		{ .crOffs = ramcfg_axisram6cr, .erkeyrOffs = ramcfg_axisram6erkeyr },
		{ .crOffs = ramcfg_ahbsram1cr, .erkeyrOffs = ramcfg_ahbsram1erkeyr },
		{ .crOffs = ramcfg_ahbsram2cr, .erkeyrOffs = ramcfg_ahbsram2erkeyr },
		{ .crOffs = ramcfg_vencramcr, .erkeyrOffs = ramcfg_vencramerkeyr },
		{ .crOffs = ramcfg_bkpsramcr, .erkeyrOffs = -1 }, /* Don't erase backup RAM on startup */
	};
	unsigned int i;

	/* Enable all memories in RCC */
	for (i = dev_axisram3; i <= dev_vencram; i++) {
		_stm32_rccSetDevClock(i, 1);
	}

	_stm32_rccSetDevClock(dev_ramcfg, 1);
	for (i = 0; i < sizeof(srams) / sizeof(srams[0]); i++) {
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
	u32 i, v;
	static const int gpioDevs[] = { dev_gpioa, dev_gpiob, dev_gpioc, dev_gpiod, dev_gpioe, dev_gpiof, dev_gpiog,
		dev_gpioh, dev_gpion, dev_gpioo, dev_gpiop, dev_gpioq };

	/* Base addresses init */
	stm32_common.iwdg = IWDG_BASE;
	stm32_common.pwr = PWR_BASE;
	stm32_common.ramcfg = RAMCFG_BASE;
	stm32_common.rcc = RCC_BASE;
	stm32_common.rtc = RTC_BASE;
	stm32_common.syscfg = SYSCFG_BASE;
	stm32_common.icb = ICB_BASE;
	stm32_common.bsec = BSEC_BASE;
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

	/* Store reset flags and then clear them */
	stm32_common.resetFlags = (*(stm32_common.rcc + rcc_rsr) >> 21);
	*(stm32_common.rcc + rcc_rsr) = 1 << 16;

	/* Enables all buses */
	for (i = dev_aclkn; i <= dev_apb5; i++) {
		_stm32_rccSetDevClock(i, 1);
	}

	_stm32_rccSetDevClock(dev_bsec, 1);
#ifndef NDEBUG
	/* Write magic values into registers to unlock debugging in flash or serial boot mode.
	 * In dev boot mode debugging is unlocked by default. */
	*(stm32_common.bsec + bsec_ap_unlock) = 0xb4;
	v = *(stm32_common.bsec + bsec_dbgcr);
	v &= 0xff;
	v |= 0xb4b4b400;
	*(stm32_common.bsec + bsec_dbgcr) = v;
#else
	(void)v;
#endif

	/* Enable System configuration controller */
	_stm32_rccSetDevClock(dev_syscfg, 1);
	_stm32_rccSetDevClock(dev_vrefbuf, 1);

	/* Enable power module */
	_stm32_rccSetDevClock(dev_pwr, 1);

	_stm32_rccSetDevClock(dev_per, 0);

	_stm32_initSRAM();

	/* Enable independent power supplies for GPIOs. */
	_stm32_initPowerSupply(pwr_supply_vddio2); /* PO[5:0], PP[15:0] */
	_stm32_initPowerSupply(pwr_supply_vddio3); /* PN[12:0] */
	_stm32_initPowerSupply(pwr_supply_vddio4); /* PC[1], PC[12:6], PH[9:2] */
	_stm32_initPowerSupply(pwr_supply_vddio5); /* PC[0], PC[5:2], PE[4] */
	/* Enable independent power supply for ADCs */
	_stm32_initPowerSupply(pwr_supply_vdda);
	/* Enable independent power supply for USB */
	_stm32_initPowerSupply(pwr_supply_vusb33);

	/* Disable all RCC interrupts */
	*(stm32_common.rcc + rcc_cier) = 0;

	_stm32_configureClocks();

	hal_cpuDataMemoryBarrier();

	/* GPIO init */
	for (i = 0; i < sizeof(gpioDevs) / sizeof(gpioDevs[0]); ++i) {
		_stm32_rccSetDevClock(gpioDevs[i], 1);
	}

	hal_cpuDataMemoryBarrier();

	otp_init();

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
