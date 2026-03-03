/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GRLIB NANDFCTRL2 flash driver
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FLASH_NANDFCTRL2_H_
#define FLASH_NANDFCTRL2_H_


#include <types.h>


#define NAND_MAX_PAGE_SIZE        ((8 * 1024) + 256) /* 8KB page + 256B spare */
#define NANDFCTRL2_BBT_MAX_BLOCKS 4096               /* Maximum blocks per die for the Bad Block Table */


/* NAND flash configuration */
typedef struct {
	const char *name;  /* Flash device name string */
	u64 size;          /* Total NAND size in bytes */
	u32 writesz;       /* Page data size in bytes */
	u32 sparesz;       /* Total spare (OOB) bytes per page */
	u32 spareavail;    /* User-accessible spare bytes */
	u32 erasesz;       /* Erase block size in bytes */
	u32 pagesPerBlock; /* Pages per erase block */
	u32 eccChunksz;    /* ECC chunk size in bytes */
	u32 eccsz;         /* ECC bytes per chunk */
	u32 eccCap;        /* ECC correction capability in bits */
} nandfctrl2_info_t;


typedef struct {
	u32 minor;
	nandfctrl2_info_t info;

	/* Bad Block Table */
	u32 bbt[NANDFCTRL2_BBT_MAX_BLOCKS / 32]; /* BBT bitmap */
} nand_die_t;


/* Common buffer, exported for use in data.c/meta.c/raw.c */
extern u8 nandfctrl2_rawBuffer[NAND_MAX_PAGE_SIZE] __attribute__((aligned(64)));


nand_die_t *nand_get(u32 minor);


int nandfctrl2_resetFlash(const nand_die_t *nand);


int nandfctrl2_pageWrite(const nand_die_t *nand, u32 page, const void *data);


int nandfctrl2_pageRead(const nand_die_t *nand, u32 page, void *data);


/* Read a page + entire metadata in raw mode. Output buffer must be at least writesz + sparesz bytes. */
int nandfctrl2_rawPageRead(const nand_die_t *nand, u32 page, void *data);


int flashdrv_metaWrite(const nand_die_t *nand, u32 page, const void *metadata, size_t size);


int flashdrv_metaRead(const nand_die_t *nand, u32 page, void *metadata, size_t size);


int nandfctrl2_eraseBlock(const nand_die_t *nand, u32 block);


int nandfctrl2_isbad(const nand_die_t *nand, u32 block);


int nandfctrl2_markbad(nand_die_t *nand, u32 block);


int nandfctrl2_init(nand_die_t *nand);


#endif
