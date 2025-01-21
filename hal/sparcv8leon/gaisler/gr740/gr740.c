/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR740 specific functions
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "gr740.h"
#include "hal/sparcv8leon/gaisler/l2cache.h"
#include "hal/sparcv8leon/gaisler/gaisler.h"

#include <types.h>

#define CGU_BASE     ((void *)0xffa04000)
#define GRGPREG_BASE ((void *)0xffa0b000)

/* Clock gating unit */

enum {
	cgu_unlock = 0, /* Unlock register        : 0x00 */
	cgu_clk_en,     /* Clock enable register  : 0x04 */
	cgu_core_reset, /* Core reset register    : 0x08 */
	cgu_override,   /* Override register      : 0x0C */
};

/* I/O & PLL configuration registers */

enum {
	ftmfunc = 0, /* FTMCTRL function enable register       : 0x00 */
	altfunc,     /* Alternate function enable register     : 0x04 */
	lvdsmclk,    /* LVDS and mem CLK pad enable register   : 0x08 */
	pllnewcfg,   /* PLL new configuration register         : 0x0C */
	pllrecfg,    /* PLL reconfigure command register       : 0x10 */
	pllcurcfg,   /* PLL current configuration register     : 0x14 */
	drvstr1,     /* Drive strength register 1              : 0x18 */
	drvstr2,     /* Drive strength register 2              : 0x1C */
	lockdown,    /* Configuration lockdown register        : 0x20 */
};


static const struct {
	vu32 *cguBase;
	vu32 *grgpregBase;
} gr740_common = {
	.cguBase = CGU_BASE,
	.grgpregBase = GRGPREG_BASE,
};


int gaisler_iomuxCfg(iomux_cfg_t *ioCfg)
{
	if (ioCfg->pin > 21) {
		return -1;
	}

	switch (ioCfg->opt) {
		case iomux_gpio:
			*(gr740_common.grgpregBase + ftmfunc) &= ~(1 << ioCfg->pin);
			*(gr740_common.grgpregBase + altfunc) &= ~(1 << ioCfg->pin);
			break;

		case iomux_alternateio:
			*(gr740_common.grgpregBase + ftmfunc) &= ~(1 << ioCfg->pin);
			*(gr740_common.grgpregBase + altfunc) |= 1 << ioCfg->pin;
			break;

		case iomux_promio:
			*(gr740_common.grgpregBase + ftmfunc) |= 1 << ioCfg->pin;
			break;

		default:
			return -1;
	}

	return 0;
}

/* CGU setup - section 25.2 GR740 manual */

void _gr740_cguClkEnable(u32 device)
{
	u32 msk = 1 << device;

	*(gr740_common.cguBase + cgu_unlock) |= msk;
	*(gr740_common.cguBase + cgu_core_reset) |= msk;
	*(gr740_common.cguBase + cgu_clk_en) |= msk;
	*(gr740_common.cguBase + cgu_clk_en) &= ~msk;
	*(gr740_common.cguBase + cgu_core_reset) &= ~msk;
	*(gr740_common.cguBase + cgu_clk_en) |= msk;
	*(gr740_common.cguBase + cgu_unlock) &= ~msk;
}


void _gr740_cguClkDisable(u32 device)
{
	u32 msk = 1 << device;

	*(gr740_common.cguBase + cgu_unlock) |= msk;
	*(gr740_common.cguBase + cgu_clk_en) &= ~msk;
	*(gr740_common.cguBase + cgu_unlock) &= ~msk;
}


int _gr740_cguClkStatus(u32 device)
{
	u32 msk = 1 << device;

	return (*(gr740_common.cguBase + cgu_clk_en) & msk) ? 1 : 0;
}


void _gr740_init(void)
{
	l2c_init((void *)0xf0000000);
}
