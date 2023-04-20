/*
 * Phoenix-RTOS
 *
 * IMX6ULL NAND flash driver.
 * Low level API to be used by flashsrv and FS implementations.
 *
 * Copyright 2018 Phoenix Systems
 * Author: Jan Sikorski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _IMX6ULL_NANDDRV_H_
#define _IMX6ULL_NANDDRV_H_

#include <stdint.h>


typedef struct _nanddrv_dma_t nanddrv_dma_t;


/* clang-format off */
enum {
	flash_reset = 0, flash_read_id, flash_read_parameter_page, flash_read_unique_id,
	flash_get_features, flash_set_features, flash_read_status, flash_read_status_enhanced,
	flash_random_data_read, flash_random_data_read_two_plane, flash_random_data_input,
	flash_program_for_internal_data_move_column, flash_read_mode, flash_read_page,
	flash_read_page_cache_sequential, flash_read_page_cache_random, flash_read_page_cache_last,
	flash_program_page, flash_program_page_cache, flash_erase_block,
	flash_read_for_internal_data_move, flash_program_for_internal_data_move,
	flash_block_unlock_low, flash_block_unlock_high, flash_block_lock, flash_block_lock_tight,
	flash_block_lock_read_status, flash_otp_data_lock_by_block, flash_otp_data_program,
	flash_otp_data_read, flash_num_commands
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
} nanddrv_meta_t;


/* information about NAND flash configuration */
typedef struct {
	const char *name;
	u64 size;     /* total NAND size in bytes */
	u32 writesz;  /* write page DATA size in bytes */
	u32 metasz;   /* write page METADATA size in bytes */
	u32 erasesz;  /* erase block size in bytes (multiply of writesize) */
	u32 oobsz;    /* out-of-bound (oob) data size */
	u32 oobavail; /* available out-of-bound (oob) data size */
} nanddrv_info_t;


/* paddr: page address, so NAND address / writesz */

extern nanddrv_dma_t *nanddrv_dma(void);


extern int nanddrv_reset(nanddrv_dma_t *dma);


extern int nanddrv_write(nanddrv_dma_t *dma, u32 paddr, void *data, char *metadata);


extern int nanddrv_read(nanddrv_dma_t *dma, u32 paddr, void *data, nanddrv_meta_t *meta);


extern int nanddrv_erase(nanddrv_dma_t *dma, u32 paddr);


extern int nanddrv_writeraw(nanddrv_dma_t *dma, u32 paddr, void *data, int sz);


extern int nanddrv_readraw(nanddrv_dma_t *dma, u32 paddr, void *data, int sz);


extern int nanddrv_isbad(nanddrv_dma_t *dma, u32 paddr);


extern int nanddrv_markbad(nanddrv_dma_t *dma, u32 paddr);


extern const nanddrv_info_t *nanddrv_info(void);


extern void nanddrv_init(void);

#endif
