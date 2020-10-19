/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Low - level routines
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "config.h"
#include "syspage.h"
#include "periphs.h"

#include "../low.h"
#include "../plostd.h"
#include "../errors.h"
#include "../serial.h"
#include "../phoenixd.h"



struct{
	u16 timeout;
	u32 kernel_entry;
} low_common;



void low_init(void)
{
	syspage_init();

	low_setLaunchTimeout(3);

	low_common.kernel_entry = 0;

	//TODO
}


void low_initphfs(phfs_handler_t *handlers)
{
	//TODO
}


void low_initdevs(cmd_device_t **devs)
{
	//TODO
}


void low_appendcmds(cmd_t *cmds)
{
	//TODO
}


void low_done(void)
{
	//TODO
}


void low_setDefaultIMAP(char *imap)
{
	//TODO
}


void low_setDefaultDMAP(char *dmap)
{
	//TODO
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
	//TODO
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
	//TODO
}



/* Opeartions on interrupts */

void low_cli(void)
{
	//TODO
}


void low_sti(void)
{
	//TODO
}


int low_irqdispatch(u16 irq)
{
	//TODO
}


void low_maskirq(u16 n, u8 v)
{
	//TODO
}


int low_irqinst(u16 irq, int (*isr)(u16, void *), void *data)
{
	//TODO
}


int low_irquninst(u16 irq)
{
	//TODO

	return 0;
}


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
