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

#include <types.h>
#include <config.h>


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


static inline int ftmctrl_portWidth(void)
{
	return (((*(vu32 *)FTMCTRL_BASE >> 8) & 0x3) == 0) ? 8 : 16;
}


#endif
