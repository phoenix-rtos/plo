/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * STM32 XSPI driver
 *
 * Copyright 2025 Phoenix Systems
 * Author: Jacek Maksymowicz, Krzysztof Radzewicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "xspi_common.h"

#define XSPI1_BASE     ((void *)0x58025000)
#define XSPI1_REG_BASE ((void *)0x90000000)
#define XSPI1_REG_SIZE (256 * 1024 * 1024)
#define XSPI1_IRQ      170
#define XSPI2_BASE     ((void *)0x5802a000)
#define XSPI2_REG_BASE ((void *)0x70000000)
#define XSPI2_REG_SIZE (256 * 1024 * 1024)
#define XSPI2_IRQ      171
#define XSPI3_BASE     ((void *)0x5802d000)
#define XSPI3_REG_BASE ((void *)0x80000000)
#define XSPI3_REG_SIZE (256 * 1024 * 1024)
#define XSPI3_IRQ      172
#define XSPIM_BASE     ((void *)0x5802b400)

#define MAX_PINS 22


enum {
	ipclk_sel_hclk5 = 0x0,
	ipclk_sel_per_ck = 0x1,
	ipclk_sel_ic3_ck = 0x2,
	ipclk_sel_ic4_ck = 0x3,
};


/* Parameters of XSPI controllers in use.
 * IMPORTANT: if you want to use XSPI1, DO NOT set its clock source to IC3 or IC4.
 * Doing so will result in a hang in BootROM after reset.
 * The same behavior exists in official STMicroelectronics code.
 */
const xspi_ctrlParams_t xspi_ctrlParams[XSPI_N_CONTROLLERS] = {
	{
		.start = XSPI2_REG_BASE,
		.size = XSPI2_REG_SIZE,
		.clksel = { .sel = ipclk_xspi2sel, .val = ipclk_sel_hclk5 },
		.divider_slow = 6,
		.divider = XSPI2_CLOCK_DIV,
		.dev = dev_xspi2,
		.rst = dev_rst_xspi2,
		.ctrl = XSPI2_BASE,
		.resetPin = { dev_gpion, 12 },
		.enabled = XSPI2,
		.spiPort = XSPIM_PORT2,
		.chipSelect = XSPI_CHIPSELECT_NCS1,
		.isHyperbus = XSPI2_IS_HYPERBUS,
	},
	{
		.start = XSPI1_REG_BASE,
		.size = XSPI1_REG_SIZE,
		.clksel = { .sel = ipclk_xspi1sel, .val = ipclk_sel_hclk5 },
		.divider_slow = 6,
		.divider = XSPI1_CLOCK_DIV,
		.dev = dev_xspi1,
		.rst = dev_rst_xspi1,
		.ctrl = XSPI1_BASE,
		.resetPin = { dev_gpioo, 5 },
		.enabled = XSPI1,
		.spiPort = XSPIM_PORT1,
		.chipSelect = XSPI_CHIPSELECT_NCS2,
		.isHyperbus = XSPI1_IS_HYPERBUS,
	},
};


/* Parameters of XSPI I/O manager */
static const struct {
	volatile u32 *base;
	u32 config;
	struct {
		u8 supplyVoltage;
		u8 pin_af;
		xspi_pin_t pins[MAX_PINS]; /* value < 0 signals end of list */
	} spiPort[2];
} mgrParams = {
	.base = XSPIM_BASE,
	.config = XSPIM_MODE_DIRECT | XSPIM_MUX_OFF,
	.spiPort = {
		[XSPIM_PORT1] = {
			.supplyVoltage = pwr_supply_vddio2,
			.pin_af = 9,
			.pins = {
				{ dev_gpioo, 0 },  /* XSPIM_P1_NCS1 */
				{ dev_gpioo, 1 },  /* XSPIM_P1_NCS2 */
				{ dev_gpioo, 2 },  /* XSPIM_P1_DQS0 */
				{ dev_gpioo, 3 },  /* XSPIM_P1_DQS1 */
				{ dev_gpioo, 4 },  /* XSPIM_P1_CLK */
				{ dev_gpiop, 0 },  /* XSPIM_P1_IO0 */
				{ dev_gpiop, 1 },  /* XSPIM_P1_IO1 */
				{ dev_gpiop, 2 },  /* XSPIM_P1_IO2 */
				{ dev_gpiop, 3 },  /* XSPIM_P1_IO3 */
				{ dev_gpiop, 4 },  /* XSPIM_P1_IO4 */
				{ dev_gpiop, 5 },  /* XSPIM_P1_IO5 */
				{ dev_gpiop, 6 },  /* XSPIM_P1_IO6 */
				{ dev_gpiop, 7 },  /* XSPIM_P1_IO7 */
				{ dev_gpiop, 8 },  /* XSPIM_P1_IO8 */
				{ dev_gpiop, 9 },  /* XSPIM_P1_IO9 */
				{ dev_gpiop, 10 }, /* XSPIM_P1_IO1O */
				{ dev_gpiop, 11 }, /* XSPIM_P1_IO11 */
				{ dev_gpiop, 12 }, /* XSPIM_P1_IO12 */
				{ dev_gpiop, 13 }, /* XSPIM_P1_IO13 */
				{ dev_gpiop, 14 }, /* XSPIM_P1_IO14 */
				{ dev_gpiop, 15 }, /* XSPIM_P1_IO15 */
				{ -1, -1 },
			},
		},
		[XSPIM_PORT2] = {
			.supplyVoltage = pwr_supply_vddio3,
			.pin_af = 9,
			.pins = {
				{ dev_gpion, 0 },  /* XSPIM_P2_DQS0 */
				{ dev_gpion, 1 },  /* XSPIM_P2_NCS1 */
				{ dev_gpion, 2 },  /* XSPIM_P2_IO0 */
				{ dev_gpion, 3 },  /* XSPIM_P2_IO1 */
				{ dev_gpion, 4 },  /* XSPIM_P2_IO2 */
				{ dev_gpion, 5 },  /* XSPIM_P2_IO3 */
				{ dev_gpion, 6 },  /* XSPIM_P2_CLK */
				{ dev_gpion, 8 },  /* XSPIM_P2_IO4 */
				{ dev_gpion, 9 },  /* XSPIM_P2_IO5 */
				{ dev_gpion, 10 }, /* XSPIM_P2_IO6 */
				{ dev_gpion, 11 }, /* XSPIM_P2_IO7 */
				{ -1, -1 },
			},
		},
	},
};


u32 xspi_memSize[XSPI_N_CONTROLLERS];


static struct {
	int xspimDone;
} xspi_common;


/* Function waits between 1 and 2 ms */
static void xspi_delay(void)
{
	time_t end = hal_timerGet() + 2;
	while (hal_timerGet() < end) {
		/* Wait */
	}
}


static int xspidrv_isValidAddress(unsigned int minor, u32 off, size_t size)
{
	size_t fsize = xspi_memSize[minor];
	size_t end = off + size;
	if (end < off) {
		/* Integer overflow has occurred */
		return 0;
	}

	if ((off < fsize) && ((off + size) <= fsize)) {
		return 1;
	}

	return 0;
}


static int xspidrv_isValidMinor(unsigned int minor)
{
	if (minor >= XSPI_N_CONTROLLERS) {
		return 0;
	}

	return xspi_ctrlParams[minor].enabled;
}


int xspi_transferFifo(unsigned int minor, u8 *data, size_t len, u8 isRead)
{
	const xspi_ctrlParams_t *p = &xspi_ctrlParams[minor];
	int i = 0, j = 0;
	u32 status;
	time_t deadline;

	while (i < len) {
		status = *(p->ctrl + xspi_sr);
		/* Optimization - check status first, then calculate deadline.
		 * Because XSPI is very fast we can get bytes in FIFO within a few CPU clock cycles. */
		if ((status & (XSPI_SR_FTF | XSPI_SR_TCF)) == 0) {
			/* Timeout is very generous - at least 1 ms per byte. Device should never time out
			 * unless there is a hardware failure. */
			deadline = hal_timerGet() + 2;
			do {
				status = *(p->ctrl + xspi_sr);
				if (hal_timerGet() >= deadline) {
					*(p->ctrl + xspi_cr) |= (1 << 1); /* Abort operation in progress */
					xspi_waitBusy(minor);
					return -ETIME;
				}
			} while ((status & (XSPI_SR_FTF | XSPI_SR_TCF)) == 0);
		}

		if (isRead != 0) {
			j = XSPI_FIFO_SIZE - ((status >> 8) & 0x7f);
			for (; j < XSPI_FIFO_SIZE && (i < len); j++, i++) {
				/* This controller allows byte and halfword reads from the XSPI_DR register */
				data[i] = *(volatile u8 *)(p->ctrl + xspi_dr);
			}
		}
		else {
			j = (status >> 8) & 0x7f;
			/* In indirect write mode, writing data triggers the operation */
			for (; j < XSPI_FIFO_SIZE && (i < len); j++, i++) {
				/* This controller allows byte and halfword writes to the XSPI_DR register */
				*(volatile u8 *)(p->ctrl + xspi_dr) = data[i];
			}
		}
	}

	while ((*(p->ctrl + xspi_sr) & XSPI_SR_TCF) == 0) {
		/* Wait for transfer completion */
	}

	*(p->ctrl + xspi_fcr) = XSPI_SR_TCF;
	return EOK;
}


void xspi_setHigherClock(unsigned int minor)
{
	const xspi_ctrlParams_t *p = &xspi_ctrlParams[minor];
	u32 v;

	if (_stm32_pwrSupplyValidateRange(mgrParams.spiPort[p->spiPort].supplyVoltage) < 0) {
		lib_printf(
				"\nERROR: GPIO port for device %d.%d is in incorrect voltage range. Clock frequency lowered.",
				DEV_STORAGE,
				minor);
		return;
	}

	*(p->ctrl + xspi_cr) &= ~1;
	hal_cpuDataMemoryBarrier();
	v = *(p->ctrl + xspi_dcr2);
	v &= ~0xff;
	v |= (p->divider - 1) & 0xff;
	*(p->ctrl + xspi_dcr2) = v;
	hal_cpuDataMemoryBarrier();
	xspi_waitBusy(minor);
	*(p->ctrl + xspi_cr) |= 1;
}


static void xspi_initPins(int p)
{
	unsigned int i;
	for (i = 0; i < MAX_PINS; i++) {
		if ((mgrParams.spiPort[p].pins[i].port == -1) || (mgrParams.spiPort[p].pins[i].pin == -1)) {
			break;
		}

		_stm32_gpioConfig(
				mgrParams.spiPort[p].pins[i].port,
				mgrParams.spiPort[p].pins[i].pin,
				gpio_mode_af,
				mgrParams.spiPort[p].pin_af,
				gpio_otype_pp,
				gpio_ospeed_vhi,
				gpio_pupd_nopull);
	}
}


static void xspi_commonInit(void)
{
	unsigned int i;
	for (i = 0; i < XSPI_N_CONTROLLERS; i++) {
		xspi_memSize[i] = 0;
	}

	/* After reset XSPI peripherals may be enabled because they were used by BootROM.
	 * We want to disable them _AND_ put them in reset.
	 * XSPIM configuration can be written ONLY if all XSPI peripherals are disabled
	 * either by reset or their XSPI_CR register bit 0 is 0.
	 * Disabling clocks for all XSPI peripherals is not enough. */
	_stm32_rccSetDevClock(dev_xspi1, 0);
	_stm32_rccSetDevClock(dev_xspi2, 0);
	_stm32_rccSetDevClock(dev_xspi3, 0);
	_stm32_rccSetDevClock(dev_xspim, 0);
	_stm32_rccDevReset(dev_rst_xspi1, 1);
	_stm32_rccDevReset(dev_rst_xspi2, 1);
	_stm32_rccDevReset(dev_rst_xspi3, 1);
	_stm32_rccDevReset(dev_rst_xspim, 1);

	/* Enable XSPIPHYCOMP and reset XSPIPHY (not sure if it does anything) */
	_stm32_rccSetDevClock(dev_xspiphycomp, 1);
	_stm32_rccDevReset(dev_rst_xspiphy1, 1);
	_stm32_rccDevReset(dev_rst_xspiphy1, 0);
	_stm32_rccDevReset(dev_rst_xspiphy2, 1);
	_stm32_rccDevReset(dev_rst_xspiphy2, 0);

	/* Take XSPIM out of reset and enable clock */
	_stm32_rccDevReset(dev_rst_xspim, 0);
	_stm32_rccSetDevClock(dev_xspim, 1);

	*(mgrParams.base) = mgrParams.config;
	hal_cpuDataMemoryBarrier();
	(void)*(mgrParams.base);
}


/* Below are functions for device's public interface */


static int xspidrv_sync(unsigned int minor)
{
	if (xspidrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	if (xspi_ctrlParams[minor].isHyperbus != 0) {
		return xspi_hb_sync(minor);
	}
	else {
		return xspi_regcom_sync(minor);
	}
}


static ssize_t xspidrv_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	if ((xspidrv_isValidMinor(minor) == 0) || (xspidrv_isValidAddress(minor, offs, len) == 0)) {
		return -EINVAL;
	}

	if (xspi_ctrlParams[minor].isHyperbus != 0) {
		return xspi_hb_read(minor, offs, buff, len, timeout);
	}
	else {
		return xspi_regcom_read(minor, offs, buff, len, timeout);
	}
}


static ssize_t xspidrv_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	if ((xspidrv_isValidMinor(minor) == 0) || (xspidrv_isValidAddress(minor, offs, len) == 0)) {
		return -EINVAL;
	}

	if (xspi_ctrlParams[minor].isHyperbus != 0) {
		return xspi_hb_write(minor, offs, buff, len);
	}
	else {
		return xspi_regcom_write(minor, offs, buff, len);
	}
}


static ssize_t xspidrv_erase(unsigned int minor, addr_t offs, size_t len, unsigned int flags)
{
	if (xspidrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	if ((len != (size_t)-1) && (xspidrv_isValidAddress(minor, offs, len) == 0)) {
		return -EINVAL;
	}

	if (xspi_ctrlParams[minor].isHyperbus != 0) {
		return xspi_hb_erase(minor, offs, len, flags);
	}
	else {
		return xspi_regcom_erase(minor, offs, len, flags);
	}
}


static int xspidrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	size_t ctrlSz;
	addr_t fStart;

	if (xspidrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	fStart = (addr_t)xspi_ctrlParams[minor].start;
	ctrlSz = xspi_ctrlParams[minor].size;
	*a = fStart;

	/* Check if region is located on device */
	if (xspidrv_isValidAddress(minor, addr, sz) == 0) {
		return -EINVAL;
	}

	/* Check if flash is mappable to map region */
	if (fStart <= memaddr && (fStart + ctrlSz) >= (memaddr + memsz)) {
		return dev_isMappable;
	}

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode) {
		return -EINVAL;
	}

	/* Data can be copied from device to map */
	return dev_isNotMappable;
}


static int xspidrv_control(unsigned int minor, int cmd, void *args)
{
	size_t *outSz = args;

	if (args == NULL) {
		return -EINVAL;
	}

	if (xspidrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	switch (cmd) {
		case DEV_CONTROL_GETPROP_TOTALSZ:
			*outSz = xspi_memSize[minor];
			return EOK;

		case DEV_CONTROL_GETPROP_BLOCKSZ:
			if (xspi_ctrlParams[minor].isHyperbus != 0) {
				*outSz = xspi_hb_getBlockSize(minor);
			}
			else {
				*outSz = xspi_regcom_getBlockSize(minor);
			}

			return (*outSz != 0) ? 0 : -EIO;

		default:
			break;
	}

	return -ENOSYS;
}


static int xspidrv_init(unsigned int minor)
{
	const xspi_ctrlParams_t *p;
	if (xspidrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	if (xspi_common.xspimDone == 0) {
		xspi_commonInit();
		xspi_common.xspimDone = 1;
	}

	p = &xspi_ctrlParams[minor];

	_stm32_rccSetIPClk(p->clksel.sel, p->clksel.val);
	_stm32_rccDevReset(p->rst, 0);
	_stm32_rccSetDevClock(p->dev, 1);
	xspi_initPins(p->spiPort);

	if ((p->resetPin.port >= 0) && (p->resetPin.pin >= 0)) {
		_stm32_gpioSet(p->resetPin.port, p->resetPin.pin, 0);
		_stm32_gpioConfig(
				p->resetPin.port,
				p->resetPin.pin,
				gpio_mode_gpo,
				0,
				gpio_otype_pp,
				gpio_ospeed_vhi,
				gpio_pupd_nopull);
		/* NOTE: These 2 ms delays are way more than necessary */
		xspi_delay();
		_stm32_gpioSet(p->resetPin.port, p->resetPin.pin, 1);
		xspi_delay();
	}

	*(p->ctrl + xspi_cr) = 0; /* Disable controller */
	hal_cpuDataSyncBarrier();

	/* NOTE: we set the NOPREF_AXI bit, during testing clearing it resulted in data corruption in some situations */
	*(p->ctrl + xspi_cr) =
			(1UL << 26) | /* Prefetch is disabled when the AXI transaction is signaled as not-prefetchable */
			(p->chipSelect << 24) |
			(1UL << 22) | /* Stop auto-polling on match */
			(3UL << 8) |  /* FIFO threshold = 4 bytes */
			(1UL << 3);   /* Enable timeout for memory-mapped mode */

	*(p->ctrl + xspi_lptr) = 64; /* Timeout for memory-mapped mode */
	xspi_waitBusy(minor);

	*(p->ctrl + xspi_dcr2) =
			(0 << 16) |                     /* Wrapped reads not supported */
			((p->divider_slow - 1) & 0xff); /* Clock prescaler */
	*(p->ctrl + xspi_dcr3) = 0;
	*(p->ctrl + xspi_dcr4) = 0;

	if (p->isHyperbus != 0) {
		return xspi_hb_init(minor);
	}
	else {
		return xspi_regcom_init(minor);
	}
}


static int xspidrv_done(unsigned int minor)
{
	if (xspidrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	return EOK;
}


__attribute__((constructor)) static void xspidrv_reg(void)
{
	static const dev_ops_t opsStorageSTM32_XSPI = {
		.sync = xspidrv_sync,
		.map = xspidrv_map,
		.control = xspidrv_control,
		.read = xspidrv_read,
		.write = xspidrv_write,
		.erase = xspidrv_erase,
	};

	static const dev_t devStorageSTM32_XSPI = {
		.name = "storage-stm32xspi",
		.init = xspidrv_init,
		.done = xspidrv_done,
		.ops = &opsStorageSTM32_XSPI,
	};

	devs_register(DEV_STORAGE, XSPI_N_CONTROLLERS, &devStorageSTM32_XSPI);
}
