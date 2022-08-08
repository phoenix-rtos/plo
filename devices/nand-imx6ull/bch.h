/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ULL BCH ECC accelerator
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _BCH_H_
#define _BCH_H_

#include <lib/lib.h>


/* BCH ECC configuration */
typedef struct {
	u16 blocksz0; /* Metadata block attached data size in bytes (multiple of 4, set to 0 for no attached data) */
	u16 blockszN; /* Data block size in bytes (multiple of 4, typically 512) */
	u8 nblocks;   /* Number of page data blocks */
	u8 ecc0;      /* Metadata block ECC strength */
	u8 eccN;      /* Data blocks ECC strength */
	u8 gf0;       /* Metadata block Galois field (13 or 14) */
	u8 gfN;       /* Data blocks Galois field (13 or 14) */
} bch_ecc_t;


static inline unsigned int bch_eccsz(unsigned int ecc, unsigned int gf)
{
	return (ecc * gf + 7) / 8;
}


extern void bch_layout(unsigned int cs, const bch_ecc_t *ecc, u8 metasz, u16 rawsz);


extern void bch_layoutMeta(unsigned int cs, const bch_ecc_t *ecc, u8 metasz);


extern void bch_layoutData(unsigned int cs, const bch_ecc_t *ecc, u8 metasz);


extern void bch_finish(void);


extern void bch_done(void);


extern void bch_init(void);


#endif
