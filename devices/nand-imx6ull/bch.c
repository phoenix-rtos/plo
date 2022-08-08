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

#include "bch.h"


/* BCH registers */
#define BCH_CTRL          0
#define BCH_CTRL_SET      1
#define BCH_CTRL_CLR      2
#define BCH_CTRL_TOG      3
#define BCH_STATUS0       4
#define BCH_STATUS0_SET   5
#define BCH_STATUS0_CLR   6
#define BCH_STATUS0_TOG   7
#define BCH_MODE          8
#define BCH_MODE_SET      9
#define BCH_MODE_CLR      10
#define BCH_MODE_TOG      11
#define BCH_ENCODEPTR     12
#define BCH_ENCODEPTR_SET 13
#define BCH_ENCODEPTR_CLR 14
#define BCH_ENCODEPTR_TOG 15
#define BCH_DATAPTR       16
#define BCH_DATAPTR_SET   17
#define BCH_DATAPTR_CLR   18
#define BCH_DATAPTR_TOG   19
#define BCH_METAPTR       20
#define BCH_METAPTR_SET   21
#define BCH_METAPTR_CLR   22
#define BCH_METAPTR_TOG   23
#define BCH_LAYOUTSEL     28
#define BCH_LAYOUTSEL_SET 29
#define BCH_LAYOUTSEL_CLR 30
#define BCH_LAYOUTSEL_TOG 31
#define BCH_LAYOUT0       32
#define BCH_LAYOUT0_SET   33
#define BCH_LAYOUT0_CLR   34
#define BCH_LAYOUT0_TOG   35
#define BCH_LAYOUT1       36
#define BCH_LAYOUT1_SET   37
#define BCH_LAYOUT1_CLR   38
#define BCH_LAYOUT1_TOG   39

/* BCH NAND layouts */
#define BCH_LAYOUT_OFFS 8


static struct {
	volatile u32 *base;
} bch_common;


void bch_layout(unsigned int cs, const bch_ecc_t *ecc, u8 metasz, u16 rawsz)
{
	u32 reg;

	/* Data blocks number */
	reg = ((u32)ecc->nblocks << 24);
	/* Metadata size */
	reg |= ((u32)metasz << 16);
	/* Metadata block ECC strength */
	reg |= ((((u32)ecc->ecc0 >> 1) & 0x3f) << 11);
	/* Metadata block Galois field */
	reg |= (((ecc->gf0 == 14) ? 1 : 0) << 10);
	/* Metadata block attached data size */
	reg |= ((ecc->blocksz0 >> 2) & 0x3ff);

	*(bch_common.base + cs * BCH_LAYOUT_OFFS + BCH_LAYOUT0) = reg;

	/* Raw page size */
	reg = ((u32)rawsz << 16);
	/* Data blocks ECC strength */
	reg |= ((((u32)ecc->eccN >> 1) & 0x3f) << 11);
	/* Data blocks Galois field */
	reg |= (((ecc->gfN == 14) ? 1 : 0) << 10);
	/* Data blocks size */
	reg |= ((ecc->blockszN >> 2) & 0x3ff);

	*(bch_common.base + cs * BCH_LAYOUT_OFFS + BCH_LAYOUT1) = reg;
}


void bch_layoutMeta(unsigned int cs, const bch_ecc_t *ecc, u8 metasz)
{
	/* No data blocks */
	*(bch_common.base + cs * BCH_LAYOUT_OFFS + BCH_LAYOUT0_CLR) = (0xffu << 24);

	/* Metadata block is the whole page */
	*(bch_common.base + cs * BCH_LAYOUT_OFFS + BCH_LAYOUT1_CLR) = (0xffffu << 16);
	*(bch_common.base + cs * BCH_LAYOUT_OFFS + BCH_LAYOUT1_SET) = (((u32)metasz + bch_eccsz(ecc->ecc0, ecc->gf0)) << 16);
}


void bch_layoutData(unsigned int cs, const bch_ecc_t *ecc, u8 metasz)
{
	/* Metadata block and its parity bits are raw byte area */
	*(bch_common.base + cs * BCH_LAYOUT_OFFS + BCH_LAYOUT0_CLR) = (0x1fff << 11);
	*(bch_common.base + cs * BCH_LAYOUT_OFFS + BCH_LAYOUT0_SET) = (((u32)metasz + bch_eccsz(ecc->ecc0, ecc->gf0)) << 16);
}


void bch_finish(void)
{
	/* Wait for BCH completion */
	while (!(*(bch_common.base + BCH_CTRL) & (1 << 0)))
		;
	/* Clear completion flag */
	*(bch_common.base + BCH_CTRL_CLR) = (1 << 0);
}


void bch_done(void)
{
	/* Disable BCH module */
	*(bch_common.base + BCH_CTRL_SET) = (1u << 31) | (1 << 30);
}


void bch_init(void)
{
	bch_common.base = (void *)0x1808000;

	/* Enable BCH clock */
	imx6ull_setDevClock(clk_rawnand_u_bch_input_apb, 3);

	/* Enable BCH module */
	*(bch_common.base + BCH_CTRL_CLR) = (1u << 31) | (1 << 30);

	/* Reset BCH module */
	*(bch_common.base + BCH_CTRL_SET) = (1u << 31);
	while (!(*(bch_common.base + BCH_CTRL) & (1 << 30)))
		;

	/* Re-enable BCH module */
	*(bch_common.base + BCH_CTRL_CLR) = (1u << 31) | (1 << 30);

	/* Disable and clear BCH interrupts */
	*(bch_common.base + BCH_CTRL_CLR) = (1 << 10) | (1 << 8) | (1 << 3) | (1 << 2) | (1 << 0);

	/* Assign separate BCH layout for each chip select */
	*(bch_common.base + BCH_LAYOUTSEL) = (3 << 6) | (2 << 4) | (1 << 2) | (0 << 0);
}
