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
#include "cmd-board.h"
#include "interrupts.h"
#include "peripherals.h"
#include "gpio.h"

#include "../timer.h"
#include "../syspage.h"


struct{
	u16 timeout;
	u32 kernel_entry;
} hal_common;


/* Board command definitions */
const cmd_t board_cmds[] = {
	{ cmd_bitstream, "bitstream", "- loads bitstream into PL, usage:\n            bitstream [<boot device>] [<name>]" },
	{ NULL, NULL, NULL }
};


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

	hal_common.timeout = 3;
	hal_common.kernel_entry = 0;
}


void hal_done(void)
{
	timer_done();
}


void hal_appendcmds(cmd_t *cmds)
{
	int i = 0;

	/* Find the last declared cmd */
	while (cmds[i++].cmd != NULL);

	if ((MAX_COMMANDS_NB - --i) < (sizeof(board_cmds) / sizeof(cmd_t)))
		return;

	hal_memcpy(&cmds[i], board_cmds, sizeof(board_cmds));
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


void hal_setKernelEntry(u32 addr)
{
	u32 offs;

	if ((u32)VADDR_KERNEL_INIT != (u32)ADDR_DDR) {
		offs = addr - VADDR_KERNEL_INIT;
		addr = ADDR_DDR + offs;
	}

	hal_common.kernel_entry = addr;
}


void hal_setLaunchTimeout(u32 timeout)
{
	hal_common.timeout = timeout;
}


u32 hal_getLaunchTimeout(void)
{
	return hal_common.timeout;
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


void hal_maskirq(u16 n, u8 v)
{
	//TODO
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
