/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ROM-API bootloader interface i.MX RT 106x
 *
 * Copyright 2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


static const struct {
	const u32 version;
	const char *copyright;
	void (*runBootloader)(void *arg);
} **volatile bootloaderTree = (void *)0x20001c;


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
