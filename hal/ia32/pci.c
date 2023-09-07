/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * PCI bus access
 *
 * Copyright 2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <hal/hal.h>
#include "types.h"


#define PCI_CONFIG_ADDR 0xcf8u
#define PCI_CONFIG_DATA 0xcfcu
#define PCI_ENABLE      0x80000000u


int hal_pciDetect(void)
{
	hal_outl((void *)PCI_CONFIG_ADDR, PCI_ENABLE);
	return (hal_inl((void *)PCI_CONFIG_ADDR) == PCI_ENABLE) ? 0 : -1;
}


u32 hal_pciAddrBDF(u8 bus, u8 dev, u8 func)
{
	return PCI_ENABLE |
		(((u32)bus & 0xffu) << 16) |
		(((u32)dev & 0x1fu) << 11) |
		(((u32)func & 7u) << 8);
}


u32 hal_pciRead32(u8 bus, u8 dev, u8 func, u8 offs)
{
	u32 bdf = hal_pciAddrBDF(bus, dev, func);
	offs &= 256u / sizeof(u32) - 1u;
	hal_outl((void *)PCI_CONFIG_ADDR, bdf + ((u32)offs << 2u));
	return hal_inl((void *)PCI_CONFIG_DATA);
}


u16 hal_pciRead16(u8 bus, u8 dev, u8 func, u8 offs)
{
	u32 bdf = hal_pciAddrBDF(bus, dev, func);
	offs &= 256u / sizeof(u16) - 1u;
	hal_outl((void *)PCI_CONFIG_ADDR, bdf + ((u32)offs << 1u));
	return hal_inw((void *)(PCI_CONFIG_DATA + (((addr_t)offs & 1u) << 1u)));
}


u8 hal_pciRead8(u8 bus, u8 dev, u8 func, u8 offs)
{
	u32 bdf = hal_pciAddrBDF(bus, dev, func);
	hal_outl((void *)PCI_CONFIG_ADDR, bdf + (u32)offs);
	return hal_inb((void *)(PCI_CONFIG_DATA + ((addr_t)offs & 3u)));
}
