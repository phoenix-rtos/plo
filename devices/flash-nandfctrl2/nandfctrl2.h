/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GRLIB NANDFCTRL2 flash driver
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Leckowski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FLASH_NANDFCTRL2_DRV_H_
#define FLASH_NANDFCTRL2_DRV_H_


#include <types.h>


typedef struct _nanddrv_dma_t nandfctrl2_dma_t;


/* clang-format off */
enum {
	cmd_read, cmd_read_multiplane, cmd_copyback_read, cmd_change_read_column, cmd_change_read_column_enhanced,
	cmd_read_cache_random, cmd_read_cache_sequential, cmd_read_cache_end, cmd_block_erase, cmd_block_erase_multiplane,
	cmd_read_status, cmd_read_status_enhanced, cmd_page_program, cmd_page_program_multiplane, cmd_page_cache_program,
	cmd_copyback_program, cmd_copyback_program_multiplane, cmd_small_data_move, cmd_change_write_column,
	cmd_change_row_address, cmd_read_id, cmd_volume_select, cmd_odt_configure, cmd_read_parameter_page,
	cmd_read_unique_id, cmd_get_features, cmd_set_features, cmd_lun_get_features, cmd_lun_set_features,
	cmd_zq_calibration_short, cmd_zq_calibration_long, cmd_reset_lun, cmd_synchronous_reset, cmd_reset,
	cmd_count
};
/* clang-format on */


/* possible values of nanddrv_meta_t->errors[] */
enum {
	flash_no_errors = 0,
	/* 0x01 - 0x28: number of bits corrected */
	flash_uncorrectable = 0xfe,
	flash_erased = 0xff
};


typedef struct {
	u8 metadata[16]; /* externally usable (by FS) */
	u8 errors[9];    /* ECC status: (one of the above enums) */
} nandfctrl2_meta_t;


/* information about NAND flash configuration */
typedef struct {
	const char *name;
	u64 size;     /* total NAND size in bytes */
	u32 writesz;  /* write page DATA size in bytes */
	u32 metasz;   /* write page METADATA size in bytes */
	u32 erasesz;  /* erase block size in bytes (multiply of writesize) */
	u32 oobsz;    /* out-of-bound (oob) data size */
	u32 oobavail; /* available out-of-bound (oob) data size */
} nandfctrl2_info_t;


/* paddr: page address, so NAND address / writesz */

nandfctrl2_dma_t *nandfctrl2_dma(void);


int nandfctrl2_reset(nandfctrl2_dma_t *dma);


int nandfctrl2_write(nandfctrl2_dma_t *dma, u32 paddr, void *data, char *metadata);


int nandfctrl2_read(nandfctrl2_dma_t *dma, u32 paddr, void *data, nandfctrl2_meta_t *meta);


int nandfctrl2_erase(nandfctrl2_dma_t *dma, u32 paddr);


int nandfctrl2_writeraw(nandfctrl2_dma_t *dma, u32 paddr, void *data, int sz);


int nandfctrl2_readraw(nandfctrl2_dma_t *dma, u32 paddr, void *data, int sz);


int nandfctrl2_isbad(nandfctrl2_dma_t *dma, u32 paddr);


int nandfctrl2_markbad(nandfctrl2_dma_t *dma, u32 paddr);


const nandfctrl2_info_t *nandfctrl2_info(void);


void nandfctrl2_init(void);


#endif
