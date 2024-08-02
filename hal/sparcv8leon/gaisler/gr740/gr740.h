/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR740 specific functions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _GR740_H_
#define _GR740_H_

#include <types.h>

/* clang-format off */

/* Clock gating unit devices */
enum { cgudev_greth0 = 0, cgudev_greth1, cgudev_spwrouter, cgudev_pci, cgudev_milStd1553, cgudev_can, cgudev_leon4stat,
	cgudev_apbuart0, cgudev_apbuart1, cgudev_spi, cgudev_promctrl };

/* Pin mux config */
enum { iomux_gpio = 0, iomux_alternateio, iomux_promio };

/* clang-format on */


static inline void hal_cpuHalt(void)
{
	/* must be performed in supervisor mode with int enabled */
	__asm__ volatile("wr %g0, %asr19");
}


void _gr740_cguClkEnable(u32 device);


void _gr740_cguClkDisable(u32 device);


int _gr740_cguClkStatus(u32 device);


void _gr740_init(void);


#endif
