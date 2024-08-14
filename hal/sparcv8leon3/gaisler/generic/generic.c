/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * LEON3 Generic specific functions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "generic.h"
#include "hal/sparcv8leon3/gaisler/gaisler.h"


int gaisler_iomuxCfg(iomux_cfg_t *ioCfg)
{
	(void)ioCfg;

	return 0;
}
