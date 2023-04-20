/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ULL NAND data interface
 *
 * Copyright 2023 Phoenix Systems
 * Author: Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _IMX6ULL_NAND_DATA_H_
#define _IMX6ULL_NAND_DATA_H_

#define NAND_MAX_CNT 1


typedef struct {
	nanddrv_dma_t *dma;
	const nanddrv_info_t *cfg;
} nand_t;


extern nand_t *nand_get(unsigned minor);


extern int data_doSync(nand_t *nand);

#endif
