/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32L4x6 basic peripherals control functions
 *
 * Copyright 2017, 2019, 2020, 2021 Phoenix Systems
 * Author: Aleksander Kaminski, Jan Sikorski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include "stm32l4.h"

static struct {
	volatile u32 *rcc;
	volatile u32 *gpio[9];
	volatile u32 *scb;
	volatile u32 *pwr;
	volatile u32 *rtc;
	volatile u32 *syscfg;
	volatile u32 *iwdg;
	volatile u32 *flash;

	u32 cpuclk;

	u32 resetFlags;
} stm32_common;

/* clang-format off */
enum { ahb1_begin = pctl_dma1, ahb1_end = pctl_dma2d, ahb2_begin = pctl_gpioa, ahb2_end = pctl_rng,
	ahb3_begin = pctl_fmc, ahb3_end = pctl_qspi, apb1_1_begin = pctl_tim2, apb1_1_end = pctl_lptim1,
	apb1_2_begin = pctl_lpuart1, apb1_2_end = pctl_lptim2, apb2_begin = pctl_syscfg, apb2_end = pctl_dfsdm1,
	misc_begin = pctl_rtc, misc_end = pctl_rtc };


enum { rcc_cr = 0, rcc_icscr, rcc_cfgr, rcc_pllcfgr, rcc_pllsai1cfgr, rcc_pllsai2cfgr, rcc_cier, rcc_cifr,
	rcc_cicr, rcc_ahb1rstr = rcc_cicr + 2, rcc_ahb2rstr, rcc_ahb3rstr, rcc_apb1rstr1 = rcc_ahb3rstr + 2,
	rcc_apb1rstr2, rcc_apb2rstr, rcc_ahb1enr = rcc_apb2rstr + 2, rcc_ahb2enr, rcc_ahb3enr,
	rcc_apb1enr1 = rcc_ahb3enr + 2, rcc_apb1enr2, rcc_apb2enr, rcc_ahb1smenr = rcc_apb2enr + 2,
	rcc_ahb2smenr, rcc_ahb3smenr, rcc_apb1smenr1 = rcc_ahb3smenr + 2, rcc_apb1smenr2, rcc_apb2smenr,
	rcc_ccipr = rcc_apb2smenr + 2, rcc_bdcr = rcc_ccipr + 2, rcc_csr, rcc_crrcr, rcc_ccipr2 };


enum { gpio_moder = 0, gpio_otyper, gpio_ospeedr, gpio_pupdr, gpio_idr,
	gpio_odr, gpio_bsrr, gpio_lckr, gpio_afrl, gpio_afrh, gpio_brr, gpio_ascr };


enum { pwr_cr1 = 0, pwr_cr2, pwr_cr3, pwr_cr4, pwr_sr1, pwr_sr2, pwr_scr, pwr_pucra, pwr_pdcra, pwr_pucrb,
	pwr_pdcrb, pwr_pucrc, pwr_pdcrc, pwr_pucrd, pwr_pdcrd, pwr_pucre, pwr_pdcre, pwr_pucrf, pwr_pdcrf,
	pwr_pucrg, pwr_pdcrg, pwr_pucrh, pwr_pdcrh, pwr_pucri, pwr_pdcri };


enum { rtc_tr = 0, rtc_dr, rtc_cr, rtc_isr, rtc_prer, rtc_wutr, rtc_alrmar = rtc_wutr + 2, rtc_alrmbr, rtc_wpr,
	rtc_ssr, rtc_shiftr, rtc_tstr, rtc_tsdr, rtc_tsssr, rtc_calr, rtc_tampcr, rtc_alrmassr, rtc_alrmbssr, rtc_or,
	rtc_bkpr };


enum { scb_actlr = 2, scb_cpuid = 832, scb_icsr, scb_vtor, scb_aircr, scb_scr, scb_ccr, scb_shp1, scb_shp2,
	scb_shp3, scb_shcsr, scb_cfsr, scb_mmsr, scb_bfsr, scb_ufsr, scb_hfsr, scb_mmar, scb_bfar, scb_afsr,
	scb_cpacr = 866, scb_fpccr = 973, scb_fpcar, scb_fpdscr };


enum { syst_csr = 4, syst_rvr, syst_cvr, syst_calib };


enum { syscfg_memrmp = 0, syscfg_cfgr1, syscfg_exticr1, syscfg_exticr2, syscfg_exticr3, syscfg_exticr4,
	syscfg_scsr, syscfg_cfgr2, syscfg_swpr, syscfg_skr, syscfg_swpr2 };


enum { iwdg_kr = 0, iwdg_pr, iwdg_rlr, iwdg_sr, iwdg_winr };


enum { flash_acr = 0, flash_pdkeyr, flash_keyr, flash_optkeyr, flash_sr, flash_cr, flash_eccr,
	flash_optr = flash_eccr + 2, flash_pcrop1sr, flash_pcrop1er, flash_wrp1ar, flash_wrp1br,
	flash_pcrop2sr = flash_wrp1br + 5, flash_pcrop2er, flash_wrp2ar, flash_wrp2br };
/* clang-format on*/


/* RCC (Reset and Clock Controller) */


int _stm32_rccSetDevClock(unsigned int d, u32 hz)
{
	u32 t;

	hz = !!hz;

	if (d <= ahb1_end) {
		t = *(stm32_common.rcc + rcc_ahb1enr) & ~(1 << (d - ahb1_begin));
		*(stm32_common.rcc + rcc_ahb1enr) = t | (hz << (d - ahb1_begin));
	}
	else if (d <= ahb2_end) {
		t = *(stm32_common.rcc + rcc_ahb2enr) & ~(1 << (d - ahb2_begin));
		*(stm32_common.rcc + rcc_ahb2enr) = t | (hz << (d - ahb2_begin));
	}
	else if (d <= ahb3_end) {
		t = *(stm32_common.rcc + rcc_ahb3enr) & ~(1 << (d - ahb3_begin));
		*(stm32_common.rcc + rcc_ahb3enr) = t | (hz << (d - ahb3_begin));
	}
	else if (d <= apb1_1_end) {
		t = *(stm32_common.rcc + rcc_apb1enr1) & ~(1 << (d - apb1_1_begin));
		*(stm32_common.rcc + rcc_apb1enr1) = t | (hz << (d - apb1_1_begin));
	}
	else if (d <= apb1_2_end) {
		t = *(stm32_common.rcc + rcc_apb1enr2) & ~(1 << (d - apb1_2_begin));
		*(stm32_common.rcc + rcc_apb1enr2) = t | (hz << (d - apb1_2_begin));
	}
	else if (d <= apb2_end) {
		t = *(stm32_common.rcc + rcc_apb2enr) & ~(1 << (d - apb2_begin));
		*(stm32_common.rcc + rcc_apb2enr) = t | (hz << (d - apb2_begin));
	}
	else if (d == pctl_rtc) {
		t = *(stm32_common.rcc + rcc_bdcr) & ~(1 << 15);
		*(stm32_common.rcc + rcc_bdcr) = t | (hz << 15);
	}
	else
		return -1;

	hal_cpuDataMemoryBarrier();

	return 0;
}


int _stm32_rccGetDevClock(unsigned int d, u32 *hz)
{
	if (d <= ahb1_end)
		*hz = !!(*(stm32_common.rcc + rcc_ahb1enr) & (1 << (d - ahb1_begin)));
	else if (d <= ahb2_end)
		*hz = !!(*(stm32_common.rcc + rcc_ahb2enr) & (1 << (d - ahb2_begin)));
	else if (d <= ahb3_end)
		*hz = !!(*(stm32_common.rcc + rcc_ahb3enr) & (1 << (d - ahb3_begin)));
	else if (d <= apb1_1_end)
		*hz = !!(*(stm32_common.rcc + rcc_apb1enr1) & (1 << (d - apb1_1_begin)));
	else if (d <= apb1_2_end)
		*hz = !!(*(stm32_common.rcc + rcc_apb1enr2) & (1 << (d - apb1_2_begin)));
	else if (d <= apb2_end)
		*hz = !!(*(stm32_common.rcc + rcc_apb2enr) & (1 << (d - apb2_begin)));
	else if (d == pctl_rtc)
		*hz = !!(*(stm32_common.rcc + rcc_bdcr) & (1 << 15));
	else
		return -1;

	return 0;
}


int _stm32_rccSetCPUClock(u32 hz)
{
	u8 range;
	u32 t;

	if (hz <= 100 * 1000) {
		range = 0;
		hz = 100 * 1000;
	}
	else if (hz <= 200 * 1000) {
		range = 1;
		hz = 200 * 1000;
	}
	else if (hz <= 400 * 1000) {
		range = 2;
		hz = 400 * 1000;
	}
	else if (hz <= 800 * 1000) {
		range = 3;
		hz = 800 * 1000;
	}
	else if (hz <= 1000 * 1000) {
		range = 4;
		hz = 1000 * 1000;
	}
	else if (hz <= 2000 * 1000) {
		range = 5;
		hz = 2000 * 1000;
	}
	else if (hz <= 4000 * 1000) {
		range = 6;
		hz = 4000 * 1000;
	}
	else if (hz <= 8000 * 1000) {
		range = 7;
		hz = 8000 * 1000;
	}
	else if (hz <= 16000 * 1000) {
		range = 8;
		hz = 16000 * 1000;
	}
	/* TODO - we need to change flash wait states to handle below frequencies
	else if (hz <= 24000 * 1000) {
		range = 9;
		hz = 24000 * 1000;
	}
	else if (hz <= 32000 * 1000) {
		range = 10;
		hz = 32000 * 1000;
	}
	else if (hz <= 48000 * 1000) {
		range = 11;
		hz = 48000 * 1000;
	}
	*/
	else {
		/* We can use HSI, if higher frequency is needed */
		return -1;
	}

	if (hz > 6000 * 1000)
		_stm32_pwrSetCPUVolt(1);

	while (!(*(stm32_common.rcc + rcc_cr) & 2))
		;

	t = *(stm32_common.rcc + rcc_cr) & ~(0xf << 4);
	*(stm32_common.rcc + rcc_cr) = t | range << 4 | (1 << 3);
	hal_cpuDataMemoryBarrier();

	if (hz <= 6000 * 1000)
		_stm32_pwrSetCPUVolt(2);

	stm32_common.cpuclk = hz;

	return 0;
}


u32 _stm32_rccGetCPUClock(void)
{
	return stm32_common.cpuclk;
}


void _stm32_rccClearResetFlags(void)
{
	*(stm32_common.rcc + rcc_csr) |= 1 << 23;
}


/* RTC */


void _stm32_rtcUnlockRegs(void)
{
	/* Set DBP bit */
	*(stm32_common.pwr + pwr_cr1) |= 1 << 8;

	/* Unlock RTC */
	*(stm32_common.rtc + rtc_wpr) = 0x000000ca;
	*(stm32_common.rtc + rtc_wpr) = 0x00000053;
}


void _stm32_rtcLockRegs(void)
{
	/* Lock RTC */
	*(stm32_common.rtc + rtc_wpr) = 0x000000ff;

	/* Reset DBP bit */
	*(stm32_common.pwr + pwr_cr1) &= ~(1 << 8);
}


/* PWR */


void _stm32_pwrSetCPUVolt(u8 range)
{
	u32 t;

	if (range != 1 && range != 2)
		return;

	t = *(stm32_common.pwr + pwr_cr1) & ~(3 << 9);
	*(stm32_common.pwr + pwr_cr1) = t | (range << 9);

	/* Wait for VOSF flag */
	while (*(stm32_common.pwr + pwr_sr2) & (1 << 10))
		;
}


/* SysTick */


int _stm32_systickInit(u32 interval)
{
	u64 load = ((u64)interval * stm32_common.cpuclk) / 1000000;
	if (load > 0x00ffffff)
		return -1;

	*(stm32_common.scb + syst_rvr) = (u32)load;
	*(stm32_common.scb + syst_cvr) = 0;

	/* Enable systick */
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

	if (d > pctl_gpioi || pin > 15)
		return -1;

	base = stm32_common.gpio[d - pctl_gpioa];

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

	if (mode == 0x3)
		*(base + gpio_ascr) |= 1 << pin;
	else
		*(base + gpio_ascr) &= ~(1 << pin);

	return 0;
}


int _stm32_gpioSet(unsigned int d, u8 pin, u8 val)
{
	volatile u32 *base;
	u32 t;

	if (d > pctl_gpioi || pin > 15)
		return -1;

	base = stm32_common.gpio[d - pctl_gpioa];

	t = *(base + gpio_odr) & ~(!(u32)val << pin);
	*(base + gpio_odr) = t | !!(u32)val << pin;

	return 0;
}


int _stm32_gpioSetPort(unsigned int d, u16 val)
{
	volatile u32 *base;

	if (d > pctl_gpioi)
		return -1;

	base = stm32_common.gpio[d - pctl_gpioa];
	*(base + gpio_odr) = val;

	return 0;
}


int _stm32_gpioGet(unsigned int d, u8 pin, u8 *val)
{
	volatile u32 *base;

	if (d > pctl_gpioi || pin > 15)
		return -1;

	base = stm32_common.gpio[d - pctl_gpioa];
	*val = !!(*(base + gpio_idr) & (1 << pin));

	return 0;
}


int _stm32_gpioGetPort(unsigned int d, u16 *val)
{
	volatile u32 *base;

	if (d > pctl_gpioi)
		return -1;

	base = stm32_common.gpio[d - pctl_gpioa];
	*val = *(base + gpio_idr);

	return 0;
}


/* Flash banks */


int _stm32_getFlashBank(void)
{
	return ((*(stm32_common.syscfg + syscfg_memrmp) & (1 << 8)) != 0) ? 1 : 0;
}


void _stm32_switchFlashBank(int bank)
{
	if (bank == 0) {
		*(stm32_common.syscfg + syscfg_memrmp) &= ~(1 << 8);
	}
	else {
		*(stm32_common.syscfg + syscfg_memrmp) |= 1 << 8;
	}
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
	static const int gpio2pctl[] = { pctl_gpioa, pctl_gpiob, pctl_gpioc,
		pctl_gpiod, pctl_gpioe, pctl_gpiof, pctl_gpiog, pctl_gpioh, pctl_gpioi };

	/* Base addresses init */
	stm32_common.rcc = (void *)0x40021000;
	stm32_common.pwr = (void *)0x40007000;
	stm32_common.scb = (void *)0xe000e000;
	stm32_common.rtc = (void *)0x40002800;
	stm32_common.syscfg = (void *)0x40010000;
	stm32_common.iwdg = (void *)0x40003000;
	stm32_common.gpio[0] = (void *)0x48000000; /* GPIOA */
	stm32_common.gpio[1] = (void *)0x48000400; /* GPIOB */
	stm32_common.gpio[2] = (void *)0x48000800; /* GPIOC */
	stm32_common.gpio[3] = (void *)0x48000c00; /* GPIOD */
	stm32_common.gpio[4] = (void *)0x48001000; /* GPIOE */
	stm32_common.gpio[5] = (void *)0x48001400; /* GPIOF */
	stm32_common.gpio[6] = (void *)0x48001800; /* GPIOG */
	stm32_common.gpio[7] = (void *)0x48001c00; /* GPIOH */
	stm32_common.gpio[8] = (void *)0x48002000; /* GPIOI */
	stm32_common.flash = (void *)0x40022000;

	/* Enable System configuration controller */
	_stm32_rccSetDevClock(pctl_syscfg, 1);

	/* Enable power module */
	_stm32_rccSetDevClock(pctl_pwr, 1);

	_stm32_rccSetCPUClock(16 * 1000 * 1000);

	/* Disable all interrupts */
	*(stm32_common.rcc + rcc_cier) = 0;

	hal_cpuDataMemoryBarrier();

	/* GPIO init */
	for (i = 0; i < sizeof(stm32_common.gpio) / sizeof(stm32_common.gpio[0]); ++i)
		_stm32_rccSetDevClock(gpio2pctl[i], 1);

	hal_cpuDataMemoryBarrier();

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

#ifdef NDEBUG
	*(u32 *)0xE0042004 = 0;
#endif
}
