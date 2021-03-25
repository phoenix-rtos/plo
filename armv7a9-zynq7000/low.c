/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * low - level routines
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "zynq.h"
#include "cmd-board.h"
#include "interrupts.h"
#include "peripherals.h"
#include "phfs-serial.h"
#include "cdc-client.h"
#include "phfs-usb.h"

#include "../low.h"
#include "../plostd.h"
#include "../serial.h"
#include "../timer.h"
#include "../syspage.h"


struct{
	u16 timeout;
	u32 kernel_entry;
} low_common;


/* Board command definitions */
const cmd_t board_cmds[] = {
	{ cmd_bitstream, "bitstream", "- loads bitstream into PL, usage:\n            bitstream [<boot device>] [<name>]" },
	{ NULL, NULL, NULL }
};


const cmd_device_t devices[] = {
	{ "com1", PDN_COM1 },
	{ "usb0", PDN_ACM0 },
	{ NULL, NULL }
};


/* Initialization functions */

void low_init(void)
{
	_zynq_init();
	interrupts_init();
	timer_init();
	syspage_init();
	syspage_setAddress((void *)SYSPAGE_ADDRESS);

	low_common.timeout = 3;
	low_common.kernel_entry = 0;
}


void low_done(void)
{
	phfs_serialDeinit();
	phfs_usbDeinit();
	timer_done();
}


void low_initphfs(phfs_handler_t *handlers)
{
	handlers[PDN_COM1].open = phfs_serialOpen;
	handlers[PDN_COM1].read = phfs_serialRead;
	handlers[PDN_COM1].write = phfs_serialWrite;
	handlers[PDN_COM1].close = phfs_serialClose;
	handlers[PDN_COM1].dn = PHFS_SERIAL_LOADER_ID;
	phfs_serialInit();

	handlers[PDN_ACM0].open = phfs_usbOpen;
	handlers[PDN_ACM0].read = phfs_usbRead;
	handlers[PDN_ACM0].write = phfs_usbWrite;
	handlers[PDN_ACM0].close = phfs_usbClose;
	handlers[PDN_ACM0].dn = endpt_bulk_acm0;
	phfs_usbInit();
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
	low_memcpy(imap, "ddr", 4);
}


void low_setDefaultDMAP(char *dmap)
{
	low_memcpy(dmap, "ddr", 4);
}


void low_setKernelEntry(u32 addr)
{
	u32 offs;

	if ((u32)VADDR_KERNEL_INIT != (u32)ADDR_DDR) {
		offs = addr - VADDR_KERNEL_INIT;
		addr = ADDR_DDR + offs;
	}

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


addr_t low_vm2phym(addr_t addr)
{
	u32 offs;
	if ((u32)VADDR_KERNEL_INIT != (u32)ADDR_DDR) {
		offs = addr - VADDR_KERNEL_INIT;
		addr = ADDR_DDR + offs;
	}

	return addr;
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

	/* Give the LPUART transmitters some time */
	timer_wait(100, TIMER_EXPIRE, NULL, 0);

	/* Tidy up */
	low_done();

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
	return 0;
}


void low_maskirq(u16 n, u8 v)
{
	//TODO
}


int low_irqinst(u16 irq, int (*isr)(u16, void *), void *data)
{
	low_cli();
	interrupts_setHandler(irq, isr, data);
	low_sti();

	return 0;
}


int low_irquninst(u16 irq)
{
	low_cli();
	interrupts_deleteHandler(irq);
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
