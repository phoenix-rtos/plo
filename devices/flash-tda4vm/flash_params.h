/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * TDA4VM Flash driver
 *
 * Copyright 2025 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _FLASH_PARAMS_H_
#define _FLASH_PARAMS_H_

#include <hal/hal.h>

/* I/O types - how many I/O lines are used for each phase of the operation.
 * OPERATION_IO_xyz means x lines are used for command phase, y for address phase and z for data phase.
 */
enum operation_io_type {
	OPERATION_IO_111 = 0,
	OPERATION_IO_112,
	OPERATION_IO_122,
	OPERATION_IO_114,
	OPERATION_IO_144,
	OPERATION_IO_222,
	OPERATION_IO_444,
	OPERATION_IO_TYPES,
};

#define ADDRMODE_3B  0 /* 3-byte only mode */
#define ADDRMODE_4BO 1 /* 4-byte optional mode */
#define ADDRMODE_4B  2 /* 4-byte only mode */


typedef struct {
	u8 readIoType;    /* One of enum operation_io_type */
	u8 readOpcode;    /* Opcode to perform a read operation */
	u8 readDummy;     /* Dummy cycles needed for a read operation */
	u8 writeIoType;   /* One of enum operation_io_type */
	u8 writeOpcode;   /* Opcode to perform a program operation */
	u8 writeDummy;    /* Dummy cycles needed for a program operation */
	u8 eraseOpcode;   /* Opcode to perform an erase operation */
	u8 addrMode;      /* One of ADDRMODE_* */
	u8 log_chipSize;  /* log2 of chip size in bytes */
	u8 log_eraseSize; /* log2 of erase size in bytes */
	u8 log_pageSize;  /* log2 of page size in bytes */
} flash_opParameters_t;


/* Initializes structure with minimum compatible parameters of typical JEDEC Flash memory */
void flashdrv_fillDefaultParams(flash_opParameters_t *res);


/* Parses SFDP data block from pointer `data`.
 * If `tryMultiIoCmd` is set to 1, will attempt to find support for 2-2-2 and 4-4-4 I/O modes,
 * otherwise only 1-*-* modes will be considered.
 */
int flashdrv_parseSfdp(const u32 *data, flash_opParameters_t *res, int tryMultiIoCmd);


#endif /* _FLASH_PARAMS_H_ */
