/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARMv8 Cortex-M
 *
 * Copyright 2021, 2022 Phoenix Systems
 * Author: Hubert Buczynski, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

/* Copy of armv7 Cortex-M architecture code */


#ifndef _HAL_CPU_H_
#define _HAL_CPU_H_


#include <hal/hal.h>


static inline void hal_cpuDataMemoryBarrier(void)
{
	__asm__ volatile("dmb");
}


static inline void hal_cpuDataSyncBarrier(void)
{
	__asm__ volatile("dsb");
}


static inline void hal_cpuInstrBarrier(void)
{
	__asm__ volatile("isb");
}


static inline void hal_cpuHalt(void)
{
	__asm__ volatile("wfi");
}


extern void hal_scbSetPriorityGrouping(u32 group);


extern u32 hal_scbGetPriorityGrouping(void);


extern void hal_scbSetPriority(s8 excpn, u32 priority);


extern u32 hal_scbGetPriority(s8 excpn);


extern void hal_enableDCache(void);


extern void hal_enableICache(void);


extern void hal_disableDCache(void);


extern void hal_disableICache(void);


extern void hal_cleanDCache(void);


extern void hal_invalDCacheAddr(void *addr, u32 sz);


extern void hal_invalDCacheAll(void);


extern unsigned int hal_cpuID(void);


extern void hal_cpuReboot(void);


extern void hal_cpuInit(void);


#endif
