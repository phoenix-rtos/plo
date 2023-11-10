/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * FTMCTRL routines
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _FTMCTRL_H_
#define _FTMCTRL_H_

#include <hal/hal.h>


#define FTMCTRL_BASE 0x80000000


static inline void ftmctrl_WrEn(void)
{
	*(vu32 *)FTMCTRL_BASE |= (1 << 11);
}


static inline void ftmctrl_WrDis(void)
{
	*(vu32 *)FTMCTRL_BASE &= ~(1 << 11);
}

static inline void ftmctrl_ioEn(void)
{
	*(vu32 *)FTMCTRL_BASE |= (1 << 19);
}


static inline void ftmctrl_ioDis(void)
{
	*(vu32 *)FTMCTRL_BASE &= ~(1 << 19);
}


#endif
