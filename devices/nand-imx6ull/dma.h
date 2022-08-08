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

#ifndef _DMA_H_
#define _DMA_H_

#include <lib/lib.h>


/* DMA descriptor flags */
#define DMA_PIO(n)   (((n) & 0xf) << 12)
#define DMA_HOT      (1 << 8)
#define DMA_W4ENDCMD (1 << 7)
#define DMA_DECSEMA  (1 << 6)
#define DMA_W4READY  (1 << 5)
#define DMA_NANDLOCK (1 << 4)
#define DMA_IRQCOMP  (1 << 3)
#define DMA_CHAIN    (1 << 2)
#define DMA_SENSE    (3 << 0)
#define DMA_READ     (2 << 0)
#define DMA_WRITE    (1 << 0)
#define DMA_NOXFER   (0 << 0)


/* DMA descriptor */
typedef struct {
	u32 next;
	u16 flags;
	u16 size;
	u32 addr;
	u32 pio[];
} __attribute__((packed)) dma_desc_t;


/* DMA descriptor chain */
typedef struct {
	dma_desc_t *first;
	dma_desc_t *last;
	u8 buff[] __attribute__((aligned(4)));
} dma_t;


extern int dma_descsz(const dma_desc_t *desc);


extern int dma_check(dma_desc_t *desc, const dma_desc_t *fail);


extern int dma_terminate(dma_desc_t *desc, int err);


extern void dma_sequence(dma_t *dma, dma_desc_t *desc);


extern void dma_finish(dma_t *dma);


extern void dma_reset(dma_t *dma);


extern int dma_run(const dma_t *dma, unsigned int chan);


extern void dma_done(void);


extern void dma_init(void);


#endif
