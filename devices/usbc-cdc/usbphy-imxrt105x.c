/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX RT106x USB physical layer controller
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


/* clang-format off */

enum {
	/* Power-Down Register */
	usbphy_pwd, usbphy_pwd_set, usbphy_pwd_clr, usbphy_pwd_tog,
	/* Transmitter & Receiver Control Registers */
	usbphy_tx, usbphy_tx_set, usbphy_tx_clr, usbphy_tx_tog,
	usbphy_rx, usbphy_rx_set, usbphy_rx_clr, usbphy_rx_tog,
	/* General Control Register */
	usbphy_ctrl, usbphy_ctrl_set, usbphy_ctrl_clr, usbphy_ctrl_tog,
	/* USB Status & Debug Registers */
	usbphy_status,
	usbphy_debug = 20, usbphy_debug_set, usbphy_debug_clr, usbphy_debug_tog,
	/* UTMI Status & Debug Registers */
	usbphy_debug0_status, usbphy_debug1 = 28,
	usbphy_debug1_set, usbphy_debug1_clr, usbphy_debug1_tog,
	/* UTMI RTL */
	usbphy_version
};

/* clang-format on */


struct {
	u8 pool[USB_POOL_SIZE] __attribute__((aligned(USB_BUFFER_SIZE)));
	size_t usedPools;
} phyusb_common;


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
	_imxrt_setDevClock(pctl_clk_iomuxc, clk_state_run_wait);
	_imxrt_setDevClock(pctl_clk_usboh3, clk_state_run_wait);
}


static void phy_initPad(void)
{
	/* USB_OTG1_ID is an instance of anatop */
	_imxrt_setIOisel(pctl_isel_anatop_usb_otg1_id, 0);

	/* GPIO_AD_B0_01 is configured as USB_OTG1_ID (input) ALT3 */
	_imxrt_setIOmux(pctl_mux_gpio_ad_b0_01, 0, 3);

	/* IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_01 = 0x10b0 */
	_imxrt_setIOpad(pctl_mux_gpio_ad_b0_01, 0, 0, 0, 1, 0, 2, 6, 0);
}


void phy_reset(void)
{
	volatile u32 *usbphy = (u32 *)USB0_PHY_BASE_ADDR;

	/* Reset PHY */
	*(usbphy + usbphy_ctrl_set) = (1 << 31);

	/* Enable clock and release the PHY from reset */
	*(usbphy + usbphy_ctrl_clr) = (1 << 31) | (1 << 30);

	/* Power up the PHY */
	*(usbphy + usbphy_pwd) = 0;
}


void phy_init(void)
{
	phyusb_common.usedPools = 0;

	phy_setClock();
	phy_initPad();
	phy_reset();
}
