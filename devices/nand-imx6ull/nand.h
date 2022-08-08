/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ULL NAND
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _NAND_H_
#define _NAND_H_

#include <lib/lib.h>


/* Max number of NAND devices (GPMI and BCH modules support up to 4 chips) */
#define NAND_MAX_CNT 1


/* NAND device configuration */
typedef struct {
	u64 size;    /* Device size in bytes */
	u32 erasesz; /* Eraseblock size in bytes */
	u32 pagesz;  /* Page size in bytes */
	u32 oobsz;   /* Page OOB area size in bytes */
	u32 metasz;  /* Page metadata size in bytes */
} nand_cfg_t;


/* NAND device */
typedef struct {
	const nand_cfg_t *cfg; /* Device configuration */
} nand_t;


extern int nand_read(const nand_t *nand, unsigned int page, void *data, void *aux, int raw);


extern int nand_write(const nand_t *nand, unsigned int page, const void *data, const void *aux, int raw);


extern int nand_erase(const nand_t *nand, unsigned int block);


extern int nand_isBad(const nand_t *nand, unsigned int block);


extern int nand_markBad(const nand_t *nand, unsigned int block);


extern const nand_t *nand_get(unsigned int cs);


extern int nand_done(unsigned int cs);


extern int nand_init(unsigned int cs);


#endif
