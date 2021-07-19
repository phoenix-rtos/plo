/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Hardware Abstraction Layer
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <syspage.h>
#include <hal/hal.h>
#include <lib/errno.h>
#include <devices/gpio-zynq7000/gpio.h>


/* Linker symbols */
extern void _end(void);
extern void _plo_bss(void);

/* Timer */
extern void timer_init(void);
extern void timer_done(void);

/* Interrupts */
extern void interrupts_init(void);

/* Console */
void console_init(void);


void hal_init(void)
{
	_zynq_init();
	interrupts_init();

	gpio_init();
	timer_init();
	console_init();

	syspage_init();
	syspage_setAddress(ADDR_SYSPAGE);
}


void hal_done(void)
{
	timer_done();
}


const char *hal_cpuInfo(void)
{
	return "Cortex-A9 Zynq 7000";
}


void hal_cpuInvCache(unsigned int type, addr_t addr, size_t sz)
{
	switch (type) {
		case hal_cpuDCache:
			/* TODO */
		case hal_cpuICache:
			/* TODO */
		default:
			break;
	}
}


addr_t hal_kernelGetAddress(addr_t addr)
{
	addr_t offs;

	if ((addr_t)VADDR_KERNEL_INIT != (addr_t)ADDR_DDR) {
		offs = addr - VADDR_KERNEL_INIT;
		addr = ADDR_DDR + offs;
	}

	return addr;
}


int hal_memAddMap(addr_t start, addr_t end, u32 attr, u32 mapId)
{
	return EOK;
}


void hal_cpuJump(addr_t addr)
{
	syspage_save();

	/* Tidy up */
	hal_done();

	hal_interruptsDisable();
	__asm__ volatile("mov r9, %1; \
		 blx %0"
		 :
		 : "r"(addr), "r"(syspage_getAddress()));

	__builtin_unreachable();
}


void hal_interruptsDisable(void)
{
	__asm__ volatile("cpsid if");
}


void hal_interruptsEnable(void)
{
	__asm__ volatile("cpsie if");
}
