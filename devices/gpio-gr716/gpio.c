/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR716 GPIO driver
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <lib/lib.h>
#include <devices/devs.h>
#include <lib/errno.h>

#include "gpio.h"

/* clang-format off */

enum {
	gpio_data = 0,    /* Port data reg : 0x00 */
	gpio_out,         /* Output reg : 0x04 */
	gpio_dir,         /* Port direction reg : 0x08 */
	gpio_imask,       /* Interrupt mask reg : 0x0C */
	gpio_ipol,        /* Interrupt polarity reg : 0x10 */
	gpio_iedge,       /* Interrupt edge reg : 0x14 */
	                  /* reserved - 0x18 */
	gpio_cap = 7,     /* Port capability reg : 0x1C */
	gpio_irqmapr,     /* Interrupt map register n : 0x20 - 0x3C */
	gpio_iavail = 16, /* Interrupt available reg : 0x40 */
	gpio_iflag,       /* Interrupt flag reg : 0x44 */
	gpio_ipen,        /* Interrupt enable reg : 0x48 */
	gpio_pulse,       /* Pulse reg : 0x4C */

	gpio_ie_lor, /* Interrupt enable logical OR reg : 0x50 */
	gpio_po_lor, /* Port output logical OR reg : 0x54 */
	gpio_pd_lor, /* Port direction logical OR reg : 0x58 */
	gpio_im_lor, /* Interrupt mask logical OR reg : 0x5C */

	gpio_ie_land, /* Interrupt enable logical AND reg : 0x60 */
	gpio_po_land, /* Port output logical AND reg : 0x64 */
	gpio_pd_land, /* Port direction logical AND reg : 0x68 */
	gpio_im_land, /* Interrupt mask logical AND reg : 0x6C */

	gpio_ie_lxor, /* Interrupt enable logical XOR reg : 0x70 */
	gpio_po_lxor, /* Port output logical XOR reg : 0x74 */
	gpio_pd_lxor, /* Port direction logical XOR reg : 0x78 */
	gpio_im_lxor, /* Interrupt mask logical XOR reg : 0x7C */

	gpio_ie_sc,      /* Interrupt enable logical set/clear reg : 0x80 - 0x8C */
	gpio_po_sc = 36, /* Port output logical set/clear reg : 0x90 - 0x9C */
	gpio_pd_sc = 40, /* Port direction logical set/clear reg : 0xA0 - 0xAC */
	gpio_im_sc = 44  /* Interrupt mask logical set/clear reg : 0xB0 - 0xBC */
};

/* clang-format on */

static struct {
	vu32 *grgpio_0;
	vu32 *grgpio_1;
} gpio_common;


static inline int gpio_pinToPort(u8 pin)
{
	return (pin >> 5);
}


int gpio_writePin(u8 pin, u8 val)
{
	int err = EOK;
	u32 msk = val << (pin & 0x1F);

	switch (gpio_pinToPort(pin)) {
		case GPIO_PORT_0:
			*(gpio_common.grgpio_0 + gpio_out) = (*(gpio_common.grgpio_0 + gpio_out) & ~msk) | msk;
			break;
		case GPIO_PORT_1:
			*(gpio_common.grgpio_1 + gpio_out) = (*(gpio_common.grgpio_1 + gpio_out) & ~msk) | msk;
			break;
		default:
			err = -EINVAL;
			break;
	}
	return err;
}


int gpio_readPin(u8 pin, u8 *val)
{
	int err = EOK;

	switch (gpio_pinToPort(pin)) {
		case GPIO_PORT_0:
			*val = (*(gpio_common.grgpio_0 + gpio_out) >> (pin & 0x1F)) & 0x1;
			break;
		case GPIO_PORT_1:
			*val = (*(gpio_common.grgpio_1 + gpio_out) >> (pin & 0x1F)) & 0x1;
			break;
		default:
			err = -EINVAL;
			break;
	}

	return err;
}


int gpio_getPinDir(u8 pin, u8 *dir)
{
	int err = EOK;

	switch (gpio_pinToPort(pin)) {
		case GPIO_PORT_0:
			*dir = (*(gpio_common.grgpio_0 + gpio_dir) >> (pin & 0x1F)) & 0x1;
			break;
		case GPIO_PORT_1:
			*dir = (*(gpio_common.grgpio_1 + gpio_dir) >> (pin & 0x1F)) & 0x1;
			break;
		default:
			err = -EINVAL;
			break;
	}

	return err;
}


int gpio_setPinDir(u8 pin, u32 dir)
{
	int err = EOK;
	u32 msk = dir << (pin & 0x1F);

	switch (gpio_pinToPort(pin)) {
		case GPIO_PORT_0:
			*(gpio_common.grgpio_0 + gpio_dir) = (*(gpio_common.grgpio_0 + gpio_dir) & ~msk) | msk;
			break;
		case GPIO_PORT_1:
			*(gpio_common.grgpio_1 + gpio_dir) = (*(gpio_common.grgpio_1 + gpio_dir) & ~msk) | msk;
			break;
		default:
			err = -EINVAL;
			break;
	}
	return err;
}


void gpio_init(void)
{
	gpio_common.grgpio_0 = GRGPIO0_BASE;
	gpio_common.grgpio_1 = GRGPIO1_BASE;
}
