/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GRLIB NANDFCTRL2 flash driver
 *
 * Low-level driver using DMA Linked List Mode (LLM).
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <hal/hal.h>
#include <lib/lib.h>

#include "nandfctrl2.h"
#include "onfi-4.h"


#define NAND_BBM_SIZE         2 /* Bad Block Marker size in bytes */
#define ECC_BITFLIP_THRESHOLD 5 /* Min number of bitflips to treat page as degrading */

#define DMA_MAX_DESCS 32 /* Maximum descriptors in a single chain */


/* ======================== Register Definitions ======================== */


/* Core control 0 register */
#define CTRL0_EE         (1U << 13)               /* EDAC enable */
#define CTRL0_RE         (1U << 12)               /* Data randomization enable */
#define CTRL0_LLM        (1U << 10)               /* Linked list mode enable */
#define CTRL0_BBM        (1U << 9)                /* Preserve Bad Block Marking */
#define CTRL0_EDO        (1U << 7)                /* EDO mode enable */
#define CTRL0_IFSEL_SHFT 5                        /* Data Interface select shift */
#define CTRL0_IFSEL_MSK  (3U << CTRL0_IFSEL_SHFT) /* Data Interface select mask */
#define CTRL0_MSEL_SHFT  3                        /* Memory select shift */
#define CTRL0_MSEL_MSK   (3U << CTRL0_MSEL_SHFT)  /* Memory select mask */

/* Core control 2 register */
#define CTRL2_STOP_LL (1U << 3) /* Stop Linked List */
#define CTRL2_ABORT   (1U << 2) /* Abort ongoing execution */
#define CTRL2_DT      (1U << 1) /* Descriptor trigger */
#define CTRL2_RST     (1U << 0) /* Software reset (APB registers) */

/* Core status 0 register */
#define STS0_DA  (1U << 3) /* Descriptor active */
#define STS0_RDY (1U << 0) /* Ready state (set when ready) */

/* Core status 1 register */
#define STS1_STOPLL (1U << 9) /* Stop linked list interrupt */
#define STS1_ABORT  (1U << 8) /* Abort interrupt (set when abort completes) */
#define STS1_TMOUT  (1U << 6) /* Ready/Busy timeout */
#define STS1_CMD    (1U << 5) /* Invalid command */
#define STS1_DL     (1U << 4) /* DMA read (uplink write) error */
#define STS1_UL     (1U << 3) /* DMA write (downlink read) error */
#define STS1_DS     (1U << 1) /* Descriptor finished */

#define STS1_FATAL_MSK (STS1_TMOUT | STS1_CMD | STS1_ABORT | STS1_DL | STS1_UL)

/* Descriptor command register */
#define DCMD_CMD2_SHFT   24
#define DCMD_CMD1_SHFT   16
#define DCMD_PRECH       (1U << 12)               /* Pre-cache */
#define DCMD_DD          (1U << 11)               /* Data DMA Disable (1 = APB, 0 = DMA) */
#define DCMD_ED          (1U << 10)               /* EDAC Disable for this descriptor */
#define DCMD_RD          (1U << 9)                /* Randomization Disable for this descriptor */
#define DCMD_SUBCMD_SHFT 7                        /* Sub command shift */
#define DCMD_SUBCMD_MSK  (3U << DCMD_SUBCMD_SHFT) /* Sub command mask */
#define DCMD_SRB         (1U << 6)                /* Skip Ready/Busy wait */
#define DCMD_SA          (1U << 5)                /* Skip Address phase */
#define DCMD_SD          (1U << 4)                /* Skip Data phase */
#define DCMD_SC2         (1U << 3)                /* Skip second command phase */
#define DCMD_WARDY       (1U << 2)                /* Wait ARDY */
#define DCMD_IRQ_DS      (1U << 1)                /* Descriptor interrupt enable */
#define DCMD_EN          (1U << 0)                /* Descriptor Enable */

/* Descriptor row & control register */
#define DROW_TAGEN (1U << 24) /* Tag enable */

/* Descriptor column & size register */
#define DCOLSIZE_SIZE_SHFT 16 /* Transfer size in bytes */

/* Descriptor status register */
#define DSTS_ECFAIL_SHFT 16                            /* ECC chunk fail bitmap shift */
#define DSTS_ECFAIL_MSK  (0xffffU << DSTS_ECFAIL_SHFT) /* ECC chunk fail bitmap mask */
#define DSTS_UE          (1U << 1)                     /* Uncorrectable ECC error */

/* Descriptor ECC status register */
#define DECCSTS_CEC_SHFT 16                          /* Corrected errors in worst chunk shift */
#define DECCSTS_CEC_MSK  (0xffU << DECCSTS_CEC_SHFT) /* Corrected errors in worst chunk mask */

/* Capability 1 register */
#define CAP1_E1CAP_SHFT   26
#define CAP1_E1CHUNK_SHFT 21
#define CAP1_E1GF_SHFT    16
#define CAP1_E0CAP_SHFT   10
#define CAP1_E0CHUNK_SHFT 5
#define CAP1_E0GF_SHFT    0

#define CAP1_E1CAP   (0x3fU << CAP1_E1CAP_SHFT)
#define CAP1_E1CHUNK (0x1fU << CAP1_E1CHUNK_SHFT)
#define CAP1_E1GF    (0x1fU << CAP1_E1GF_SHFT)
#define CAP1_E0CAP   (0x3fU << CAP1_E0CAP_SHFT)
#define CAP1_E0CHUNK (0x1fU << CAP1_E0CHUNK_SHFT)
#define CAP1_E0GF    (0x1fU << CAP1_E0GF_SHFT)


/* Capability 2/3/4 register */
#define CAP_MSEL_SHFT   31
#define CAP_MSPARE_SHFT 16

#define CAP_MSEL   (1U << CAP_MSEL_SHFT)
#define CAP_MSPARE (0x1fffU << CAP_MSPARE_SHFT)
#define CAP_MDATA  0xffffU


/* ======================== Type Definitions ======================== */


typedef struct {
	u32 ctrl0;    /* 0x000: Core control 0 register */
	u32 ctrl1;    /* 0x004: Core control 1 register */
	u32 ctrl2;    /* 0x008: Core control 2 register */
	u32 ctrl3;    /* 0x00c: Core control 3 register */
	u32 ctrl4;    /* 0x010: Core control 4 register */
	u32 res0[3];  /* 0x014 - 0x01f: Reserved */
	u32 sts0;     /* 0x020: Core status 0 register */
	u32 sts1;     /* 0x024: Core status 1 register */
	u32 sts2;     /* 0x028: Core status 2 register */
	u32 sts3;     /* 0x02c: Core status 3 register */
	u32 res1[2];  /* 0x030 - 0x037: Reserved */
	u32 cap0;     /* 0x038: Capability 0 register */
	u32 cap1;     /* 0x03c: Capability 1 register */
	u32 cap2;     /* 0x040: Capability 2 register */
	u32 cap3;     /* 0x044: Capability 3 register */
	u32 cap4;     /* 0x048: Capability 4 register */
	u32 cap5;     /* 0x04c: Capability 5 register */
	u32 tme0;     /* 0x050: Programmable timing 0 register */
	u32 tme1;     /* 0x054: Programmable timing 1 register */
	u32 tme2;     /* 0x058: Programmable timing 2 register */
	u32 tme3;     /* 0x05c: Programmable timing 3 register */
	u32 tme4;     /* 0x060: Programmable timing 4 register */
	u32 tme5;     /* 0x064: Programmable timing 5 register */
	u32 tme6;     /* 0x068: Programmable timing 6 register */
	u32 tme7;     /* 0x06c: Programmable timing 7 register */
	u32 tme8;     /* 0x070: Programmable timing 8 register */
	u32 tme9;     /* 0x074: Programmable timing 9 register */
	u32 tme10;    /* 0x078: Programmable timing 10 register */
	u32 tme11;    /* 0x07c: Programmable timing 11 register */
	u32 res2[16]; /* 0x080 - 0x0bf: Reserved */
	u32 txskew0;  /* 0x0c0: Skew Control Transfer register */
	u32 rxskew0;  /* 0x0c4: Skew Control Receive register */
	u32 res3[2];  /* 0x0c8 - 0x0cf: Reserved */
	u32 tout0;    /* 0x0d0: Programmable timeout 0 register */
	u32 res4[31]; /* 0x0d4 - 0x14f: Reserved */
	u32 llpl;     /* 0x150: Linked list pointer low register */
	u32 res5;     /* 0x154: Reserved */
	u32 res6[2];  /* 0x158 - 0x15f: Reserved */
	u32 dcmd;     /* 0x160: Descriptor command register */
	u32 dtarsel0; /* 0x164: Descriptor target select 0 register */
	u32 dtarsel1; /* 0x168: Descriptor target select 1 register */
	u32 dchsel;   /* 0x16c: Descriptor channel select register */
	u32 drbsel;   /* 0x170: Descriptor ready/busy select register */
	u32 drow;     /* 0x174: Descriptor row & control register */
	u32 dcolsize; /* 0x178: Descriptor column & size register */
	u32 dsts;     /* 0x17c: Descriptor status register */
	u32 deccsts0; /* 0x180: Descriptor ECC status register */
	u32 res7[3];  /* 0x184 - 0x18f: Reserved */
	u32 ddpl;     /* 0x190: Descriptor data pointer low register */
	u32 res8[19]; /* 0x194 - 0x1df: Reserved */
	u32 din0;     /* 0x1e0: Data-in 0 register */
	u32 din1;     /* 0x1e4: Data-in 1 register */
	u32 din2;     /* 0x1e8: Data-in 2 register */
	u32 din3;     /* 0x1ec: Data-in 3 register */
	u32 dout0;    /* 0x1f0: Data-out 0 register */
	u32 dout1;    /* 0x1f4: Data-out 1 register */
	u32 dout2;    /* 0x1f8: Data-out 2 register */
	u32 dout3;    /* 0x1fc: Data-out 3 register */
} nandfctrl2_regs_t;


/*
 * LLM descriptor - must be 64-byte aligned.
 * dsts and deccsts are written back by hardware after execution.
 */
typedef struct {
	u32 dcmd;     /* 0x00: Command, flags */
	u32 dtarsel0; /* 0x04: Target select low 32 bits */
	u32 dtarsel1; /* 0x08: Target select high 32 bits */
	u32 dchsel;   /* 0x0c: Channel select mask */
	u32 drbsel;   /* 0x10: Ready/Busy select mask */
	u32 drow;     /* 0x14: Row address + control */
	u32 dcolsize; /* 0x18: Size (31:16) | Column address (15:0) */
	u32 dsts;     /* 0x1c: Descriptor status (written by HW) */
	u32 deccsts;  /* 0x20: ECC status (written by HW) */
	u32 res1[3];  /* 0x24-0x2c */
	u32 ddpl;     /* 0x30: DMA data pointer (physical address) */
	u32 res2;     /* 0x34 */
	u32 dllpl;    /* 0x38: Next descriptor physical address (0 = end) */
	u32 res3;     /* 0x3c */
} nandfctrl2_desc_t;


typedef struct {
	u64 ceMask;
	u32 rbMask;
	u32 channelMask;
} nand_layoutMap_t;


typedef struct {
	u32 rowAddr;         /* 24-bit Page/Block address */
	u16 colAddr;         /* 16-bit Byte offset within the page */
	size_t transferSize; /* Number of bytes to read/write */
	addr_t data;         /* RAM pointer for DMA (0 if no data phase) */
} dma_xfer_t;


typedef struct {
	u8 manufacturerId;
	u8 deviceId;
	u8 bytes[3];
} __attribute__((packed)) flash_id_t;


/* Common buffer, exported for use in data.c/meta.c/raw.c */
u8 nandfctrl2_rawBuffer[NAND_MAX_PAGE_SIZE] __attribute__((aligned(64)));


static const nand_layoutMap_t nand_flashMap[NAND_DIE_CNT] = NAND_DIE_MAP;


static struct {
	volatile nandfctrl2_regs_t *regs;

	u8 initialized;

	/* DMA descriptor chain */
	u32 ndesc;
	nandfctrl2_desc_t descs[DMA_MAX_DESCS + 1] __attribute__((aligned(64))); /* +1 for terminator */

	/* Scratch buffer for status reads and small transfers */
	u8 scratchBuf[256] __attribute__((aligned(64)));

	/* General purpose DMA buffer */
	u8 dmaDataBuf[16 * 1024] __attribute__((aligned(64)));
} nand_common;


/* ======================== DMA Chain Helpers ======================== */


static void dmaChainReset(void)
{
	nand_common.ndesc = 0;
}


static void dma_appendDesc(const nand_die_t *nand, u32 dcmd, const dma_xfer_t *xfer)
{
	u32 idx = nand_common.ndesc;

	nandfctrl2_desc_t *desc = &nand_common.descs[idx];

	hal_memset(desc, 0, sizeof(*desc));

	desc->dcmd = dcmd;

	/* Target routing */
	desc->dtarsel0 = (u32)(nand_flashMap[nand->minor].ceMask & 0xffffffffU);
	desc->dtarsel1 = (u32)(nand_flashMap[nand->minor].ceMask >> 32);
	desc->dchsel = nand_flashMap[nand->minor].channelMask;
	desc->drbsel = nand_flashMap[nand->minor].rbMask;

	if (xfer != NULL) {
		desc->drow = xfer->rowAddr;
		desc->dcolsize = (xfer->transferSize << DCOLSIZE_SIZE_SHFT) | xfer->colAddr;
		if (xfer->data != 0U) {
			desc->ddpl = (u32)xfer->data;
		}
	}

	/* Link previous descriptor to this one */
	if (idx > 0U) {
		nand_common.descs[idx - 1U].dllpl = (u32)(addr_t)&nand_common.descs[idx];
	}

	++nand_common.ndesc;
}


/* ======================== Descriptor Builder Functions ======================== */


static void dmaAddReset(const nand_die_t *nand)
{
	/* RESET (0xFF), no CMD2, no address, no data */
	u32 dcmd = (0xffU << DCMD_CMD1_SHFT) | DCMD_SC2 | DCMD_SA | DCMD_SD | DCMD_ED | DCMD_EN;
	dma_appendDesc(nand, dcmd, NULL);
}


static void dmaAddReadId(const nand_die_t *nand, const dma_xfer_t *xfer)
{
	/* READ ID (0x90), no CMD2, no EDAC */
	u32 dcmd = (0x90U << DCMD_CMD1_SHFT) | DCMD_SC2 | DCMD_ED | DCMD_EN;
	dma_appendDesc(nand, dcmd, xfer);
}


static void dmaAddReadParamPage(const nand_die_t *nand, const dma_xfer_t *xfer)
{
	/* READ PARAMETER PAGE (0xEC), no CMD2, no EDAC */
	u32 dcmd = (0xecU << DCMD_CMD1_SHFT) | DCMD_SC2 | DCMD_ED | DCMD_EN;
	dma_appendDesc(nand, dcmd, xfer);
}


static void dmaAddRead(const nand_die_t *nand, const dma_xfer_t *xfer, int withEcc)
{
	/* READ (0x00 / 0x30) */
	u32 dcmd = (0x30U << DCMD_CMD2_SHFT) | (0x00U << DCMD_CMD1_SHFT) | DCMD_EN;
	if (!withEcc) {
		dcmd |= DCMD_ED;
	}
	dma_appendDesc(nand, dcmd, xfer);
}


static void dmaAddChangeReadColumn(const nand_die_t *nand, const dma_xfer_t *xfer)
{
	/* CHANGE READ COLUMN (0x05 / 0xE0), no EDAC, skip R/B */
	u32 dcmd = (0xe0U << DCMD_CMD2_SHFT) | (0x05U << DCMD_CMD1_SHFT) | DCMD_ED | DCMD_SRB | DCMD_EN;
	dma_appendDesc(nand, dcmd, xfer);
}


static void dmaAddPageProgram(const nand_die_t *nand, const dma_xfer_t *xfer, int withEcc, int commit)
{
	/* PAGE PROGRAM (0x80), CMD2=0x10 only if commit */
	u32 dcmd = (0x80U << DCMD_CMD1_SHFT) | DCMD_EN;
	if (!commit) {
		dcmd |= DCMD_SC2;
	}
	else {
		dcmd |= (0x10U << DCMD_CMD2_SHFT);
	}
	if (!withEcc) {
		dcmd |= DCMD_ED;
	}
	dma_appendDesc(nand, dcmd, xfer);
}


static void dmaAddChangeWriteColumn(const nand_die_t *nand, const dma_xfer_t *xfer, int withEcc, int commit)
{
	/* CHANGE WRITE COLUMN (0x85), SUBCMD=2 */
	u32 dcmd = (0x85U << DCMD_CMD1_SHFT) | (2U << DCMD_SUBCMD_SHFT) | DCMD_EN;
	if (!commit) {
		dcmd |= DCMD_SC2;
	}
	else {
		dcmd |= (0x10U << DCMD_CMD2_SHFT);
	}
	if (!withEcc) {
		dcmd |= DCMD_ED;
	}
	dma_appendDesc(nand, dcmd, xfer);
}


static void dmaAddBlockErase(const nand_die_t *nand, u32 row)
{
	/* BLOCK ERASE (0x60 / 0xD0), no EDAC */
	u32 dcmd = (0xd0U << DCMD_CMD2_SHFT) | (0x60U << DCMD_CMD1_SHFT) | DCMD_ED | DCMD_SD | DCMD_EN;
	dma_xfer_t xfer = { .rowAddr = row };
	dma_appendDesc(nand, dcmd, &xfer);
}


static void dmaAddStatusRead(const nand_die_t *nand)
{
	/* READ STATUS (0x70), no CMD2, skip address, no EDAC */
	u32 dcmd = (0x70U << DCMD_CMD1_SHFT) | DCMD_SC2 | DCMD_SA | DCMD_ED | DCMD_EN;
	dma_xfer_t xfer = { .transferSize = 1U, .data = (addr_t)nand_common.scratchBuf };
	dma_appendDesc(nand, dcmd, &xfer);
}


/* ======================== DMA Execution ======================== */


static void abortOp(void)
{
	nand_common.regs->ctrl2 = CTRL2_ABORT;

	while (((nand_common.regs->sts1 & STS1_ABORT) == 0) || ((nand_common.regs->sts0 & STS0_RDY) == 0)) { }

	/* Clear error status */
	nand_common.regs->sts1 = nand_common.regs->sts1;
}


/*
 * Trigger LLM execution of the descriptor chain.
 * Returns 0 on success, negative errno on fatal controller error.
 * ECC errors are NOT fatal and must be checked separately via descriptor dsts.
 */
static int dmaRun(void)
{
	u32 lastIdx = nand_common.ndesc - 1U;
	nandfctrl2_desc_t *term;
	u32 sts1;

	/* Append a terminating descriptor (EN bit = 0) after the last active one */
	term = &nand_common.descs[lastIdx + 1U];
	hal_memset(term, 0, sizeof(*term));
	nand_common.descs[lastIdx].dllpl = (u32)(addr_t)term;

	/* Clear any pending status */
	nand_common.regs->sts1 = nand_common.regs->sts1;

	/* Load first descriptor address and trigger */
	nand_common.regs->llpl = (u32)(addr_t)&nand_common.descs[0];
	nand_common.regs->ctrl2 = CTRL2_DT;

	/* Poll for completion - controller has HW timeout */
	while ((nand_common.regs->sts0 & STS0_DA) != 0U) { }

	sts1 = nand_common.regs->sts1;
	nand_common.regs->sts1 = sts1;

	if ((sts1 & STS1_FATAL_MSK) != 0U) {
		abortOp();

		if ((sts1 & STS1_TMOUT) != 0U) {
			return -ETIME;
		}

		return -EIO;
	}

	return 0;
}


/*
 * Execute a descriptor chain that ends with a read_status descriptor.
 * Returns 0 on success, -EIO if the flash reports FAIL, or other negative errno.
 */
static int dmaRunAndCheckStatus(void)
{
	int err = dmaRun();
	if (err < 0) {
		return err;
	}

	/* NAND status: bit 0 set = FAIL */
	if ((nand_common.scratchBuf[0] & 0x01U) != 0U) {
		return -EIO;
	}

	return 0;
}


/* ======================== Erased Page Verification ======================== */


/* Count the number of zero-bits (bitflips from erased 0xFF state) in a byte buffer. */
static u32 countBitflips(const u8 *data, size_t len)
{
	u32 flips = 0;
	size_t i;

	for (i = 0; i < len; i++) {
		if (data[i] != 0xffU) {
			flips += 8U - (u32)__builtin_popcount(data[i]);
		}
	}

	return flips;
}


/*
 * Read a raw page (data + spare) with ECC disabled via DMA chain.
 * Output buffer must be at least writesz + sparesz bytes.
 */
static int readRawPage(const nand_die_t *nand, u32 page, void *data)
{
	dma_xfer_t xfer;

	dmaChainReset();

	/* Read data area without ECC */
	xfer = (dma_xfer_t) {
		.rowAddr = page,
		.colAddr = 0U,
		.transferSize = nand->info.writesz,
		.data = (addr_t)data,
	};
	dmaAddRead(nand, &xfer, 0);

	/* Read spare area via CHANGE READ COLUMN */
	xfer = (dma_xfer_t) {
		.rowAddr = page,
		.colAddr = (u16)nand->info.writesz,
		.transferSize = nand->info.sparesz,
		.data = (addr_t)((u8 *)data + nand->info.writesz),
	};
	dmaAddChangeReadColumn(nand, &xfer);

	return dmaRun();
}


/*
 * Verify whether an uncorrectable ECC error is truly uncorrectable or
 * caused by an erased page (erased pages have no valid ECC parity).
 *
 * Re-reads the page in raw mode and counts zero-bits per ECC chunk.
 * If all chunks have fewer flips than the ECC capability, the page is
 * considered erased (or near-erased) and the output buffer is filled with 0xFF.
 *
 * Returns 0 if the page is erased/clean, -EIO if truly uncorrectable.
 */
static int verifyEccError(const nand_die_t *nand, u32 page, void *outBuf)
{
	const u32 chunksz = nand->info.eccChunksz;
	const u32 nchunks = nand->info.writesz / chunksz;
	const u32 chunkEccsz = nand->info.eccsz / nchunks;
	const u32 eccCap = nand->info.eccCap;

	u8 *rawBuf = nand_common.dmaDataBuf;
	u32 i, maxFlips = 0, dataOffs, eccOffs, flips;

	int err = readRawPage(nand, page, rawBuf);
	if (err < 0) {
		return err;
	}

	for (i = 0; i < nchunks; i++) {
		dataOffs = i * chunksz;
		eccOffs = nand->info.writesz + NAND_BBM_SIZE + (i * chunkEccsz);

		flips = countBitflips(rawBuf + dataOffs, chunksz);
		flips += countBitflips(rawBuf + eccOffs, chunkEccsz);

		if (flips > eccCap) {
			/* A single chunk exceeded the ECC capability. Truly uncorrectable. */
			return -EIO;
		}

		if (flips > maxFlips) {
			maxFlips = flips;
		}
	}

	/* Page is erased or has tolerable bitflips - return clean data */
	hal_memset(outBuf, 0xffU, nand->info.writesz);

	return 0;
}


/* ======================== ONFI Timing Setup ======================== */


static u32 ns2cycles(u32 ns, u32 coreClk)
{
	u32 cycles = (u32)(((u64)ns * coreClk + 999999999U) / 1000000000U);
	return (cycles > 0U) ? cycles : 1U;
}


static void setOnfiTimingMode(unsigned int mode, const onfi_paramPage_t *paramPage, u32 coreClk, int edoEn)
{
	u32 tCS, tWW, tRR, tWB, tRHW, tWHR, tCCS, tADL, tREH, tRP, tRC, tWH, tWP, tWC, tVDLY;
	const onfi_timingMode_t *timings = onfi_getTimingModeSDR(mode);

	/* Most timings are programmed as (number of cycles - 1) */
	tCS = (ns2cycles(timings->tCS3, coreClk) - 1U) & 0xffffU;
	tWW = (ns2cycles(timings->tWW, coreClk) - 1U) & 0xffffU;

	nand_common.regs->tme0 = (tCS << 16) | tWW;

	tRR = (ns2cycles(timings->tRR, coreClk) - 1U) & 0xffffU;
	tWB = (ns2cycles(timings->tWB, coreClk) - 1U) & 0xffffU;

	nand_common.regs->tme1 = (tRR << 16) | tWB;

	tRHW = (ns2cycles(timings->tRHW, coreClk) - 1U) & 0xffffU;
	tWHR = (ns2cycles(timings->tWHR, coreClk) - 1U) & 0xffffU;

	nand_common.regs->tme2 = (tRHW << 16) | tWHR;

	if (paramPage == NULL) {
		tCCS = (ns2cycles(ONFI_TCCS_BASE, coreClk) - 1U) & 0xffffU;
		tADL = (ns2cycles(timings->tADL, coreClk) - 1U) & 0xffffU;
	}
	else {
		tCCS = (ns2cycles(paramPage->tCCS, coreClk) - 1U) & 0xffffU;
		tADL = (ns2cycles(paramPage->tADL, coreClk) - 1U) & 0xffffU;
	}

	nand_common.regs->tme3 = (tADL << 16) | tCCS;

	tREH = ns2cycles(timings->tREH, coreClk) & 0xffffU;
	tRP = (edoEn != 0) ? (ns2cycles(timings->tRP, coreClk) & 0xffffU) : (ns2cycles(timings->tREA, coreClk) & 0xffffU);
	tRC = ns2cycles(timings->tRC, coreClk);

	/* Ensure sum meets tRC */
	if ((tREH + tRP) < tRC) {
		tREH += (tRC - (tREH + tRP));
	}

	nand_common.regs->tme4 = ((tREH - 1U) << 16) | (tRP - 1U);

	tWH = ns2cycles(timings->tWH, coreClk) & 0xffffU;
	tWP = ns2cycles(timings->tWP, coreClk) & 0xffffU;
	tWC = ns2cycles(timings->tWC, coreClk);

	/* Ensure sum meets tWC */
	if ((tWH + tWP) < tWC) {
		tWH += (tWC - (tWH + tWP));
	}

	nand_common.regs->tme5 = ((tWH - 1U) << 16) | (tWP - 1U);

	/* tme6-tme10 not used in SDR mode */
	nand_common.regs->tme6 = 0;
	nand_common.regs->tme7 = 0;
	nand_common.regs->tme8 = 0;
	nand_common.regs->tme9 = 0;
	nand_common.regs->tme10 = 0;

	tVDLY = (ns2cycles(ONFI_TVDLY, coreClk) - 1U) & 0xffffU;

	nand_common.regs->tme11 = (tCS << 16) | tVDLY;

	if (edoEn != 0) {
		nand_common.regs->ctrl0 |= CTRL0_EDO;
	}
	else {
		nand_common.regs->ctrl0 &= ~CTRL0_EDO;
	}
}


/* ======================== Controller Initialization ======================== */


static void resetNandfctrl2(void)
{
	abortOp();

	/* Clear any pending status */
	nand_common.regs->sts1 = nand_common.regs->sts1;

	nand_common.regs->ctrl2 = CTRL2_RST;

	/* Wait for reset to complete */
	while ((nand_common.regs->ctrl2 & CTRL2_RST) != 0U) { }

	nand_common.regs->ctrl0 = CTRL0_BBM | CTRL0_LLM | CTRL0_EE;
	/* Disable all interrupts */
	nand_common.regs->ctrl1 = 0U;
	/* Disable write protection */
	nand_common.regs->ctrl3 = 0U;
	/* Set SEFI low */
	nand_common.regs->ctrl4 = 0U;

	setOnfiTimingMode(0, NULL, SYSCLK_FREQ, 0);

	/* Max timeout */
	nand_common.regs->tout0 = 0xffffffffU;
}


static int setMemorySelect(u32 pageSz, u16 spareSz)
{
	const u32 cap2 = nand_common.regs->cap2;
	const u32 cap3 = nand_common.regs->cap3;
	const u32 cap4 = nand_common.regs->cap4;

	u32 msel;

	if (((cap2 & CAP_MDATA) == pageSz) && (((cap2 & CAP_MSPARE) >> CAP_MSPARE_SHFT) <= spareSz)) {
		msel = 0U;
	}
	else if (((cap3 & CAP_MDATA) == pageSz) && (((cap3 & CAP_MSPARE) >> CAP_MSPARE_SHFT) <= spareSz)) {
		msel = 1U;
	}
	else if (((cap4 & CAP_MDATA) == pageSz) && (((cap4 & CAP_MSPARE) >> CAP_MSPARE_SHFT) <= spareSz)) {
		msel = 2U;
	}
	else {
		return -EINVAL;
	}

	nand_common.regs->ctrl0 = (nand_common.regs->ctrl0 & ~CTRL0_MSEL_MSK) | (msel << CTRL0_MSEL_SHFT);

	return 0;
}


static void fillSpareLayout(nandfctrl2_info_t *info)
{
	const u32 msel = (nand_common.regs->ctrl0 & CTRL0_MSEL_MSK) >> CTRL0_MSEL_SHFT;
	const volatile u32 *cap = &nand_common.regs->cap2 + msel;
	const u32 eccSel = (*cap & CAP_MSEL) >> CAP_MSEL_SHFT;
	const u32 mspare = (*cap & CAP_MSPARE) >> CAP_MSPARE_SHFT;
	const u32 cap1 = nand_common.regs->cap1;

	u32 chunksz, gfsz, eccCap;

	if (eccSel == 0) {
		chunksz = (cap1 & CAP1_E0CHUNK) >> CAP1_E0CHUNK_SHFT;
		gfsz = (cap1 & CAP1_E0GF) >> CAP1_E0GF_SHFT;
		eccCap = (cap1 & CAP1_E0CAP) >> CAP1_E0CAP_SHFT;
	}
	else {
		chunksz = (cap1 & CAP1_E1CHUNK) >> CAP1_E1CHUNK_SHFT;
		gfsz = (cap1 & CAP1_E1GF) >> CAP1_E1GF_SHFT;
		eccCap = (cap1 & CAP1_E1CAP) >> CAP1_E1CAP_SHFT;
	}

	info->eccChunksz = 1U << chunksz;
	info->eccsz = 2 * ((eccCap * gfsz + 15U) / 16U) * (info->writesz / info->eccChunksz);
	info->eccCap = eccCap;

	info->sparesz = mspare;
	info->spareavail = info->sparesz - info->eccsz - NAND_BBM_SIZE;
}


static int setupFlash(nand_die_t *nand)
{
	int edoEn, err;
	unsigned int timingMode;
	onfi_paramPage_t paramPage;
	dma_xfer_t xfer;
	flash_id_t *flashId;
	u32 timeout;

	/* Reset + Read ID in one chain */
	dmaChainReset();
	dmaAddReset(nand);

	xfer = (dma_xfer_t) {
		.transferSize = sizeof(flash_id_t),
		.data = (addr_t)nand_common.scratchBuf,
	};
	dmaAddReadId(nand, &xfer);

	err = dmaRun();
	if (err < 0) {
		lib_printf("\nnandfctrl2/flash%u: couldn't read flash ID", nand->minor);
		return err;
	}

	flashId = (flash_id_t *)nand_common.scratchBuf;

	if ((flashId->manufacturerId == 0x2cU) && (flashId->deviceId == 0xacU)) {
		nand->info.name = "Micron MT29F4G08ABBFA";
		/* 10 ms */
		timeout = 10 * SYSCLK_FREQ / 1000U;
	}
	else if ((flashId->manufacturerId == 0x2cU) && (flashId->deviceId == 0x68U)) {
		nand->info.name = "Micron MT29F32G08ABAAA";
		/* 7 ms */
		timeout = 7 * SYSCLK_FREQ / 1000U;
	}
	else {
		nand->info.name = "Unknown NAND flash";
		timeout = 0xffffffffU;
	}

	nand_common.regs->tout0 = timeout;

	/* Read ONFI parameter page */
	dmaChainReset();

	xfer = (dma_xfer_t) {
		.transferSize = sizeof(onfi_paramPage_t),
		.data = (addr_t)nand_common.dmaDataBuf,
	};
	dmaAddReadParamPage(nand, &xfer);

	err = dmaRun();
	if (err < 0) {
		lib_printf("\nnandfctrl2/flash%u: couldn't read ONFI parameter page", nand->minor);
		return -EIO;
	}

	onfi_deserializeParamPage(&paramPage, nand_common.dmaDataBuf);

	if ((paramPage.signature[0] != 'O') || (paramPage.signature[1] != 'N') || (paramPage.signature[2] != 'F') || (paramPage.signature[3] != 'I')) {
		lib_printf("\nnandfctrl2/flash%u: couldn't read ONFI parameter page", nand->minor);
		return -EIO;
	}

	timingMode = onfi_calcTimingMode(&paramPage);

	edoEn = (timingMode > 3) ? 1 : 0;

	setOnfiTimingMode(timingMode, &paramPage, SYSCLK_FREQ, edoEn);

	if (setMemorySelect(paramPage.bytesPerPage, paramPage.spareBytesPerPage) != 0) {
		lib_printf("\nnandfctrl2/flash%u: unsupported config (pageSz: %u, spareSz: %u)", nand->minor, paramPage.bytesPerPage, paramPage.spareBytesPerPage);
		return -EINVAL;
	}

	nand->info.erasesz = paramPage.bytesPerPage * paramPage.pagesPerBlock;
	nand->info.size = (u64)nand->info.erasesz * paramPage.blocksPerLun * paramPage.numLuns;
	nand->info.writesz = paramPage.bytesPerPage;
	nand->info.pagesPerBlock = paramPage.pagesPerBlock;
	fillSpareLayout(&nand->info);

	return 0;
}


/* ======================== Bad Block Management ======================== */


static int isBlockBadPhys(const nand_die_t *nand, u32 block)
{
	dma_xfer_t xfer;

	dmaChainReset();

	xfer = (dma_xfer_t) {
		.rowAddr = block * nand->info.pagesPerBlock,
		.colAddr = (u16)nand->info.writesz,
		.transferSize = 1U,
		.data = (addr_t)nand_common.scratchBuf,
	};
	dmaAddRead(nand, &xfer, 0);

	int err = dmaRun();
	if (err < 0) {
		return 1; /* Read error: assume bad */
	}

	/* 0x00 in first spare byte = bad block marker */
	return (nand_common.scratchBuf[0] == 0x00U) ? 1 : 0;
}


static void badBlockTableSet(nand_die_t *nand, u32 block)
{
	nand->bbt[block >> 5] |= (1U << (block & 31));
}


static int scanBadBlocks(nand_die_t *nand)
{
	u32 block;
	const u32 numBlocks = nand->info.size / nand->info.erasesz;

	if (numBlocks > NANDFCTRL2_BBT_MAX_BLOCKS) {
		lib_printf("nandfctrl2/flash%u: unsupported config: %u blocks\n", nand->minor, numBlocks);
		return -EINVAL;
	}

	for (block = 0; block < numBlocks; block++) {
		if (isBlockBadPhys(nand, block)) {
			badBlockTableSet(nand, block);
		}
	}

	return 0;
}


/* ======================== Public API ======================== */


nand_die_t *nand_get(u32 minor)
{
	static nand_die_t dev[NAND_DIE_CNT];

	if (minor < NAND_DIE_CNT) {
		return &dev[minor];
	}
	return NULL;
}


int nandfctrl2_resetFlash(const nand_die_t *nand)
{
	dmaChainReset();
	dmaAddReset(nand);
	return dmaRun();
}


int nandfctrl2_pageWrite(const nand_die_t *nand, u32 page, const void *data)
{
	const u32 chunksz = nand->info.eccChunksz;
	const u32 nChunks = nand->info.writesz / chunksz;
	const u32 tagOfs = nand->info.writesz + nand->info.sparesz - nand->info.spareavail;

	const u8 *dataPtr = (const u8 *)data;
	dma_xfer_t xfer;
	u32 i;

	dmaChainReset();

	/* Write data area chunk by chunk */
	for (i = 0; i < nChunks; i++) {
		xfer = (dma_xfer_t) {
			.rowAddr = page,
			.colAddr = (u16)(i * chunksz),
			.transferSize = chunksz,
			.data = (addr_t)(dataPtr + (i * chunksz)),
		};

		if (i == 0) {
			/* First chunk: PAGE_PROGRAM, no commit, ECC on */
			dmaAddPageProgram(nand, &xfer, 1, 0);
		}
		else {
			/* Subsequent chunks: CHANGE_WRITE_COLUMN */
			dmaAddChangeWriteColumn(nand, &xfer, 1, 0);
		}
	}

	/* Tag area: CHANGE_WRITE_COLUMN with 0xFF padding + commit
	 * This prevents the controller from zero-padding the spare area. */
	hal_memset(nand_common.scratchBuf, 0xffU, nand->info.spareavail);
	xfer = (dma_xfer_t) {
		.rowAddr = page,
		.colAddr = (u16)tagOfs,
		.transferSize = nand->info.spareavail,
		.data = (addr_t)nand_common.scratchBuf,
	};
	dmaAddChangeWriteColumn(nand, &xfer, 0, 1);

	/* Status read */
	dmaAddStatusRead(nand);

	return dmaRunAndCheckStatus();
}


int nandfctrl2_pageRead(const nand_die_t *nand, u32 page, void *data)
{
	dma_xfer_t xfer;
	int err;
	u32 dsts;

	dmaChainReset();

	xfer = (dma_xfer_t) {
		.rowAddr = page,
		.colAddr = 0U,
		.transferSize = nand->info.writesz,
		.data = (addr_t)data,
	};
	dmaAddRead(nand, &xfer, 1);

	err = dmaRun();
	if (err < 0) {
		return err;
	}

	/* Check descriptor status for ECC errors */
	dsts = nand_common.descs[0].dsts;

	if ((dsts & (DSTS_UE | DSTS_ECFAIL_MSK)) != 0U) {
		/*
		 * Uncorrectable ECC error reported. This can happen for:
		 * 1. Erased pages (no valid ECC parity) - common and harmless
		 * 2. Truly corrupted data - must be flagged
		 *
		 * Verify by re-reading in raw mode and checking bitflip counts.
		 */
		return verifyEccError(nand, page, data);
	}

	return EOK;
}


int nandfctrl2_rawPageRead(const nand_die_t *nand, u32 page, void *data)
{
	/* Read page + entire metadata */
	dma_xfer_t xfer;
	dmaChainReset();

	/* Whole Page Access for the Data Area */
	xfer = (dma_xfer_t) {
		.rowAddr = page,
		.colAddr = 0U,
		.transferSize = nand->info.writesz,
		.data = (addr_t)data,
	};
	dmaAddRead(nand, &xfer, 0);

	/* Free Size Access for the Spare Area via CHANGE_READ_COLUMN */
	xfer = (dma_xfer_t) {
		.rowAddr = page,
		.colAddr = (u16)nand->info.writesz,
		.transferSize = nand->info.sparesz,
		.data = (addr_t)((u8 *)data + nand->info.writesz),
	};
	dmaAddChangeReadColumn(nand, &xfer);

	return dmaRun();
}


int flashdrv_metaWrite(const nand_die_t *nand, u32 page, const void *data, size_t size)
{
	u32 tagOfs = nand->info.writesz + nand->info.sparesz - nand->info.spareavail;
	dma_xfer_t xfer;

	dmaChainReset();

	xfer = (dma_xfer_t) {
		.rowAddr = page,
		.colAddr = (u16)tagOfs,
		.transferSize = size,
		.data = (addr_t)data,
	};
	dmaAddPageProgram(nand, &xfer, 0, 1);

	dmaAddStatusRead(nand);

	return dmaRunAndCheckStatus();
}


int flashdrv_metaRead(const nand_die_t *nand, u32 page, void *data, size_t size)
{
	u32 tagOfs = nand->info.writesz + nand->info.sparesz - nand->info.spareavail;
	dma_xfer_t xfer;

	dmaChainReset();

	xfer = (dma_xfer_t) {
		.rowAddr = page,
		.colAddr = (u16)tagOfs,
		.transferSize = size,
		.data = (addr_t)data,
	};
	dmaAddRead(nand, &xfer, 0);

	return dmaRun();
}


int nandfctrl2_eraseBlock(const nand_die_t *nand, u32 block)
{
	dmaChainReset();

	dmaAddBlockErase(nand, block * nand->info.pagesPerBlock);
	dmaAddStatusRead(nand);

	return dmaRunAndCheckStatus();
}


int nandfctrl2_isbad(const nand_die_t *nand, u32 block)
{
	return (nand->bbt[block >> 5] >> (block & 31)) & 1U;
}


int nandfctrl2_markbad(nand_die_t *nand, u32 block)
{
	int err;
	dma_xfer_t xfer;

	nand_common.scratchBuf[0] = 0x00U;

	dmaChainReset();

	xfer = (dma_xfer_t) {
		.rowAddr = block * nand->info.pagesPerBlock,
		.colAddr = (u16)nand->info.writesz,
		.transferSize = 1U,
		.data = (addr_t)nand_common.scratchBuf,
	};
	dmaAddPageProgram(nand, &xfer, 0, 1);

	dmaAddStatusRead(nand);

	err = dmaRunAndCheckStatus();
	if (err == 0) {
		badBlockTableSet(nand, block);
	}

	return err;
}


int nandfctrl2_init(nand_die_t *nand)
{
	int err;

	if (nand_common.initialized == 0U) {

		nand_common.regs = (nandfctrl2_regs_t *)NANDFCTRL2_BASE;

		resetNandfctrl2();

		nand_common.initialized = 1U;
	}

	nandfctrl2_resetFlash(nand);

	err = setupFlash(nand);
	if (err < 0) {
		return err;
	}

	err = nandfctrl2_resetFlash(nand);
	if (err < 0) {
		return err;
	}

	err = scanBadBlocks(nand);

	return err;
}
