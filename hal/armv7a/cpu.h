/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARMv7 Cortex-A
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _CPU_H_
#define _CPU_H_


/* ARMv7 processor modes */
#define USR_MODE 0x10 /* unprivileged mode in which most applications run                           */
#define FIQ_MODE 0x11 /* entered on an FIQ interrupt exception                                      */
#define IRQ_MODE 0x12 /* entered on an IRQ interrupt exception                                      */
#define SVC_MODE 0x13 /* entered on reset or when a Supervisor Call instruction ( SVC ) is executed */
#define MON_MODE 0x16 /* security extensions                                                        */
#define ABT_MODE 0x17 /* entered on a memory access exception                                       */
#define HYP_MODE 0x1a /* virtualization extensions                                                  */
#define UND_MODE 0x1b /* entered when an undefined instruction executed                             */
#define SYS_MODE 0x1f /* privileged mode, sharing the register view with User mode                  */

#define MODE_MASK   0x1f
#define NO_ABORT    0x100             /* mask to disable Abort Exception */
#define NO_IRQ      0x80              /* mask to disable IRQ             */
#define NO_FIQ      0x40              /* mask to disable FIQ             */
#define NO_INT      (NO_IRQ | NO_FIQ) /* mask to disable IRQ and FIQ     */
#define THUMB_STATE 0x20


#ifndef __ASSEMBLY__

static inline void hal_cpuDataMemoryBarrier(void)
{
	__asm__ volatile ("dmb");
}


static inline void hal_cpuDataSyncBarrier(void)
{
	__asm__ volatile ("dsb");
}


static inline void hal_cpuInstrBarrier(void)
{
	__asm__ volatile ("isb");
}


static inline void hal_cpuHalt(void)
{
	__asm__ volatile ("wfi");
}

#endif

#endif
