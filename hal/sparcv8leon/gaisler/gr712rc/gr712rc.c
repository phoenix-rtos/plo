/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR712RC specific functions
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "gr712rc.h"
#include "../gaisler.h"
#include "../cpu.h"


#define CGU_BASE ((void *)0x80000D00)

/* Clock gating unit */

#define CGU_UNLOCK     0 /* Unlock register        : 0x00 */
#define CGU_CLK_EN     1 /* Clock enable register  : 0x04 */
#define CGU_CORE_RESET 2 /* Core reset register    : 0x08 */


static struct {
	vu32 *cgu_base;
} gr712rc_common;


int gaisler_iomuxCfg(iomux_cfg_t *ioCfg)
{
	(void)ioCfg;

	return 0;
}

/* CGU setup - section 28.2 GR712RC manual */

void _gr712rc_cguClkEnable(u32 device)
{
	u32 msk = 1 << device;

	*(gr712rc_common.cgu_base + CGU_UNLOCK) |= msk;
	*(gr712rc_common.cgu_base + CGU_CORE_RESET) |= msk;
	*(gr712rc_common.cgu_base + CGU_CLK_EN) |= msk;
	*(gr712rc_common.cgu_base + CGU_CORE_RESET) &= ~msk;
	*(gr712rc_common.cgu_base + CGU_UNLOCK) &= ~msk;
}


void _gr712rc_cguClkDisable(u32 device)
{
	u32 msk = 1 << device;

	*(gr712rc_common.cgu_base + CGU_UNLOCK) |= msk;
	*(gr712rc_common.cgu_base + CGU_CORE_RESET) |= msk;
	*(gr712rc_common.cgu_base + CGU_CLK_EN) &= ~msk;
	*(gr712rc_common.cgu_base + CGU_UNLOCK) &= ~msk;
}


int _gr712rc_cguClkStatus(u32 device)
{
	u32 msk = 1 << device;

	return (*(gr712rc_common.cgu_base + CGU_CLK_EN) & msk) ? 1 : 0;
}


void _gr712rc_init(void)
{
	gr712rc_common.cgu_base = CGU_BASE;
}
