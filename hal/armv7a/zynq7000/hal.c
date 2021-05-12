/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
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

#include "hal.h"
#include "zynq.h"
#include "interrupts.h"
#include "peripherals.h"
#include "gpio.h"

#include "../timer.h"
#include "../syspage.h"


struct{
	addr_t kernel_entry;
} hal_common;


/* Initialization functions */

void hal_init(void)
{
	_zynq_init();
	interrupts_init();

	gpio_init();
	timer_init();
	hal_consoleInit();

	syspage_init();
	syspage_setAddress((void *)SYSPAGE_ADDRESS);
}


void hal_done(void)
{
	timer_done();
}


/* Setters and getters for common data */

void hal_setDefaultIMAP(char *imap)
{
	hal_memcpy(imap, "ddr", 4);
}


void hal_setDefaultDMAP(char *dmap)
{
	hal_memcpy(dmap, "ddr", 4);
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
	syspage_save();

	/* Give the LPUART transmitters some time */
	timer_wait(100, TIMER_EXPIRE, NULL, 0);

	/* Tidy up */
	hal_done();

	hal_cli();
	asm("mov r9, %1; \
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


/* Opeartions on interrupts */

void hal_cli(void)
{
	asm("cpsid if");
}


void hal_sti(void)
{
	asm("cpsie if");
}


int low_irqdispatch(u16 irq)
{
	return 0;
}


int hal_irqinst(u16 irq, int (*isr)(u16, void *), void *data)
{
	hal_cli();
	interrupts_setHandler(irq, isr, data);
	hal_sti();

	return 0;
}


int hal_irquninst(u16 irq)
{
	hal_cli();
	interrupts_deleteHandler(irq);
	hal_sti();

	return 0;
}
