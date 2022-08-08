/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ULL GPMI
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "gpmi.h"


/* GPMI registers */
#define GPMI_CTRL0       0
#define GPMI_CTRL0_SET   1
#define GPMI_CTRL0_CLR   2
#define GPMI_CTRL0_TOG   3
#define GPMI_COMPARE     4
#define GPMI_ECCCTRL     8
#define GPMI_ECCCTRL_SET 9
#define GPMI_ECCCTRL_CLR 10
#define GPMI_ECCCTRL_TOG 11
#define GPMI_ECCCOUNT    12
#define GPMI_PAYLOAD     16
#define GPMI_AUXILIARY   20
#define GPMI_CTRL1       24
#define GPMI_CTRL1_SET   25
#define GPMI_CTRL1_CLR   26
#define GPMI_CTRL1_TOG   27
#define GPMI_TIMING0     28
#define GPMI_TIMING1     32
#define GPMI_TIMING2     36

/* GPMI CTRL0 flags */
#define GPMI_LOCKCS  (1 << 27)
#define GPMI_UDMA    (1 << 26)
#define GPMI_WRITE   (0 << 24)
#define GPMI_READ    (1 << 24)
#define GPMI_READCMP (2 << 24)
#define GPMI_W4READY (3 << 24)
#define GPMI_8BIT    (1 << 23)
#define GPMI_CS(cs)  ((cs) << 20)
#define GPMI_DATA    (0 << 17)
#define GPMI_CLE     (1 << 17)
#define GPMI_ALE     (2 << 17)
#define GPMI_ADDRINC (1 << 16)

/* GPMI ECCCTRL register layout */
#define GPMI_ECC_ENCODE (1 << 13)
#define GPMI_ECC        (1 << 12)
#define GPMI_ECC_AUX    0x100
#define GPMI_ECC_PAGE   0x1ff


static struct {
	volatile u32 *base;
} gpmi_common;


int gpmi_w4ready(dma_desc_t *desc, unsigned int cs)
{
	desc->flags = DMA_PIO(1) | DMA_HOT | DMA_W4ENDCMD | DMA_W4READY | DMA_NOXFER;
	desc->size = 0;
	desc->addr = 0;
	desc->pio[0] = GPMI_W4READY | GPMI_8BIT | GPMI_CS(cs);

	return dma_descsz(desc);
}


int gpmi_cmdaddr(dma_desc_t *desc, unsigned int cs, const void *buff, u16 addrsz)
{
	desc->flags = DMA_PIO(3) | DMA_HOT | DMA_W4ENDCMD | DMA_NANDLOCK | DMA_READ;
	desc->size = addrsz + 1;
	desc->addr = (u32)buff;
	desc->pio[0] = GPMI_LOCKCS | GPMI_WRITE | GPMI_8BIT | GPMI_CS(cs) | GPMI_CLE | ((addrsz > 0) ? GPMI_ADDRINC : 0) | desc->size;
	desc->pio[1] = 0;
	desc->pio[2] = 0;

	return dma_descsz(desc);
}


int gpmi_readcmp(dma_desc_t *desc, unsigned int cs, u16 mask, u16 val)
{
	desc->flags = DMA_PIO(3) | DMA_HOT | DMA_W4ENDCMD | DMA_NANDLOCK | DMA_NOXFER;
	desc->size = 0;
	desc->addr = 0;
	desc->pio[0] = GPMI_READCMP | GPMI_8BIT | GPMI_CS(cs) | GPMI_DATA | 1;
	desc->pio[1] = ((u32)mask << 16) | val;
	desc->pio[2] = 0;

	return dma_descsz(desc);
}


int gpmi_disableBCH(dma_desc_t *desc, unsigned int cs)
{
	desc->flags = DMA_PIO(3) | DMA_HOT | DMA_W4ENDCMD | DMA_NANDLOCK | DMA_NOXFER;
	desc->size = 0;
	desc->addr = 0;
	desc->pio[0] = GPMI_LOCKCS | GPMI_W4READY | GPMI_8BIT | GPMI_CS(cs) | GPMI_DATA;
	desc->pio[1] = 0;
	desc->pio[2] = 0;

	return dma_descsz(desc);
}


int gpmi_read(dma_desc_t *desc, unsigned int cs, void *buff, u16 size)
{
	desc->flags = DMA_PIO(3) | DMA_HOT | DMA_W4ENDCMD | DMA_NANDLOCK | DMA_WRITE;
	desc->size = size;
	desc->addr = (u32)buff;
	desc->pio[0] = GPMI_READ | GPMI_8BIT | GPMI_CS(cs) | GPMI_DATA | size;
	desc->pio[1] = 0;
	desc->pio[2] = 0;

	return dma_descsz(desc);
}


int gpmi_ecread(dma_desc_t *desc, unsigned int cs, void *buff, void *aux, u16 size)
{
	desc->flags = DMA_PIO(6) | DMA_HOT | DMA_W4ENDCMD | DMA_NANDLOCK | DMA_NOXFER;
	desc->size = 0;
	desc->addr = 0;
	desc->pio[0] = GPMI_READ | GPMI_8BIT | GPMI_CS(cs) | GPMI_DATA | size;
	desc->pio[1] = 0;
	desc->pio[2] = GPMI_ECC | ((buff != NULL) ? GPMI_ECC_PAGE : GPMI_ECC_AUX);
	desc->pio[3] = size;
	desc->pio[4] = (u32)buff;
	desc->pio[5] = (u32)aux;

	return dma_descsz(desc);
}


int gpmi_write(dma_desc_t *desc, unsigned int cs, const void *buff, u16 size)
{
	desc->flags = DMA_PIO(3) | DMA_HOT | DMA_W4ENDCMD | DMA_NANDLOCK | DMA_READ;
	desc->size = size;
	desc->addr = (u32)buff;
	desc->pio[0] = GPMI_LOCKCS | GPMI_WRITE | GPMI_8BIT | GPMI_CS(cs) | GPMI_DATA | size;
	desc->pio[1] = 0;
	desc->pio[2] = 0;

	return dma_descsz(desc);
}


int gpmi_ecwrite(dma_desc_t *desc, unsigned int cs, const void *buff, const void *aux, u16 size)
{
	desc->flags = DMA_PIO(6) | DMA_HOT | DMA_W4ENDCMD | DMA_NANDLOCK | DMA_NOXFER;
	desc->size = 0;
	desc->addr = 0;
	desc->pio[0] = GPMI_LOCKCS | GPMI_WRITE | GPMI_8BIT | GPMI_CS(cs) | GPMI_DATA;
	desc->pio[1] = 0;
	desc->pio[2] = GPMI_ECC_ENCODE | GPMI_ECC | GPMI_ECC_PAGE;
	desc->pio[3] = size;
	desc->pio[4] = (u32)buff;
	desc->pio[5] = (u32)aux;

	return dma_descsz(desc);
}


void gpmi_done(void)
{
	/* Disable GPMI module */
	*(gpmi_common.base + GPMI_CTRL0_SET) = (1u << 31) | (1 << 30);
}


void gpmi_init(void)
{
	gpmi_common.base = (void *)0x1806000;

	/* Enable GPMI clocks */
	imx6ull_setDevClock(clk_rawnand_u_gpmi_input_apb, 3);
	imx6ull_setDevClock(clk_rawnand_u_gpmi_bch_input_gpmi_io, 3);
	imx6ull_setDevClock(clk_rawnand_u_gpmi_bch_input_bch, 3);

	/* Enable GPMI module */
	*(gpmi_common.base + GPMI_CTRL0_CLR) = (1u << 31) | (1 << 30);

	/* Reset GPMI module */
	*(gpmi_common.base + GPMI_CTRL0_SET) = (1u << 31);
	while (!(*(gpmi_common.base + GPMI_CTRL0) & (1 << 30)))
		;

	/* Re-enable GPMI module */
	*(gpmi_common.base + GPMI_CTRL0_CLR) = (1u << 31) | (1 << 30);

	/* Disable and clear GPMI interrupts */
	*(gpmi_common.base + GPMI_CTRL1_CLR) = (1 << 20) | (1 << 10) | (1 << 9);

	/* DECOUPLE_CS, WRN no delay, GANGED_RDYBUSY, BCH, RDN_DELAY, BURST_EN, WP, #R/B busy-low */
	*(gpmi_common.base + GPMI_CTRL1_SET) = (1 << 24) | (3 << 22) | (1 << 19) | (1 << 18) | (14 << 12) | (1 << 8) | (1 << 3) | (1 << 2);

	/* Enable DLL */
	*(gpmi_common.base + GPMI_CTRL1_SET) = (1 << 17);

	/* Configure timings (GPMI clock = 198 MHz, ~5ns period), address setup = 3 (~15ns), data hold = 2 (~10ns), data setup = 3 (~15ns) */
	*(gpmi_common.base + GPMI_TIMING0) = (3 << 16) | (2 << 8) | (3 << 0);
}
