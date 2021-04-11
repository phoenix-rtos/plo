/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Hardware Abstraction Layer
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski, Marcin Baran
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "imxrt.h"
#include "phfs-flash.h"
#include "uart.h"
#include "phfs-serial.h"
#include "config.h"
#include "peripherals.h"
#include "flashdrv.h"
#include "cmd-board.h"


#include "../hal.h"
#include "../plostd.h"
#include "../errors.h"
#include "../syspage.h"
#include "../phoenixd.h"
#include "../timer.h"



typedef struct {
	void *data;
	int (*isr)(u16, void *);
} intr_handler_t;


/* Board command definitions */
const cmd_t board_cmds[] = {
	{ cmd_flexram,    "flexram", " - define flexram value, usage: flexram <value>" },
	{ NULL, NULL, NULL }
};


const cmd_device_t devices[] = {
	{ "flash0", PDN_FLASH0 },
	{ "com1", PDN_COM1 },
	{ NULL, NULL }
};



struct{
	u16 timeout;
	u32 kernel_entry;
	intr_handler_t irqs[SIZE_INTERRUPTS];
} hal_common;


/* Initialization functions */

void hal_init(void)
{
	int i;

	for (i = 0; i < SIZE_INTERRUPTS; ++i) {
		hal_common.irqs[i].data = NULL;
		hal_common.irqs[i].isr = NULL;
	}

	hal_common.kernel_entry = 0;


	_imxrt_init();

	timer_init();

	hal_setLaunchTimeout(3);

	syspage_init();

	syspage_setAddress((void *)SYSPAGE_ADDRESS);

	/* Add entries related to plo image */
	syspage_addEntries((u32)plo_bss, (u32)_end - (u32)plo_bss + STACK_SIZE);
}


void hal_done(void)
{
	//TODO
}


void hal_initphfs(phfs_handler_t *handlers)
{
	/* Handlers for flash memory */
	handlers[PDN_FLASH0].open = phfsflash_open;
	handlers[PDN_FLASH0].read = phfsflash_read;
	handlers[PDN_FLASH0].write = phfsflash_write;
	handlers[PDN_FLASH0].close = phfsflash_close;
	handlers[PDN_FLASH0].dn = PDN_FLASH0;
	phfsflash_init();

	handlers[PDN_COM1].open = phfs_serialOpen;
	handlers[PDN_COM1].read = phfs_serialRead;
	handlers[PDN_COM1].write = phfs_serialWrite;
	handlers[PDN_COM1].close = phfs_serialClose;
	handlers[PDN_COM1].dn = PHFS_SERIAL_LOADER_ID;
	phfs_serialInit();
}


void hal_initdevs(cmd_device_t **devs)
{
	*devs = (cmd_device_t *)devices;
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
	hal_memcpy(imap, "ocram2", 7);
}


void hal_setDefaultDMAP(char *dmap)
{
	hal_memcpy(dmap, "ocram2", 7);
}


void hal_setKernelEntry(u32 addr)
{
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
	return addr;
}


void hal_memcpy(void *dst, const void *src, unsigned int l)
{
	asm volatile(" \
		orr r3, %0, %1; \
		ands r3, #3; \
		bne 2f; \
	1: \
		cmp %2, #4; \
		ittt hs; \
		ldrhs r3, [%1], #4; \
		strhs r3, [%0], #4; \
		subshs %2, #4; \
		bhs 1b; \
	2: \
		cmp %2, #0; \
		ittt ne; \
		ldrbne r3, [%1], #1; \
		strbne r3, [%0], #1; \
		subsne %2, #1; \
		bne 2b"
	: "+l" (dst), "+l" (src), "+l" (l)
	:
	: "r3", "memory", "cc");
}


int hal_launch(void)
{
	syspage_save();

	/* Give the LPUART transmitters some time */
	timer_wait(100, TIMER_EXPIRE, NULL, 0);

	/* Tidy up */
	uart_done();
	timer_done();

	hal_done();

	_imxrt_cleanDCache();

	hal_cli();
	asm("mov r9, %1; \
		 blx %0"
		 :
		 : "r"(hal_common.kernel_entry), "r"(syspage_getAddress()));
	hal_sti();

	return -1;
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
	if (hal_common.irqs[irq].isr == NULL)
		return -1;

	hal_common.irqs[irq].isr(irq, hal_common.irqs[irq].data);

	return 0;
}


void hal_maskirq(u16 n, u8 v)
{
	//TODO
}


int hal_irqinst(u16 irq, int (*isr)(u16, void *), void *data)
{
	if (irq >= SIZE_INTERRUPTS)
		return ERR_ARG;

	hal_cli();
	hal_common.irqs[irq].isr = isr;
	hal_common.irqs[irq].data = data;

	_imxrt_nvicSetPriority(irq - 0x10, 1);
	_imxrt_nvicSetIRQ(irq - 0x10, 1);
	hal_sti();

	return 0;
}


int hal_irquninst(u16 irq)
{
	hal_cli();
	_imxrt_nvicSetIRQ(irq - 0x10, 0);
	hal_sti();

	return 0;
}


/* Communication functions */

void hal_setattr(char attr)
{
	switch (attr) {
	case ATTR_DEBUG:
		uart_safewrite(UART_CONSOLE, (u8 *)"\033[0m\033[32m", 9);
		break;
	case ATTR_USER:
		uart_safewrite(UART_CONSOLE, (u8 *)"\033[0m", 4);
		break;
	case ATTR_INIT:
		uart_safewrite(UART_CONSOLE, (u8 *)"\033[0m\033[35m", 9);
		break;
	case ATTR_LOADER:
		uart_safewrite(UART_CONSOLE, (u8 *)"\033[0m\033[1m", 8);
		break;
	case ATTR_ERROR:
		uart_safewrite(UART_CONSOLE, (u8 *)"\033[0m\033[31m", 9);
		break;
	}

	return;
}


void hal_putc(const char ch)
{
	uart_write(UART_CONSOLE, (u8 *)&ch, 1);
}


void hal_getc(char *c, char *sc)
{
	while (uart_read(UART_CONSOLE, (u8 *)c, 1, 500) <= 0)
		;
	*sc = 0;

	/* Translate backspace */
	if (*c == 127)
		*c = 8;

	/* Simple parser for VT100 commands */
	else if (*c == 27) {
		while (uart_read(UART_CONSOLE, (u8 *)c, 1, 500) <= 0)
			;

		switch (*c) {
		case 91:
			while (uart_read(UART_CONSOLE, (u8 *)c, 1, 500) <= 0)
				;

			switch (*c) {
			case 'A':             /* UP */
				*sc = 72;
				break;
			case 'B':             /* DOWN */
				*sc = 80;
				break;
			}
			break;
		}
		*c = 0;
	}
}


int hal_keypressed(void)
{
	return !uart_rxEmpty(UART_CONSOLE);
}
