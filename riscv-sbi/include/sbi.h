/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * SBI functions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _SBI_H_
#define _SBI_H_


#define SBI_SUCCESS               0
#define SBI_ERR_FAILED            -1
#define SBI_ERR_NOT_SUPPORTED     -2
#define SBI_ERR_INVALID_PARAM     -3
#define SBI_ERR_DENIED            -4
#define SBI_ERR_INVALID_ADDRESS   -5
#define SBI_ERR_ALREADY_AVAILABLE -6
#define SBI_ERR_ALREADY_STARTED   -7
#define SBI_ERR_ALREADY_STOPPED   -8
#define SBI_ERR_NO_SHMEM          -9

/* Supported SBI extensions */

#define SBI_EXT_0_1_CONSOLE_PUTCHAR 0x1
#define SBI_EXT_0_1_CONSOLE_GETCHAR 0x2

#define SBI_EXT_BASE   0x10
#define SBI_EXT_TIME   0x54494D45
#define SBI_EXT_IPI    0x735049
#define SBI_EXT_RFENCE 0x52464E43
#define SBI_EXT_HSM    0x48534D


/* HSM extension: Hart states */
#define SBI_HSM_STARTED         0
#define SBI_HSM_STOPPED         1
#define SBI_HSM_START_PENDING   2
#define SBI_HSM_STOP_PENDING    3
#define SBI_HSM_SUSPENDED       4
#define SBI_HSM_SUSPEND_PENDING 5
#define SBI_HSM_RESUME_PENDING  6


#define SIZEOF_SBI_PERHARTDATA 40


#ifndef __ASSEMBLY__


#include "types.h"


typedef unsigned long sbi_param;


typedef struct {
	long error;
	long value;
} sbiret_t;


typedef struct {
	int eid;
	sbiret_t (*handler)(sbi_param a0, sbi_param a1, sbi_param a2, sbi_param a3, sbi_param a4, sbi_param a5, int fid);
} sbi_ext_t;


typedef struct {
	addr_t mstack;            /* M-mode sp */
	addr_t scratch;           /* temporary storage */
	volatile addr_t state;    /* current hart state */
	volatile addr_t nextArg1; /* 'a1' register for next boot stage */
	volatile addr_t nextAddr; /* address of next boot stage */
} __attribute__((packed, aligned(8))) sbi_perHartData_t;


_Static_assert(sizeof(sbi_perHartData_t) == SIZEOF_SBI_PERHARTDATA, "sbi_perHartData_t size changed, update SIZEOF_SBI_PERHARTDATA");


sbi_perHartData_t *sbi_getPerHartData(u32 hartid);


u32 sbi_getHartCount(void);


void __attribute__((noreturn)) sbi_initCold(u32 hartid, const void *fdt);


void __attribute__((noreturn)) sbi_initWarm(u32 hartid);


#endif


#endif
