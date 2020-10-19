/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Low - level routines
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
#include "config.h"
#include "peripherals.h"
#include "flashdrv.h"
#include "cmd-board.h"


#include "../low.h"
#include "../plostd.h"
#include "../errors.h"
#include "../serial.h"
#include "../syspage.h"
#include "../phoenixd.h"



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
	{ "flash1", PDN_FLASH1 },
	{ "com1", PDN_COM1 },
	{ NULL, NULL }
};



struct{
	u16 timeout;
	u32 kernel_entry;
	intr_handler_t irqs[SIZE_INTERRUPTS];
} low_common;


/* Initialization functions */

void low_init(void)
{
	int i;

	_imxrt_init();

	low_setLaunchTimeout(3);

	phfsflash_init();

	syspage_init();

	/* TODO: get ITCM & DTCM & OCRAM from GPR17 */
	syspage_addmap("itcm", (void *)0, (void *)0x40000, mAttrRead | mAttrWrite | maAttrExec);
	syspage_addmap("dtcm", (void *)0x20000000, (void *)0x20028000, mAttrRead | mAttrWrite);
	syspage_addmap("ocram2", (void *)0x20200000, (void *)0x2027fe00, mAttrRead | mAttrWrite | maAttrExec);

	syspage_addmap("xip1", (void *)0x70000000, (void *)0x70400000, mAttrRead | maAttrExec);
	syspage_addmap("xip2", (void *)0x60000000, (void *)0x64000000, mAttrRead | maAttrExec);

	syspage_setAddress((void *)SYSPAGE_ADDRESS);

	/* Add entries related to plo image */
	syspage_addEntries((u32)__bss_start__, (u32)__bss_start__ - (u32)_end + STACK_SIZE);

	for (i = 0; i < SIZE_INTERRUPTS; ++i) {
		low_common.irqs[i].data = NULL;
		low_common.irqs[i].isr = NULL;
	}

	low_common.kernel_entry = 0;
}


void low_done(void)
{
	//TODO
}


void low_initphfs(phfs_handler_t *handlers)
{
	int i;

	/* Handlers for flash memories */
	for (i = 0; i < 2; ++i) {
		handlers[PDN_FLASH0 + i].open = phfsflash_open;
		handlers[PDN_FLASH0 + i].read = phfsflash_read;
		handlers[PDN_FLASH0 + i].write = phfsflash_write;
		handlers[PDN_FLASH0 + i].close = phfsflash_close;
		handlers[PDN_FLASH0 + i].dn = i;
	}

	handlers[PDN_COM1].open = phoenixd_open;
	handlers[PDN_COM1].read = phoenixd_read;
	handlers[PDN_COM1].write = phoenixd_write;
	handlers[PDN_COM1].close = phoenixd_close;
	handlers[PDN_COM1].dn = PHFS_SERIAL_LOADER_ID;
}


void low_initdevs(cmd_device_t **devs)
{
	*devs = (cmd_device_t *)devices;
}


void low_appendcmds(cmd_t *cmds)
{
	int i = 0;

	/* Find the last declared cmd */
	while (cmds[i++].cmd != NULL);

	if ((MAX_COMMANDS_NB - --i) < (sizeof(board_cmds) / sizeof(cmd_t)))
		return;

	low_memcpy(&cmds[i], board_cmds, sizeof(board_cmds));
}



/* Setters and getters for common data */

void low_setDefaultIMAP(char *imap)
{
	low_memcpy(imap, "ocram2", 7);
}


void low_setDefaultDMAP(char *dmap)
{
	low_memcpy(dmap, "ocram2", 7);
}


void low_setKernelEntry(u32 addr)
{
	low_common.kernel_entry = addr;
}


void low_setLaunchTimeout(u32 timeout)
{
	low_common.timeout = timeout;
}


u32 low_getLaunchTimeout(void)
{
	return low_common.timeout;
}



/* Functions modify registers */

u8 low_inb(u16 addr)
{
	//TODO
	return 0;
}


void low_outb(u16 addr, u8 b)
{
	//TODO
}


u32 low_ind(u16 addr)
{
	//TODO
	return 0;
}


void low_outd(u16 addr, u32 d)
{
	//TODO
}



/* Functions make operations on memory */

void low_setfar(u16 segm, u16 offs, u16 v)
{
	//TODO
}


u16 low_getfar(u16 segm, u16 offs)
{
	//TODO
	return 0;
}


void low_setfarbabs(u32 addr, u8 v)
{
	//TODO
}


u8 low_getfarbabs(u32 addr)
{
	//TODO
	return 0;
}


void low_setfarabs(u32 addr, u32 v)
{
	//TODO
}


u32 low_getfarabs(u32 addr)
{
	//TODO
	return 0;
}


void low_copyto(u16 segm, u16 offs, void *src, unsigned int l)
{
	//TODO
}


void low_copyfrom(u16 segm, u16 offs, void *dst, unsigned int l)
{
	//TODO
}


void low_memcpy(void *dst, const void *src, unsigned int l)
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


void low_copytoabs(u32 addr, void *src, unsigned int l)
{
	//TODO
}


void low_copyfromabs(u32 addr, void *dst, unsigned int l)
{
	//TODO
}


u16 low_getcs(void)
{
	//TODO
	return 0;
}


int low_mmcreate(void)
{
	//TODO
	return 0;
}


int low_mmget(unsigned int n, low_mmitem_t *mmitem)
{
	//TODO
	return 0;
}


int low_launch(void)
{
	syspage_save();

	_imxrt_cleanDCache();

	low_cli();
	asm("mov r9, %1; \
		 blx %0"
		 :
		 : "r"(low_common.kernel_entry), "r"(syspage_getAddress()));
	low_sti();

	return -1;
}



/* Opeartions on interrupts */

void low_cli(void)
{
	asm("cpsid if");
}


void low_sti(void)
{
	asm("cpsie if");
}


int low_irqdispatch(u16 irq)
{
	if (low_common.irqs[irq].isr == NULL)
		return -1;

	low_common.irqs[irq].isr(irq, low_common.irqs[irq].data);

	return 0;
}


void low_maskirq(u16 n, u8 v)
{
	//TODO
}


int low_irqinst(u16 irq, int (*isr)(u16, void *), void *data)
{
	if (irq > SIZE_INTERRUPTS)
		return ERR_ARG;

	low_cli();
	low_common.irqs[irq].isr = isr;
	low_common.irqs[irq].data = data;

	_imxrt_nvicSetPriority(irq - 0x10, 1);
	_imxrt_nvicSetIRQ(irq - 0x10, 1);
	low_sti();

	return 0;
}


int low_irquninst(u16 irq)
{
	low_cli();
	_imxrt_nvicSetIRQ(irq - 0x10, 0);
	low_sti();

	return 0;
}


/* Communication functions */

void low_setattr(char attr)
{
	switch (attr) {
	case ATTR_DEBUG:
		serial_safewrite(UART_CONSOLE, (u8 *)"\033[0m\033[32m", 9);
		break;
	case ATTR_USER:
		serial_safewrite(UART_CONSOLE, (u8 *)"\033[0m", 4);
		break;
	case ATTR_INIT:
		serial_safewrite(UART_CONSOLE, (u8 *)"\033[0m\033[35m", 9);
		break;
	case ATTR_LOADER:
		serial_safewrite(UART_CONSOLE, (u8 *)"\033[0m\033[1m", 8);
		break;
	case ATTR_ERROR:
		serial_safewrite(UART_CONSOLE, (u8 *)"\033[0m\033[31m", 9);
		break;
	}

	return;
}


void low_putc(const char ch)
{
	serial_write(UART_CONSOLE, (u8 *)&ch, 1);
}


void low_getc(char *c, char *sc)
{
	while (serial_read(UART_CONSOLE, (u8 *)c, 1, 500) <= 0)
		;
	*sc = 0;

	/* Translate backspace */
	if (*c == 127)
		*c = 8;

	/* Simple parser for VT100 commands */
	else if (*c == 27) {
		while (serial_read(UART_CONSOLE, (u8 *)c, 1, 500) <= 0)
			;

		switch (*c) {
		case 91:
			while (serial_read(UART_CONSOLE, (u8 *)c, 1, 500) <= 0)
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


int low_keypressed(void)
{
	return !serial_rxEmpty(UART_CONSOLE);
}
