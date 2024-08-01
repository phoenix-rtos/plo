/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR716 specific functions
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _GR716_H_
#define _GR716_H_

#include "../types.h"

/* clang-format off */

enum { cgu_primary = 0, cgu_secondary };

/* Clock gating unit (primary) devices */

enum { cgudev_spi2ahb = 0, cgudev_i2c2ahb, cgudev_grpwrx, cgudev_grpwtx, cgudev_ftmctrl, cgudev_spimctrl0, cgudev_spimctrl1, 
cgudev_spictrl0, cgudev_spictrl1, cgudev_i2cmst0, cgudev_i2cmst1, cgudev_i2clsv0, cgudev_i2clsv1, cgudev_grdacadc,
cgudev_grpwm1, cgudev_grpwm2, cgudev_apbuart0, cgudev_apbuart1, cgudev_apbuart2, cgudev_apbuart3, cgudev_apbuart4,
cgudev_apbuart5, cgudev_ioDisable = 23, cgudev_l3stat, cgudev_ahbuart, cgudev_memprot, cgudev_asup, cgudev_grspwtdp,
cgudev_spi4s, cgudev_nvram };

/* Clock gating unit (secondary) devices */

enum { cgudev_grdmac0 = 0, cgudev_grdmac1, cgudev_grdmac2, cgudev_grdmac3, cgudev_gr1553b, cgudev_grcan0, cgudev_grcan1, 
cgudev_grspw, cgudev_grdac0, cgudev_grdac1, cgudev_grdac2, cgudev_grdac3, cgudev_gradc0, cgudev_gradc1, cgudev_gradc2,
cgudev_gradc3, cgudev_gradc4, cgudev_gradc5, cgudev_gradc6, cgudev_gradc7, cgudev_gpioseq0, cgudev_gpioseq1 };

/* clang-format on */


static inline void hal_cpuHalt(void)
{
	/* must be performed in supervisor mode with int enabled */
	__asm__ volatile("wr %g0, %asr19");
}


void _gr716_cguClkEnable(u32 cgu, u32 device);


void _gr716_cguClkDisable(u32 cgu, u32 device);


int _gr716_cguClkStatus(u32 cgu, u32 device);


void _gr716_init(void);


#endif
