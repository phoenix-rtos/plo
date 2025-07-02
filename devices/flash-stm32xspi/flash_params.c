/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32 XSPI Flash driver
 * SFDP parameter table parser
 *
 * Copyright 2025 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>

#include "flash_params.h"


#define SFDP_SIGNATURE 0x50444653

/* Lookup table for checking support for different I/O types */
static const struct {
	u8 checkIdx;
	u8 checkShift;
	u8 opcodeIdx;
	u8 opcodeShift;
	u8 dummyIdx;
	u8 dummyShift;
	u8 modeCycIdx;
	u8 modeCycShift;
} sfdpOperationLookup[] = {
	[operation_io_112] = { 0, 16, 3, 8, 3, 0, 3, 5 },
	[operation_io_122] = { 0, 20, 3, 24, 3, 16, 3, 21 },
	[operation_io_114] = { 0, 22, 2, 24, 2, 16, 2, 21 },
	[operation_io_144] = { 0, 21, 2, 8, 2, 0, 2, 5 },
	[operation_io_222] = { 4, 0, 5, 24, 5, 16, 5, 21 },
	[operation_io_444] = { 4, 4, 6, 24, 6, 16, 6, 21 },
};

static const struct {
	u8 sizeIdx;
	u8 sizeShift;
	u8 opcodeShift;
	u8 timeShift;
} sfdpEraseLookup[] = {
	{ 7, 0, 8, 4 },
	{ 7, 16, 24, 11 },
	{ 8, 0, 8, 18 },
	{ 8, 16, 24, 25 },
};


void flashdrv_fillDefaultParams(flash_opParameters_t *res)
{
	res->opcodeType = flash_opcode_8b;
	res->readIoType = operation_io_111;
	res->readOpcode = 0x03; /* READ opcode */
	res->readDummy = 0;
	res->readModeCyc = 0;
	res->writeIoType = operation_io_111;
	res->writeOpcode = 0x02; /* PAGE PROGRAM opcode */
	res->writeDummy = 0;
	res->addrMode = ADDRMODE_3B;
	res->log_chipSize = 24;        /* 16 MB */
	res->eraseOpcode = 0xd8;       /* Sector erase */
	res->log_eraseSize = 16;       /* 64 KB sector size */
	res->log_pageSize = 8;         /* 256 B page size */
	res->eraseBlockTimeout = 1000; /* 1 second to erase block */
	res->eraseChipTimeout = 60000; /* 60 seconds to erase chip */
}


static u32 flashdrv_calcEraseTime(u8 timeValue, int isChipErase)
{
	static const u16 eraseChipUnits[4] = { 16, 256, 4000, 64000 };
	static const u16 eraseBlockUnits[4] = { 1, 16, 128, 1000 };
	if (isChipErase != 0) {
		return eraseChipUnits[(timeValue >> 5) & 0x3] * (timeValue & 0x1f);
	}
	else {
		return eraseBlockUnits[(timeValue >> 5) & 0x3] * (timeValue & 0x1f);
	}
}


int flashdrv_parseSfdp(const u32 *data, flash_opParameters_t *res, int tryMultiIoCmd)
{
	unsigned n_headers = 0, i;
	const u32 *header_table = &data[2];
	u32 ptable_len = 0, ptable_offset, log_sizeBits;
	u8 log_eraseSize = 0, eraseOpcode = 0, eraseTimeShift = 0;
	u8 candidate_size, eraseTimeValue, eraseTimeoutMultiplier;
	const u32 *ptable = NULL;
	if (data[0] != SFDP_SIGNATURE) {
		return -EINVAL;
	}

	if (((data[1] >> 8) & 0xff) != 0x01) {
		/* Major revision different from 0x01 - format may not be compatible */
		return -EINVAL;
	}

	n_headers = ((data[1] >> 16) & 0xff) + 1;
	for (i = 0; i < n_headers; i++) {
		/* We are only looking for JEDEC Basic Parameter Table with a major revision of 0x01 */
		if (((header_table[i * 2] & 0xff00ff) != 0x010000) || (header_table[i * 2 + 1] >> 24) != 0xff) {
			continue;
		}

		ptable_len = header_table[i * 2] >> 24;
		ptable_offset = header_table[i * 2 + 1] & 0xffffff;
		ptable = &data[ptable_offset / 4];
	}

	if ((ptable == NULL) || (ptable_len < 2)) {
		return -EINVAL;
	}

	res->addrMode = (ptable[0] >> 17) & 0x3;
	if (res->addrMode == 0x3) {
		/* Reserved value - set to default 3-byte mode */
		res->addrMode = ADDRMODE_3B;
	}

	/* Determine size */
	if (ptable[1] < 0x80000000) {
		if (ptable[1] < 8) {
			return -EINVAL;
		}

		res->log_chipSize = 32 - __builtin_clz(ptable[1]) - 3;
	}
	else {
		log_sizeBits = ptable[1] & 0x7fffffff;
		if ((log_sizeBits < 3) || (log_sizeBits > (255 + 3))) {
			/* < 1 byte flash and flash larger than the universe not supported */
			return -EINVAL;
		}

		res->log_chipSize = log_sizeBits - 3;
	}

	res->readIoType = operation_io_111;
	res->readOpcode = 0x3;
	res->readDummy = 0;
	/* Determine fastest I/O mode */
	i = (tryMultiIoCmd == 0) ? operation_io_144 : operation_io_444;
	for (; i >= operation_io_112; i--) {
		if (sfdpOperationLookup[i].checkIdx >= ptable_len) {
			continue;
		}

		if (((ptable[sfdpOperationLookup[i].checkIdx] >> sfdpOperationLookup[i].checkShift) & 1) != 0) {
			res->readIoType = i;
			res->readOpcode = (ptable[sfdpOperationLookup[i].opcodeIdx] >> sfdpOperationLookup[i].opcodeShift) & 0xff;
			res->readDummy = (ptable[sfdpOperationLookup[i].dummyIdx] >> sfdpOperationLookup[i].dummyShift) & 0x1f;
			res->readModeCyc = (ptable[sfdpOperationLookup[i].modeCycIdx] >> sfdpOperationLookup[i].modeCycShift) & 0x7;
			break;
		}
	}

	if (ptable_len >= 9) {
		/* Find largest available erase operation */
		for (i = 0; i < 4; i++) {
			candidate_size = (ptable[sfdpEraseLookup[i].sizeIdx] >> sfdpEraseLookup[i].sizeShift) & 0xff;
			if (candidate_size > log_eraseSize) {
				eraseOpcode = (ptable[sfdpEraseLookup[i].sizeIdx] >> sfdpEraseLookup[i].opcodeShift) & 0xff;
				log_eraseSize = candidate_size;
				eraseTimeShift = sfdpEraseLookup[i].timeShift;
			}
		}
	}

	if (log_eraseSize != 0) {
		res->log_eraseSize = log_eraseSize;
		res->eraseOpcode = eraseOpcode;
		if (ptable_len >= 10) {
			eraseTimeoutMultiplier = 2 * ((ptable[9] & 0xf) + 1);
			eraseTimeValue = ptable[9] >> eraseTimeShift;
			res->eraseBlockTimeout = eraseTimeoutMultiplier * flashdrv_calcEraseTime(eraseTimeValue, 0);
		}
	}

	if (ptable_len >= 11) {
		eraseTimeoutMultiplier = 2 * ((ptable[9] & 0xf) + 1);
		eraseTimeValue = ptable[10] >> 24;
		res->eraseChipTimeout = eraseTimeoutMultiplier * flashdrv_calcEraseTime(eraseTimeValue, 1);
		res->log_pageSize = (ptable[10] >> 4) & 0xf;
	}

	return 0;
}
