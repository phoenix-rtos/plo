/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARMv7 Cortex-R
 *
 * Copyright 2021, 2024 Phoenix Systems
 * Author: Hubert Buczynski, Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CPU_H_
#define _CPU_H_


/* ARMv7 processor modes */
#define MODE_USR 0x10 /* unprivileged mode in which most applications run                           */
#define MODE_FIQ 0x11 /* entered on an FIQ interrupt exception                                      */
#define MODE_IRQ 0x12 /* entered on an IRQ interrupt exception                                      */
#define MODE_SVC 0x13 /* entered on reset or when a Supervisor Call instruction ( SVC ) is executed */
#define MODE_MON 0x16 /* security extensions                                                        */
#define MODE_ABT 0x17 /* entered on a memory access exception                                       */
#define MODE_HYP 0x1a /* virtualization extensions                                                  */
#define MODE_UND 0x1b /* entered when an undefined instruction executed                             */
#define MODE_SYS 0x1f /* privileged mode, sharing the register view with User mode                  */

#define MODE_MASK   0x1f
#define NO_ABORT    0x100             /* mask to disable Abort Exception */
#define NO_IRQ      0x80              /* mask to disable IRQ             */
#define NO_FIQ      0x40              /* mask to disable FIQ             */
#define NO_INT      (NO_IRQ | NO_FIQ) /* mask to disable IRQ and FIQ     */
#define THUMB_STATE 0x20


#ifndef __ASSEMBLY__


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


#endif


#endif
