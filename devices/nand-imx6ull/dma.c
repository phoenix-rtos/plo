/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ULL APBH DMA
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "dma.h"


/* DMA registers */
#define DMA_CTRL0     0
#define DMA_CTRL0_SET 1
#define DMA_CTRL0_CLR 2
#define DMA_CTRL0_TOG 3
#define DMA_CTRL1     4
#define DMA_CTRL1_SET 5
#define DMA_CTRL1_CLR 6
#define DMA_CTRL1_TOG 7
#define DMA_CTRL2     8
#define DMA_CTRL2_SET 9
#define DMA_CTRL2_CLR 10
#define DMA_CTRL2_TOG 11
#define DMA_CHAN      12
#define DMA_CHAN_SET  13
#define DMA_CHAN_CLR  14
#define DMA_CHAN_TOG  15
#define DMA_DEVSEL    16
#define DMA_CURCMDAR  64
#define DMA_NXTCMDAR  68
#define DMA_CMD       72
#define DMA_BAR       76
#define DMA_SEMA      80
#define DMA_DEBUG     84
#define DMA_DEBUG2    88
#define DMA_VERSION   512

/* DMA channels */
#define DMA_CHAN_OFFS 28


static struct {
	volatile u32 *base;
} dma_common;


int dma_descsz(const dma_desc_t *desc)
{
	return sizeof(*desc) + ((desc->flags >> 12) & 0xf) * sizeof(desc->pio[0]);
}


int dma_check(dma_desc_t *desc, const dma_desc_t *fail)
{
	desc->flags = DMA_HOT | DMA_SENSE;
	desc->size = 0;
	desc->addr = (u32)fail;

	return dma_descsz(desc);
}


int dma_terminate(dma_desc_t *desc, int err)
{
	desc->flags = DMA_DECSEMA | DMA_IRQCOMP | DMA_NOXFER;
	desc->size = 0;
	desc->addr = (u32)err;

	return dma_descsz(desc);
}


void dma_sequence(dma_t *dma, dma_desc_t *desc)
{
	if (dma->last != dma->first) {
		dma->last->flags |= DMA_CHAIN;
		dma->last->next = (u32)desc;
	}
	else {
		dma->first = desc;
	}
	dma->last = desc;
}


void dma_finish(dma_t *dma)
{
	u8 *next = (u8 *)dma->last;
	dma_desc_t *desc;

	if (dma->last != dma->first) {
		next += dma_descsz(dma->last);
	}

	desc = (dma_desc_t *)next;
	next += dma_terminate(desc, EOK);
	dma_sequence(dma, desc);
}


void dma_reset(dma_t *dma)
{
	dma->first = (dma_desc_t *)dma->buff;
	dma->last = (dma_desc_t *)dma->buff;
}


int dma_run(const dma_t *dma, unsigned int chan)
{
	int comp = 0, err = 0;
	u32 status;

	/* Run DMA channel */
	*(dma_common.base + chan * DMA_CHAN_OFFS + DMA_NXTCMDAR) = (u32)dma->first;
	*(dma_common.base + chan * DMA_CHAN_OFFS + DMA_SEMA) = 1;

	/* Wait for DMA completion */
	do {
		/* Completion status */
		status = *(dma_common.base + DMA_CTRL1);
		if (status & (1u << chan)) {
			comp++;
		}

		/* Error status, 0: no error, 1: DMA termination, 2: AHB bus error */
		status = *(dma_common.base + DMA_CTRL2);
		if (status & (1u << chan)) {
			if (status & (1u << (chan + 16))) {
				err++;
			}
			err++;
		}
	} while ((comp == 0) && (err == 0));

	/* DMA termination with completion flag is not treated as an error */
	if ((err == 1) && (comp == 1)) {
		err = 0;
	}

	return -err;
}


void dma_done(void)
{
	/* Disable DMA module */
	*(dma_common.base + DMA_CTRL0_SET) = (1u << 31) | (1 << 30);
}


void dma_init(void)
{
	dma_common.base = (void *)0x1804000;

	/* Enable DMA clock */
	imx6ull_setDevClock(clk_apbhdma, 3);

	/* Enable DMA module */
	*(dma_common.base + DMA_CTRL0_CLR) = (1u << 31) | (1 << 30);

	/* Reset DMA module */
	*(dma_common.base + DMA_CTRL0_SET) = (1u << 31);
	while (!(*(dma_common.base + DMA_CTRL0) & (1 << 30)))
		;

	/* Re-enable DMA module */
	*(dma_common.base + DMA_CTRL0_CLR) = (1u << 31) | (1 << 30);

	/* Disable and clear DMA interrupts */
	*(dma_common.base + DMA_CTRL1) = 0;
	*(dma_common.base + DMA_CTRL2) = 0;

	/* Enable DMA burst */
	*(dma_common.base + DMA_CTRL0_SET) = (1 << 29) | (1 << 28);

	/* Enable clock gating for all DMA channels */
	*(dma_common.base + DMA_CTRL0_CLR) = 0xffff;
}
