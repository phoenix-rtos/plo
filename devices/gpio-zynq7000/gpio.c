/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GPIO controller
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "gpio.h"

#include <lib/errno.h>


#define BANK_REGS_OFFS 0x10

/* clang-format off */
enum {
	data_lsw = 0, data_msw, /* up to MAX_BANK_NB pairs of 2 registers*/
	data = 0x10, /* up to MAX_BANK_NB registers */
	data_ro = 0x18, /* up to MAX_BANK_NB registers */
	dirm = 0x81, oen, int_mask, int_en, int_dis, int_stat, int_type, int_pol, int_any, /* MAX_BANK_NB banks, BANK_REGS_OFFS stride */
};
/* clang-format on */

#if defined(__CPU_ZYNQ7000)
#define MAX_BANK_NB 4

const struct {
	u32 min;
	u32 max;
} pinsPos[MAX_BANK_NB] = {
	{ 0, 31 },   /* Bank0: MIO pins[0:31]      */
	{ 32, 53 },  /* Bank1: MIO pins[32:53]     */
	{ 54, 85 },  /* Bank2: EMIO signals[0:31]  */
	{ 86, 117 }, /* Bank3: EMIO signals[32:63] */
};

#elif defined(__CPU_ZYNQMP)
#define MAX_BANK_NB 6

const struct {
	u32 min;
	u32 max;
} pinsPos[MAX_BANK_NB] = {
	{ 0, 25 },            /* Bank0: MIO pins[0:25]      */
	{ 26, 51 },           /* Bank1: MIO pins[26:51]     */
	{ 52, 77 },           /* Bank2: MIO pins[52:77]     */
	{ 78 + 0, 78 + 31 },  /* Bank3: EMIO signals[0:31]  */
	{ 78 + 32, 78 + 63 }, /* Bank4: EMIO signals[32:63]  */
	{ 78 + 64, 78 + 95 }, /* Bank5: EMIO signals[64:95]  */
};

#else
#error "Unknown target platform"
#endif


struct {
	volatile u32 *base;
} gpio_common;


static int gpio_getPinPos(u8 pin, u8 *bank, u8 *pos)
{
	int i;

	if (pin > pinsPos[MAX_BANK_NB - 1].max) {
		return -EINVAL;
	}

	for (i = 0; i < MAX_BANK_NB; ++i) {
		if (pin >= pinsPos[i].min && pin <= pinsPos[i].max) {
			*bank = i;
			*pos = pin - pinsPos[i].min;

			return EOK;
		}
	}

	return -EINVAL;
}


int gpio_writePin(u8 pin, u8 val)
{
	u32 v;
	u8 bank, pos;

	if (gpio_getPinPos(pin, &bank, &pos) < 0) {
		return -EINVAL;
	}

	v = *(gpio_common.base + data + (BANK_REGS_OFFS * bank));
	*(gpio_common.base + data + (BANK_REGS_OFFS * bank)) = (v & ~(1 << pos)) | (!!val << pos);

	return EOK;
}


int gpio_readPin(u8 pin, u8 *val)
{
	u32 v;
	u8 bank, pos;

	if (gpio_getPinPos(pin, &bank, &pos) < 0) {
		return -EINVAL;
	}

	v = *(gpio_common.base + data_ro + (BANK_REGS_OFFS * bank));
	*val = !!(v & (1 << pos));

	return EOK;
}


int gpio_getPinDir(u8 pin, u8 *dir)
{
	u32 d;
	u8 bank, pos;

	if (gpio_getPinPos(pin, &bank, &pos) < 0) {
		return -EINVAL;
	}

	d = *(gpio_common.base + dirm + (BANK_REGS_OFFS * bank));
	*dir = !!(d & (1 << pos));

	return EOK;
}


int gpio_setPinDir(u8 pin, u8 dir)
{
	u32 d, v, offs;
	u8 bank, pos;

	if (gpio_getPinPos(pin, &bank, &pos) < 0) {
		return -EINVAL;
	}

	offs = BANK_REGS_OFFS * bank;
	d = *(gpio_common.base + dirm + offs);
	v = (d & ~(1 << pos)) | (!!dir << pos);

	*(gpio_common.base + dirm + offs) = v;
	*(gpio_common.base + oen + offs) = v;

	return EOK;
}


int gpio_writeBank(u8 bank, u32 val)
{
	if (bank >= MAX_BANK_NB) {
		return -EINVAL;
	}

	*(gpio_common.base + data + (BANK_REGS_OFFS * bank)) = val;

	return EOK;
}


int gpio_readBank(u8 bank, u32 *val)
{
	if (bank >= MAX_BANK_NB) {
		return -EINVAL;
	}

	*val = *(gpio_common.base + data_ro + (BANK_REGS_OFFS * bank));

	return EOK;
}


int gpio_getBankDir(u8 bank, u32 *dir)
{
	if (bank >= MAX_BANK_NB) {
		return -EINVAL;
	}

	*dir = *(gpio_common.base + dirm + (BANK_REGS_OFFS * bank));

	return EOK;
}


int gpio_setBankDir(u8 bank, u32 dir)
{
	if (bank >= MAX_BANK_NB) {
		return -EINVAL;
	}

	*(gpio_common.base + dirm + (BANK_REGS_OFFS * bank)) = dir;
	*(gpio_common.base + oen + (BANK_REGS_OFFS * bank)) = dir;

	return EOK;
}


void gpio_init(void)
{
	gpio_common.base = GPIO_BASE_ADDR;

#if defined(__CPU_ZYNQ7000)
	_zynq_setAmbaClk(amba_gpio_clk, clk_enable);
#elif defined(__CPU_ZYNQMP)
	_zynqmp_devReset(ctl_reset_lpd_gpio, 0);
#endif
}


void gpio_deinit(void)
{
#if defined(__CPU_ZYNQ7000)
	_zynq_setAmbaClk(amba_gpio_clk, clk_disable);
#elif defined(__CPU_ZYNQMP)
	_zynqmp_devReset(ctl_reset_lpd_gpio, 1);
#endif
}
