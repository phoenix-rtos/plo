/*
 * Phoenix-RTOS
 *
 * IMX6ULL NAND flash driver.
 *
 * Copyright 2018, 2023 Phoenix Systems
 * Author: Jan Sikorski, Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/lib.h>

#include "drv.h"


/* linker symbols */
extern u8 nand_dma[];  /* NAND DMA chain buffer */
extern u8 nand_buff[]; /* NAND page buffer */


/* clang-format off */
#ifdef DEBUG
#define assert(X) do {if((X)==0){log_error("nand-imx6ull: ASSERT FAILED");}} while(0)
#else
#define assert(X)
#endif

enum {
	apbh_ctrl0 = 0, apbh_ctrl0_set, apbh_ctrl0_clr, apbh_ctrl0_tog,
	apbh_ctrl1, apbh_ctrl1_set, apbh_ctrl1_clr, apbh_ctrl1_tog,
	apbh_ctrl2, apbh_ctrl2_set, apbh_ctrl2_clr, apbh_ctrl2_tog,
	apbh_channel_ctrl, apbh_channel_ctrl_set, apbh_channel_ctrl_clr, apbh_channel_ctrl_tog, apbh_devsel,

	apbh_ch0_curcmdar = 64, apbh_ch0_nxtcmdar = 68, apbh_ch0_cmd = 72, apbh_ch0_bar = 76,
	apbh_ch0_sema = 80, apbh_ch0_debug1 = 84, apbh_ch0_debug2 = 88,

	apbh_version = 512,
	apbh_next_channel = 92,
};


enum {
	dma_noxfer = 0,	dma_write = 1, dma_read = 2, dma_sense = 3,

	dma_chain = 1 << 2,
	dma_irqcomp = 1 << 3,
	dma_nandlock = 1 << 4,
	dma_w4ready = 1 << 5,
	dma_decrsema = 1 << 6,
	dma_w4endcmd = 1 << 7,
	dma_hot = 1 << 8,
};


typedef struct {
	u32 next;
	u16 flags;
	u16 bufsz;
	u32 buffer;
	u32 pio[];
} dma_t;


enum { bch_ctrl = 0, bch_ctrl_set, bch_ctrl_clr, bch_ctrl_tog, bch_status0,
	bch_status0_set, bch_status0_clr, bch_status0_tog, bch_mode, bch_mode_set,
	bch_mode_clr, bch_mode_tog, bch_encodeptr, bch_encodeptr_set,
	bch_encodeptr_clr, bch_encodeptr_tog, bch_dataptr, bch_dataptr_set,
	bch_dataptr_clr, bch_dataptr_tog, bch_metaptr, bch_metaptr_set,
	bch_metaptr_clr, bch_metaptr_tog, bch_layoutselect = 28,
	bch_layoutselect_set, bch_layoutselect_clr, bch_layoutselect_tog,
	bch_flash0layout0, bch_flash0layout0_set, bch_flash0layout0_clr,
	bch_flash0layout0_tog, bch_flash0layout1, bch_flash0layout1_set,
	bch_flash0layout1_clr, bch_flash0layout1_tog, bch_flash1layout0,
	bch_flash1layout0_set, bch_flash1layout0_clr, bch_flash1layout0_tog,
	bch_flash1layout1, bch_flash1layout1_set, bch_flash1layout1_clr,
	bch_flash1layout1_tog, bch_flash2layout0, bch_flash2layout0_set,
	bch_flash2layout0_clr, bch_flash2layout0_tog, bch_flash2layout1,
	bch_flash2layout1_set, bch_flash2layout1_clr, bch_flash2layout1_tog,
	bch_flash3layout0, bch_flash3layout0_set, bch_flash3layout0_clr,
	bch_flash3layout0_tog, bch_flash3layout1, bch_flash3layout1_set,
	bch_flash3layout1_clr, bch_flash3layout1_tog, bch_debug0, bch_debug0_set,
	bch_debug0_clr, bch_debug0_tog, bch_dbgkesread, bch_dbgkesread_set,
	bch_dbgkesread_clr, bch_dbgkesread_tog, bch_dbgcsferead, bch_dbgcsferead_set,
	bch_dbgcsferead_clr, bch_dbgcsferead_tog, bch_dbgsyndgenread,
	bch_dbgsyndgenread_set, bch_dbgsyndgenread_clr, bch_dbgsyndgenread_tog,
	bch_dbgahbmread, bch_dbgahbmread_set, bch_dbgahbmread_clr,
	bch_dbgahbmread_tog, bch_blockname, bch_blockname_set, bch_blockname_clr,
	bch_blockname_tog, bch_version, bch_version_set, bch_version_clr,
	bch_version_tog, bch_debug1, bch_debug1_set, bch_debug1_clr, bch_debug1_tog };


enum {
	gpmi_ctrl0 = 0, gpmi_ctrl0_set, gpmi_ctrl0_clr, gpmi_ctrl0_tog, gpmi_compare,
	gpmi_eccctrl = gpmi_compare + 4, gpmi_eccctrl_set, gpmi_eccctrl_clr, gpmi_eccctrl_tog,
	gpmi_ecccount, gpmi_payload = gpmi_ecccount + 4,

	gpmi_auxiliary = gpmi_payload + 4, gpmi_ctrl1 = gpmi_auxiliary + 4, gpmi_ctrl1_set,
	gpmi_ctrl1_clr, gpmi_ctrl1_tog, gpmi_timing0, gpmi_timing1 = gpmi_timing0 + 4,
	gpmi_timing2 = gpmi_timing1 + 4, gpmi_data = gpmi_timing2 + 4, gpmi_stat = gpmi_data + 4,
	gpmi_debug = gpmi_stat + 4, gpmi_version = gpmi_debug + 4, gpmi_debug2 = gpmi_version + 4,
	gpmi_debug3 = gpmi_debug2 + 4, gpmi_read_ddr_dll_ctrl = gpmi_debug3 + 4,
	gpmi_write_ddr_dll_ctrl = gpmi_read_ddr_dll_ctrl + 4,
	gpmi_read_ddr_dll_sts = gpmi_write_ddr_dll_ctrl + 4,
	gpmi_write_ddr_dll_sts = gpmi_read_ddr_dll_sts + 4,
};


enum {
	gpmi_address_increment = 1u << 16,
	gpmi_data_bytes = 0, gpmi_command_bytes = 1u << 17, gpmi_address_bytes = 2u << 17,
	gpmi_chip = 1u << 20,
	gpmi_8bit = 1u << 23,
	gpmi_write = 0, gpmi_read = 1u << 24, gpmi_read_compare = 2u << 24, gpmi_wait_for_ready = 3u << 24,
	gpmi_lock_cs = 1u << 27,
};
/* clang-format on */


typedef struct {
	dma_t dma;
	u32 ctrl0;
} gpmi_dma1_t;


typedef struct {
	dma_t dma;
	u32 ctrl0;
	u32 compare;
	u32 eccctrl;
} gpmi_dma3_t;


typedef struct {
	dma_t dma;
	u32 ctrl0;
	u32 compare;
	u32 eccctrl;
	u32 ecccount;
	u32 payload;
	u32 auxiliary;
} gpmi_dma6_t;


typedef struct {
	char cmd1;
	char addrsz;
	signed char data;
	char cmd2;
} nanddrv_command_t;


static const nanddrv_command_t commands[flash_num_commands] = {
	{ 0xff, 0, 0, 0x00 },  /* reset */
	{ 0x90, 1, 0, 0x00 },  /* read_id */
	{ 0xec, 1, 0, 0x00 },  /* read_parameter_page */
	{ 0xed, 1, 0, 0x00 },  /* read_unique_id */
	{ 0xee, 1, 0, 0x00 },  /* get_features */
	{ 0xef, 1, 4, 0x00 },  /* set_features */
	{ 0x70, 0, 0, 0x00 },  /* read_status */
	{ 0x78, 3, 0, 0x00 },  /* read_status_enhanced */
	{ 0x05, 2, 0, 0xe0 },  /* random_data_read */
	{ 0x06, 5, 0, 0xe0 },  /* random_data_read_two_plane */
	{ 0x85, 2, -2, 0x00 }, /* random_data_input */
	{ 0x85, 5, -2, 0x00 }, /* program_for_internal_data_move_column */
	{ 0x00, 0, 0, 0x00 },  /* read_mode */
	{ 0x00, 5, 0, 0x30 },  /* read_page */
	{ 0x31, 0, 0, 0x00 },  /* read_page_cache_sequential */
	{ 0x00, 5, 0, 0x31 },  /* read_page_cache_random */
	{ 0x3f, 0, 0, 0x00 },  /* read_page_cache_last */
	{ 0x80, 5, -1, 0x10 }, /* program_page */
	{ 0x80, 5, -1, 0x15 }, /* program_page_cache */
	{ 0x60, 3, 0, 0xd0 },  /* erase_block */
	{ 0x00, 5, 0, 0x35 },  /* read_for_internal_data_move */
	{ 0x85, 5, -2, 0x10 }, /* program_for_internal_data_move */
	{ 0x23, 3, 0, 0x00 },  /* block_unlock_low */
	{ 0x24, 3, 0, 0x00 },  /* block_unlock_high */
	{ 0x2a, 0, 0, 0x00 },  /* block_lock */
	{ 0x2c, 0, 0, 0x00 },  /* block_lock_tight */
	{ 0x7a, 3, 0, 0x00 },  /* block_lock_read_status */
	{ 0x80, 5, 0, 0x10 },  /* otp_data_lock_by_block */
	{ 0x80, 5, -1, 0x10 }, /* otp_data_program */
	{ 0x00, 5, 0, 0x30 },  /* otp_data_read */
};


typedef struct _nanddrv_dma_t {
	dma_t *last;
	dma_t *first;
	char buffer[];
} nanddrv_dma_t;


typedef struct {
	u8 manufacturerid;
	u8 deviceid;
	u8 bytes[3];
} __attribute__((packed)) flash_id_t;


struct {
	volatile u32 *gpmi;
	volatile u32 *bch;
	volatile u32 *dma;
	volatile u32 *mux;

	unsigned rawmetasz; /* user metadata + ECC16 metadata bytes */

	volatile int result, bch_status, bch_done;

	u8 *uncached_buf;
	nanddrv_info_t info;
} nanddrv_common;


static inline int dma_pio(unsigned pio)
{
	return (pio & 0xfu) << 12;
}


static inline int dma_size(dma_t *dma)
{
	return sizeof(dma_t) + ((dma->flags >> 12) & 0xfu) * sizeof(u32);
}


static int dma_terminate(dma_t *dma, int err)
{
	hal_memset(dma, 0, sizeof(*dma));

	dma->flags = dma_irqcomp | dma_decrsema | dma_noxfer;
	dma->buffer = (u32)err;

	return sizeof(*dma);
}


static int dma_check(dma_t *dma, dma_t *fail)
{
	hal_memset(dma, 0, sizeof(*dma));

	dma->flags = dma_hot | dma_sense;
	dma->buffer = (u32)fail;

	return sizeof(*dma);
}


static void dma_sequence(dma_t *prev, dma_t *next)
{
	if (prev != NULL) {
		prev->flags |= dma_chain;
		prev->next = (u32)next;
	}
}


static void dma_run(dma_t *dma, int channel)
{
	*(nanddrv_common.dma + apbh_ch0_nxtcmdar + (channel * apbh_next_channel)) = (u32)dma;
	*(nanddrv_common.dma + apbh_ch0_sema + (channel * apbh_next_channel)) = 1;
}


static int dma_irqHandler(unsigned int n, void *data)
{
	int comp, err;

	/* Check interrupt flags */
	comp = *(nanddrv_common.dma + apbh_ctrl1) & (1u << 0);
	err = *(nanddrv_common.dma + apbh_ctrl2) & ((1u << 16) | (1u << 0));

	/* Transform error status: 0: no error, 1: DMA termination, 2: AHB bus error */
	/* (DMA termination with completion flag is not treated as error) */
	err = (err >> 16) + (err & 1);
	err -= err & comp;

	/* Clear interrupt flags */
	*(nanddrv_common.dma + apbh_ctrl1_clr) = 1;
	*(nanddrv_common.dma + apbh_ctrl2_clr) = 1;

	/* If no error was detected return value set by DMA terminator descriptor (dma->buffer field prepared in dma_terminate()) */
	nanddrv_common.result = (err == 0) ? *(nanddrv_common.dma + apbh_ch0_bar) : -err;

	return 1;
}


static int bch_irqHandler(unsigned int n, void *data)
{
	/* Clear interrupt flags */
	nanddrv_common.bch_status = *(nanddrv_common.bch + bch_status0);
	nanddrv_common.bch_done = 1;
	*(nanddrv_common.bch + bch_ctrl_clr) = 1;
	return 1;
}


static int gpmi_irqHandler(unsigned int n, void *data)
{
	return 1;
}


static int nand_cmdaddr(gpmi_dma3_t *cmd, int chip, void *buffer, u16 addrsz)
{
	hal_memset(cmd, 0, sizeof(*cmd));

	cmd->dma.flags = dma_hot | dma_w4endcmd | dma_nandlock | dma_read | dma_pio(3);
	cmd->dma.bufsz = (addrsz & 0x7) + 1;
	cmd->dma.buffer = (u32)buffer;

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
	cmd->dma.buffer = (u32)buffer;

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
	if (payload != NULL) {
		cmd->payload = (u32)payload;
	}
	cmd->auxiliary = (u32)auxiliary;

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
	cmd->dma.buffer = (u32)buffer;

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
		cmd->payload = (u32)payload;
	}
	cmd->auxiliary = (u32)auxiliary;

	return sizeof(*cmd);
}


static int nand_w4ready(gpmi_dma1_t *cmd, int chip)
{
	hal_memset(cmd, 0, sizeof(*cmd));

	cmd->dma.flags = dma_hot | dma_w4endcmd | dma_w4ready | dma_noxfer | dma_pio(1);
	cmd->ctrl0 = (chip * gpmi_chip) | gpmi_wait_for_ready | gpmi_8bit;

	return sizeof(*cmd);
}


nanddrv_dma_t *nanddrv_dma(void)
{
	nanddrv_dma_t *dma = (nanddrv_dma_t *)nand_dma;
	dma->last = NULL;
	dma->first = NULL;

	return dma;
}


int nanddrv_wait4ready(nanddrv_dma_t *dma, int chip, int err)
{
	void *next = dma->last, *prev = dma->last;
	int sz;
	dma_t *terminator;

	if (next != NULL) {
		next += dma_size(dma->last);
	}
	else {
		next = dma->buffer;
	}

	terminator = next;

	if (err != 0) {
		sz = dma_terminate(terminator, err);
		next += sz;
	}

	sz = nand_w4ready(next, chip);
	dma_sequence(prev, next);
	dma->last = next;
	next += sz;

	if (dma->first == NULL) {
		dma->first = dma->last;
	}

	sz = dma_check(next, terminator);
	dma_sequence(dma->last, next);
	dma->last = next;

	return 0;
}


int nanddrv_disablebch(nanddrv_dma_t *dma, int chip)
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


int nanddrv_finish(nanddrv_dma_t *dma)
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

int nanddrv_issue(nanddrv_dma_t *dma, int c, int chip, void *addr, unsigned datasz, void *data, void *aux)
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


int nanddrv_readback(nanddrv_dma_t *dma, int chip, int bufsz, void *buf, void *aux)
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


int nanddrv_readcompare(nanddrv_dma_t *dma, int chip, u16 mask, u16 value, int err)
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


int nanddrv_reset(nanddrv_dma_t *dma)
{
	int chip = 0, channel = 0;
	dma->first = NULL;
	dma->last = NULL;

	nanddrv_issue(dma, flash_reset, chip, NULL, 0, NULL, NULL);
	nanddrv_finish(dma);

	nanddrv_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (nanddrv_common.result > 0) {
		hal_cpuHalt();
	}

	return nanddrv_common.result;
}


int nanddrv_readid(nanddrv_dma_t *dma, flash_id_t *flash_id)
{
	int chip = 0, channel = 0;
	char addr[1] = { 0 };

	dma->first = NULL;
	dma->last = NULL;

	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_read_id, chip, addr, 0, NULL, NULL);
	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_readback(dma, chip, 5, flash_id, NULL);
	nanddrv_disablebch(dma, chip);
	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_finish(dma);

	nanddrv_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (nanddrv_common.result > 0) {
		hal_cpuHalt();
	}

	return nanddrv_common.result;
}


int nanddrv_write(nanddrv_dma_t *dma, u32 paddr, void *data, char *aux)
{
	int chip = 0, channel = 0, sz;
	char addr[5] = { 0 };
	int skipMeta = 0, err;
	hal_memcpy(addr + 2, &paddr, 3);

	if (data == NULL) {
		sz = nanddrv_common.rawmetasz;
	}
	else {
		sz = nanddrv_common.info.writesz + nanddrv_common.info.metasz;
		if (aux == NULL) {
			aux = (char *)nanddrv_common.uncached_buf;
			skipMeta = 1;
		}
	}

	dma->first = NULL;
	dma->last = NULL;

	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_program_page, chip, addr, sz, data, aux);
	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_read_status, 0, NULL, 0, NULL, NULL);
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
		hal_memset(aux, 0xff, nanddrv_common.rawmetasz);

		/* Treat metadata and its ECC as raw byte area without ECC */
		*(nanddrv_common.bch + bch_flash0layout0) &= ~(0x1fffu << 11);
		*(nanddrv_common.bch + bch_flash0layout0) |= nanddrv_common.rawmetasz << 16;
	}

	nanddrv_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (nanddrv_common.result > 0) {
		hal_cpuHalt();
	}

	err = nanddrv_common.result;

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


int nanddrv_read(nanddrv_dma_t *dma, u32 paddr, void *data, nanddrv_meta_t *aux)
{
	int chip = 0, channel = 0, sz = 0;
	char addr[5] = { 0 };
	hal_memcpy(addr + 2, &paddr, 3);

	if (data != NULL) {
		sz = nanddrv_common.info.writesz + nanddrv_common.info.metasz;
	}
	else {
		sz = nanddrv_common.rawmetasz;
	}

	dma->first = NULL;
	dma->last = NULL;

	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_read_page, chip, addr, 0, NULL, NULL);
	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_readback(dma, chip, sz, data, aux);
	nanddrv_disablebch(dma, chip);
	nanddrv_finish(dma);

	nanddrv_common.result = 1;
	nanddrv_common.bch_done = 0;
	dma_run((dma_t *)dma->first, channel);

	while (nanddrv_common.bch_done == 0) {
		hal_cpuHalt();
	}

	while (nanddrv_common.result > 0) {
		hal_cpuHalt();
	}

	return nanddrv_common.bch_status;
}


int nanddrv_erase(nanddrv_dma_t *dma, u32 paddr)
{
	int chip = 0, channel = 0;
	dma->first = NULL;
	dma->last = NULL;

	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_erase_block, chip, &paddr, 0, NULL, NULL);
	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_read_status, 0, NULL, 0, NULL, NULL);
	nanddrv_readcompare(dma, chip, 0x1, 0, -1);
	nanddrv_finish(dma);

	nanddrv_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (nanddrv_common.result > 0) {
		hal_cpuHalt();
	}

	return nanddrv_common.result;
}


int nanddrv_writeraw(nanddrv_dma_t *dma, u32 paddr, void *data, int sz)
{
	int chip = 0, channel = 0;
	char addr[5] = { 0 };
	hal_memcpy(addr + 2, &paddr, 3);

	dma->first = NULL;
	dma->last = NULL;

	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_program_page, chip, addr, sz, data, NULL);
	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_read_status, 0, NULL, 0, NULL, NULL);
	nanddrv_readcompare(dma, 0, 0x3, 0, -1);
	nanddrv_finish(dma);

	nanddrv_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (nanddrv_common.result > 0) {
		hal_cpuHalt();
	}

	return nanddrv_common.result;
}


int nanddrv_readraw(nanddrv_dma_t *dma, u32 paddr, void *data, int sz)
{
	int chip = 0, channel = 0;
	char addr[5] = { 0 };
	hal_memcpy(addr + 2, &paddr, 3);

	dma->first = NULL;
	dma->last = NULL;

	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_read_page, chip, addr, 0, NULL, NULL);
	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_readback(dma, chip, sz, data, NULL);
	nanddrv_disablebch(dma, chip);
	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_finish(dma);

	nanddrv_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (nanddrv_common.result > 0) {
		hal_cpuHalt();
	}

	return nanddrv_common.result;
}


/* valid addresses are only the beginning of erase block */
int nanddrv_isbad(nanddrv_dma_t *dma, u32 paddr)
{
	int chip = 0, channel = 0, isbad = 0;
	u8 *data = nanddrv_common.uncached_buf;
	char addr[5] = { 0 };
	hal_memcpy(addr + 2, &paddr, 3);

	assert((paddr % 64) == 0);

	dma->first = NULL;
	dma->last = NULL;

	/* read RAW first 16 bytes (metadata) to skip ECC checks */
	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_read_page, chip, addr, 0, NULL, NULL);
	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_readback(dma, chip, 16, data, NULL);
	nanddrv_disablebch(dma, chip);
	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_finish(dma);

	nanddrv_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (nanddrv_common.result > 0) {
		hal_cpuHalt();
	}

	isbad = nanddrv_common.result;
	if (isbad < 0) {
		isbad = 1; /* read error, assume badblock */
	}
	else {
		isbad = (data[0] == 0x00u) ? 1 : 0; /* badblock marker present */
	}

	return isbad;
}


int nanddrv_markbad(nanddrv_dma_t *dma, u32 paddr)
{
	int chip = 0, channel = 0;
	u8 *data = nanddrv_common.uncached_buf;
	const unsigned int metasz = 16;
	char addr[5] = { 0 };
	hal_memcpy(addr + 2, &paddr, 3);

	assert((paddr % 64) == 0);

	dma->first = NULL;
	dma->last = NULL;

	/* NOTE: writing without ECC */
	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_program_page, chip, addr, metasz, data, NULL);
	nanddrv_wait4ready(dma, chip, 0);
	nanddrv_issue(dma, flash_read_status, 0, NULL, 0, NULL, NULL);
	nanddrv_readcompare(dma, 0, 0x3, 0, -1);
	nanddrv_finish(dma);

	hal_memset(data, 0xff, nanddrv_common.info.writesz + nanddrv_common.info.metasz);
	hal_memset(data, 0x0, metasz);

	nanddrv_common.result = 1;
	dma_run((dma_t *)dma->first, channel);

	while (nanddrv_common.result > 0) {
		hal_cpuHalt();
	}

	return nanddrv_common.result;
}


static void nanddrv_setupFlashInfo(void)
{
	flash_id_t *flash_id = (flash_id_t *)nanddrv_common.uncached_buf;
	nanddrv_dma_t *dma = nanddrv_dma();

	hal_memset(flash_id, 0, sizeof(*flash_id));

	nanddrv_reset(dma);
	nanddrv_readid(dma, flash_id);

	if ((flash_id->manufacturerid == 0x98) && (flash_id->deviceid == 0xd3)) {
		nanddrv_common.info.name = "Kioxia TH58NV 16Gbit NAND";
		/* FIXME: The 2nd GB is on a separate die controlled by a separate chip select signal */
		/* Limit available chip size to 4096 blocks (1GB) until multiple chip selects support is implemented in the driver */
		nanddrv_common.info.size = 4096ull * 64ull * 4096ull;
		nanddrv_common.info.writesz = 4096u;
		nanddrv_common.info.metasz = 256u;
		nanddrv_common.info.erasesz = 4096u * 64u;
		nanddrv_common.info.oobsz = 16u;
		nanddrv_common.info.oobavail = 16u;
	}
	else if ((flash_id->manufacturerid == 0x2c) && (flash_id->deviceid == 0xd3)) {
		nanddrv_common.info.name = "Micron MT29F8G 8Gbit NAND";
		nanddrv_common.info.size = 4096ull * 64ull * 4096ull;
		nanddrv_common.info.writesz = 4096u;
		nanddrv_common.info.metasz = 224u;
		nanddrv_common.info.erasesz = 4096u * 64u;
		nanddrv_common.info.oobsz = 16u;
		nanddrv_common.info.oobavail = 16u;
	}
	else {
		/* use sane defaults */
		nanddrv_common.info.name = "Unknown 8Gbit NAND";
		nanddrv_common.info.size = 4096ull * 64ull * 4096ull;
		nanddrv_common.info.writesz = 4096u;
		nanddrv_common.info.metasz = 224u;
		nanddrv_common.info.erasesz = 4096u * 64u;
		nanddrv_common.info.oobsz = 16u;
		nanddrv_common.info.oobavail = 16u;
	}
}


void nanddrv_init(void)
{
	nanddrv_common.dma = (u32 *)0x1804000;
	nanddrv_common.gpmi = (u32 *)0x1806000;
	nanddrv_common.bch = (u32 *)0x1808000;
	nanddrv_common.mux = (u32 *)0x20e0000;
	nanddrv_common.uncached_buf = nand_buff;

	/* 16 bytes of user metadada + ECC16 * bits per parity level (13) / 8 = 26 bytes for ECC  */
	nanddrv_common.rawmetasz = 16u + 26u;

	imx6ull_setDevClock(clk_apbhdma, 3);
	imx6ull_setDevClock(clk_rawnand_u_gpmi_input_apb, 3);
	imx6ull_setDevClock(clk_rawnand_u_gpmi_bch_input_gpmi_io, 3);
	imx6ull_setDevClock(clk_rawnand_u_gpmi_bch_input_bch, 3);
	imx6ull_setDevClock(clk_rawnand_u_bch_input_apb, 3);

	imx6ull_setDevClock(clk_iomuxc, 3);

	*(nanddrv_common.dma + apbh_ctrl0) &= ~(1u << 31 | 1u << 30);
	*(nanddrv_common.gpmi + gpmi_ctrl0) &= ~(1u << 31 | 1u << 30);

	*(nanddrv_common.bch + bch_ctrl_clr) = (1u << 31);
	*(nanddrv_common.bch + bch_ctrl_clr) = (1u << 30);

	*(nanddrv_common.bch + bch_ctrl_set) = (1u << 31);
	while ((*(nanddrv_common.bch + bch_ctrl) & (1u << 30)) == 0) {
		;
	}

	*(nanddrv_common.bch + bch_ctrl_clr) = (1u << 31);
	*(nanddrv_common.bch + bch_ctrl_clr) = (1u << 30);

	/* GPMI clock = 198 MHz (~5ns period), address setup = 3 (~15ns), data hold = 2 (~10ns), data setup = 3 (~15ns) */
	*(nanddrv_common.gpmi + gpmi_timing0) = (3u << 16) | (2u << 8) | (3u << 0);
	/* Set wait for ready timeout */
	*(nanddrv_common.gpmi + gpmi_timing1) = 0xffffu << 16;

	/* enable irq on channel 0 */
	*(nanddrv_common.dma + apbh_ctrl1) |= 1u << 16;

	/* flush irq flag */
	if ((*(nanddrv_common.dma + apbh_ctrl1) & 1) != 0) {
		*(nanddrv_common.dma + apbh_ctrl1_clr) = 1;
	}

	for (int i = 0; i < 17; ++i) {
		/* set all NAND pins to NAND function */
		*(nanddrv_common.mux + i + 94) = 0;
	}

	/* Set DECOUPLE_CS, WRN no delay, GANGED_RDYBUSY, BCH, RDN_DELAY, WP, #R/B busy-low */
	*(nanddrv_common.gpmi + gpmi_ctrl1) = (1u << 24) | (3u << 22) | (1u << 19) | (1u << 18) | (14u << 12) | (1u << 3) | (1u << 2);
	/* Enable DLL */
	*(nanddrv_common.gpmi + gpmi_ctrl1_set) = (1u << 17);

	hal_interruptsSet(32 + 13, dma_irqHandler, NULL);
	hal_interruptsSet(32 + 16, gpmi_irqHandler, NULL);

	/* read NAND configuration before BCH setup */
	nanddrv_setupFlashInfo();

	/* set BCH up */
	*(nanddrv_common.bch + bch_ctrl_set) = 1u << 8;
	*(nanddrv_common.bch + bch_layoutselect) = 0;

	/* 8 blocks/page, 16 bytes metadata, ECC16, GF13, 0 word data0 */
	*(nanddrv_common.bch + bch_flash0layout0) = (8u << 24) | (16u << 16) | (8u << 11) | (0u << 10) | 0u;

	/* flash layout takes only 4096 + 224 bytes, but it's universal across used NAND chips */
	/* 4096 + 256 page size, ECC14, GF13, 128 word dataN (512 bytes) */
	*(nanddrv_common.bch + bch_flash0layout1) = ((nanddrv_common.info.writesz + nanddrv_common.info.metasz) << 16) | (7u << 11) | (0u << 10) | 128u;

	hal_interruptsSet(32 + 15, bch_irqHandler, NULL);
}


const nanddrv_info_t *nanddrv_info(void)
{
	return &nanddrv_common.info;
}
