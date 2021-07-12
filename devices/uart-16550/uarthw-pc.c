/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * UART 16550 (generic PC)
 *
 * Copyright 2020, 2021 Phoenix Systems
 * Author: Julia Kosowska, Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>

#include "uart-16550.h"


typedef struct {
	volatile u8 *base;
	unsigned int irq;
} uarthw_context_t;


static const uarthw_context_t uarts[] = {
	{ (void *)(UART_COM1 | 0x1), UART_COM1_IRQ },
	{ (void *)(UART_COM2 | 0x1), UART_COM2_IRQ },
	{ (void *)UART_GALILEO1, UART_GALILEO1_IRQ },
};


unsigned char uarthw_read(void *hwctx, unsigned int reg)
{
	uarthw_context_t *ctx = (uarthw_context_t *)hwctx;
	unsigned int addr = (unsigned int)ctx->base;

	/* Read from IO-port */
	if (addr & 0x1)
		return hal_inb((void *)((addr & ~0x3) + reg));

	/* Read from memory */
	return *(ctx->base + reg);
}


void uarthw_write(void *hwctx, unsigned int reg, unsigned char val)
{
	uarthw_context_t *ctx = (uarthw_context_t *)hwctx;
	unsigned int addr = (unsigned int)ctx->base;

	/* Write to IO-port */
	if (addr & 0x1) {
		hal_outb((void *)((addr & ~0x3) + reg), val);
		return;
	}

	/* Write to memory */
	*(ctx->base + reg) = val;
}


/* SCH311X Super IO controller */


static unsigned char uarthw_SCH311Xinb(void *base, unsigned int reg)
{
	hal_outb(base, reg);
	return hal_inb(base + 1);
}


static void uarthw_SCH311Xoutb(void *base, unsigned int reg, unsigned char val)
{
	hal_outb(base, reg);
	hal_outb(base + 1, val);
}


static int uarthw_SCH311Xdetect(void *base)
{
	int ret;

	/* Enter configuration mode */
	hal_outb(base, 0x55);

	/* Check device ID */
	switch (ret = uarthw_SCH311Xinb(base, 0x20)) {
		/* SCH3112 */
		case 0x7c:
		/* SCH3114 */
		case 0x7d:
		/* SCH3116 */
		case 0x7f:
			break;

		default:
			ret = -ENODEV;
	}

	/* Exit configuration mode */
	hal_outb(base, 0xaa);

	return ret;
}


static int uarthw_SCH311Xmode(void *base, unsigned char n, unsigned char mode)
{
	/* Enter configuration mode */
	hal_outb(base, 0x55);

	/* Select logical UART device */
	uarthw_SCH311Xoutb(base, 0x07, n);
	/* Set mode */
	uarthw_SCH311Xoutb(base, 0xf0, mode);

	/* Exit configuration mode */
	hal_outb(base, 0xaa);

	return EOK;
}


unsigned int uarthw_irq(void *hwctx)
{
	return ((uarthw_context_t *)hwctx)->irq;
}


int uarthw_init(unsigned int n, void *hwctx, unsigned int *baud)
{
	if (n >= sizeof(uarts) / sizeof(uarts[0]))
		return -EINVAL;

	/* Set UART hardware context */
	*(uarthw_context_t *)hwctx = uarts[n];

	/* Set preferred baudrate */
	if (baud != NULL) {
		/* SCH311X Super IO controller has at least 2 high speed UARTS */
		if ((n < 2) && (uarthw_SCH311Xdetect((void *)0x2e) >= 0)) {
			*baud = bps_460800;
			uarthw_SCH311Xmode((void *)0x2e, 0x04 + n, bauds[*baud].mode);
		}
		else {
			*baud = bps_115200;
		}
	}

	return EOK;
}
