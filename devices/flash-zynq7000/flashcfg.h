/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Common Flash Interface for flash driver
 *
 * Copyright 2021-2022 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _FLASHCFG_H_
#define _FLASHCFG_H_

#include <hal/hal.h>

/* Return timeouts in ms */
#define CFI_TIMEOUT_MAX_PROGRAM(typical, max) (((1 << typical) * (1 << max)) / 1000)
#define CFI_TIMEOUT_MAX_ERASE(typical, max)   ((1 << typical) * (1 << max))

/* Return size in bytes */
#define CFI_SIZE_FLASH(val)                (1 << val)
#define CFI_SIZE_SECTION(val)              (val * 0x100)
#define CFI_SIZE_PAGE(val)                 (1 << val)
#define CFI_SIZE_REGION(regSize, regCount) (CFI_SIZE_SECTION(regSize) * (regCount + 1))


/* Order in command's table */
enum {
	flash_cmd_rdid = 0,
	flash_cmd_rdsr1,
	flash_cmd_wrdi,
	flash_cmd_wren,
	flash_cmd_read,
	flash_cmd_4read,
	flash_cmd_fast_read,
	flash_cmd_4fast_read,
	flash_cmd_dor,
	flash_cmd_4dor,
	flash_cmd_qor,
	flash_cmd_4qor,
	flash_cmd_dior,
	flash_cmd_4dior,
	flash_cmd_qior,
	flash_cmd_4qior,
	flash_cmd_pp,
	flash_cmd_4pp,
	flash_cmd_qpp,
	flash_cmd_4qpp,
	flash_cmd_p4e,
	flash_cmd_4p4e,
	flash_cmd_p64e,
	flash_cmd_4p64e,
	flash_cmd_be,
	flash_cmd_end
};


typedef struct {
	u8 byteWrite;
	u8 pageWrite;
	u8 sectorErase;
	u8 chipErase;
} __attribute__((packed)) flash_cfi_timeout_t;


typedef struct {
	u8 vendorData[0x10];
	u8 qry[3];
	u16 cmdSet1;
	u16 addrExt1;
	u16 cmdSet2;
	u16 addrExt2;
	struct {
		u8 vccMin;
		u8 vccMax;
		u8 vppMin;
		u8 vppMax;
	} __attribute__((packed)) voltages;
	flash_cfi_timeout_t timeoutTypical;
	flash_cfi_timeout_t timeoutMax;
	u8 chipSize;
	u16 fdiDesc;
	u16 pageSize;
	u8 regsCount;
	struct {
		u16 count;
		u16 size;
	} __attribute__((packed)) regs[4];
} __attribute__((packed)) flash_cfi_t;


typedef struct {
	u8 opCode;
	u8 size;
	u8 dummyCyc;
} flash_cmd_t;


typedef struct {
	flash_cfi_t cfi;
	flash_cmd_t cmds[flash_cmd_end];

	enum { flash_3byteAddr, flash_4byteAddr } addrMode; /* Address mode based on chip size */
	int readCmd;                              /* Default read command define for specific flash memory */
	int ppCmd;                                /* Default page program command define for specific flash memory */
	const char *name;
} flash_info_t;


extern void flashcfg_jedecIDGet(flash_cmd_t *cmd);


extern int flashcfg_infoResolve(flash_info_t *info);


#endif
