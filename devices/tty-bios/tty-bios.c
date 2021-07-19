/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Terminal emulator (based on BIOS interrupt calls)
 *
 * Copyright 2012, 2020, 2021 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <devices/devs.h>
#include <hal/hal.h>
#include <lib/errno.h>


/* ANSI escape sequence states */
enum {
	esc_init, /* normal */
	esc_esc,  /* esc */
	esc_csi,  /* esc [ */
	esc_csiqm /* esc [ ? */
};


/* Extended key codes (index to ekeys table) */
enum {
	ekey_up,    /* up */
	ekey_down,  /* down */
	ekey_right, /* right */
	ekey_left,  /* left */
	ekey_delete /* delete */
};


/* Extended key codes table */
static const char *const ekeys[] = {
	"\033[A",  /* ekey_up */
	"\033[B",  /* ekey_down */
	"\033[C",  /* ekey_right */
	"\033[D",  /* ekey_left */
	"\033[3~", /* ekey_delete */
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


typedef struct {
	unsigned char rows;      /* Terminal height */
	unsigned char cols;      /* Terminal width */
	unsigned char attr;      /* Character attribute */
	unsigned char esc;       /* Escape sequence state */
	unsigned char parmi;     /* Escape sequence parameter index */
	unsigned char parms[10]; /* Escape sequence parameters buffer */
	const char *ekey;        /* Extended key code to send */
} ttybios_t;


struct {
	ttybios_t ttys[TTYBIOS_MAX_CNT];
} ttybios_common;


/* Retrieves cursor position */
static void ttybios_getCursor(unsigned char *row, unsigned char *col)
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
static void ttybios_setCursor(unsigned char row, unsigned char col)
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


/* Checks for available characters at terminal input */
static int ttybios_checkc(void)
{
	int ret;

	__asm__ volatile(
		"movb $0x1, %%ah; "
		"pushl $0x16; "
		"pushl $0x0; "
		"pushl $0x0; "
		"call _interrupts_bios; "
		"jnz 0f; "
		"xorl %%eax, %%eax; "
		"jmp 1f; "
		"0: "
		"movl $0x1, %%eax; "
		"1: "
		"addl $0xc, %%esp; "
	: "=a" (ret)
	:: "memory", "cc");

	return ret;
}


/* Reads character and its scan code from terminal input */
static void ttybios_getc(char *sc, char *c)
{
	unsigned short key;

	__asm__ volatile(
		"xorb %%ah, %%ah; "
		"pushl $0x16; "
		"pushl $0x0; "
		"pushl $0x0; "
		"call _interrupts_bios; "
		"addl $0xc, %%esp; "
	: "=a" (key)
	:: "memory", "cc");

	*sc = key >> 8;
	*c = key;
}


/* Writes character to terminal output */
static void ttybios_putc(unsigned char attr, char c)
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


/* Returns terminal instance */
static ttybios_t *ttybios_get(unsigned int minor)
{
	if (minor >= TTYBIOS_MAX_CNT)
		return NULL;

	return &ttybios_common.ttys[minor];
}


/* Reads characters from terminal input */
static ssize_t ttybios_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	ttybios_t *tty;
	time_t start;
	char sc, c;
	size_t n;

	if ((tty = ttybios_get(minor)) == NULL)
		return -EINVAL;

	if (len && (timeout != -1) && (tty->ekey == NULL)) {
		start = hal_timerGet();
		while (!ttybios_checkc()) {
			if (hal_timerGet() - start >= timeout)
				return -ETIME;
		}
	}

	for (n = 0; n < len; n++) {
		/* Complete extended key code */
		if (tty->ekey != NULL) {
			c = *tty->ekey++;

			if (!(*tty->ekey))
				tty->ekey = NULL;
		}
		/* New key code */
		else {
			ttybios_getc(&sc, &c);

			/* New extended key code */
			if (!c) {
				switch (sc) {
					case 0x48:
						tty->ekey = ekeys[ekey_up];
						c = *tty->ekey++;
						break;

					case 0x50:
						tty->ekey = ekeys[ekey_down];
						c = *tty->ekey++;
						break;
				}
			}
		}

		*((char *)buff + n) = c;
	}

	return n;
}


/* Writes characters to terminal output */
static ssize_t ttybios_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	ttybios_t *tty;
	unsigned char row, col;
	size_t i, j, n;
	char c;

	if ((tty = ttybios_get(minor)) == NULL)
		return -EINVAL;

	for (n = 0; n < len; n++) {
		c = *((char *)buff + n);

		/* Control character */
		if ((c < 0x20) || (c == '\177')) {
			switch (c) {
				case '\n':
					ttybios_putc(tty->attr, '\r');
					ttybios_putc(tty->attr, c);
					break;

				case '\e':
					hal_memset(tty->parms, 0, sizeof(tty->parms));
					tty->parmi = 0;
					tty->esc = esc_esc;
					break;

				default:
					ttybios_putc(tty->attr, c);
					break;
			}
		}
		/* Process character according to escape sequence state */
		else {
			switch (tty->esc) {
				case esc_init:
					ttybios_putc(tty->attr, c);
					break;

				case esc_esc:
					switch (c) {
						case '[':
							hal_memset(tty->parms, 0, sizeof(tty->parms));
							tty->parmi = 0;
							tty->esc = esc_csi;
							break;

						default:
							tty->esc = esc_init;
							break;
					}
					break;

				case esc_csi:
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
							tty->parms[tty->parmi] *= 10;
							tty->parms[tty->parmi] += c - '0';
							break;

						case ';':
							if (tty->parmi + 1 < sizeof(tty->parms))
								tty->parmi++;
							break;

						case '?':
							tty->esc = esc_csiqm;
							break;

						case 'H':
							if (tty->parms[0] < 1)
								tty->parms[0] = 1;
							else if (tty->parms[0] > tty->rows)
								tty->parms[0] = tty->rows;

							if (tty->parms[1] < 1)
								tty->parms[1] = 1;
							else if (tty->parms[1] > tty->cols)
								tty->parms[1] = tty->cols;

							ttybios_setCursor(tty->parms[0] - 1, tty->parms[1] - 1);
							tty->esc = esc_init;
							break;

						case 'J':
							if (tty->parms[0] < 3) {
								ttybios_getCursor(&row, &col);

								if (!tty->parms[0]) {
									j = (tty->rows - row - 1) * tty->cols + tty->cols - col - 1;
								}
								else {
									ttybios_setCursor(0, 0);

									if (tty->parms[0] == 1)
										j = row * tty->cols + col + 1;
									else
										j = tty->rows * tty->cols;
								}

								for (i = 0; i < j; i++)
									ttybios_putc(tty->attr, ' ');

								ttybios_setCursor(row, col);
							}
							tty->esc = esc_init;
							break;

						case 'm':
							i = 0;
							do {
								switch (tty->parms[i]) {
									case 0:
										tty->attr = 0x07;
										break;

									case 1:
										tty->attr = 0x0f;
										break;

									case 30:
									case 31:
									case 32:
									case 33:
									case 34:
									case 35:
									case 36:
									case 37:
										tty->attr = (tty->attr & 0xf0) | ansi2fg[(tty->parms[i] - 30) & 0x7];
										break;

									case 40:
									case 41:
									case 42:
									case 43:
									case 44:
									case 45:
									case 46:
									case 47:
										tty->attr = ansi2bg[(tty->parms[i] - 40) & 0x7] | (tty->attr & 0x0f);
										break;
								}
							} while (i++ < tty->parmi);

						default:
							tty->esc = esc_init;
							break;
					}
					break;

				case esc_csiqm:
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
							tty->parms[tty->parmi] *= 10;
							tty->parms[tty->parmi] += c - '0';

						case ';':
							if (tty->parmi + 1 < sizeof(tty->parms))
								tty->parmi++;
							break;

						default:
							tty->esc = esc_init;
							break;
					}
					break;
			}
		}
	}

	return len;
}


static int ttybios_sync(unsigned int minor)
{
	ttybios_t *tty;

	if ((tty = ttybios_get(minor)) == NULL)
		return -EINVAL;

	return EOK;
}


static int ttybios_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	ttybios_t *tty;

	if ((tty = ttybios_get(minor)) == NULL)
		return -EINVAL;

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode)
		return -EINVAL;

	return dev_isNotMappable;
}


static int ttybios_done(unsigned int minor)
{
	return ttybios_sync(minor);
}


static int ttybios_init(unsigned int minor)
{
	ttybios_t *tty;

	if ((tty = ttybios_get(minor)) == NULL)
		return -EINVAL;

	/* Default 80x30 graphics mode with magenta color attribute */
	tty->rows = 30;
	tty->cols = 80;
	tty->attr = 0x05;
	tty->ekey = NULL;

	return EOK;
}


__attribute__((constructor)) static void ttybios_register(void)
{
	static const dev_handler_t h = {
		.read = ttybios_read,
		.write = ttybios_write,
		.sync = ttybios_sync,
		.map = ttybios_map,
		.done = ttybios_done,
		.init = ttybios_init,
	};

	devs_register(DEV_TTY, TTYBIOS_MAX_CNT, &h);
}
