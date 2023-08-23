/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Terminal emulator (based on BIOS 0x16 interrupt call for data input)
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


#ifndef TTYBIOS_TAB_WIDTH
#define TTYBIOS_TAB_WIDTH 8u
#endif


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
	volatile u16 *vram;      /* Video memory */
	void *crtc;              /* CRT controller register */
	unsigned int rows;       /* Terminal height */
	unsigned int cols;       /* Terminal width */
	unsigned char attr;      /* Character attribute */
	unsigned char esc;       /* Escape sequence state */
	unsigned char parmi;     /* Escape sequence parameter index */
	unsigned char parms[10]; /* Escape sequence parameters buffer */
	const char *ekey;        /* Extended key code to send */
} ttybios_t;


struct {
	ttybios_t ttys[TTYBIOS_MAX_CNT];
} ttybios_common;


static void ttybios_memset(volatile u16 *vram, u16 val, unsigned int n)
{
	unsigned int i;

	for (i = 0; i < n; i++)
		*(vram + i) = val;
}


static void ttybios_memmove(volatile u16 *dst, volatile u16 *src, unsigned int n)
{
	unsigned int i;

	if (dst < src) {
		for (i = 0; i < n; i++)
			dst[i] = src[i];
	}
	else {
		for (i = n; i--;)
			dst[i] = src[i];
	}
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
		"0: "
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
	unsigned int i, row, col, pos;
	size_t n;
	char c;

	if ((tty = ttybios_get(minor)) == NULL)
		return -EINVAL;

	/* Print from current cursor position */
	hal_outb(tty->crtc, 0x0f);
	pos = hal_inb((void *)((addr_t)tty->crtc + 1));
	hal_outb(tty->crtc, 0x0e);
	pos |= (u16)hal_inb((void *)((addr_t)tty->crtc + 1)) << 8;
	row = pos / tty->cols;
	col = pos % tty->cols;

	for (n = 0; n < len; n++) {
		c = *((char *)buff + n);

		/* Control character */
		if ((c < ' ') || (c == '\177')) {
			switch (c) {
				case '\b':
				case '\177':
					if (col) {
						col--;
					}
					else if (row) {
						row--;
						col = tty->cols - 1;
					}
					break;

				case '\t':
					col += TTYBIOS_TAB_WIDTH - (col % TTYBIOS_TAB_WIDTH);
					break;
				case '\n':
					row++;
				case '\r':
					col = 0;
					break;

				case '\e':
					hal_memset(tty->parms, 0, sizeof(tty->parms));
					tty->parmi = 0;
					tty->esc = esc_esc;
					break;
			}
		}
		/* Process character according to escape sequence state */
		else {
			switch (tty->esc) {
				case esc_init:
					*(tty->vram + row * tty->cols + col) = (u16)tty->attr << 8 | c;
					col++;
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

							row = tty->parms[0] - 1;
							col = tty->parms[1] - 1;
							tty->esc = esc_init;
							break;

						case 'J':
							switch (tty->parms[0]) {
								case 0:
									ttybios_memset(tty->vram + row * tty->cols + col, (u16)tty->attr << 8 | ' ', tty->cols * (tty->rows - row) - col);
									break;

								case 1:
									ttybios_memset(tty->vram, (u16)tty->attr << 8 | ' ', row * tty->cols + col + 1);
									break;

								case 2:
									ttybios_memset(tty->vram, (u16)tty->attr << 8 | ' ', tty->rows * tty->cols);
									break;
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
							break;

						case ';':
							if (tty->parmi + 1 < sizeof(tty->parms))
								tty->parmi++;
							break;

						case 'h':
							switch (tty->parms[0]) {
								case 25:
									hal_outb(tty->crtc, 0x0a);
									hal_outb((void *)((addr_t)tty->crtc + 1), hal_inb((void *)((addr_t)tty->crtc + 1)) & ~0x20);
									break;
							}
							tty->esc = esc_init;
							break;

						case 'l':
							switch (tty->parms[0]) {
								case 25:
									hal_outb(tty->crtc, 0x0a);
									hal_outb((void *)((addr_t)tty->crtc + 1), hal_inb((void *)((addr_t)tty->crtc + 1)) | 0x20);
									break;
							}
							tty->esc = esc_init;
							break;

						default:
							tty->esc = esc_init;
							break;
					}
					break;
			}
		}

		/* End of line */
		if (col == tty->cols) {
			row++;
			col = 0;
		}

		/* Scroll down */
		if (row == tty->rows) {
			i = tty->cols * (tty->rows - 1);
			ttybios_memmove(tty->vram, tty->vram + tty->cols, i);
			ttybios_memset(tty->vram + i, (u16)tty->attr << 8 | ' ', tty->cols);
			row--;
			col = 0;
		}

		/* Update cursor */
		i = row * tty->cols + col;
		hal_outb(tty->crtc, 0x0e);
		hal_outb((void *)((addr_t)tty->crtc + 1), i >> 8);
		hal_outb(tty->crtc, 0x0f);
		hal_outb((void *)((addr_t)tty->crtc + 1), i);
		*((u8 *)(tty->vram + i) + 1) = tty->attr;
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
	unsigned char color;

	if ((tty = ttybios_get(minor)) == NULL)
		return -EINVAL;

	/* Check color support */
	color = hal_inb((void *)0x3cc) & 0x01;

	/* Initialize VGA */
	tty->vram = (u16 *)(color ? 0xb8000 : 0xb0000);
	tty->crtc = (void *)(color ? 0x3d4 : 0x3b4);

	/* Default 80x25 text mode with magenta color attribute */
	tty->rows = 25;
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
		.erase = NULL,
		.sync = ttybios_sync,
		.map = ttybios_map,
		.done = ttybios_done,
		.init = ttybios_init,
	};

	devs_register(DEV_TTY, TTYBIOS_MAX_CNT, &h);
}
