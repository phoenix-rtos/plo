/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GRLIB FTMCTRL Flash driver
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CFI_H_
#define _CFI_H_


#include <types.h>

/* Timeouts in ms */
#define CFI_TIMEOUT_MAX_PROGRAM(typical, maximum) (((1u << (typical)) * (1u << (maximum))) / 1000u)
#define CFI_TIMEOUT_MAX_ERASE(typical, maximum)   ((1u << (typical)) * (1u << (maximum)))

#define CFI_SIZE(size) (1u << ((u32)size))

/* Common flash commands */
#define CMD_RD_QUERY  0x98u /* Read/Enter Query */
#define CMD_RD_STATUS 0x70u /* Read Status Register */

/* Intel command set */
#define INTEL_CMD_RESET      0xffu /* Reset/Read Array */
#define INTEL_CMD_WR_BUF     0xe8u /* Write to Buffer */
#define INTEL_CMD_WR_CONFIRM 0xd0u /* Write Confirm */
#define INTEL_CMD_CLR_STATUS 0x50u /* Clear Status Register */
#define INTEL_CMD_BE_CYC1    0x20u /* Block Erase (1st bus cycle) */

/* AMD command set */
#define AMD_CMD_RESET      0xf0u /* Reset/ASO Exit */
#define AMD_CMD_WR_BUF     0x25u /* Write to Buffer */
#define AMD_CMD_WR_CONFIRM 0x29u /* Write Confirm */
#define AMD_CMD_CLR_STATUS 0x71u /* Clear Status Register */
#define AMD_CMD_CE_CYC1    0x80u /* Chip Erase (1st bus cycle) */
#define AMD_CMD_CE_CYC2    0x10u /* Chip Erase (2nd bus cycle) */
#define AMD_CMD_BE_CYC1    0x80u /* Block Erase (1st bus cycle) */
#define AMD_CMD_BE_CYC2    0x30u /* Block Erase (2nd bus cycle) */
#define AMD_CMD_EXIT_QUERY 0xffu /* Exit Query */


typedef struct {
	u8 (*statusRead)(void);
	int (*statusCheck)(const char *msg);
	void (*statusClear)(void);
	void (*issueReset)(void);
	void (*issueWriteBuffer)(addr_t sectorAddr, addr_t programAddr, size_t len);
	void (*issueWriteConfirm)(addr_t sectorAddr);
	void (*issueSectorErase)(addr_t addr);
	void (*issueChipErase)(void);
	void (*enterQuery)(addr_t sectorAddr);
	void (*exitQuery)(void);
} cfi_ops_t;


typedef struct {
	const u8 statusRdyMask;
	const u8 usePolling;
	const u8 chipWidth;
	const u8 vendor;
	const u16 device;
	const char *name;
	const cfi_ops_t *ops;
} cfi_dev_t;


typedef struct {
	u8 wordProgram;
	u8 bufWrite;
	u8 blkErase;
	u8 chipErase;
} __attribute__((packed)) cfi_timeout_t;


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
	cfi_timeout_t toutTypical;
	cfi_timeout_t toutMax;
	u8 chipSz;
	u16 fdiDesc;
	u16 bufSz;
	u8 regionCnt;
	struct {
		u16 count;
		u16 size;
	} __attribute__((packed)) regions[4];
} __attribute__((packed)) cfi_info_t;


void amd_register(void);


void intel_register(void);


#endif
