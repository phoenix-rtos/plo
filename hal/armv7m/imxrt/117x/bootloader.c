/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ROM-API bootloader interface i.MX RT 117x
 *
 * Copyright 2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>


static const struct {
	void (*runBootloader)(void *arg);
	const u32 version;
	const char *copyright;
} * *volatile bootloaderTree;


int bootloader_init(void)
{
	u32 base;

	/*
	 * On the i.MX RT1176, the bootrom entry point may be located at
	 * different addresses depending on the revision of the chip:
	 * - MIMXRT1176 0x0021001c
	 * - PIMXRT1176 0x0020001c
	 */

	for (base = 0x00210000; base >= 0x00200000uL; base -= 0x10000) {
		bootloaderTree = (void *)(base + 0x1c);
		if ((((u32)(*bootloaderTree)->copyright) & ~0xffffuL) == base) {
			return EOK;
		}
	}

	bootloaderTree = (void *)-1;

	return -ENOSYS;
}


u32 bootloader_getVersion(void)
{
	return (*bootloaderTree)->version;
}


const char *bootloader_getVendorString(void)
{
	return (*bootloaderTree)->copyright;
}


__attribute__((noreturn)) void bootloader_run(u32 bootcfg)
{
	u32 arg = (0xebuL << 24) | (bootcfg & (0x130003uL));

	(*bootloaderTree)->runBootloader(&arg);

	for (;;) {
		hal_cpuHalt();
	}
}
