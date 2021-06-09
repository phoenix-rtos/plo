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


struct{
	addr_t kernel_entry;
} hal_common;


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
	syspage_setAddress((void *)SYSPAGE_ADDRESS);
}


void hal_done(void)
{
	timer_done();
}


const char *hal_cpuInfo(void)
{
	return "Cortex-A9 Zynq 7000";
}


void hal_setKernelEntry(addr_t addr)
{
	addr_t offs;

	if ((addr_t)VADDR_KERNEL_INIT != (addr_t)ADDR_DDR) {
		offs = addr - VADDR_KERNEL_INIT;
		addr = ADDR_DDR + offs;
	}

	hal_common.kernel_entry = addr;
}


addr_t hal_vm2phym(addr_t addr)
{
	u32 offs;
	if ((u32)VADDR_KERNEL_INIT != (u32)ADDR_DDR) {
		offs = addr - VADDR_KERNEL_INIT;
		addr = ADDR_DDR + offs;
	}

	return addr;
}


int hal_launch(void)
{
	time_t start;
	syspage_save();

	/* Give the LPUART transmitters some time */
	start = hal_getTime();
	while ((hal_getTime() - start) < 100);

	/* Tidy up */
	hal_done();

	hal_cli();
	__asm__ volatile("mov r9, %1; \
		 blx %0"
		 :
		 : "r"(hal_common.kernel_entry), "r"(syspage_getAddress()));

	hal_sti();

	return -1;
}


extern void hal_invalDCacheAll(void)
{
	/* TODO */
}


void hal_invalDCacheAddr(addr_t addr, size_t sz)
{
	/* TODO */
}


void hal_cli(void)
{
	__asm__ volatile("cpsid if");
}


void hal_sti(void)
{
	__asm__ volatile("cpsie if");
}
