/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * LEON Generic specific functions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "generic.h"
#include "hal/sparcv8leon/gaisler/gaisler.h"


int gaisler_iomuxCfg(iomux_cfg_t *ioCfg)
{
	(void)ioCfg;

	return 0;
}
