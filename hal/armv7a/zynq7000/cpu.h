/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * ARMv7 Cortex - A9 related routines for Zynq - 7000
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
#define USR_MODE        0x10    /* unprivileged mode in which most applications run                           */
#define FIQ_MODE        0x11    /* entered on an FIQ interrupt exception                                      */
#define IRQ_MODE        0x12    /* entered on an IRQ interrupt exception                                      */
#define SVC_MODE        0x13    /* entered on reset or when a Supervisor Call instruction ( SVC ) is executed */
#define MON_MODE        0x16    /* security extensions                                                        */
#define ABT_MODE        0x17    /* entered on a memory access exception                                       */
#define HYP_MODE        0x1a    /* virtualization extensions                                                  */
#define UND_MODE        0x1b    /* entered when an undefined instruction executed                             */
#define SYS_MODE        0x1f    /* privileged mode, sharing the register view with User mode                  */

#define MODE_MASK       0x1f
#define NO_ABORT        0x100               /* mask to disable Abort Exception */
#define NO_IRQ          0x80                /* mask to disable IRQ             */
#define NO_FIQ          0x40                /* mask to disable FIQ             */
#define NO_INT          (NO_IRQ | NO_FIQ)   /* mask to disable IRQ and FIQ     */
#define THUMB_STATE     0x20

/* Stack definition */
#define ADDR_STACK      0xfffffff0          /* Hihgh adress of OCRAM */
#define SIZE_STACK      5 * 1024


/* Zynq-7000 System Adress Map */
#define ADDR_OCRAM_LOW   0x00000000
#define SIZE_OCRAM_LOW   192 * 1024
#define ADDR_OCRAM_HIGH  0xffff0000
#define SIZE_OCRAM_HIGH  64 * 1024

#define ADDR_DDR         0x00100000
#define SIZE_DDR         512 * 1024 * 1024

#endif
