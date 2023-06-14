/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Zynq 7000 USB physical layer controller
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "client.h"
#include "usbphy.h"
#include <devices/gpio-zynq7000/gpio.h>


struct {
	u8 pool[USB_POOL_SIZE];
	size_t usedPools;
} phyusb_common __attribute__((section(".uncached_ddr"), aligned(USB_BUFFER_SIZE)));


void *usbclient_allocBuff(u32 size)
{
	size_t org = USB_BUFFER_SIZE * phyusb_common.usedPools;

	if ((size % USB_BUFFER_SIZE) != 0 || org + size > USB_POOL_SIZE)
		return NULL;

	phyusb_common.usedPools += size / USB_BUFFER_SIZE;

	return &phyusb_common.pool[org];
}


void usbclient_buffReset(void)
{
	phyusb_common.usedPools = 0;
}


void *phy_getBase(void)
{
	return USB0_BASE_ADDR;
}


u32 phy_getIrq(void)
{
	return USB0_IRQ;
}


static void phy_setClock(void)
{
	_zynq_setAmbaClk(amba_usb0_clk, clk_enable);
}


static void phy_setPins(void)
{
	ctl_mio_t ctl;

	/* Set default configuration for USB pins */
	ctl.l0 = ctl.l2 = ctl.l3 = 0;
	ctl.l1 = 1;
	ctl.speed = 1;
	ctl.ioType = 1;
	ctl.pullup = 0;
	ctl.disableRcvr = 0;

	/* Set data I/O pins */
	ctl.triEnable = 0;

	/* USB ULPI DATA4 */
	ctl.pin = mio_pin_28;
	_zynq_setMIO(&ctl);

	/* USB ULPI STP */
	ctl.pin = mio_pin_30;
	_zynq_setMIO(&ctl);

	/* USB ULPI DATA0 - DATA7 */
	for (u32 pin = mio_pin_32; pin <= mio_pin_39; ++pin) {
		if (pin == mio_pin_36)
			continue;

		ctl.pin = pin;
		_zynq_setMIO(&ctl);
	}

	/* Set input pins */
	ctl.triEnable = 1;

	/* USB ULPI CLK */
	ctl.pin = mio_pin_36;
	_zynq_setMIO(&ctl);

	/* USB ULPI NXT */
	ctl.pin = mio_pin_31;
	_zynq_setMIO(&ctl);

	/* USB ULPI DIR */
	ctl.pin = mio_pin_29;
	_zynq_setMIO(&ctl);


	/* USB phy RESET */
	ctl.pin = mio_pin_07;
	ctl.l0 = ctl.l1 = ctl.l2 = ctl.l3 = 0;
	ctl.triEnable = 0;
	_zynq_setMIO(&ctl);
}


void phy_init(void)
{
	phyusb_common.usedPools = 0;

	phy_setPins();
	phy_reset();
	phy_setClock();
}


void phy_reset(void)
{
	/* Set USB phy reset */
	gpio_setPinDir(mio_pin_07, gpio_dir_out);
	gpio_writePin(mio_pin_07, 0);
	gpio_writePin(mio_pin_07, 1);

	phyusb_common.usedPools = 0;
	_zynq_setAmbaClk(amba_usb0_clk, clk_disable);
}
