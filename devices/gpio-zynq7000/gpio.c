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


#define MAX_BANK_NB    4
#define BANK_REGS_OFFS 0x10


enum {
	data_lsw = 0, data_msw, mask_data1_lsw, mask_data1_msw, mask_data2_lsw, mask_data2_msw, mask_data3_lsw, mask_data3_msw,
	data = 0x10, data1, data2, data3,
	data_ro = 0x18, data1_ro, data2_ro, data3_ro,
	dirm = 0x81, oen, int_mask, int_en, int_dis, int_stat, int_type, int_pol, int_any,
	dirm1 = 0x91, oen1, int_mask1, int_en1, int_dis1, int_stat1, int_type1, int_pol1, int_any1,
	dirm2 = 0xa1, oen2, int_mask2, int_en2, int_dis2, int_stat2, int_type2, int_pol2, int_any2,
	dirm3 = 0xb1, oen3, int_mask3, int_en3, int_dis3, int_stat3, int_type3, int_pol3, int_any3,
};


const struct {
	u32 min;
	u32 max;
} pinsPos[MAX_BANK_NB] = { { 0, 31 },     /* Bank0: MIO pins[0:31]      */
                           { 32, 53 },    /* Bank1: MIO pins[32:53]     */
                           { 54, 85 },    /* Bank2: EMIO signals[0:31]  */
                           { 86, 117 } }; /* Bank3: EMIO signals[32:63] */


struct {
	volatile u32 *base;
} gpio_common;


static int gpio_getPinPos(u8 pin, u8 *bank, u8 *pos)
{
	int i;

	if (pin > pinsPos[MAX_BANK_NB - 1].max)
		return -EINVAL;

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

	if (gpio_getPinPos(pin, &bank, &pos) < 0)
		return -EINVAL;

	v = *(gpio_common.base + data + (BANK_REGS_OFFS * bank));
	*(gpio_common.base + data + (BANK_REGS_OFFS * bank)) = (v & ~(1 << pos)) | (!!val << pos);

	return EOK;
}


int gpio_readPin(u8 pin, u8 *val)
{
	u32 v;
	u8 bank, pos;

	if (gpio_getPinPos(pin, &bank, &pos) < 0)
		return -EINVAL;

	v = *(gpio_common.base + data_ro + (BANK_REGS_OFFS * bank));
	*val = !!(v & (1 << pos));

	return EOK;
}


int gpio_getPinDir(u8 pin, u8 *dir)
{
	u32 d;
	u8 bank, pos;

	if (gpio_getPinPos(pin, &bank, &pos) < 0)
		return -EINVAL;

	d = *(gpio_common.base + dirm + (BANK_REGS_OFFS * bank));
	*dir = !!(d & (1 << pos));

	return EOK;
}


int gpio_setPinDir(u8 pin, u8 dir)
{
	u32 d, v, offs;
	u8 bank, pos;

	if (gpio_getPinPos(pin, &bank, &pos) < 0)
		return -EINVAL;

	offs = BANK_REGS_OFFS * bank;
	d = *(gpio_common.base + dirm + offs);
	v = (d & ~(1 << pos)) | (!!dir << pos);

	*(gpio_common.base + dirm + offs) = v;
	*(gpio_common.base + oen + offs) = v;

	return EOK;
}


int gpio_writeBank(u8 bank, u32 val)
{
	if (bank >= MAX_BANK_NB)
		return -EINVAL;

	*(gpio_common.base + data + (BANK_REGS_OFFS * bank)) = val;

	return EOK;
}


int gpio_readBank(u8 bank, u32 *val)
{
	if (bank >= MAX_BANK_NB)
		return -EINVAL;

	*val = *(gpio_common.base + data_ro + (BANK_REGS_OFFS * bank));

	return EOK;
}


int gpio_getBankDir(u8 bank, u32 *dir)
{
	if (bank >= MAX_BANK_NB)
		return -EINVAL;

	*dir = *(gpio_common.base + dirm + (BANK_REGS_OFFS * bank));

	return EOK;
}


int gpio_setBankDir(u8 bank, u32 dir)
{
	if (bank >= MAX_BANK_NB)
		return -EINVAL;

	*(gpio_common.base + dirm + (BANK_REGS_OFFS * bank)) = dir;
	*(gpio_common.base + oen + (BANK_REGS_OFFS * bank)) = dir;

	return EOK;
}


void gpio_init(void)
{
	gpio_common.base = GPIO_BASE_ADDR;

	_zynq_setAmbaClk(amba_gpio_clk, clk_enable);
}


void gpio_deinit(void)
{
	_zynq_setAmbaClk(amba_gpio_clk, clk_disable);
}
