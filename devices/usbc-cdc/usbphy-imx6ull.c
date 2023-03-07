/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * USB physical layer controller
 *
 * Copyright 2021, 2023 Phoenix Systems
 * Author: Hubert Buczynski, Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "client.h"
#include "usbphy.h"

#define USB0_PHY (0x020c9000)

#define USB_PHY_CTRL         (0x30u / sizeof(u32))
#define USB_PHY_CTRL_CLKGATE (1u << 30)
#define USB_PHY_CTRL_SFTRST  (1u << 31)

#define USB_PHY_PWD (0)

#define CCM_ANALOG_PLL_USB0             (0x20c8010)
#define CCM_ANALOG_PLL_USB0_ENABLE      (1u << 13)
#define CCM_ANALOG_PLL_USB0_POWER       (1u << 12)
#define CCM_ANALOG_PLL_USB0_LOCK        (1u << 31)
#define CCM_ANALOG_PLL_USB0_BYPASS      (1u << 16)
#define CCM_ANALOG_PLL_USB0_EN_USB_CLKS (1u << 6)


static struct {
	u8 pool[USB_POOL_SIZE];
	size_t usedPools;
} phyusb_common __attribute__((section(".uncached_ddr"), aligned(USB_BUFFER_SIZE)));


void *usbclient_allocBuff(u32 size)
{
	size_t org = USB_BUFFER_SIZE * phyusb_common.usedPools;

	if (((size % USB_BUFFER_SIZE) != 0u) || ((org + size) > USB_POOL_SIZE)) {
		return NULL;
	}

	phyusb_common.usedPools += size / USB_BUFFER_SIZE;

	return &phyusb_common.pool[org];
}


void usbclient_buffReset(void)
{
	phyusb_common.usedPools = 0;
}


void *phy_getBase(void)
{
	return (void *)0x02184000;
}


u32 phy_getIrq(void)
{
	return 75u;
}


static void phy_setClock(void)
{
	volatile u32 *ccmAnalogPllUSB0 = (u32 *)CCM_ANALOG_PLL_USB0;

	imx6ull_setDevClock(clk_usboh3, 0x03);

	*ccmAnalogPllUSB0 |= CCM_ANALOG_PLL_USB0_ENABLE;
	hal_cpuDataMemoryBarrier();
	*ccmAnalogPllUSB0 |= CCM_ANALOG_PLL_USB0_POWER;
	hal_cpuDataMemoryBarrier();
	while ((*ccmAnalogPllUSB0 & CCM_ANALOG_PLL_USB0_LOCK) == 0u) {
	}
	*ccmAnalogPllUSB0 &= ~CCM_ANALOG_PLL_USB0_BYPASS;
	*ccmAnalogPllUSB0 |= CCM_ANALOG_PLL_USB0_EN_USB_CLKS;
	hal_cpuDataMemoryBarrier();
}

void phy_reset(void)
{
	volatile u32 *usbphy = (u32 *)USB0_PHY;

	*(usbphy + USB_PHY_CTRL) |= USB_PHY_CTRL_SFTRST;
	hal_cpuDataMemoryBarrier();
	*(usbphy + USB_PHY_CTRL) &= ~USB_PHY_CTRL_CLKGATE;
	hal_cpuDataMemoryBarrier();
	*(usbphy + USB_PHY_CTRL) &= ~USB_PHY_CTRL_SFTRST;
	hal_cpuDataMemoryBarrier();
	*(usbphy + USB_PHY_PWD) = 0u;
	hal_cpuDataMemoryBarrier();
}


static void phy_config(void)
{
	volatile u32 *usbphy = (u32 *)USB0_PHY;

	*(usbphy + USB_PHY_CTRL) |= (3u << 14) | (1u << 11) | 2u;
	hal_cpuDataMemoryBarrier();
}


void phy_init(void)
{
	phyusb_common.usedPools = 0;

	phy_setClock();
	phy_reset();
	phy_config();
}
