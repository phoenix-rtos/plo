/*
 * Phoenix-RTOS
 *
 * Zynq-7000 SDIO peripheral initialization
 *
 * Copyright 2023 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "zynq7000-sdio.h"

#include <lib/lib.h>
#include <board_config.h>

/* This platform has 2 SDIO hosts, but a slot was only implemented on one */
#define IMPLEMENTED_SLOTS 1

#define SDIO0_ADDR 0xE0100000
#define SDIO0_IRQ  56
#define SDIO1_ADDR 0xE0101000
#define SDIO1_IRQ  79

/* Set ref clock for SDIO peripheral
 * Set IO PLL as source clock and set divider:
 * IO_PLL / 20 :  1000 MHz / 20 = 50 MHz
 * 50 MHz is enough because we never use SD clock faster than that
 */
static int sdio_setDevClk(int dev, int state)
{
	ctl_clock_t ctl;

	ctl.dev = ctrl_sdio_clk;
	if (_zynq_getCtlClock(&ctl) < 0) {
		return -EIO;
	}

	switch (dev) {
		case 0:
			ctl.pll.clkact0 = state;
			break;

		case 1:
			ctl.pll.clkact1 = state;
			break;

		default:
			return -EINVAL;
	}

	ctl.pll.srcsel = 0;
	ctl.pll.divisor0 = 20;

	return _zynq_setCtlClock(&ctl);
}


/* Activate AMBA clocks for the SDIO peripheral
 */
static int sdio_setAmbaClk(int dev, u32 state)
{
	return _zynq_setAmbaClk((dev == 0) ? amba_sdi0_clk : amba_sdi1_clk, state);
}


/* Set pin functions for SDIO peripheral within platform's I/O mux. */
static int sdio_setPin(int dev, u32 pin)
{
	ctl_mio_t ctl;

	/* Pin should not be configured by the driver */
	if (pin < 0) {
		return EOK;
	}

	if (dev != 0) {
		/* Currently only pins for SDIO0 are supported */
		return -EINVAL;
	}

	/* Select pin */
	ctl.pin = pin;
	ctl.disableRcvr = 1;
	ctl.pullup = 0;

	if (pin >= 40 && pin <= 45) {
		/* Route through the MIO mux levels to get the right function */
		ctl.ioType = 1;
		ctl.speed = 0x1;
		ctl.l0 = 0;
		ctl.l1 = 0;
		ctl.l2 = 0;
		ctl.l3 = 0b100;
	}
	else if (pin == SD_CARD_CD || pin == SD_CARD_WP) {
		ctl.ioType = 1;
		ctl.speed = 0;
		ctl.l0 = 0;
		ctl.l1 = 0;
		ctl.l2 = 0;
		ctl.l3 = 0;
	}
	else {
		return -EINVAL;
	}

	ctl.triEnable = 0;

	return _zynq_setMIO(&ctl);
}


/* Set WP (write protect) and CD (card detect) pins for peripheral
 * These are handled differently from other pins, as they are not routed directly
 * but are set as GPIO pins and we need to inform the peripheral which pins
 * were selected.
 */
static int sdio_setSdWpCdPins(int dev, int wpPin, int cdPin)
{
	if (wpPin < 0) {
		wpPin = 0;
	}

	if (cdPin < 0) {
		cdPin = 0;
	}

	return _zynq_setSDWpCd(dev, wpPin, cdPin);
}


/* Initialize all pins on the peripheral.
 */
static int sdio_initPins(int dev)
{
	int res;
	static const int pins[] = {
		SD_CARD_CLK,
		SD_CARD_CMD,
		SD_CARD_D0,
		SD_CARD_D1,
		SD_CARD_D2,
		SD_CARD_D3,
		SD_CARD_CD,
		SD_CARD_WP,
	};

	for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
		/* Pin should not be configured by the driver */
		if (pins[i] < 0) {
			continue;
		}

		res = sdio_setPin(dev, pins[i]);
		if (res < 0) {
			return res;
		}
	}

	return sdio_setSdWpCdPins(dev, SD_CARD_WP, SD_CARD_CD);
}


int sdio_platformConfigure(unsigned int slot, sdio_platformInfo_t *infoOut)
{
	int res;

	if (slot > 0) {
		/* Currently this code only supports SDIO0 */
		return -ENOENT;
	}

	res = sdio_setDevClk(slot, 1);
	if (res < 0) {
		return -EIO;
	}

	res = sdio_setAmbaClk(slot, clk_enable);
	if (res < 0) {
		return -EIO;
	}

	res = sdio_initPins(slot);
	if (res < 0) {
		return -EIO;
	}

	infoOut->refclkFrequency = 50UL * 1000 * 1000;
	infoOut->regBankPhys = SDIO0_ADDR;
	infoOut->interruptNum = SDIO0_IRQ;
	infoOut->isCDPinSupported = (SD_CARD_CD >= 0) ? 1 : 0;
	infoOut->isWPPinSupported = (SD_CARD_WP >= 0) ? 1 : 0;
	return 0;
}
