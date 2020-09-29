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
#include "config.h"
#include "flashdrv.h"
#include "../low.h"
#include "../plostd.h"
#include "../errors.h"
#include "../serial.h"

#define SIZE_INTERRUPTS 167

#define SYSPAGE_ADDRESS      0x20200000

#define MAX_PROGRAMS         32
#define MAX_MAPS             16
#define MAX_ARGS             256

typedef struct {
	void *data;
	int (*isr)(u16, void *);
} intr_handler_t;


struct{
	intr_handler_t irqs[SIZE_INTERRUPTS];
} low_common;


u16  _plo_timeout = 3;
char _welcome_str[] = WELCOME;
char _plo_command[CMD_SIZE] = DEFAULT_CMD;


char args[MAX_ARGS];
syspage_map_t maps[MAX_MAPS];
syspage_program_t programs[MAX_PROGRAMS];


/* Init and deinit functions */
static void low_initSyspage(void)
{
	plo_syspage = (void *)SYSPAGE_ADDRESS;

	plo_syspage->arg = args;
	plo_syspage->maps = maps;
	plo_syspage->progs = programs;

	plo_syspage->mapssz = 0;
	plo_syspage->progssz = 0;
	plo_syspage->syspagesz = sizeof(syspage_t);

	plo_syspage->kernel.data = NULL;
	plo_syspage->kernel.datasz = 0;

	plo_syspage->kernel.bss = NULL;
	plo_syspage->kernel.bsssz = 0;

	plo_syspage->kernel.text = NULL;
	plo_syspage->kernel.textsz = 0;
}


static void low_initMaps(void)
{
	/* Get size ITCM & DTCM from GPR17 */

	/* TODO: get ITCM & DTCM & OCRAM from GPR17 */


	/* DTCM */
	plo_syspage->maps[plo_syspage->mapssz].id = plo_syspage->mapssz + 1;
	plo_syspage->maps[plo_syspage->mapssz].start = 0x20000000;
	plo_syspage->maps[plo_syspage->mapssz].end = 0x20028000;
	plo_syspage->maps[plo_syspage->mapssz].attr = mAttrRead | mAttrWrite;
	low_memcpy(plo_syspage->maps[plo_syspage->mapssz].name, "DTCM", 5);
	++plo_syspage->mapssz;

	/* OCRAM2 */
	plo_syspage->maps[plo_syspage->mapssz].id = plo_syspage->mapssz + 1;
	plo_syspage->maps[plo_syspage->mapssz].start = 0x20200000;
	plo_syspage->maps[plo_syspage->mapssz].end = 0x2027fe00;
	plo_syspage->maps[plo_syspage->mapssz].attr = mAttrRead | mAttrWrite | maAttrExec;
	low_memcpy(plo_syspage->maps[plo_syspage->mapssz].name, "OCRAM2", 7);
	++plo_syspage->mapssz;

	/* TODO: OCRAM based on GPR17 */

	/* ITCM */
	plo_syspage->maps[plo_syspage->mapssz].id = plo_syspage->mapssz + 1;
	plo_syspage->maps[plo_syspage->mapssz].start = 0;
	plo_syspage->maps[plo_syspage->mapssz].end = 0x40000;
	plo_syspage->maps[plo_syspage->mapssz].attr = mAttrRead | maAttrExec;
	low_memcpy(plo_syspage->maps[plo_syspage->mapssz].name, "ITCM", 5);
	++plo_syspage->mapssz;
}


void low_init(void)
{
	int i;

	_imxrt_init();

	low_initSyspage();

	low_initMaps();

	for (i = 0; i < SIZE_INTERRUPTS; ++i) {
		low_common.irqs[i].data = NULL;
		low_common.irqs[i].isr = NULL;
	}
}


void low_done(void)
{
	//TODO
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
		mov r1, %2; \
		mov r3, %1; \
		mov r4, %0; \
		orr r2, r3, r4; \
		ands r2, #3; \
		bne 2f; \
	1: \
		cmp r1, #4; \
		ittt hs; \
		ldrhs r2, [r3], #4; \
		strhs r2, [r4], #4; \
		subshs r1, #4; \
		bhs 1b; \
	2: \
		cmp r1, #0; \
		ittt ne; \
		ldrbne r2, [r3], #1; \
		strbne r2, [r4], #1; \
		subsne r1, #1; \
		bne 2b"
	:
	: "r" (dst), "r" (src), "r" (l)
	: "r1", "r2", "r3", "r4", "memory", "cc");
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
	plo_syspage->arg = (char *)(plo_syspage + plo_syspage->syspagesz);
	plo_syspage->syspagesz += MAX_ARGS;
	low_memcpy((void *)(plo_syspage->arg), args, MAX_ARGS);

	plo_syspage->progs = (syspage_program_t *)(plo_syspage + plo_syspage->syspagesz);
	plo_syspage->syspagesz += (plo_syspage->progssz * sizeof(syspage_program_t));
	low_memcpy((void *)(plo_syspage->progs), programs, plo_syspage->progssz * sizeof(syspage_program_t));

	plo_syspage->maps = (syspage_map_t *)(plo_syspage + plo_syspage->syspagesz);
	plo_syspage->syspagesz += (plo_syspage->mapssz * sizeof(syspage_map_t));
	low_memcpy((void *)(plo_syspage->maps), maps, plo_syspage->mapssz * sizeof(syspage_map_t));

	_imxrt_cleanDCache();

	low_cli();
	asm("mov r9, %1; \
		 blx %0"
		 :
		 : "r"(kernel_entry), "r"(plo_syspage));
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
