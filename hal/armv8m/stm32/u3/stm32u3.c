/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32U3 basic peripherals control functions
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
#include <lib/errno.h>
#include "stm32u3.h"


#define GPIOA_BASE ((void *)0x42020000)
#define GPIOB_BASE ((void *)0x42020400)
#define GPIOC_BASE ((void *)0x42020800)
#define GPIOD_BASE ((void *)0x42020c00)
#define GPIOE_BASE ((void *)0x42021000)
#define GPIOF_BASE ((void *)0x42021400)
#define GPIOG_BASE ((void *)0x42021800)
#define GPIOH_BASE ((void *)0x42021c00)

#define IWDG_BASE   ((void *)0x40003000)
#define PWR_BASE    ((void *)0x40030800)
#define RCC_BASE    ((void *)0x40030c00)
#define SYSCFG_BASE ((void *)0x40040400)
#define RAMCFG_BASE ((void *)0x40026000)
#define ICB_BASE    ((void *)0xe000e000)
#define FLASH_BASE  ((void *)0x40022000)

/* OPTWERR | PGSERR | SIZERR | PGAERR | WRPERR | PROGERR | OPERR */
#define FLASH_ERROR_MASK ((1 << 13) | (1 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 1))
#define FLASH_KEY1       0x45670123
#define FLASH_KEY2       0xcdef89ab
#define FLASH_OPTKEY1    0x08192a3b
#define FLASH_OPTKEY2    0x4c5d6e7f

static struct {
	volatile u32 *rcc;
	volatile u32 *gpio[8];
	volatile u32 *icb;
	volatile u32 *pwr;
	volatile u32 *syscfg;
	volatile u32 *iwdg;
	volatile u32 *ramcfg;
	volatile u32 *flash;

	u32 cpuclk;
	u32 hsiRefs;

	u32 resetFlags;
} stm32_common;


/* Systick registers */
enum {
	icb_syst_csr = 0x4,
	icb_syst_rvr,
	icb_syst_cvr,
	icb_syst_calib,
};

/* Clock owners */
typedef enum {
	clko_sysclk = 0,
	clko_booster,
} clock_owner_t;


/* MSIS clock range mappings */
typedef struct {
	u32 hz;
	int range;
} range_mapping_t;

#define FREQUENCY_RANGE_COUNT 7
static const range_mapping_t frequency_ranges[FREQUENCY_RANGE_COUNT] = {
	{ 3000000, 7 }, { 6000000, 6 }, { 12000000, 5 }, /* MSIRC1 */
	{ 16000000, -1 },                                /* HSI */
	{ 24000000, 4 },                                 /* MSIRC1 */
	{ 48000000, 1 }, { 96000000, 0 },                /* MSIRC0 */
};

#define BOOSTEN             (1 << 8)
#define VOLTAGE_RANGE_COUNT 3
static const range_mapping_t voltage_ranges[VOLTAGE_RANGE_COUNT] = {
	{ 24000000, 2 },
	{ 48000000, 2 - BOOSTEN },
	{ 96000000, 1 - BOOSTEN },
};

unsigned int hal_getBootReason(void)
{
	return stm32_common.resetFlags;
}


/*
 * HSE is external crystal oscillator (4 to 50 MHz).
 * HSI (16 MHz) and MSIS (3 to 96 MHz, 12 MHz on reset) are internal RC
 * oscillators. MSIS selects between MSIRC0 (96 MHz) and MSIRC1 (24 MHz),
 * applying prescaler of 1, 2, 4, or 8.
 *
 * On STM32U3, allowed system clock sources are HSE, HSI16, and MSIS.
 *
 * Peripheral bus clock is derived as follows:
 *
 *
 *    ┌─────┐   HSE ┌────┐
 *    │ OSC ├───────┤    │
 *    └─────┘   HSI │    │ SYSCLK               HCLK
 *     16 MHz├──────┤ SW ├───/HPRE───┬────────────────┤CPU, AHB1, AHB2
 *    ┌─────┐  MSIS │    │           │          PCLK1
 *    │ RC0 ├───────┤    │           ├───/PPRE1───────┤APB1
 *    │ MSI │  MSIK └────┘           │          PCLK2
 *    │ RC1 ├──────┐                 ├───/PPRE2───────┤APB2
 *    └─────┘      ┴                 │          PCLK3
 *           (opt) SPI, I2C, CAN...  └───/PPRE3───────┤APB3
 */
static void _stm32_configureClocks(void)
{
	u32 v;

	/* Disable HSE */
	*(stm32_common.rcc + rcc_cr) &= ~(1 << 16); /* HSEON off */

	/* Select the system clock frequency */
	_stm32_rccSetCPUClock(24 * 1000 * 1000);

	/* Set peripheral bus clock prescalers */
	v = *(stm32_common.rcc + rcc_cfgr2);
	v &= ~((0x7 << 8) | (0x7 << 4) | (0xF << 0)); /* Clear PPRE2, PPRE1, HPRE */
	v |= (0x4 << 8) | (0x4 << 4) | (0x7 << 0);    /* Set PPRE1 ~ 2 to 2, HPRE to 1 */
	*(stm32_common.rcc + rcc_cfgr2) = v;

	v = *(stm32_common.rcc + rcc_cfgr3);
	v &= ~(0x7 << 4); /* Clear PPRE3 */
	v |= (0x4 << 4);  /* Set PPRE3 to 2 */
	*(stm32_common.rcc + rcc_cfgr3) = v;
}


static const range_mapping_t *_stm32_findRange(u32 hz, const range_mapping_t *ranges, int length)
{
	int i;

	for (i = 0; i < length; i++) {
		if (hz <= ranges[i].hz) {
			return ranges + i;
		}
	}

	return NULL;
}


static void _stm32_acquireHsi(clock_owner_t owner)
{
	if (stm32_common.hsiRefs == 0) {
		*(stm32_common.rcc + rcc_cr) |= (1 << 11); /* Set HSION */
		hal_cpuDataMemoryBarrier();

		while ((*(stm32_common.rcc + rcc_cr) & (1 << 13)) == 0) {
			/* Wait for HSIRDY */
		}
	}

	stm32_common.hsiRefs |= 1 << owner;
}


static void _stm32_releaseHsi(clock_owner_t owner)
{
	if (stm32_common.hsiRefs == 0) {
		return;
	}

	stm32_common.hsiRefs &= ~(1 << owner);

	if (stm32_common.hsiRefs == 0) {
		*(stm32_common.rcc + rcc_cr) &= ~(1 << 11); /* Clear HSION */
		hal_cpuDataMemoryBarrier();

		while ((*(stm32_common.rcc + rcc_cr) & (1 << 13)) != 0) {
			/* Wait for HSIRDY */
		}
	}
}


int _stm32_rccSetCPUClock(u32 hz)
{
	u32 v, clock_source = 0;
	const range_mapping_t *frequency_range, *voltage_range;

	if (!(frequency_range = _stm32_findRange(hz, frequency_ranges, FREQUENCY_RANGE_COUNT))) {
		return -ERANGE;
	}

	if (!(voltage_range = _stm32_findRange(hz, voltage_ranges, VOLTAGE_RANGE_COUNT))) {
		return -ERANGE;
	}

	/* Step up */
	if (voltage_range->range < _stm32_pwrGetCPUVolt()) {
		_stm32_pwrSetCPUVolt(voltage_range->range);
	}

	if (frequency_range->range >= 0) {
		/* Configure MSIS */
		v = *(stm32_common.rcc + rcc_icscr1) & ~(0x7 << 29);
		v |= ((frequency_range->range & 0x7) << 29) | (1 << 23); /* Set MSISSEL | MSISDIV | MSIRGSEL */
		*(stm32_common.rcc + rcc_icscr1) = v;
		hal_cpuDataMemoryBarrier();

		while ((*(stm32_common.rcc + rcc_cr) & (1 << 2)) == 0) {
			/* Wait for MSISRDY */
		}
		_stm32_releaseHsi(clko_sysclk);
	}
	else {
		/* Configure HSI */
		clock_source = 1;
		_stm32_acquireHsi(clko_sysclk);
	}
	hal_cpuDataMemoryBarrier();

	/* Configure SYSCLK source */
	v = *(stm32_common.rcc + rcc_cfgr1);
	v &= ~(0x3 << 0);
	v |= (clock_source << 0); /* Set SW */
	*(stm32_common.rcc + rcc_cfgr1) = v;
	hal_cpuDataMemoryBarrier();

	while (((*(stm32_common.rcc + rcc_cfgr1) >> 2) & 0x3) != clock_source) {
		/* Wait for SWS */
	}

	/* Step down */
	if (voltage_range->range > _stm32_pwrGetCPUVolt()) {
		_stm32_pwrSetCPUVolt(voltage_range->range);
	}

	/* Update the current frequency */
	stm32_common.cpuclk = frequency_range->hz;
	return 0;
}


static int _stm32_getDevClockRegShift(unsigned int dev, unsigned int *shift_out)
{
	unsigned int reg = dev / 32;
	if (reg > (rcc_apb3enr - rcc_ahb1enr1)) {
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
		*(stm32_common.rcc + reg + rcc_ahb1enr1) |= (1 << shift);
		*(stm32_common.rcc + reg + rcc_ahb1slpenr1) |= (1 << shift);
	}
	else {
		*(stm32_common.rcc + reg + rcc_ahb1enr1) &= ~(1 << shift);
		*(stm32_common.rcc + reg + rcc_ahb1slpenr1) &= ~(1 << shift);
	}
	hal_cpuDataSyncBarrier();

	return 0;
}


u32 _stm32_rccGetCPUClock(void)
{
	return stm32_common.cpuclk;
}


u32 _stm32_rccGetPclkClock(void)
{
	return stm32_common.cpuclk / 2;
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
		[ipclk_usart1sel] = { rcc_ccipr1, 0x1, 0 },
		[ipclk_usart3sel] = { rcc_ccipr1, 0x1, 2 },
		[ipclk_uart4sel] = { rcc_ccipr1, 0x1, 4 },
		[ipclk_uart5sel] = { rcc_ccipr1, 0x1, 6 },
		[ipclk_i3c1sel] = { rcc_ccipr1, 0x1, 8 },
		[ipclk_i2c1sel] = { rcc_ccipr1, 0x1, 10 },
		[ipclk_i2c2sel] = { rcc_ccipr1, 0x1, 12 },
		[ipclk_i3c2sel] = { rcc_ccipr1, 0x1, 14 },
		[ipclk_spi2sel] = { rcc_ccipr1, 0x1, 16 },
		[ipclk_lptim2sel] = { rcc_ccipr1, 0x3, 18 },
		[ipclk_spi1sel] = { rcc_ccipr1, 0x1, 20 },
		[ipclk_systicksel] = { rcc_ccipr1, 0x3, 22 },
		[ipclk_fdcansel] = { rcc_ccipr1, 0x1, 24 },
		[ipclk_iclksel] = { rcc_ccipr1, 0x3, 26 },
		[ipclk_usb1sel] = { rcc_ccipr1, 0x1, 28 },
		[ipclk_timicsel] = { rcc_ccipr1, 0x7, 29 },
		[ipclk_adf1sel] = { rcc_ccipr2, 0x3, 0 },
		[ipclk_spi3sel] = { rcc_ccipr2, 0x1, 3 },
		[ipclk_sai1sel] = { rcc_ccipr2, 0x3, 5 },
		[ipclk_spi4sel] = { rcc_ccipr2, 0x1, 7 },
		[ipclk_i2c4sel] = { rcc_ccipr2, 0x1, 9 },
		[ipclk_rngsel] = { rcc_ccipr2, 0x1, 11 },
		[ipclk_adcdacsel] = { rcc_ccipr2, 0x3, 16 },
		[ipclk_dac1shsel] = { rcc_ccipr2, 0x1, 19 },
		[ipclk_octospisel] = { rcc_ccipr2, 0x1, 20 },
		[ipclk_usart2sel] = { rcc_ccipr2, 0x1, 22 },
		[ipclk_lpuart1sel] = { rcc_ccipr3, 0x3, 0 },
		[ipclk_i2c3sel] = { rcc_ccipr3, 0x1, 6 },
		[ipclk_lptim34sel] = { rcc_ccipr3, 0x3, 8 },
		[ipclk_lptim1sel] = { rcc_ccipr3, 0x3, 10 },
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


static void _stm32_initPowerSupply(unsigned int supply)
{
	u32 enableBit, validBit;
	static const u8 supplies[pwr_supplies_count] = {
		[pwr_supply_vddio2] = 25,
		[pwr_supply_vusb] = 24,
		[pwr_supply_vdda] = 26,
	};

	if (supply >= pwr_supplies_count) {
		return;
	}

	enableBit = 1 << supplies[supply]; /* xVMyEN, xVMyRDY */
	validBit = enableBit << 4;         /* xV */

	*(stm32_common.pwr + pwr_svmcr) |= enableBit; /* Set xVMyEN */
	hal_cpuDataMemoryBarrier();
	while ((*(stm32_common.pwr + pwr_svmsr) & enableBit) == 0) {
		/* Wait for xVMyRDY */
	}

	*(stm32_common.pwr + pwr_svmcr) |= validBit;   /* Set xV */
	*(stm32_common.pwr + pwr_svmcr) &= ~enableBit; /* Clear xVMyEN */
}


int _stm32_pwrGetCPUVolt(void)
{
	u32 range = *(stm32_common.pwr + pwr_vosr) & (BOOSTEN | 0x3);
	return (range & BOOSTEN) ? ((int)range - 2 * BOOSTEN) : range;
}


void _stm32_pwrSetCPUVolt(int range)
{
	u32 t, boosten = range < 0;
	range += boosten ? BOOSTEN : 0;

	if ((range < 1) || (range > 2))
		return;

	t = *(stm32_common.rcc + rcc_cfgr4) & ~((0xf << 12) | (0x3 << 0)); /* Clear BOOSTDIV (bypass) */
	if (boosten) {
		_stm32_acquireHsi(clko_booster);
		*(stm32_common.rcc + rcc_cfgr4) = t | 2; /* Set BOOSTSEL to HSI16 */
		range |= BOOSTEN;
	}
	else {
		*(stm32_common.rcc + rcc_cfgr4) = t; /* Clear BOOSTSEL */
		_stm32_releaseHsi(clko_booster);
	}
	hal_cpuDataMemoryBarrier();

	t = *(stm32_common.pwr + pwr_vosr) & ~(BOOSTEN | 0x3);
	*(stm32_common.pwr + pwr_vosr) = t | range;
	hal_cpuDataMemoryBarrier();

	while (((*(stm32_common.pwr + pwr_vosr) >> 16) & (BOOSTEN | 0x3)) != range) {
		/* Wait for BOOSTRDY and RxRDY */
	}
}


/* SysTick */


int _stm32_systickInit(u32 interval)
{
	u32 load = (stm32_common.cpuclk / interval) - 1;
	if (load > 0x00ffffff) {
		return -1;
	}

	*(stm32_common.icb + icb_syst_rvr) = load;
	*(stm32_common.icb + icb_syst_cvr) = 0;

	*(stm32_common.icb + icb_syst_csr) |= (1 << 2) | (1 << 1) | (1 << 0); /* CLKSOURCE | TICKINT | ENABLE */

	return 0;
}


void _stm32_systickDone(void)
{
	*(stm32_common.icb + icb_syst_csr) = 0;
}


/* GPIO */


static volatile u32 *_stm32_gpioGetBase(unsigned int d, u8 pin)
{
	if ((d < dev_gpioa) || (d > dev_gpioh) || (pin > 15)) {
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


/* Flash banks */


int _stm32_getFlashBank(void)
{
	return (*(stm32_common.flash + flash_optr) >> 20) & 1;
}


int _stm32_switchFlashBank(int bank)
{
	int err = -EIO, vrange;
	time_t start;
	u32 optr;

	if (bank == _stm32_getFlashBank()) {
		return 0;
	}

	if ((bank < 0) || (bank > 1)) {
		return -EINVAL;
	}

	if (*(stm32_common.flash + flash_sr) & (1 << 16)) { /* BSY */
		return -EBUSY;
	}

	/* Unlock Flash control registers */
	*(stm32_common.flash + flash_keyr) = FLASH_KEY1;
	*(stm32_common.flash + flash_keyr) = FLASH_KEY2;
	if (*(stm32_common.flash + flash_cr) & (1 << 31)) { /* LOCK */
		return -EPERM;
	}

	/* Unlock Flash option-byte */
	*(stm32_common.flash + flash_optkeyr) = FLASH_OPTKEY1;
	*(stm32_common.flash + flash_optkeyr) = FLASH_OPTKEY2;
	if (*(stm32_common.flash + flash_cr) & (1 << 30)) { /* OPTLOCK */
		return -EPERM;
	}

	/* Modify bank selection */
	optr = *(stm32_common.flash + flash_optr) & ~(1 << 20);
	if (bank != 0) {
		optr |= (1 << 20); /* Set SWAP_BANK */
	}
	*(stm32_common.flash + flash_optr) = optr;

	/* Clear previous errors */
	*(stm32_common.flash + flash_sr) |= FLASH_ERROR_MASK;

	/* Ensure proper voltage range for Flash programming */
	vrange = _stm32_pwrGetCPUVolt();
	if ((vrange == 2) || (vrange == 2 - BOOSTEN)) {
		_stm32_pwrSetCPUVolt(vrange - 1);
	}

	/* Commit option-byte */
	start = hal_timerGet();
	*(stm32_common.flash + flash_cr) |= (1 << 17); /* OPTSTRT */
	while (*(stm32_common.flash + flash_sr) & (1 << 16)) {
		/* Wait for BSY to clear */
		if ((hal_timerGet() - start) > 100) {
			_stm32_pwrSetCPUVolt(vrange);
			return -ETIME;
		}
	}

	if ((*(stm32_common.flash + flash_sr) & FLASH_ERROR_MASK) == 0) {
		/* Reload option-byte */
		start = hal_timerGet();
		*(stm32_common.flash + flash_cr) |= (1 << 27); /* OBL_LAUNCH */
		while (*(stm32_common.flash + flash_cr) & (1 << 27)) {
			/* Wait for reset to happen (no return on success) */
			if ((hal_timerGet() - start) > 100) {
				err = -ETIME;
				break;
			}
		}

		/* On success, option byte loader reset should've happened by this point */
	}

	/* We only reach this point in case of failure */
	_stm32_pwrSetCPUVolt(vrange);
	return err;
}


/* Watchdog */


static void _stm32_wdgInit(void)
{
#if defined(WATCHDOG)
	/* Enable write access to IWDG */
	*(stm32_common.iwdg + iwdg_kr) = 0x5555;

	/* 30 s timeout * 32 kHz clock / 256 prescaler */
	*(stm32_common.iwdg + iwdg_pr) = 6;
	*(stm32_common.iwdg + iwdg_rlr) = ((30 * 32000) / 256) - 1;

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
	 * SRAM3 */
	static const struct {
		int dev;
		int crOffs;
		int erkeyrOffs;
	} srams[] = {
#if RAM_BANK_SIZE > 0x00040000 /* SRAM1 + SRAM2 */
		{ .dev = dev_sram3, .crOffs = ramcfg_sram3cr, .erkeyrOffs = ramcfg_sram3erkeyr },
#endif
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
	stm32_common.flash = FLASH_BASE;
	stm32_common.gpio[0] = GPIOA_BASE;
	stm32_common.gpio[1] = GPIOB_BASE;
	stm32_common.gpio[2] = GPIOC_BASE;
	stm32_common.gpio[3] = GPIOD_BASE;
	stm32_common.gpio[4] = GPIOE_BASE;
	stm32_common.gpio[5] = GPIOF_BASE;
	stm32_common.gpio[6] = GPIOG_BASE;
	stm32_common.gpio[7] = GPIOH_BASE;
	stm32_common.hsiRefs = 0;

	/* Store reset flags and then clear them */
	stm32_common.resetFlags = (*(stm32_common.rcc + rcc_csr) >> 25); /* LPWRRSTF ~ OBLRSTF */
	_stm32_rccClearResetFlags();

	/* Enable System configuration controller */
	_stm32_rccSetDevClock(dev_syscfg, 1);
	_stm32_rccSetDevClock(dev_vref, 1);

	/* Enable power module */
	_stm32_rccSetDevClock(dev_pwr, 1);

	_stm32_initSRAM();

	/* Enable independent power supply for ADCs */
	_stm32_initPowerSupply(pwr_supply_vdda);
	/* Enable independent power supply for USB */
	_stm32_initPowerSupply(pwr_supply_vusb);

	/* Disable all RCC interrupts */
	*(stm32_common.rcc + rcc_cier) = 0;

	_stm32_configureClocks();

	hal_cpuDataMemoryBarrier();

	/* GPIO init */
	for (i = dev_gpioa; i <= dev_gpioh; i++) {
		_stm32_rccSetDevClock(i, 1);
	}

	hal_cpuDataMemoryBarrier();

	_stm32_wdgInit();
}
