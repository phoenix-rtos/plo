/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GRLIB NANDFCTRL2 flash driver
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Leckowski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <hal/hal.h>
#include <lib/lib.h>

#include <stdbool.h>

#include "nandfctrl2.h"
#include "onfi-4.h"


/* Core control 0 register */
#define CTRL0_EE         (1U << 13) /* EDAC enable */
#define CTRL0_RE         (1U << 12) /* Data randomization enable */
#define CTRL0_LLM        (1U << 10) /* Linked list mode enable */
#define CTRL0_BBM        (1U << 9)  /* Preserve Bad Block Marking */
#define CTRL0_IFSEL_SHFT 5          /* Data Interface select shift */
#define CTRL0_IFSEL_MSK  (3U << 5)  /* Data Interface select mask */
#define CTRL0_MSEL_SHFT  3          /* Memory select shift */
#define CTRL0_MSEL_MSK   (3U << 3)  /* Memory select mask */


/* Core control 2 register */
#define CTRL2_STOP_LL (1U << 3) /* Stop Linked List */
#define CTRL2_ABORT   (1U << 2) /* Abort ongoing execution */
#define CTRL2_DT      (1U << 1) /* Descriptor trigger */
#define CTRL2_RST     (1U << 0) /* Software reset (APB registers) */


/* Core status 0 register */
#define STS0_DA  (1U << 1) /* Descriptor active */
#define STS0_RDY (1U << 0) /* Ready state (set when ready) */


/* Core status 1 register */
#define STS1_STOPLL (1U << 9) /* Stop linked list interrupt */
#define STS1_ABORT  (1U << 8) /* Abort interrupt (set when abort completes) */
#define STS1_TMOUT  (1U << 6) /* Ready/Busy timeout */
#define STS1_CMD    (1U << 5) /* Invalid command */
#define STS1_DS     (1U << 1) /* Descriptor finished */


/* Descriptor command register */
#define DCMD_CMD2_SHFT 24
#define DCMD_CMD1_SHFT 16
#define DCMD_PRECH     (1U << 12) /* Pre-cache */
#define DCMD_DD        (1U << 11) /* Data DMA Disable (1 = APB, 0 = DMA) */
#define DCMD_ED        (1U << 10) /* EDAC Disable for this descriptor */
#define DCMD_RD        (1U << 9)  /* Randomization Disable for this descriptor */
#define DCMD_SRB       (1U << 6)  /* Skip Ready/Busy wait */
#define DCMD_SA        (1U << 5)  /* Skip Address phase */
#define DCMD_SD        (1U << 4)  /* Skip Data phase */
#define DCMD_SC2       (1U << 3)  /* Skip second command phase */
#define DCMD_WARDY     (1U << 2)  /* Wait ARDY */
#define DCMD_IRQ_DS    (1U << 1)  /* Descriptor interrupt enable */
#define DCMD_EN        (1U << 0)  /* Descriptor Enable */

/* Descriptor row & control register */
#define DROW_TAGEN        (1U << 24) /* Tag enable */
#define DROW_ROWADDR_SHFT 0          /* 24-bit Row Address (3 bytes) */

/* Descriptor column & size register */
#define DCOLSIZE_SIZE_SHFT 16 /* Transfer size in bytes */
#define DCOLSIZE_COL_SHFT  0  /* 16-bit Column Address */


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


typedef struct {
	u32 dcmd;     /* Descriptor command register */
	u32 dtarsel0; /* Descriptor target select 0 register */
	u32 dtarsel1; /* Descriptor target select 1 register */
	u32 dchsel;   /* Descriptor channel select register */
	u32 drbsel;   /* Descriptor ready/busy select register */
	u32 drow;     /* Descriptor row & control register */
	u32 dcolsize; /* Descriptor column & size register */
	u32 dsts;     /* Descriptor status register */
	u32 deccsts0; /* Descriptor ECC status register */
	u32 res0[3];  /* Reserved */
	u32 ddpl;     /* Descriptor data pointer low register */
	u32 res1;     /* Reserved */
	u32 dllpl;    /* Descriptor linked list pointer low register */
	u32 res2;     /* Reserved */
} dma_desc_t;


typedef struct {
	u64 ceMask;
	u32 rbMask;
	u32 channelMask;
} nand_layoutMap_t;


typedef struct {
	u8 cmd1;
	u8 cmd2;
	u8 addrsz;
	u8 skipCmd2;
} nand_cmd_t;


static const nand_cmd_t commands[cmd_count] = {
	{ 0x00U, 0x30U, 5U, 0U }, /* cmd_read */
	{ 0x00U, 0x32U, 5U, 0U }, /* cmd_read_multiplane */
	{ 0x00U, 0x35U, 5U, 0U }, /* cmd_copyback_read */
	{ 0x05U, 0xe0U, 5U, 0U }, /* cmd_change_read_column */
	{ 0x06U, 0xe0U, 5U, 0U }, /* cmd_change_read_column_enhanced */
	{ 0x00U, 0x31U, 5U, 0U }, /* cmd_read_cache_random */
	{ 0x31U, 0x00U, 5U, 1U }, /* cmd_read_cache_sequential */
	{ 0x3fU, 0x00U, 5U, 1U }, /* cmd_read_cache_end */
	{ 0x60U, 0xd0U, 3U, 0U }, /* cmd_block_erase */
	{ 0x60U, 0xd1U, 3U, 0U }, /* cmd_block_erase_multiplane */
	{ 0x70U, 0x00U, 1U, 1U }, /* cmd_read_status */
	{ 0x78U, 0x00U, 3U, 1U }, /* cmd_read_status_enhanced */
	{ 0x80U, 0x10U, 5U, 0U }, /* cmd_page_program */
	{ 0x80U, 0x11U, 5U, 0U }, /* cmd_page_program_multiplane */
	{ 0x80U, 0x15U, 5U, 0U }, /* cmd_page_cache_program */
	{ 0x85U, 0x10U, 5U, 0U }, /* cmd_copyback_program */
	{ 0x85U, 0x11U, 5U, 0U }, /* cmd_copyback_program_multiplane */
	{ 0x85U, 0x11U, 2U, 0U }, /* cmd_small_data_move */
	{ 0x85U, 0x00U, 5U, 1U }, /* cmd_change_write_column */
	{ 0x85U, 0x00U, 5U, 1U }, /* cmd_change_row_address */
	{ 0x90U, 0x00U, 1U, 1U }, /* cmd_read_id */
	{ 0xe1U, 0x00U, 0U, 1U }, /* cmd_volume_select */
	{ 0xe2U, 0x00U, 0U, 1U }, /* cmd_odt_configure */
	{ 0xecU, 0x00U, 1U, 1U }, /* cmd_read_parameter_page */
	{ 0xedU, 0x00U, 1U, 1U }, /* cmd_read_unique_id */
	{ 0xeeU, 0x00U, 1U, 1U }, /* cmd_get_features */
	{ 0xefU, 0x00U, 5U, 1U }, /* cmd_set_features */
	{ 0xd4U, 0x00U, 0U, 1U }, /* cmd_lun_get_features */
	{ 0xd5U, 0x00U, 0U, 1U }, /* cmd_lun_set_features */
	{ 0xd9U, 0x00U, 0U, 1U }, /* cmd_zq_calibration_short */
	{ 0xf9U, 0x00U, 0U, 1U }, /* cmd_zq_calibration_long */
	{ 0xfaU, 0x00U, 0U, 1U }, /* cmd_reset_lun */
	{ 0xfcU, 0x00U, 0U, 1U }, /* cmd_synchronous_reset */
	{ 0xffU, 0x00U, 0U, 1U }  /* cmd_reset */
};


static const nand_layoutMap_t fctrl2_flashMap[NAND_FLASH_CNT] = NAND_FLASH_MAP;


typedef struct {
	dma_t *last;
	dma_t *first;
	char buffer[];
} nandfctrl2_dma_t;


typedef struct {
	u8 manufacturerId;
	u8 deviceId;
	u8 bytes[3];
} __attribute__((packed)) flash_id_t;


struct {
	volatile nandfctrl2_regs_t *regs;

	bool initialized;

	u32 rawmetasz; /* user metadata + ECC16 metadata bytes */

	volatile int result, bch_status, bch_done;

	nandfctrl2_info_t info;

	u8 dmaDataBuf[16 * 1024] __attribute__((aligned(64)));
	u8 dmaDescBuf[16 * sizeof(dma_desc_t)] __attribute__((aligned(64)));
} fctrl2_common;


static void setupRoutingReg(u32 minor)
{
	fctrl2_common.regs->dtarsel0 = (u32)(fctrl2_flashMap[minor].ceMask & 0xffffffffU);
	fctrl2_common.regs->dtarsel1 = (u32)(fctrl2_flashMap[minor].ceMask >> 32);
	fctrl2_common.regs->drbsel = fctrl2_flashMap[minor].rbMask;
	fctrl2_common.regs->dchsel = fctrl2_flashMap[minor].channelMask;
}


static void setupRoutingDesc(u32 minor, dma_desc_t *desc)
{
	desc->dtarsel0 = (u32)(fctrl2_flashMap[minor].ceMask & 0xffffffffU);
	desc->dtarsel1 = (u32)(fctrl2_flashMap[minor].ceMask >> 32);
	desc->drbsel = fctrl2_flashMap[minor].rbMask;
	desc->dchsel = fctrl2_flashMap[minor].channelMask;
}


static int nand_cmdaddr(gpmi_dma3_t *cmd, int chip, void *buffer, u16 addrsz)
{
	hal_memset(cmd, 0, sizeof(*cmd));

	cmd->dma.flags = dma_hot | dma_w4endcmd | dma_nandlock | dma_read | dma_pio(3);
	cmd->dma.bufsz = (addrsz & 0x7) + 1;

	cmd->ctrl0 = (chip * gpmi_chip) | gpmi_write | gpmi_command_bytes | gpmi_lock_cs | gpmi_8bit | cmd->dma.bufsz;

	if (addrsz != 0u) {
		cmd->ctrl0 |= gpmi_address_increment;
	}

	return sizeof(*cmd);
}


static int nand_read(gpmi_dma3_t *cmd, int chip, void *buffer, u16 bufsz)
{
	hal_memset(cmd, 0, sizeof(*cmd));

	cmd->dma.flags = dma_hot | dma_nandlock | dma_w4endcmd | dma_write | dma_pio(3);
	cmd->dma.bufsz = bufsz;

	cmd->ctrl0 = (chip * gpmi_chip) | gpmi_read | gpmi_data_bytes | gpmi_8bit | cmd->dma.bufsz;

	return sizeof(*cmd);
}


static int nand_readcompare(gpmi_dma3_t *cmd, int chip, u16 mask, u16 value)
{
	hal_memset(cmd, 0, sizeof(*cmd));

	cmd->dma.flags = dma_hot | dma_nandlock | dma_w4endcmd | dma_noxfer | dma_pio(3);

	cmd->ctrl0 = (chip * gpmi_chip) | gpmi_read_compare | gpmi_data_bytes | gpmi_8bit | 1;
	cmd->compare = (mask << 16) | value;

	return sizeof(*cmd);
}


static int nand_ecread(gpmi_dma6_t *cmd, int chip, void *payload, void *auxiliary, u16 bufsz)
{
	int eccmode = (payload == NULL) ? 0x100 : 0x1ff;
	hal_memset(cmd, 0, sizeof(*cmd));

	cmd->dma.flags = dma_hot | dma_nandlock | dma_w4endcmd | dma_noxfer | dma_pio(6);
	cmd->dma.bufsz = 0;
	cmd->dma.buffer = 0;

	cmd->ctrl0 = (chip * gpmi_chip) | gpmi_read | gpmi_data_bytes | gpmi_8bit | bufsz;
	cmd->compare = 0;
	cmd->eccctrl = (1 << 12) | eccmode;
	cmd->ecccount = bufsz;

	return sizeof(*cmd);
}


static int nand_disablebch(gpmi_dma3_t *cmd, int chip)
{
	hal_memset(cmd, 0, sizeof(*cmd));

	cmd->dma.flags = dma_hot | dma_w4endcmd | dma_nandlock | dma_noxfer | dma_pio(3);
	cmd->ctrl0 = (chip * gpmi_chip) | gpmi_wait_for_ready | gpmi_lock_cs | gpmi_data_bytes | gpmi_8bit;

	return sizeof(*cmd);
}


static int nand_write(gpmi_dma3_t *cmd, int chip, void *buffer, u16 bufsz)
{
	hal_memset(cmd, 0, sizeof(*cmd));

	cmd->dma.flags = dma_hot | dma_nandlock | dma_w4endcmd | dma_read | dma_pio(3);
	cmd->dma.bufsz = bufsz;

	cmd->ctrl0 = (chip * gpmi_chip) | gpmi_write | gpmi_lock_cs | gpmi_data_bytes | gpmi_8bit | cmd->dma.bufsz;

	return sizeof(*cmd);
}


static int nand_ecwrite(gpmi_dma6_t *cmd, int chip, void *payload, void *auxiliary, u16 bufsz)
{
	int eccmode = (payload == NULL) ? 0x100 : 0x1ff;

	hal_memset(cmd, 0, sizeof(*cmd));

	cmd->dma.flags = dma_hot | dma_nandlock | dma_w4endcmd | dma_noxfer | dma_pio(6);
	cmd->dma.bufsz = 0;
	cmd->dma.buffer = 0;

	cmd->ctrl0 = (chip * gpmi_chip) | gpmi_write | gpmi_lock_cs | gpmi_data_bytes | gpmi_8bit;
	cmd->compare = 0;
	cmd->eccctrl = (1u << 13) | (1u << 12) | eccmode;
	cmd->ecccount = bufsz;
	if (payload != NULL) {
	}

	return sizeof(*cmd);
}


static int nand_w4ready(gpmi_dma1_t *cmd, int chip)
{
	hal_memset(cmd, 0, sizeof(*cmd));

	cmd->dma.flags = dma_hot | dma_w4endcmd | dma_w4ready | dma_noxfer | dma_pio(1);
	cmd->ctrl0 = (chip * gpmi_chip) | gpmi_wait_for_ready | gpmi_8bit;

	return sizeof(*cmd);
}


nandfctrl2_dma_t *nandfctrl2_dma(void)
{
	nandfctrl2_dma_t *dma = (nandfctrl2_dma_t *)nand_dma;
	dma->last = NULL;
	dma->first = NULL;

	return dma;
}


static int nandfctrl2_wait4ready(void)
{
	while ((fctrl2_common.regs->sts0 & STS0_RDY) == 0) { }
}

static int nandfctrl2_wait4DescriptorDone(void)
{
	while ((fctrl2_common.regs->sts0 & STS0_DA) != 0) { }
}

int nanddrv_disablebch(nandfctrl2_dma_t *dma, int chip)
{
	void *next = dma->last;

	if (next != NULL) {
		next += dma_size(dma->last);
	}
	else {
		next = dma->buffer;
	}

	nand_disablebch(next, chip);
	dma_sequence(dma->last, next);
	dma->last = next;

	if (dma->first == NULL) {
		dma->first = dma->last;
	}

	return 0;
}


int nanddrv_finish(nandfctrl2_dma_t *dma)
{
	void *next = dma->last;

	if (next != NULL) {
		next += dma_size(dma->last);
	}
	else {
		next = dma->buffer;
	}

	dma_terminate(next, 0);
	dma_sequence(dma->last, next);
	dma->last = next;

	if (dma->first == NULL) {
		dma->first = dma->last;
	}

	return 0;
}

int nanddrv_issue(nandfctrl2_dma_t *dma, int c, int chip, void *addr, unsigned datasz, void *data, void *aux)
{
	void *next = dma->last;
	int sz;
	char *cmdaddr;

	if (next != NULL) {
		next += dma_size(dma->last);
	}
	else {
		next = dma->buffer;
	}

	if ((commands[c].data > 0) && (datasz != commands[c].data)) {
		return -1;
	}

	if ((commands[c].data == -1) && (datasz == 0u)) {
		return -1;
	}

	if ((commands[c].data != 0) && (datasz != 0)) {
		return -1;
	}

	cmdaddr = next;
	cmdaddr[0] = commands[c].cmd1;
	hal_memcpy(cmdaddr + 1, addr, commands[c].addrsz);
	cmdaddr[7] = commands[c].cmd2;
	next += 8;

	sz = nand_cmdaddr(next, chip, cmdaddr, commands[c].addrsz);
	dma_sequence(dma->last, next);
	dma->last = next;
	next += sz;

	if (dma->first == NULL) {
		dma->first = dma->last;
	}

	if (datasz != 0u) {
		if (aux == NULL) {
			/* No error correction */
			sz = nand_write(next, chip, data, datasz);
		}
		else {
			sz = nand_ecwrite(next, chip, data, aux, datasz);
		}

		dma_sequence(dma->last, next);
		dma->last = next;
		next += sz;
	}

	if (commands[c].cmd2 != 0) {
		sz = nand_cmdaddr(next, chip, cmdaddr + 7, 0);
		dma_sequence(dma->last, next);
		dma->last = next;
	}

	return 0;
}


int nanddrv_readback(nandfctrl2_dma_t *dma, int chip, int bufsz, void *buf, void *aux)
{
	void *next = dma->last;

	if (next != NULL) {
		next += dma_size(dma->last);
	}
	else {
		next = dma->buffer;
	}

	if (aux == NULL) {
		/* No error correction */
		nand_read(next, chip, buf, bufsz);
	}
	else {
		nand_ecread(next, chip, buf, aux, bufsz);
	}

	dma_sequence(dma->last, next);
	dma->last = next;

	if (dma->first == NULL) {
		dma->first = dma->last;
	}

	return 0;
}


int nanddrv_readcompare(nandfctrl2_dma_t *dma, int chip, u16 mask, u16 value, int err)
{
	void *next = dma->last, *terminator;
	int sz;

	if (next != NULL) {
		next += dma_size(dma->last);
	}
	else {
		next = dma->buffer;
	}

	terminator = next;
	sz = dma_terminate(terminator, err);
	next += sz;

	sz = nand_readcompare(next, chip, mask, value);
	dma_sequence(dma->last, next);
	dma->last = next;
	next += sz;

	dma_check(next, terminator);
	dma_sequence(dma->last, next);
	dma->last = next;

	if (dma->first == NULL) {
		dma->first = dma->last;
	}

	return 0;
}


int nandfctrl2_reset(nandfctrl2_dma_t *dma)
{
	int chip = 0, channel = 0;
	dma->first = NULL;
	dma->last = NULL;

	nanddrv_issue(dma, cmd_reset, chip, NULL, 0, NULL, NULL);
	nanddrv_finish(dma);

	fctrl2_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (fctrl2_common.result > 0) {
		hal_cpuHalt();
	}

	return fctrl2_common.result;
}


static int nand_issue(unsigned int cmd, bool useDma)
{
	if (cmd >= cmd_count) {
		return -EINVAL;
	}

	u32 dcmd = (commands[cmd].cmd1 << DCMD_CMD1_SHFT) | (commands[cmd].cmd2 << DCMD_CMD2_SHFT);
	if (!useDma) {
		dcmd |= DCMD_DD;
	}
}


int nandfctrl2_write(nandfctrl2_dma_t *dma, u32 paddr, void *data, char *aux)
{
	int chip = 0, channel = 0, sz;
	char addr[5] = { 0 };
	int skipMeta = 0, err;
	hal_memcpy(addr + 2, &paddr, 3);

	if (data == NULL) {
		sz = fctrl2_common.rawmetasz;
	}
	else {
		sz = fctrl2_common.info.writesz + fctrl2_common.info.metasz;
		if (aux == NULL) {
			aux = (char *)fctrl2_common.uncached_buf;
			skipMeta = 1;
		}
	}

	dma->first = NULL;
	dma->last = NULL;

	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, cmd_program_page, chip, addr, sz, data, aux);
	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, cmd_read_status, 0, NULL, 0, NULL, NULL);
	nanddrv_readcompare(dma, chip, 0x3, 0, -1);
	nanddrv_finish(dma);

	if (data == NULL) {
		/* Trick BCH controller into thinking that the whole page consists of just the metadata block */
		*(nanddrv_common.bch + bch_flash0layout0) &= ~(0xffu << 24);

		*(nanddrv_common.bch + bch_flash0layout1) &= ~(0xffffu << 16);
		*(nanddrv_common.bch + bch_flash0layout1) |= nanddrv_common.rawmetasz << 16;
	}
	else if (skipMeta != 0) {
		/* Perform partial page programming (don't change metadata and its ECC) */
		hal_memset(aux, 0xff, fctrl2_common.rawmetasz);

		/* Treat metadata and its ECC as raw byte area without ECC */
		*(nanddrv_common.bch + bch_flash0layout0) &= ~(0x1fffu << 11);
		*(nanddrv_common.bch + bch_flash0layout0) |= nanddrv_common.rawmetasz << 16;
	}

	fctrl2_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (fctrl2_common.result > 0) {
		hal_cpuHalt();
	}

	err = fctrl2_common.result;

	if (data == NULL) {
		*(nanddrv_common.bch + bch_flash0layout0) |= 8u << 24;

		*(nanddrv_common.bch + bch_flash0layout1) &= ~(0xffffu << 16);
		*(nanddrv_common.bch + bch_flash0layout1) |= (nanddrv_common.info.writesz + nanddrv_common.info.metasz) << 16;
	}
	else if (skipMeta != 0) {
		*(nanddrv_common.bch + bch_flash0layout0) &= ~(0x1fffu << 11);
		*(nanddrv_common.bch + bch_flash0layout0) |= (16u << 16) | (8u << 11);
	}

	return err;
}


int nandfctrl2_read(nandfctrl2_dma_t *dma, u32 paddr, void *data, nandfctrl2_meta_t *aux)
{
	int chip = 0, channel = 0, sz = 0;
	char addr[5] = { 0 };
	hal_memcpy(addr + 2, &paddr, 3);

	if (data != NULL) {
		sz = fctrl2_common.info.writesz + fctrl2_common.info.metasz;
	}
	else {
		sz = fctrl2_common.rawmetasz;
	}

	dma->first = NULL;
	dma->last = NULL;

	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_read_page, chip, addr, 0, NULL, NULL);
	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_readback(dma, chip, sz, data, aux);
	nanddrv_disablebch(dma, chip);
	nanddrv_finish(dma);

	fctrl2_common.result = 1;
	fctrl2_common.bch_done = 0;
	dma_run((dma_t *)dma->first, channel);

	while (fctrl2_common.bch_done == 0) {
		hal_cpuHalt();
	}

	while (fctrl2_common.result > 0) {
		hal_cpuHalt();
	}

	return fctrl2_common.bch_status;
}


int nandfctrl2_erase(nandfctrl2_dma_t *dma, u32 paddr)
{
	int chip = 0, channel = 0;
	dma->first = NULL;
	dma->last = NULL;

	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_erase_block, chip, &paddr, 0, NULL, NULL);
	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_read_status, 0, NULL, 0, NULL, NULL);
	nanddrv_readcompare(dma, chip, 0x1, 0, -1);
	nanddrv_finish(dma);

	fctrl2_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (fctrl2_common.result > 0) {
		hal_cpuHalt();
	}

	return fctrl2_common.result;
}


int nandfctrl2_writeraw(nandfctrl2_dma_t *dma, u32 paddr, void *data, int sz)
{
	int chip = 0, channel = 0;
	char addr[5] = { 0 };
	hal_memcpy(addr + 2, &paddr, 3);

	dma->first = NULL;
	dma->last = NULL;

	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_program_page, chip, addr, sz, data, NULL);
	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_read_status, 0, NULL, 0, NULL, NULL);
	nanddrv_readcompare(dma, 0, 0x3, 0, -1);
	nanddrv_finish(dma);

	fctrl2_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (fctrl2_common.result > 0) {
		hal_cpuHalt();
	}

	return fctrl2_common.result;
}


int nandfctrl2_readraw(nandfctrl2_dma_t *dma, u32 paddr, void *data, int sz)
{
	int chip = 0, channel = 0;
	char addr[5] = { 0 };
	hal_memcpy(addr + 2, &paddr, 3);

	dma->first = NULL;
	dma->last = NULL;

	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_read_page, chip, addr, 0, NULL, NULL);
	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_readback(dma, chip, sz, data, NULL);
	nanddrv_disablebch(dma, chip);
	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_finish(dma);

	fctrl2_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (fctrl2_common.result > 0) {
		hal_cpuHalt();
	}

	return fctrl2_common.result;
}


/* valid addresses are only the beginning of erase block */
int nandfctrl2_isbad(nandfctrl2_dma_t *dma, u32 paddr)
{
	int chip = 0, channel = 0, isbad = 0;
	u8 *data = fctrl2_common.uncached_buf;
	char addr[5] = { 0 };
	hal_memcpy(addr + 2, &paddr, 3);

	assert((paddr % 64) == 0);

	dma->first = NULL;
	dma->last = NULL;

	/* read RAW first 16 bytes (metadata) to skip ECC checks */
	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_read_page, chip, addr, 0, NULL, NULL);
	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_readback(dma, chip, 16, data, NULL);
	nanddrv_disablebch(dma, chip);
	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_finish(dma);

	fctrl2_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (fctrl2_common.result > 0) {
		hal_cpuHalt();
	}

	isbad = fctrl2_common.result;
	if (isbad < 0) {
		isbad = 1; /* read error, assume badblock */
	}
	else {
		isbad = (data[0] == 0x00u) ? 1 : 0; /* badblock marker present */
	}

	return isbad;
}


int nandfctrl2_markbad(nandfctrl2_dma_t *dma, u32 paddr)
{
	int chip = 0, channel = 0;
	u8 *data = fctrl2_common.uncached_buf;
	const unsigned int metasz = 16;
	char addr[5] = { 0 };
	hal_memcpy(addr + 2, &paddr, 3);

	assert((paddr % 64) == 0);

	dma->first = NULL;
	dma->last = NULL;

	/* NOTE: writing without ECC */
	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_program_page, chip, addr, metasz, data, NULL);
	nandfctrl2_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_read_status, 0, NULL, 0, NULL, NULL);
	nanddrv_readcompare(dma, 0, 0x3, 0, -1);
	nanddrv_finish(dma);

	hal_memset(data, 0xff, fctrl2_common.info.writesz + fctrl2_common.info.metasz);
	hal_memset(data, 0x0, metasz);

	fctrl2_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (fctrl2_common.result > 0) {
		hal_cpuHalt();
	}

	return fctrl2_common.result;
}


static int readFlashId(u32 minor, flash_id_t *flashId)
{
	const nand_cmd_t *cmd = &commands[cmd_read_id];

	u32 dcmd = (cmd->cmd1 << DCMD_CMD1_SHFT) | (cmd->cmd2 << DCMD_CMD2_SHFT);
	dcmd |= cmd->skipCmd2 ? DCMD_SC2 : 0;
	dcmd |= DCMD_ED | DCMD_RD; /* Disable EDAC & randomization */
	dcmd |= DCMD_EN;           /* Enable descriptor */

	setupRoutingReg(minor);

	fctrl2_common.regs->drow = 0U;
	fctrl2_common.regs->dcolsize = (5U << DCOLSIZE_SIZE_SHFT);
	fctrl2_common.regs->ddpl = (u32)((addr_t)flashId & 0xffffffffU);
	fctrl2_common.regs->dcmd = dcmd;

	/* Trigger descriptor */
	fctrl2_common.regs->ctrl2 = CTRL2_DT;

	nandfctrl2_wait4DescriptorDone();

	u32 status = fctrl2_common.regs->sts1;

	/* Clear status */
	fctrl2_common.regs->sts1 = status;

	return status;
}


static int readOnfiParameter_Page(u32 minor, onfi_paramPage_t *page)
{
	const nand_cmd_t *cmd = &commands[cmd_read_parameter_page];

	setupRoutingReg(minor);

	fctrl2_common.regs->ddpl = (u32)((addr_t)page & 0xffffffffU);

	fctrl2_common.regs->drow = 0U;

	fctrl2_common.regs->dcolsize = (256U << DCOLSIZE_SIZE_SHFT);

	u32 dcmd = (cmd->cmd1 << DCMD_CMD1_SHFT) | (cmd->cmd2 << DCMD_CMD2_SHFT);
	dcmd |= cmd->skipCmd2 ? DCMD_SC2 : 0;
	dcmd |= DCMD_ED | DCMD_RD; /* Disable EDAC & randomization */
	dcmd |= DCMD_EN;           /* Enable descriptor */

	fctrl2_common.regs->dcmd = dcmd;

	/* Trigger descriptor */
	fctrl2_common.regs->ctrl2 = CTRL2_DT;

	nandfctrl2_wait4DescriptorDone();

	return 0;
}


static int nandfctrl2_issueCmdReg(u32 minor, u32 command, u8 *rowAddr, u8 *colAddr, void *data, u16 dataSize, u32 dcmdFlags)
{
	u32 dcmd;
	const nand_cmd_t *cmd = &commands[command];

	setupRoutingReg(minor);

	fctrl2_common.regs->ddpl = (u32)((addr_t)page & 0xffffffffU);

	fctrl2_common.regs->drow = 0U;

	fctrl2_common.regs->dcolsize = (dataSize << DCOLSIZE_SIZE_SHFT);

	dcmd = (cmd->cmd1 << DCMD_CMD1_SHFT) | (cmd->cmd2 << DCMD_CMD2_SHFT);
	dcmd |= (cmd->skipCmd2 != 0) ? DCMD_SC2 : 0;
	dcmd |= DCMD_ED | DCMD_RD; /* Disable EDAC & randomization */
	dcmd |= DCMD_EN;           /* Enable descriptor */

	fctrl2_common.regs->dcmd = dcmd;

	/* Trigger descriptor */
	fctrl2_common.regs->ctrl2 = CTRL2_DT;
}


static u32 ns2cycles(u32 ns, u32 coreClk)
{
	return (u32)(((u64)ns * coreClk + 999999999ULL) / 1000000000ULL);
}


static void setOnfiTimingMode(unsigned int mode, const onfi_paramPage_t *paramPage, u32 coreClk, bool edoEn)
{
	const onfi_timingMode_t *timings = onfi_getTimingModeSDR(mode);

	/* Most timings are programmed as (number of cycles - 1) */
	u32 tCS = (ns2cycles(timings->tCS3, coreClk) - 1) & 0xffffU;
	u32 tWW = (ns2cycles(timings->tWW, coreClk) - 1) & 0xffffU;

	fctrl2_common.regs->tme0 = (tCS << 16) | tWW;

	u32 tRR = (ns2cycles(timings->tRR, coreClk) - 1) & 0xffffU;
	u32 tWB = (ns2cycles(timings->tWB, coreClk) - 1) & 0xffffU;

	fctrl2_common.regs->tme1 = (tRR << 16) | tWB;

	u32 tRHW = (ns2cycles(timings->tRHW, coreClk) - 1) & 0xffffU;
	u32 tWHR = (ns2cycles(timings->tWHR, coreClk) - 1) & 0xffffU;

	fctrl2_common.regs->tme2 = (tRHW << 16) | tWHR;

	u32 tCCS, tADL;

	if (paramPage == NULL) {
		tCCS = (ns2cycles(ONFI_TCCS_BASE, coreClk) - 1) & 0xffffU;
		tADL = (ns2cycles(timings->tADL, coreClk) - 1) & 0xffffU;
	}
	else {
		tCCS = (ns2cycles(paramPage->tCCS, coreClk) - 1) & 0xffffU;
		tADL = (ns2cycles(paramPage->tADL, coreClk) - 1) & 0xffffU;
	}

	fctrl2_common.regs->tme3 = (tADL << 16) | tCCS;

	u32 tREH = ns2cycles(timings->tREH, coreClk) & 0xffffU;
	u32 tRP = edoEn ? (ns2cycles(timings->tRP, coreClk) & 0xffffU) : (ns2cycles(timings->tREA, coreClk) & 0xffffU);
	u32 tRC = ns2cycles(timings->tRC, coreClk);

	/* Ensure sum meets tRC */
	if ((tREH + tRP) < tRC) {
		tREH += (tRC - (tREH + tRP));
	}

	fctrl2_common.regs->tme4 = ((tREH - 1) << 16) | (tRP - 1);

	u32 tWH = ns2cycles(timings->tWH, coreClk) & 0xffffU;
	u32 tWP = ns2cycles(timings->tWP, coreClk) & 0xffffU;
	u32 tWC = ns2cycles(timings->tWC, coreClk);

	/* Ensure sum meets tWC */
	if ((tWH + tWP) < tWC) {
		tWH += (tWC - (tWH + tWP));
	}

	fctrl2_common.regs->tme5 = ((tWH - 1) << 16) | (tWP - 1);

	/* tme6-tme10 not used in SDR mode */
	fctrl2_common.regs->tme6 = 0;
	fctrl2_common.regs->tme7 = 0;
	fctrl2_common.regs->tme8 = 0;
	fctrl2_common.regs->tme9 = 0;
	fctrl2_common.regs->tme10 = 0;

	u32 tVDLY = (ns2cycles(ONFI_TVDLY, coreClk) - 1) & 0xffffU;

	fctrl2_common.regs->tme11 = (tCS << 16) | tVDLY;
}


static void nandfctrl2_reset(void)
{
	fctrl2_common.regs->ctrl2 = CTRL2_ABORT;

	while (((fctrl2_common.regs->sts1 & STS1_ABORT) == 0) || ((fctrl2_common.regs->sts0 & STS0_RDY) == 0)) {
		/* Wait until ready */
	}

	fctrl2_common.regs->ctrl2 = CTRL2_RST;

	fctrl2_common.regs->ctrl0 = CTRL0_BBM;
	/* Disable all interrupts */
	fctrl2_common.regs->ctrl1 = 0U;
	/* Disable write protection */
	fctrl2_common.regs->ctrl3 = 0U;
	/* Set SEFI low */
	fctrl2_common.regs->ctrl4 = 0U;

	setOnfiTimingMode(0, NULL, SYSCLK_FREQ, false);

	/* Disable timeout */
	fctrl2_common.regs->tout0 = 0U;
}


static void nandfctrl2_setupFlashInfo(u32 minor)
{
	flash_id_t *flashId = (flash_id_t *)fctrl2_common.dmaDataBuf;

	hal_memset(flashId, 0, sizeof(*flashId));

	readFlashId(minor, flashId);

	if ((flashId->manufacturerid == 0x2cU) && (flashId->deviceid == 0xacU)) {
		/* Micron MT29F4G08ABBFA */
	}
}


void nandfctrl2_init(unsigned int minor)
{
	if (!fctrl2_common.initialized) {
		fctrl2_common.regs = (nandfctrl2_regs_t *)NANDFCTRL2_BASE;

		nandfctrl2_reset();

		fctrl2_common.initialized = true;
	}
}


const nandfctrl2_info_t *nandfctrl2_info(void)
{
	return &fctrl2_common.info;
}
