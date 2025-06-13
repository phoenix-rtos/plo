/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * STM32 XSPI Flash driver
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
 * operation_io_xyz means x lines are used for command phase, y for address phase and z for data phase.
 * "d" suffix means that DTR is used.
 */
enum operation_io_type {
	operation_io_111 = 0,
	operation_io_112,
	operation_io_122,
	operation_io_114,
	operation_io_144,
	operation_io_222,
	operation_io_444,
	operation_io_444d,
	operation_io_888,
	operation_io_888d,
	operation_io_types,
};

#define ADDRMODE_3B  0 /* 3-byte only mode */
#define ADDRMODE_4BO 1 /* 4-byte optional mode */
#define ADDRMODE_4B  2 /* 4-byte only mode */


enum flash_opcode_type {
	flash_opcode_8b = 0,     /* Command is 8 bits */
	flash_opcode_8b_repeat,  /* Command is 16 bits - the same 8 bits repeated twice */
	flash_opcode_8b_inverse, /* Command is 16 bits - 8 bits followed by inverse of these 8 bits */
	flash_opcode_16b,        /* Command is 16 bits */
};

typedef struct {
	u16 readOpcode;        /* Opcode to perform a read operation */
	u16 writeOpcode;       /* Opcode to perform a program operation */
	u8 opcodeType;         /* One of enum flash_opcode_type */
	u8 readIoType;         /* One of enum operation_io_type */
	u8 readModeCyc;        /* Mode cycles needed for a read operation */
	u8 readDummy;          /* Dummy cycles needed for a read operation */
	u8 writeIoType;        /* One of enum operation_io_type */
	u8 writeDummy;         /* Dummy cycles needed for a program operation */
	u8 eraseOpcode;        /* Opcode to perform an erase operation */
	u8 addrMode;           /* One of ADDRMODE_* */
	u8 log_chipSize;       /* log2 of chip size in bytes */
	u8 log_eraseSize;      /* log2 of erase size in bytes */
	u8 log_pageSize;       /* log2 of page size in bytes */
	u32 eraseBlockTimeout; /* Max time in ms to erase block of (1 << log_eraseSize) bytes */
	u32 eraseChipTimeout;  /* Max time in ms to erase the whole chip */
} flash_opParameters_t;


/* Initializes structure with default or reasonable parameters of typical JEDEC Flash memory */
void flashdrv_fillDefaultParams(flash_opParameters_t *res);


/* Parses SFDP data block from pointer `data`. Results will be stored in `res`.
 * If `tryMultiIoCmd` is set to 1, will attempt to find support for 2-2-2 and 4-4-4 I/O modes,
 * otherwise only 1-*-* modes will be considered.
 */
int flashdrv_parseSfdp(const u32 *data, flash_opParameters_t *res, int tryMultiIoCmd);


#endif /* _FLASH_PARAMS_H_ */
