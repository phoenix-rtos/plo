/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX RT117x USB physical layer controller
 *
 * Copyright 2021 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "client.h"
#include "usbphy.h"


static struct {
	u8 pool[USB_POOL_SIZE] __attribute__((aligned(USB_BUFFER_SIZE)));
	size_t usedPools;
} phyusb_common;


enum {
	/* Power-Down Register */
	usbphy_pwd = 0, usbphy_pwd_set, usbphy_pwd_clr, usbphy_pwd_tog,
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
	usbphy_version,
	/* PLL Control & Status */
	usbphy_pll_sic = 40, usbphy_pll_sic_set, usbphy_pll_sic_clr, usbphy_pll_sic_tog,
	/* VBUS Detector Control */
	usbphy_vbus_detect = 48,
	usbphy_vbus_detect_set, usbphy_vbus_detect_clr, usbphy_vbus_detect_tog, usbphy_vbus_detect_stat,
	/* Charger Detect Control */
	usbphy_chrg_detect = 56,
	usbphy_chrg_detect_set, usbphy_chrg_detect_clr, usbphy_chrg_detect_tog, usbphy_chrg_detect_stat,
	/* Analog Control Register */
	usbphy_anactrl = 64,
	usbphy_anactrl_set, usbphy_anactrl_clr, usbphy_anactrl_tog,
	/* Loopback Control */
	usbphy_loopback, usbphy_loopback_set, usbphy_loopback_clr, usbphy_loopback_tog,
	/* Loopback Packet Number Select Register */
	usbphy_loopback_hsfscnt, usbphy_loopback_hsfscnt_set, usbphy_loopback_hsfscnt_clr, usbphy_loopback_hsfscnt_tog,
	/* Trim Override Enable */
	usbphy_trim_override_en, usbphy_trim_override_en_set, usbphy_trim_override_en_clr, usbphy_trim_override_en_tog,
};


enum { usbotg_usbcmd = 0x50 };


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
	u32 tmp;
	volatile u32 *usbphy = (u32 *)USB0_PHY_BASE_ADDR;
	volatile u32 *usbcmd = (u32 *)USB0_BASE_ADDR + usbotg_usbcmd;

	phyusb_common.usedPools = 0;

	/* Enable USB clock gate, that clock is needed in RUN level only */
	_imxrt_setDirectLPCG(pctl_lpcg_usb, clk_state_run);
	_imxrt_setLevelLPCG(pctl_lpcg_usb, clk_level_run);

	/* Controller reset */
	*usbcmd |= (1 << 1);

	/* PHY soft reset */
	*(usbphy + usbphy_ctrl_set) = (1 << 31);

	/* Clear UTMI CLKGATE */
	*(usbphy + usbphy_ctrl) &= ~(1 << 30);
	*(usbphy + usbphy_ctrl) |= (1 << 15) | (1 << 14);
	*(usbphy + usbphy_ctrl) |= (1 << 30) | (1 << 20);

	/* Enable regulator */
	*(usbphy + usbphy_pll_sic) |= (1 << 21);

	/* 24 MHz input clock (480MHz / 20) */
	tmp = *(usbphy + usbphy_pll_sic) & ~(7 << 22);
	*(usbphy + usbphy_pll_sic) = tmp | (3 << 22);

	/* Enable the power */
	*(usbphy + usbphy_pll_sic) |= (1 << 13) | (1 << 12);

	/* Wait until the PLL locks */
	while ((*(usbphy + usbphy_pll_sic) & (1 << 31)) == 0)
		;

	/* Enable usb clocks */
	*(usbphy + usbphy_pll_sic) |= (1 << 6);

	/* Clear bypass bit */
	*(usbphy + usbphy_pll_sic_clr) = (1 << 16);

	/* Enable clock and release the PHY from reset */
	*(usbphy + usbphy_ctrl_clr) = (1 << 31) | (1 << 30);

	/* Power up the PHY */
	*(usbphy + usbphy_pwd) = 0;
}
