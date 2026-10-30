/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR716 Flash driver
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <lib/errno.h>

#include <devices/flash-spimctrl/spimctrl.h>


#define FLASH_CMD_NOP 0x00u

/* Configuration register */
#define EXTENDED_ADDRESS (1 << 14)
#define READ_CMD_MASK    (0xFFu)
#define WRITE_CMD_MASK   (0xFFu << 24)

/* Control register */

#define USR_CTRL (1 << 0)
#define CHIP_SEL (1 << 3)
#define CORE_RST (1 << 4)

/* Status register */

#define OPER_DONE   (1 << 0)
#define CORE_BUSY   (1 << 1)
#define INITIALIZED (1 << 2)


enum {
	flash_cfg = 0, /* Flash configuration : 0x00 */
	flash_ctrl,    /* Flash control       : 0x04 */
	flash_stat,    /* Flash status        : 0x08 */
	flash_rx,      /* Flash receive       : 0x0C */
	flash_tx,      /* Flash transmit      : 0x10 */
	flash_econf,   /* EDAC configuration  : 0x14 */
	flash_estat    /* EDAC status         : 0x18 */
};


#if defined(__CPU_GR716)


static void spimctrl_cguClkEnable(unsigned int minor)
{
	_gr716_cguClkEnable(cgu_primary, cgudev_spimctrl0 + minor);
}


static int spimctrl_cguClkStatus(unsigned int minor)
{
	return _gr716_cguClkStatus(cgu_primary, cgudev_spimctrl0 + minor);
}


#else


static void spimctrl_cguClkEnable(unsigned int minor)
{
	(void)minor;
}


static int spimctrl_cguClkStatus(unsigned int minor)
{
	(void)minor;

	return 1;
}


#endif


static void spimctrl_enterUserCtrl(vu32 *spimctrlBase)
{
	u32 ctrl = *(spimctrlBase + flash_ctrl);

	/* assert CS high before entering user control */
	ctrl |= CHIP_SEL;
	*(spimctrlBase + flash_ctrl) = ctrl;

	/* add a few cycles of a delay */
	__asm__ volatile("nop");
	__asm__ volatile("nop");
	__asm__ volatile("nop");

	/* enter user control */
	ctrl |= USR_CTRL;
	ctrl &= ~CHIP_SEL;
	*(spimctrlBase + flash_ctrl) = ctrl;
}


static void spimctrl_exitUserCtrl(vu32 *spimctrlBase)
{
	u32 ctrl = *(spimctrlBase + flash_ctrl);

	ctrl &= ~USR_CTRL;
	ctrl |= CHIP_SEL;
	*(spimctrlBase + flash_ctrl) = ctrl;
}


static int spimctrl_busy(spimctrl_t *spimctrl)
{
	return (*(spimctrl->base + flash_stat) & CORE_BUSY) >> 1;
}


static int spimctrl_ready(spimctrl_t *spimctrl)
{
	u32 val = (*(spimctrl->base + flash_stat) & (INITIALIZED | OPER_DONE));

	return (val == INITIALIZED) ? 1 : 0;
}


static void spimctrl_addressMode(spimctrl_t *spimctrl)
{
	u32 cfg = *(spimctrl->base + flash_cfg);

#if defined(USE_4BYTE_MODE) && (USE_4BYTE_MODE != 0)
	u8 readcmd = 0x13u;
	spimctrl->extendedAddress = 1;
	cfg &= ~READ_CMD_MASK;
	cfg |= readcmd;
	cfg |= EXTENDED_ADDRESS;
#else
	u8 readcmd = 0x03u;
	cfg &= ~READ_CMD_MASK;
	cfg |= readcmd;
	spimctrl->extendedAddress = 0;
	cfg &= ~EXTENDED_ADDRESS;
#endif /* USE_4BYTE_MODE */

	*(spimctrl->base + flash_cfg) = cfg;
}


static void spimctrl_tx(vu32 *spimctrlBase, u8 cmd)
{
	*(spimctrlBase + flash_tx) = cmd;
	while ((*(spimctrlBase + flash_stat) & OPER_DONE) == 0) { }
	*(spimctrlBase + flash_stat) |= OPER_DONE;
}


static u8 spimctrl_rx(vu32 *spimctrlBase)
{
	// return *(spimctrlBase + flash_rx) & 0xff;

	u32 val = *(spimctrlBase + flash_rx) & 0xff;
	*(spimctrlBase + flash_stat) |= OPER_DONE;
	return val;
}


static void spimctrl_read(spimctrl_t *spimctrl, struct xferOp *op)
{
	size_t i;

	spimctrl_enterUserCtrl(spimctrl->base);

	/* send command */
	for (i = 0; i < op->cmdLen; i++) {
		spimctrl_tx(spimctrl->base, op->cmd[i]);
	}

	/* read data */
	for (i = 0; i < op->dataLen; i++) {
		spimctrl_tx(spimctrl->base, FLASH_CMD_NOP);
		op->rxData[i] = spimctrl_rx(spimctrl->base);
	}

	spimctrl_exitUserCtrl(spimctrl->base);
}


static void spimctrl_write(spimctrl_t *spimctrl, struct xferOp *op)
{
	size_t i;
	spimctrl_enterUserCtrl(spimctrl->base);

	/* Send command */
	for (i = 0; i < op->cmdLen; i++) {
		spimctrl_tx(spimctrl->base, op->cmd[i]);
	}

	/* Send data */
	for (i = 0; i < op->dataLen; i++) {
		spimctrl_tx(spimctrl->base, op->txData[i]);
	}

	spimctrl_exitUserCtrl(spimctrl->base);
}


int spimctrl_xfer(spimctrl_t *spimctrl, struct xferOp *op)
{
	if ((spimctrl_busy(spimctrl) == 1) || spimctrl_ready(spimctrl) == 0) {
		return -EBUSY;
	}

	switch (op->type) {
		case xfer_opRead:
			spimctrl_read(spimctrl, op);
			break;
		case xfer_opWrite:
			spimctrl_write(spimctrl, op);
			break;
		default:
			return -EINVAL;
	}
	return EOK;
}


void spimctrl_reset(spimctrl_t *spimctrl)
{
	*(spimctrl->base + flash_ctrl) = CORE_RST;
}


void spimctrl_init(spimctrl_t *spimctrl, unsigned int minor)
{
	/* Enable clock if needed */
	if (spimctrl_cguClkStatus(minor) == 0) {
		spimctrl_cguClkEnable(minor);
	}

	/* Reset core */
	spimctrl_reset(spimctrl);

	spimctrl_addressMode(spimctrl);
}
