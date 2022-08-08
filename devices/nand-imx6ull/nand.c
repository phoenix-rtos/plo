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

#include "bch.h"
#include "dma.h"
#include "gpmi.h"
#include "nand.h"


/* Linker symbols */
extern u8 nand_dma[];  /* NAND DMA chain buffer */
extern u8 nand_buff[]; /* NAND page buffer */


/* NAND commands */
#define NAND_RESET                 0
#define NAND_READ_ID               1
#define NAND_READ_PARAM_PAGE       2
#define NAND_READ_UNIQUE_ID        3
#define NAND_GET_FEATURES          4
#define NAND_SET_FEATURES          5
#define NAND_STATUS                6
#define NAND_STATUS_EXT            7
#define NAND_RND_READ              8
#define NAND_RND_READ_2PLANE       9
#define NAND_RND_INPUT             10
#define NAND_PROG_DATA_MOVE_COLUMN 11
#define NAND_READ_MODE             12
#define NAND_READ_PAGE             13
#define NAND_READ_PAGE_CACHE_SEQ   14
#define NAND_READ_PAGE_CACHE_RND   15
#define NAND_READ_PAGE_CACHE_LAST  16
#define NAND_PROG_PAGE             17
#define NAND_PROG_PAGE_CACHE       18
#define NAND_ERASE_BLOCK           19
#define NAND_READ_DATA_MOVE        20
#define NAND_PROG_DATA_MOVE        21
#define NAND_BLOCK_UNLOCK_LOW      22
#define NAND_BLOCK_UNLOCK_HIGH     23
#define NAND_BLOCK_LOCK            24
#define NAND_BLOCK_LOCK_TIGHT      25
#define NAND_BLOCK_LOCK_STATUS     26
#define NAND_OTP_LOCK              27
#define NAND_OTP_PROG              28
#define NAND_OTP_READ              29

/* NAND command max address bytes */
#define NAND_MAX_ADDRSZ 5


static const struct {
	u8 cmd1;
	u8 cmd2;
	u16 addrsz;
} nand_cmd[] = {
	{ 0xff, 0x00, 0 }, /* reset */
	{ 0x90, 0x00, 1 }, /* read_id */
	{ 0xec, 0x00, 1 }, /* read_parameter_page */
	{ 0xed, 0x00, 1 }, /* read_unique_id */
	{ 0xee, 0x00, 1 }, /* get_features */
	{ 0xef, 0x00, 1 }, /* set_features */
	{ 0x70, 0x00, 0 }, /* read_status */
	{ 0x78, 0x00, 3 }, /* read_status_enhanced */
	{ 0x05, 0xe0, 2 }, /* random_data_read */
	{ 0x06, 0xe0, 5 }, /* random_data_read_two_plane */
	{ 0x85, 0x00, 2 }, /* random_data_input */
	{ 0x85, 0x00, 5 }, /* program_for_internal_data_move_column */
	{ 0x00, 0x00, 0 }, /* read_mode */
	{ 0x00, 0x30, 5 }, /* read_page */
	{ 0x31, 0x00, 0 }, /* read_page_cache_sequential */
	{ 0x00, 0x31, 5 }, /* read_page_cache_random */
	{ 0x3f, 0x00, 0 }, /* read_page_cache_last */
	{ 0x80, 0x10, 5 }, /* program_page */
	{ 0x80, 0x15, 5 }, /* program_page_cache */
	{ 0x60, 0xd0, 3 }, /* erase_block */
	{ 0x00, 0x35, 5 }, /* read_for_internal_data_move */
	{ 0x85, 0x10, 5 }, /* program_for_internal_data_move */
	{ 0x23, 0x00, 3 }, /* block_unlock_low */
	{ 0x24, 0x00, 3 }, /* block_unlock_high */
	{ 0x2a, 0x00, 0 }, /* block_lock */
	{ 0x2c, 0x00, 0 }, /* block_lock_tight */
	{ 0x7a, 0x00, 3 }, /* block_lock_read_status */
	{ 0x80, 0x10, 5 }, /* otp_data_lock_by_block */
	{ 0x80, 0x10, 5 }, /* otp_data_program */
	{ 0x00, 0x30, 5 }, /* otp_data_read */
};


/* NAND device ID */
typedef struct {
	u8 manid;   /* Manufacturer ID */
	u8 devid;   /* Device ID */
	u8 data[3]; /* Other bytes */
} __attribute__((packed)) nand_id_t;


/* NAND device data */
typedef struct {
	const nand_cfg_t *cfg; /* Device configuration */
	const bch_ecc_t *ecc;  /* Device BCH ECC configuration */
	unsigned int id;       /* Device ID (chip select line) */
} nand_dev_t;


/* Supported BCH ECC configurations */
static const bch_ecc_t nand_ecc[] = {
	{ 0, 512, 8, 16, 14, 13, 13 }, /* 4KB page with 16B metadata */
};


/* Supported NAND devices info */
static const struct {
	const char *name;     /* Device name */
	u8 manid;             /* Manufacturer ID */
	u8 devid;             /* Device ID */
	nand_cfg_t cfg;       /* Device configuration */
	const bch_ecc_t *ecc; /* Device BCH ECC configuration */
} nand_info[] = {
	{
		"Kioxia TH58NVG4", 0x98, 0xd3,
		{ (u64)8192 * 64 * 4096, 64 * 4096, 4096, 256, 16 },
		&nand_ecc[0],
	},
	{
		"Micron MT29F8G", 0x2c, 0xd3,
		{ (u64)4096 * 64 * 4096, 64 * 4096, 4096, 224, 16 },
		&nand_ecc[0],
	},
};


/* NAND mux configuration */
static const struct {
	int mux;
	char mode;
} nand_mux[] = {
	{ mux_nand_d0, 0 }, { mux_nand_d1, 0 }, { mux_nand_d2, 0} , { mux_nand_d3, 0},
	{ mux_nand_d4, 0 }, { mux_nand_d5, 0 }, { mux_nand_d6, 0 }, { mux_nand_d7, 0 },
	{ mux_nand_cle, 0 }, { mux_nand_ale, 0}, { mux_nand_we, 0 }, { mux_nand_wp, 0 },
	{ mux_nand_rdy, 0 }, { mux_nand_re, 0 }, { mux_nand_ce0, 0}, { mux_nand_ce1, 0 },
	{ mux_nand_dqs, 0},
};


static struct {
	nand_dev_t dev[NAND_MAX_CNT];
} nand_common;


static void nand_w4ready(dma_t *dma, unsigned int cs)
{
	u8 *next = (u8 *)dma->last;
	dma_desc_t *w4ready, *desc;

	if (dma->last != dma->first) {
		next += dma_descsz(dma->last);
	}

	w4ready = desc = (dma_desc_t *)next;
	next += gpmi_w4ready(desc, cs);
	dma_sequence(dma, desc);

	desc = (dma_desc_t *)next;
	next += dma_check(desc, w4ready);
	dma_sequence(dma, desc);
}


static void nand_issue(dma_t *dma, unsigned int cmd, unsigned int cs, const void *addr, const void *data, const void *aux, u16 size)
{
	u8 *cmdaddr, *next = (u8 *)dma->last;
	dma_desc_t *desc;

	if (dma->last != dma->first) {
		next += dma_descsz(dma->last);
	}

	cmdaddr = next;
	cmdaddr[0] = nand_cmd[cmd].cmd1;
	if (addr != NULL) {
		hal_memcpy(cmdaddr + 1, addr, nand_cmd[cmd].addrsz);
	}
	cmdaddr[7] = nand_cmd[cmd].cmd2;
	next += 8;

	desc = (dma_desc_t *)next;
	next += gpmi_cmdaddr(desc, cs, cmdaddr, nand_cmd[cmd].addrsz);
	dma_sequence(dma, desc);

	if (size > 0) {
		desc = (dma_desc_t *)next;
		next += (aux != NULL) ? gpmi_ecwrite(desc, cs, data, aux, size) : gpmi_write(desc, cs, data, size);
		dma_sequence(dma, desc);
	}

	if (nand_cmd[cmd].cmd2) {
		desc = (dma_desc_t *)next;
		next += gpmi_cmdaddr(desc, cs, cmdaddr + 7, 0);
		dma_sequence(dma, desc);
	}
}


static void nand_readback(dma_t *dma, unsigned int cs, void *data, void *aux, u16 size)
{
	u8 *next = (u8 *)dma->last;
	dma_desc_t *desc;

	if (dma->last != dma->first) {
		next += dma_descsz(dma->last);
	}

	desc = (dma_desc_t *)next;
	next += (aux != NULL) ? gpmi_ecread(desc, cs, data, aux, size) : gpmi_read(desc, cs, data, size);
	dma_sequence(dma, desc);

	if (aux != NULL) {
		desc = (dma_desc_t *)next;
		next += gpmi_disableBCH(desc, cs);
		dma_sequence(dma, desc);
	}
}


static void nand_readcmp(dma_t *dma, unsigned int cs, u16 mask, u16 val, int err)
{
	u8 *next = (u8 *)dma->last;
	dma_desc_t *desc, *term;

	if (dma->last != dma->first) {
		next += dma_descsz(dma->last);
	}

	term = (dma_desc_t *)next;
	next += dma_terminate(term, err);

	desc = (dma_desc_t *)next;
	next += gpmi_readcmp(desc, cs, mask, val);
	dma_sequence(dma, desc);

	desc = (dma_desc_t *)next;
	next += dma_check(desc, term);
	dma_sequence(dma, desc);
}


/* NAND interface */

const nand_t *nand_get(unsigned int cs)
{
	if (cs >= sizeof(nand_common.dev) / sizeof(nand_common.dev[0])) {
		return NULL;
	}

	return (nand_t *)&nand_common.dev[cs];
}


static unsigned int nand_checkErased(const void *buff, unsigned int boffs, unsigned int blen)
{
	const u32 *buff32 = buff;
	const u8 *buff8 = buff;
	unsigned int ret = 0;
	u32 data32;
	u8 data8;

	buff8 += boffs / 8;
	boffs %= 8;

	/* Check first byte */
	if (boffs > 0) {
		data8 = *buff8++;
		data8 |= (u8)(0xff << (8 - boffs));

		/* Is it also last byte? */
		if (boffs + blen < 8) {
			data8 |= (u8)(0xff >> (boffs + blen));
			blen = 0;
		}
		else {
			blen -= 8 - boffs;
		}
		ret += 8 - __builtin_popcount(data8);
	}

	/* Check bytes until 32-bit aligned address */
	while ((blen > 8) && ((addr_t)buff8 % sizeof(data32))) {
		data8 = *buff8++;
		blen -= 8;
		ret += 8 - __builtin_popcount(data8);
	}

	/* Check 32-bit words */
	buff32 = (const u32 *)buff8;
	while (blen > 8 * sizeof(data32)) {
		data32 = *buff32++;
		blen -= 8 * sizeof(data32);

		if (data32 == 0xffffffff) {
			continue;
		}
		ret += 8 * sizeof(data32) - __builtin_popcount(data32);
	}

	/* Check rest of the bytes */
	buff8 = (const u8 *)buff32;
	while (blen > 8) {
		data8 = *buff8++;
		blen -= 8;
		ret += 8 - __builtin_popcount(data8);
	}

	/* Check last byte */
	if (blen > 0) {
		data8 = *buff8;
		data8 |= (u8)(0xff >> blen);
		ret += 8 - __builtin_popcount(data8);
	}

	return ret;
}


static int nand_checkECC(const nand_t *nand, unsigned int page, u8 *data, u8 *aux, unsigned int chunks)
{
	const nand_dev_t *dev = (const nand_dev_t *)nand;
	const unsigned int mlen = 8 * dev->cfg->metasz + dev->ecc->ecc0 * dev->ecc->gf0;
	const unsigned int dlen = 8 * dev->ecc->blockszN + dev->ecc->eccN * dev->ecc->gfN;
	u8 *status = aux + (dev->cfg->metasz + sizeof(u32) - 1) / sizeof(u32), *raw = NULL;
	unsigned int i, boffs, blen, flips;
	int err;

	for (i = 0; i < chunks; i++) {
		/* BCH reported uncorrectable chunk */
		if (status[i] == 0xfe) {
			/* Check for an erased chunk with bitflips */
			/* Read raw page data and count bitflips manually */
			if (raw == NULL) {
				raw = nand_buff;
				err = nand_read(nand, page, raw, NULL, 1);
				if (err < 0) {
					return err;
				}
			}

			/* Metadata chunk */
			if (i == 0) {
				boffs = 0;
				blen = mlen;
			}
			/* Data chunk */
			else {
				boffs = mlen + (i - 1) * dlen;
				blen = dlen;
			}

			flips = nand_checkErased(raw, boffs, blen);
			if (flips == 0) {
				continue;
			}

			/* Handle metadata bitflips */
			if (i == 0) {
				/* Too many metadata bitflips */
				if (flips > dev->ecc->ecc0) {
					return -EFAULT;
				}

				/* Correct metadata chunk */
				hal_memset(aux, 0xff, dev->cfg->metasz);
			}
			/* Handle data chunk bitflips */
			else {
				if (flips > dev->ecc->eccN) {
					return -EFAULT;
				}

				/* Correct data chunk */
				hal_memset(data + (i - 1) * dev->ecc->blockszN, 0xff, dev->ecc->blockszN);
			}
		}
	}

	return EOK;
}


int nand_read(const nand_t *nand, unsigned int page, void *data, void *aux, int raw)
{
	const nand_dev_t *dev = (const nand_dev_t *)nand;
	dma_t *dma = (dma_t *)nand_dma;
	u8 addr[NAND_MAX_ADDRSZ];
	u16 size;
	int err;

	/* Prepare page address (skip 2 bytes for column address) */
	hal_memset(addr, 0, nand_cmd[NAND_READ_PAGE].addrsz);
	hal_memcpy(addr + 2, &page, sizeof(addr) - 2);

	/* Read whole page */
	if (data != NULL) {
		size = dev->cfg->pagesz + dev->cfg->oobsz;
		/* Read without ECC */
		if (raw != 0) {
			aux = NULL;
		}
	}
	/* Read only metadata */
	else {
		size = dev->cfg->metasz + bch_eccsz(dev->ecc->ecc0, dev->ecc->gf0);
		/* Read without ECC */
		if (raw != 0) {
			data = aux;
			aux = NULL;
		}
	}

	dma_reset(dma);
	nand_w4ready(dma, dev->id);
	nand_issue(dma, NAND_READ_PAGE, dev->id, addr, NULL, NULL, 0);
	nand_w4ready(dma, dev->id);
	nand_readback(dma, dev->id, data, aux, size);
	dma_finish(dma);

	err = dma_run(dma, 0);

	/* Read with ECC, wait for BCH to finish and check results */
	if ((err >= 0) && (raw == 0)) {
		bch_finish();
		err = nand_checkECC(nand, page, data, aux, (data != NULL) ? 1 + dev->ecc->nblocks : 1);
	}

	return err;
}


int nand_write(const nand_t *nand, unsigned int page, const void *data, const void *aux, int raw)
{
	const nand_dev_t *dev = (const nand_dev_t *)nand;
	dma_t *dma = (dma_t *)nand_dma;
	u8 addr[NAND_MAX_ADDRSZ];
	u16 size;
	int err;

	/* Prepare page address (skip 2 bytes for column address) */
	hal_memset(addr, 0, nand_cmd[NAND_PROG_PAGE].addrsz);
	hal_memcpy(addr + 2, &page, sizeof(addr) - 2);

	/* Write whole page */
	if (data != NULL) {
		size = dev->cfg->pagesz + dev->cfg->oobsz;
		/* Write without ECC */
		if (raw != 0) {
			aux = NULL;
		}
		/* Write data only */
		else if (aux == NULL) {
			/* Partial write (data only BCH page layout) */
			aux = nand_buff;
			hal_memset((void *)aux, 0xff, dev->cfg->metasz + bch_eccsz(dev->ecc->ecc0, dev->ecc->gf0));
			bch_layoutData(dev->id, dev->ecc, dev->cfg->metasz);
		}
	}
	/* Write only metadata */
	else {
		size = dev->cfg->metasz + bch_eccsz(dev->ecc->ecc0, dev->ecc->gf0);
		/* Write without ECC */
		if (raw != 0) {
			data = aux;
			aux = NULL;
		}
		else {
			/* Metadata only BCH page layout */
			bch_layoutMeta(dev->id, dev->ecc, dev->cfg->metasz);
		}
	}

	dma_reset(dma);
	nand_w4ready(dma, dev->id);
	nand_issue(dma, NAND_PROG_PAGE, dev->id, addr, data, aux, size);
	nand_w4ready(dma, dev->id);
	nand_issue(dma, NAND_STATUS, dev->id, NULL, NULL, NULL, 0);
	nand_readcmp(dma, dev->id, 0x3, 0, -1);
	dma_finish(dma);

	err = dma_run(dma, 0);

	/* Restore default BCH page layout */
	bch_layout(dev->id, dev->ecc, dev->cfg->metasz, dev->cfg->pagesz + dev->cfg->oobsz);

	return err;
}


int nand_erase(const nand_t *nand, unsigned int block)
{
	const nand_dev_t *dev = (const nand_dev_t *)nand;
	dma_t *dma = (dma_t *)nand_dma;
	unsigned int page = block * (dev->cfg->erasesz / dev->cfg->pagesz);

	dma_reset(dma);
	nand_w4ready(dma, dev->id);
	nand_issue(dma, NAND_ERASE_BLOCK, dev->id, &page, NULL, NULL, 0);
	nand_w4ready(dma, dev->id);
	nand_issue(dma, NAND_STATUS, dev->id, NULL, NULL, NULL, 0);
	nand_readcmp(dma, dev->id, 0x1, 0, -1);
	dma_finish(dma);

	return dma_run(dma, 0);
}


int nand_isBad(const nand_t *nand, unsigned int block)
{
	const nand_dev_t *dev = (const nand_dev_t *)nand;
	unsigned int page = block * (dev->cfg->erasesz / dev->cfg->pagesz);
	u8 *meta = nand_buff;
	int err;

	/* Read raw metadata */
	err = nand_read(nand, page, NULL, meta, 1);
	if (err < 0) {
		/* Read error, assume bad block */
		return 1;
	}

	/* First byte is the bad block marker */
	return (meta[0] == 0x00);
}


int nand_markBad(const nand_t *nand, unsigned int block)
{
	const nand_dev_t *dev = (const nand_dev_t *)nand;
	unsigned int page = block * (dev->cfg->erasesz / dev->cfg->pagesz);
	u8 *meta = nand_buff;

	/* Partial write (only bad block marker) */
	hal_memset(meta, 0xff, dev->cfg->metasz + bch_eccsz(dev->ecc->ecc0, dev->ecc->gf0));
	meta[0] = 0x00;

	/* Write raw metadata */
	return nand_write(nand, page, NULL, meta, 1);
}


static int nand_readID(const nand_dev_t *dev, nand_id_t *id)
{
	dma_t *dma = (dma_t *)nand_dma;
	u8 addr[NAND_MAX_ADDRSZ];

	/* Clear address bytes */
	hal_memset(addr, 0, nand_cmd[NAND_READ_ID].addrsz);

	dma_reset(dma);
	nand_w4ready(dma, dev->id);
	nand_issue(dma, NAND_READ_ID, dev->id, &addr, NULL, NULL, 0);
	nand_w4ready(dma, dev->id);
	nand_readback(dma, dev->id, id, NULL, sizeof(*id));
	dma_finish(dma);

	return dma_run(dma, 0);
}


static int nand_reset(const nand_dev_t *dev)
{
	dma_t *dma = (dma_t *)nand_dma;

	dma_reset(dma);
	nand_issue(dma, NAND_RESET, dev->id, NULL, NULL, NULL, 0);
	dma_finish(dma);

	return dma_run(dma, 0);
}


int nand_done(unsigned int cs)
{
	nand_dev_t *dev = (nand_dev_t *)nand_get(cs);

	if (dev == NULL) {
		return -ENODEV;
	}

	dev->id = 0;
	dev->ecc = NULL;
	dev->cfg = NULL;

	return EOK;
}


int nand_init(unsigned int cs)
{
	nand_dev_t *dev = (nand_dev_t *)nand_get(cs);
	nand_id_t *id = (nand_id_t *)nand_buff;
	unsigned int i;
	int err;

	if (dev == NULL) {
		return -ENODEV;
	}

	/* NAND device already initialized */
	if ((dev->id == cs) && (dev->ecc != NULL) && (dev->cfg != NULL)) {
		return EOK;
	}

	/* Reset and detect NAND device */
	err = nand_reset(dev);
	if (err < 0) {
		return err;
	}

	err = nand_readID(dev, id);
	if (err < 0) {
		return err;
	}

	for (i = 0; i < sizeof(nand_info) / sizeof(nand_info[0]); i++) {
		/* Found supported NAND device */
		if ((id->manid == nand_info[i].manid) && (id->devid == nand_info[i].devid)) {
			dev->id = cs;
			dev->ecc = nand_info[i].ecc;
			dev->cfg = &nand_info[i].cfg;

			/* Set BCH page layout */
			bch_layout(dev->id, dev->ecc, dev->cfg->metasz, dev->cfg->pagesz + dev->cfg->oobsz);

			return EOK;
		}
	}

	return -ENODEV;
}


__attribute__((destructor)) static void _nand_done(void)
{
	/* Disable NAND modules */
	gpmi_done();
	dma_done();
	bch_done();
}


__attribute__((constructor)) static void _nand_init(void)
{
	unsigned int i;

	/* Configure NAND pins */
	for (i = 0; i < sizeof(nand_mux) / sizeof(nand_mux[0]); i++) {
		imx6ull_setIOmux(nand_mux[i].mux, 0, nand_mux[i].mode);
	}

	/* Initialize NAND modules */
	bch_init();
	dma_init();
	gpmi_init();
}
