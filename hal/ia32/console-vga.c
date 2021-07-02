/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * VGA console
 *
 * Copyright 2012, 2016, 2020, 2021 Phoenix Systems
 * Copyright 2001, 2005-2008 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


/* ANSI escape sequence states */
enum {
	ESC_INIT, /* normal */
	ESC_ESC,  /* ESC */
	ESC_CSI,  /* ESC [ */
	ESC_CSIQM /* ESC [ ? */
};


/* ANSI code to VGA foreground character attribute conversion table */
static const unsigned char ansi2fg[] = {
	0x00, 0x04, 0x02, 0x06,
	0x01, 0x05, 0x03, 0x07
};


/* ANSI code to VGA background character attribute conversion table */
static const unsigned char ansi2bg[] = {
	0x00, 0x40, 0x20, 0x60,
	0x10, 0x50, 0x30, 0x70
};


struct {
	unsigned char rows;      /* Console height */
	unsigned char cols;      /* Console width */
	unsigned char attr;      /* Character attribute */
	unsigned char esc;       /* Escape sequence state */
	unsigned char parmi;     /* Escape sequence parameter index */
	unsigned char parms[10]; /* Escape sequence parameters buffer */
} halconsole_common;


/* Retrieves cursor position */
static void console_getCursor(unsigned char *row, unsigned char *col)
{
	unsigned short pos;

	__asm__ volatile(
		"xorb %%bh, %%bh; "
		"movb $0x3, %%ah; "
		"pushl $0x10; "
		"pushl $0x0; "
		"pushl $0x0; "
		"call _interrupts_bios; "
		"addl $0xc, %%esp; "
	: "=d" (pos)
	:: "eax", "ebx", "ecx", "memory", "cc");

	*row = pos >> 8;
	*col = pos;
}


/* Sets cursor position */
static void console_setCursor(unsigned char row, unsigned char col)
{
	__asm__ volatile(
		"xorb %%bh, %%bh; "
		"movb $0x2, %%ah; "
		"pushl $0x10; "
		"pushl $0x0; "
		"pushl $0x0; "
		"call _interrupts_bios; "
		"addl $0xc, %%esp; "
	:: "d" (((unsigned short)row << 8) | col)
	: "eax", "ebx", "memory", "cc");
}


/* Writes character on screen */
static void console_write(unsigned char attr, char c)
{
	__asm__ volatile(
		"pushl $0x10; "
		"pushl $0x0; "
		"pushl $0x0; "
		"call _interrupts_bios; "
		"addl $0xc, %%esp; "
	:: "a" (0xe00 | c), "b" (attr)
	: "memory", "cc");
}


void hal_consolePrint(const char *s)
{
	unsigned char row, col;
	unsigned int i, n;
	char c;

	while ((c = *s++)) {
		/* Control character */
		if ((c < 0x20) || (c == '\177')) {
			switch (c) {
				case '\n':
					console_write(halconsole_common.attr, '\r');
					console_write(halconsole_common.attr, c);
					break;

				case '\e':
					for (i = 0; i < sizeof(halconsole_common.parms); i++)
						halconsole_common.parms[i] = 0;
					halconsole_common.parmi = 0;
					halconsole_common.esc = ESC_ESC;
					break;

				default:
					console_write(halconsole_common.attr, c);
					break;
			}
		}
		/* Process character according to escape sequence state */
		else {
			switch (halconsole_common.esc) {
				case ESC_INIT:
					console_write(halconsole_common.attr, c);
					break;

				case ESC_ESC:
					switch (c) {
						case '[':
							for (i = 0; i < sizeof(halconsole_common.parms); i++)
								halconsole_common.parms[i] = 0;
							halconsole_common.parmi = 0;
							halconsole_common.esc = ESC_CSI;
							break;

						default:
							halconsole_common.esc = ESC_INIT;
							break;
					}
					break;

				case ESC_CSI:
					switch (c) {
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							halconsole_common.parms[halconsole_common.parmi] *= 10;
							halconsole_common.parms[halconsole_common.parmi] += c - '0';
							break;

						case ';':
							halconsole_common.parmi++;
							break;

						case '?':
							halconsole_common.esc = ESC_CSIQM;
							break;

						case 'H':
							if (halconsole_common.parms[0] < 1)
								halconsole_common.parms[0] = 1;
							else if (halconsole_common.parms[0] > halconsole_common.rows)
								halconsole_common.parms[0] = halconsole_common.rows;

							if (halconsole_common.parms[1] < 1)
								halconsole_common.parms[1] = 1;
							else if (halconsole_common.parms[1] > halconsole_common.cols)
								halconsole_common.parms[1] = halconsole_common.cols;

							console_setCursor(halconsole_common.parms[0] - 1, halconsole_common.parms[1] - 1);
							halconsole_common.esc = ESC_INIT;
							break;

						case 'J':
							if (halconsole_common.parms[0] < 3) {
								console_getCursor(&row, &col);

								if (!halconsole_common.parms[0]) {
									n = (halconsole_common.rows - row - 1) * halconsole_common.cols + halconsole_common.cols - col - 1;
								}
								else {
									console_setCursor(0, 0);

									if (halconsole_common.parms[0] == 1)
										n = row * halconsole_common.cols + col + 1;
									else
										n = halconsole_common.rows * halconsole_common.cols;
								}

								for (i = 0; i < n; i++)
									console_write(halconsole_common.attr, ' ');

								console_setCursor(row, col);
							}
							halconsole_common.esc = ESC_INIT;
							break;

						case 'm':
							i = 0;
							do {
								switch (halconsole_common.parms[i]) {
									case 0:
										halconsole_common.attr = 0x07;
										break;

									case 1:
										halconsole_common.attr = 0x0f;
										break;

									case 30:
									case 31:
									case 32:
									case 33:
									case 34:
									case 35:
									case 36:
									case 37:
										halconsole_common.attr = (halconsole_common.attr & 0xf0) | ansi2fg[(halconsole_common.parms[i] - 30) & 0x7];
										break;

									case 40:
									case 41:
									case 42:
									case 43:
									case 44:
									case 45:
									case 46:
									case 47:
										halconsole_common.attr = ansi2bg[(halconsole_common.parms[i] - 40) & 0x7] | (halconsole_common.attr & 0x0f);
										break;
								}
							} while (i++ < halconsole_common.parmi);

						default:
							halconsole_common.esc = ESC_INIT;
							break;
					}
					break;

				case ESC_CSIQM:
					switch (c) {
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							halconsole_common.parms[halconsole_common.parmi] *= 10;
							halconsole_common.parms[halconsole_common.parmi] += c - '0';

						case ';':
							halconsole_common.parmi++;
							break;

						default:
							halconsole_common.esc = ESC_INIT;
							break;
					}
					break;
			}
		}
	}
}


void hal_consoleInit(void)
{
	/* Default 80x30 graphics mode with magenta color attribute */
	halconsole_common.rows = 30;
	halconsole_common.cols = 80;
	halconsole_common.attr = 0x05;
}
