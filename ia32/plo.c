/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Loader console
 *
 * Copyright 2012, 2017, 2020 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "config.h"
#include "low.h"
#include "plostd.h"
#include "timer.h"
#include "serial.h"
#include "a20.h"

extern cmds;

struct {
	int  ll;
	int  cl;
	char lines[HISTSZ][LINESZ + 1];
} history;


#ifdef CONSOLE_SERIAL
void low_putc(char attr, char c)
{
	static lattr = 0;

	if (attr == lattr) {
		serial_write(0, &c, 1);
		return;
	}

	switch (attr) {
	case ATTR_DEBUG:
		serial_write(0, "\033[0m\033[32m", 9);
		break;
	case ATTR_USER:
		serial_write(0, "\033[0m", 4);
		break;
	case ATTR_INIT:
		serial_write(0, "\033[0m\033[35m", 9);
		break;
	case ATTR_LOADER:
		serial_write(0, "\033[0m\033[1m", 8);
		break;
	case ATTR_ERROR:
		serial_write(0, "\033[0m\033[31m", 9);
		break;
	}

	lattr = attr;
	serial_write(0, &c, 1);
}


void low_getc(char *c, char *sc)
{
	u8 buff[3];

	serial_read(0, c, 1, -1);
	*sc = 0;

	/* Translate backspace */
	if (*c == 127)
		*c = 8;

	/* Simple parser for VT100 commands */
	else if (*c == 27) {
		serial_read(0, c, 1, -1);

		switch (*c) {
		case 91:
			serial_read(0, c, 1, -1);

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
#endif


void plo_drawspaces(char attr, unsigned int len)
{
	unsigned int k;

	for (k = 0; k < len; k++)
		low_putc(attr, ' ');
	return;
}


void plo_cmdloop(void)
{
	char c = 0, sc = 0;
	int pos = 0;
	int k, chgfl = 0, ncl;

	history.ll = 0;
	history.cl = 0;

	for (k = 0; k < HISTSZ; k++)
		history.lines[k][0] = 0;

	plostd_printf(ATTR_LOADER, "%s", PROMPT);
	while (c != '#') {
		low_getc(&c, &sc);

		/* Regular characters */
		if (c) {
			if (c == '\r') {
				if (pos) {
					history.lines[history.ll][pos] = 0;

					cmd_parse(history.lines[history.ll]);

					history.ll = (++history.ll % HISTSZ);
					history.cl = history.ll;
				}
				pos = 0;
				plostd_printf(ATTR_LOADER, "\n%s", PROMPT);
				continue;
			}

			/* If character isn't backspace add it to line buffer */
			if ((c != 8) && (pos < LINESZ)) {
				low_putc(ATTR_USER, c);
				history.lines[history.ll][pos++] = c;
				history.lines[history.ll][pos] = 0;
			}

			/* Remove character before cursor */
			if ((c == 8) && (pos > 0)) {
				history.lines[history.ll][--pos] = 0;
				plostd_printf(ATTR_USER, "%c %c", 8, 8);
			}
		}

		/* Control characters */
		else {
			switch (sc) {
			case 72:
				ncl = ((history.cl + HISTSZ - 1) % HISTSZ);
				if ((ncl != history.ll) && (history.lines[ncl][0])) {
					history.cl = ncl;
					low_memcpy(history.lines[history.ll], history.lines[history.cl], plostd_strlen(history.lines[history.cl]) + 1);
					chgfl = 1;
				}
				break;

			case 80:
				ncl = ((history.cl + 1) % HISTSZ);
				if (history.cl != history.ll) {
					history.cl = ncl;
					chgfl = 1;

					if (ncl != history.ll)
						low_memcpy(history.lines[history.ll], history.lines[history.cl], plostd_strlen(history.lines[history.cl]) + 1);
					else
						history.lines[history.ll][0] = 0;
				}
				break;
			}

			if (chgfl) {
				plostd_printf(ATTR_LOADER, "\r%s", PROMPT);
				plo_drawspaces(ATTR_USER, pos);
				pos = plostd_strlen(history.lines[history.ll]);
				plostd_printf(ATTR_LOADER, "\r%s", PROMPT);
				plostd_printf(ATTR_USER, "%s", history.lines[history.ll]);
				chgfl = 0;
			}
		}
	}
	return;
}


void plo_unreal(void)
{
	/* Prepare ring 0 code segment descriptor (16-bit) - selector 0x08 */
	low_setfar(GDT_SEG, 8, 0xffff);
	low_setfar(GDT_SEG, 10, 0x0000);
	low_setfar(GDT_SEG, 12, 0x9a00);
	low_setfar(GDT_SEG, 14, 0x0000);

	/* Prepare ring 0 data segment descriptor - selector 0x10 */
	low_setfar(GDT_SEG, 16, 0xffff);
	low_setfar(GDT_SEG, 18, 0x0000);
	low_setfar(GDT_SEG, 20, 0x9200);
	low_setfar(GDT_SEG, 22, 0x00cf);

	/* Prepare GDTR pseudodescriptor */
	low_setfar(SYSPAGE_SEG, SYSPAGE_OFFS_GDTR + 0, GDT_SIZE - 1);
	low_setfar(SYSPAGE_SEG, SYSPAGE_OFFS_GDTR + 2, GDT_SEG << 4);
	low_setfar(SYSPAGE_SEG, SYSPAGE_OFFS_GDTR + 4, GDT_SEG >> 12);

	low_cli();

#asm
	; Save real mode segment registers
	push ds
	push es
	push fs
	push gs

	; Load GDT into GDTR
	mov ax, #SYSPAGE_SEG
	mov es, ax
	seg es
	lgdt 0

	; Switch to pmode by setting pmode bit
	mov eax, cr0
	inc eax
	mov cr0, eax

	; jmp far prot
	db 0x66, 0xea
	dd prot + 0x7c00
	dw 0x08

prot:
	; Reload the segment registers to activate the new segment limits
	mov ax, #0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	; Back to realmode by toggling bit again
	mov eax, cr0
	dec eax
	mov cr0, eax
	jmp 0x7c0:real

real:
	; Reload the segment registers to match the base address and the selector
	pop gs
	pop fs
	pop es
	pop ds
#endasm

	low_sti();
}


void plo_init(void)
{
	u16 t;
	u8 c;

	/* Enter unreal mode */
	plo_unreal();
	/* Enable A20 line */
	a20_enable();
	/* From now on we should have access to 4GB of memory */
	/* through zeroed segment registers ds, es, fs and gs */

	low_init();
	timer_init();
	serial_init(BPS_115200);
	phfs_init();

#ifdef CONSOLE_SERIAL
	serial_write(0, "\033[?25h", 6);
#endif

	plostd_printf(ATTR_LOADER, "%s\n", WELCOME);

	/* Wait and execute saved loader command */
	for (t = _plo_timeout; t; t--) {
		plostd_printf(ATTR_INIT, "\r%d seconds to automatic boot      ", t);

#ifdef CONSOLE_SERIAL
		if (serial_read(0, &c, 1, 1000) > 0)
			break;
#else
		if (timer_wait(1000, TIMER_KEYB, NULL, 0))
			break;
#endif
	}

	if (t == 0) {
		plostd_printf(ATTR_INIT, "\n%s%s", PROMPT, _plo_command);
		cmd_parse(_plo_command);
	}
	plostd_printf(ATTR_INIT, "\n");

	/* Enter to interactive mode */
	plo_cmdloop();

	serial_done();
	timer_done();

	low_done();
	return;
}
