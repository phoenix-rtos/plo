/*
 * Phoenix-RTOS
 *
 * SD card driver header file
 *
 * Copyright 2022, 2023 Phoenix Systems
 * Author: Artur Miller, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _SDCARD_H_
#define _SDCARD_H_

#include <types.h>

#define SDCARD_MAX_TRANSFER 1024 /* Maximum size of a single transfer in bytes */
#define SDCARD_BLOCKLEN     512  /* Block size in bytes used for sdcard_transferBlocks */

typedef enum {
	sdio_read,
	sdio_write
} sdio_dir_t;

typedef enum SDCARD_INSERTION {
	SDCARD_INSERTION_OUT = -1,
	SDCARD_INSERTION_UNSTABLE = 0,
	SDCARD_INSERTION_IN = 1,
} sdcard_insertion_t;

/* Returns > 0 if there are remaining slots to initialize, 0 if all slots were initialized, < 0 on failure
 * Buffer must be SDCARD_MAX_TRANSFER bytes and not cross 4K page boundary.
 */
extern int sdcard_initHost(unsigned int slot, char *dataBuffer);

/* If fallbackMode == 1, the card will be left in very low speed mode after initialization */
extern int sdcard_initCard(unsigned int slot, int fallbackMode);

extern void sdcard_free(unsigned int slot);

extern int sdcard_transferBlocks(unsigned int slot, sdio_dir_t dir, u32 blockOffset, u32 blocks, time_t deadline);

extern u32 sdcard_getSizeBlocks(unsigned int slot);

/* Returns the minimum number of blocks to be erased at a time */
extern u32 sdcard_getEraseSizeBlocks(unsigned int slot);

/* On SD cards erase is not necessary to write blocks.
 * State after erase is dependent on implementation of the SD card.
 */
extern int sdcard_eraseBlocks(unsigned int slot, u32 blockOffset, u32 nBlocks);

/* Sets all bytes in selected range of blocks to a value of 0xFF.
 * This is meant for emulating erase behavior of NOR flash.
 */
extern int sdcard_writeFF(unsigned int slot, u32 blockOffset, u32 nBlocks);

sdcard_insertion_t sdcard_isInserted(unsigned int slot);

#endif /* _SDCARD_H_ */
