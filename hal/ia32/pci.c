/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * PCI bus access
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <hal/hal.h>
#include "types.h"
#include "pci.h"


#define PCI_CONFIG_ADDR 0xcf8u
#define PCI_CONFIG_DATA 0xcfcu
#define PCI_ENABLE      0x80000000u


int hal_pciDetect(void)
{
	hal_outl(PCI_CONFIG_ADDR, PCI_ENABLE);
	return (hal_inl(PCI_CONFIG_ADDR) == PCI_ENABLE) ? 0 : -1;
}


u32 hal_pciAddrBDF(hal_pciAddr_t *bdf)
{
	return PCI_ENABLE |
		(((u32)bdf->bus & 0xffu) << 16) |
		(((u32)bdf->device & 0x1fu) << 11) |
		(((u32)bdf->function & 7u) << 8);
}


u32 hal_pciRead32(hal_pciAddr_t *bdf, u8 offs)
{
	u32 addr = hal_pciAddrBDF(bdf);
	offs &= 256u / sizeof(u32) - 1u;
	hal_outl(PCI_CONFIG_ADDR, addr + ((u32)offs << 2u));
	return hal_inl(PCI_CONFIG_DATA);
}


void hal_pciWrite32(hal_pciAddr_t *bdf, u8 offs, u32 value)
{
	u32 addr = hal_pciAddrBDF(bdf);
	offs &= 256u / sizeof(u32) - 1u;
	hal_outl(PCI_CONFIG_ADDR, addr + ((u32)offs << 2u));
	hal_outl(PCI_CONFIG_DATA, value);
}


u16 hal_pciRead16(hal_pciAddr_t *bdf, u8 offs)
{
	u32 addr = hal_pciAddrBDF(bdf);
	offs &= 256u / sizeof(u16) - 1u;
	hal_outl(PCI_CONFIG_ADDR, addr + ((u32)offs << 1u));
	return hal_inw(PCI_CONFIG_DATA + (((addr_t)offs & 1u) << 1u));
}


void hal_pciWrite16(hal_pciAddr_t *bdf, u8 offs, u16 value)
{
	u32 addr = hal_pciAddrBDF(bdf);
	offs &= 256u / sizeof(u16) - 1u;
	hal_outl(PCI_CONFIG_ADDR, addr + ((u32)offs << 1u));
	hal_outw(PCI_CONFIG_DATA + (((addr_t)offs & 1u) << 1u), value);
}


u8 hal_pciRead8(hal_pciAddr_t *bdf, u8 offs)
{
	u32 addr = hal_pciAddrBDF(bdf);
	hal_outl(PCI_CONFIG_ADDR, addr + (u32)offs);
	return hal_inb(PCI_CONFIG_DATA + ((addr_t)offs & 3u));
}


void hal_pciWrite8(hal_pciAddr_t *bdf, u8 offs, u8 value)
{
	u32 addr = hal_pciAddrBDF(bdf);
	hal_outl(PCI_CONFIG_ADDR, addr + (u32)offs);
	hal_outb(PCI_CONFIG_DATA + ((addr_t)offs & 3u), value);
}


void hal_pciBusMaster(hal_pciAddr_t *bdf, int enableBM, int enableIO, int enableMem)
{
	u16 command = hal_pciRead16(bdf, HAL_PCI_REG_COMMAND / sizeof(u16));

	/* Enable/Disable bus mastering */
	if (enableBM > 0) {
		command |= HAL_PCI_COMMAND_BUS_MASTER;
	}
	if (enableBM == 0) {
		command &= ~HAL_PCI_COMMAND_BUS_MASTER;
	}

	/* Enable/Disable address spacess */
	if (enableIO > 0) {
		command |= HAL_PCI_COMMAND_IO_ENABLED;
	}
	if (enableIO == 0) {
		command &= ~HAL_PCI_COMMAND_IO_ENABLED;
	}
	if (enableMem > 0) {
		command |= HAL_PCI_COMMAND_MEM_ENABLED;
	}
	if (enableMem == 0) {
		command &= ~HAL_PCI_COMMAND_MEM_ENABLED;
	}

	hal_pciWrite16(bdf, HAL_PCI_REG_COMMAND / sizeof(u16), command);
}


void hal_pciIterate(int (*cb)(hal_pciAddr_t bdf, u32 id, void *data), void *cbData)
{
	hal_pciAddr_t bdf;

	for (bdf.bus = 0u; bdf.bus < HAL_PCI_NUM_BUS; ++bdf.bus) {
		for (bdf.device = 0u; bdf.device < HAL_PCI_NUM_DEVICES; ++bdf.device) {
			for (bdf.function = 0u; bdf.function < HAL_PCI_NUM_FUNCS; ++bdf.function) {
				u32 id = hal_pciRead32(&bdf, HAL_PCI_REG_PCI_ID / sizeof(u32));

				/* Check if there is a device present */
				if (((id >> 16u) & 0xffffu) == 0xffffu) {
					if (bdf.function == 0u) {
						/*
						 * Devices are required to implement func=0,
						 * so if it is missing then there is not present
						 */
						break;
					}
					else {
						continue;
					}
				}

				if (cb(bdf, id, cbData) != 0) {
					return;
				}

				/* Probe only func=0 if the device if not multifunction */
				if (bdf.function == 0u) {
					u32 hdr = hal_pciRead32(&bdf, HAL_PCI_REG_CACHELINE / sizeof(u32));
					if ((hdr & (1uL << 23u)) != 0uL) {
						break;
					}
				}
			}
		}
	}
}
