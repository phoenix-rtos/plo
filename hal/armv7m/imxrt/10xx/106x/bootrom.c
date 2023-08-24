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
#include <lib/errno.h>


static const struct {
	const u32 version;
	const char *copyright;
	void (*runBootloader)(void *arg);
} * *volatile bootloaderTree;


int bootrom_init(void)
{
	bootloaderTree = (void *)0x20001c;

	return EOK;
}


u32 bootrom_getVersion(void)
{
	return (*bootloaderTree)->version;
}


const char *bootrom_getVendorString(void)
{
	return (*bootloaderTree)->copyright;
}


__attribute__((noreturn)) void bootrom_run(u32 bootcfg)
{
	u32 arg = (0xebu << 24) | (bootcfg & (0x130003u));

	(*bootloaderTree)->runBootloader(&arg);

	for (;;) {
		hal_cpuHalt();
	}
}
