/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32 XSPI Flash driver
 *
 * Copyright 2020, 2025 Phoenix Systems
 * Author: Aleksander Kaminski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/lib.h>
#include <devices/devs.h>

#include <board_config.h>

#include <hal/armv8m/stm32/n6/stm32n6.h>

#include "flash_params.h"


#define XSPI1_BASE     ((void *)0x58025000)
#define XSPI1_REG_BASE ((void *)0x90000000)
#define XSPI1_REG_SIZE (256 * 1024 * 1024)
#define XSPI1_IRQ      170
#define XSPI2_BASE     ((void *)0x5802a000)
#define XSPI2_REG_BASE ((void *)0x70000000)
#define XSPI2_REG_SIZE (256 * 1024 * 1024)
#define XSPI2_IRQ      171
#define XSPI3_BASE     ((void *)0x5802d000)
#define XSPI3_REG_BASE ((void *)0x80000000)
#define XSPI3_REG_SIZE (256 * 1024 * 1024)
#define XSPI3_IRQ      172
#define XSPIM_BASE     ((void *)0x5802b400)
#define FIFO_SIZE      64 /* Size of hardware FIFO */

#define MAX_PINS      22
#define N_CONTROLLERS 1


enum {
	ipclk_sel_hclk5 = 0x0,
	ipclk_sel_per_ck = 0x1,
	ipclk_sel_ic3_ck = 0x2,
	ipclk_sel_ic4_ck = 0x3,
};


enum xspi_regs {
	xspi_cr = 0x0,
	xspi_dcr1 = 0x2,
	xspi_dcr2,
	xspi_dcr3,
	xspi_dcr4,
	xspi_sr = 0x8,
	xspi_fcr,
	xspi_dlr = 0x10,
	xspi_ar = 0x12,
	xspi_dr = 0x14,
	xspi_psmkr = 0x20,
	xspi_psmar = 0x22,
	xspi_pir = 0x24,
	xspi_ccr = 0x40,
	xspi_tcr = 0x42,
	xspi_ir = 0x44,
	xspi_abr = 0x48,
	xspi_lptr = 0x4c,
	xspi_wpccr = 0x50,
	xspi_wptcr = 0x52,
	xspi_wpir = 0x54,
	xspi_wpabr = 0x58,
	xspi_wccr = 0x60,
	xspi_wtcr = 0x62,
	xspi_wir = 0x64,
	xspi_wabr = 0x68,
	xspi_hlcr = 0x80,
	xspi_calfcr = 0x84,
	xspi_calmr = 0x86,
	xspi_calsor = 0x88,
	xspi_calsir = 0x8a,
};


#define XSPI_SR_BUSY (1 << 5)
#define XSPI_SR_TOF  (1 << 4)
#define XSPI_SR_SMF  (1 << 3)
#define XSPI_SR_FTF  (1 << 2)
#define XSPI_SR_TCF  (1 << 1)
#define XSPI_SR_TEF  (1 << 0)

#define XSPI_CR_MODE_IWRITE   (0 << 28)
#define XSPI_CR_MODE_IREAD    (1 << 28)
#define XSPI_CR_MODE_AUTOPOLL (2 << 28)
#define XSPI_CR_MODE_MEMORY   (3 << 28)
#define XSPI_CR_MODE_MASK     (3 << 28)

#define XSPI_DCR1_MTYP_MICRON       (0 << 24)
#define XSPI_DCR1_MTYP_MACRONIX     (1 << 24)
#define XSPI_DCR1_MTYP_STANDARD     (2 << 24)
#define XSPI_DCR1_MTYP_MACRONIX_RAM (3 << 24)

#define XSPIM_PORT1   0
#define XSPIM_PORT2   1
#define XSPIM_MUX_OFF 0 /* No multiplexed accesses */
#define XSPIM_MUX_ON  1 /* XSPI1 and XSPI2 do multiplexed accesses on the same port */
/* If MUX_OFF: XSPI1 to Port 1, XSPI2 to Port 2
 * If MUX_ON: XSPI1 and XSPI2 muxed to Port 1, XSPI3 to Port 2*/
#define XSPIM_MODE_DIRECT 0
/* If MUX_OFF: XSPI1 to Port 2, XSPI2 to Port 1
 * If MUX_ON: XSPI1 and XSPI2 muxed to Port 2, XSPI3 to Port 1 */
#define XSPIM_MODE_SWAPPED (1 << 1)


typedef struct {
	u32 ccr;
	u32 tcr;
	u32 ir;
} flash_xspiSetup_t;

typedef struct {
	flash_xspiSetup_t reg;
	u32 addr;
	u32 dataLen;
	u8 isRead;
} flash_opDefinition_t;

typedef struct {
	s16 port;
	s8 pin;
} flash_pin_t;


/* TODO: Frequency after divider must be at most 100 MHz, otherwise we get data corruption
 * or controller hangs. */
/* Parameters of XSPI controllers in use.
 * IMPORTANT: if you want to use XSPI1, DO NOT set its clock source to IC3 or IC4.
 * Doing so will result in a hang in BootROM after reset.
 * The same behavior exists in official STMicroelectronics code.
 */
static const struct flash_ctrlParams {
	void *start;
	u32 size;
	volatile u32 *ctrl;
	struct {
		u16 sel; /* Clock mux (ipclk_xspi?sel) */
		u8 val;  /* Clock source (one of ipclk_sel_*) */
	} clksel;
	u16 divider_slow; /* Divider used for initialization - resulting clock must be under 50 MHz */
	u16 divider;      /* Divider used for normal operation - can be as fast as Flash can handle */
	u16 dev;
	u16 rst;
	u8 spiPort;
} controllerParams[N_CONTROLLERS] = {
	{
		.start = XSPI2_REG_BASE,
		.size = XSPI2_REG_SIZE,
		.clksel = { .sel = ipclk_xspi2sel, .val = ipclk_sel_ic3_ck },
		.divider_slow = 10,
		.divider = 4,
		.dev = dev_xspi2,
		.rst = dev_rst_xspi2,
		.ctrl = XSPI2_BASE,
		.spiPort = XSPIM_PORT2,
	},
};


/* Parameters of XSPI I/O manager */
static const struct flash_mgrParams {
	volatile u32 *base;
	u32 config;
	struct {
		u8 pin_af;
		flash_pin_t pins[MAX_PINS]; /* value < 0 signals end of list */
	} spiPort[2];
} mgrParams = {
	.base = XSPIM_BASE,
	.config = XSPIM_MODE_DIRECT | XSPIM_MUX_OFF,
	.spiPort = {
		[XSPIM_PORT1] = {
			.pin_af = 9,
			.pins = {
				{ dev_gpioo, 0 },  /* XSPIM_P1_NCS1 */
				{ dev_gpioo, 2 },  /* XSPIM_P1_DQS0 */
				{ dev_gpioo, 3 },  /* XSPIM_P1_DQS1 */
				{ dev_gpioo, 4 },  /* XSPIM_P1_CLK */
				{ dev_gpiop, 0 },  /* XSPIM_P1_IO0 */
				{ dev_gpiop, 1 },  /* XSPIM_P1_IO1 */
				{ dev_gpiop, 2 },  /* XSPIM_P1_IO2 */
				{ dev_gpiop, 3 },  /* XSPIM_P1_IO3 */
				{ dev_gpiop, 4 },  /* XSPIM_P1_IO4 */
				{ dev_gpiop, 5 },  /* XSPIM_P1_IO5 */
				{ dev_gpiop, 6 },  /* XSPIM_P1_IO6 */
				{ dev_gpiop, 7 },  /* XSPIM_P1_IO7 */
				{ dev_gpiop, 8 },  /* XSPIM_P1_IO8 */
				{ dev_gpiop, 9 },  /* XSPIM_P1_IO9 */
				{ dev_gpiop, 10 }, /* XSPIM_P1_IO1O */
				{ dev_gpiop, 11 }, /* XSPIM_P1_IO11 */
				{ dev_gpiop, 12 }, /* XSPIM_P1_IO12 */
				{ dev_gpiop, 13 }, /* XSPIM_P1_IO13 */
				{ dev_gpiop, 14 }, /* XSPIM_P1_IO14 */
				{ dev_gpiop, 15 }, /* XSPIM_P1_IO15 */
				{ -1, -1 },
			},
		},
		[XSPIM_PORT2] = {
			.pin_af = 9,
			.pins = {
				{ dev_gpion, 0 },  /* XSPIM_P2_DQS0 */
				{ dev_gpion, 1 },  /* XSPIM_P2_NCS1 */
				{ dev_gpion, 2 },  /* XSPIM_P2_IO0 */
				{ dev_gpion, 3 },  /* XSPIM_P2_IO1 */
				{ dev_gpion, 4 },  /* XSPIM_P2_IO2 */
				{ dev_gpion, 5 },  /* XSPIM_P2_IO3 */
				{ dev_gpion, 6 },  /* XSPIM_P2_CLK */
				{ dev_gpion, 8 },  /* XSPIM_P2_IO4 */
				{ dev_gpion, 9 },  /* XSPIM_P2_IO5 */
				{ dev_gpion, 10 }, /* XSPIM_P2_IO6 */
				{ dev_gpion, 11 }, /* XSPIM_P2_IO7 */
				{ -1, -1 },
			},
		},
	},
};


/* Parameters of Flash memory connected to a controller */
static struct flash_memParams {
	unsigned char device_id[6];
	int (*init_fn)(int minor);
	u32 size;
	flash_opParameters_t params;
	u32 memoryType;
	flash_xspiSetup_t read;
	flash_xspiSetup_t write;
	flash_xspiSetup_t erase;
	const flash_opDefinition_t *chipErase;
	const flash_opDefinition_t *status;
	const flash_opDefinition_t *wrEn;
	const flash_opDefinition_t *wrDis;
	const char *name;
} memParams[N_CONTROLLERS];


static struct {
	int xspimDone;
} xspi_common = { 0 };


#define PHASE_TYPE_NO  0
#define PHASE_TYPE_S1  1
#define PHASE_TYPE_S2  2
#define PHASE_TYPE_S4  3
#define PHASE_TYPE_S8  4
#define PHASE_TYPE_D1  (1 | 8)
#define PHASE_TYPE_D2  (2 | 8)
#define PHASE_TYPE_D4  (3 | 8)
#define PHASE_TYPE_D8  (4 | 8)
#define PHASE_TYPE_D16 (5 | 8)

#define MAKE_CCR_VALUE(itype, atype, mtype, dtype, ilen, alen, mlen, dqs) ( \
		((PHASE_TYPE_##itype) << 0) | \
		((PHASE_TYPE_##atype) << 8) | \
		((PHASE_TYPE_##mtype) << 16) | \
		((PHASE_TYPE_##dtype) << 24) | \
		((((ilen) - 1) & 0x3) << 4) | \
		((((alen) - 1) & 0x3) << 12) | \
		((((mlen) - 1) & 0x3) << 20) | \
		(((dqs) & 0x1) << 29))

/* Definitions of default SPI Flash commands */

static const flash_opDefinition_t opDef_read_id = {
	.reg = {
		.ccr = MAKE_CCR_VALUE(S1, NO, NO, S1, 1, 0, 0, 0),
		.tcr = 0,
		.ir = 0x9f,
	},
	.dataLen = 6,
	.addr = 0,
	.isRead = 1,
};


static const flash_opDefinition_t opDef_enter_4byte = {
	.reg = {
		.ccr = MAKE_CCR_VALUE(S1, NO, NO, NO, 1, 0, 0, 0),
		.tcr = 0,
		.ir = 0xb7,
	},
	.dataLen = 0,
	.addr = 0,
	.isRead = 0,
};


static const flash_opDefinition_t opDef_read_status = {
	.reg = {
		.ccr = MAKE_CCR_VALUE(S1, NO, NO, S1, 1, 0, 0, 0),
		.tcr = 0,
		.ir = 0x05,
	},
	.dataLen = 1,
	.addr = 0,
	.isRead = 1,
};


static const flash_opDefinition_t opDef_write_enable = {
	.reg = {
		.ccr = MAKE_CCR_VALUE(S1, NO, NO, NO, 1, 0, 0, 0),
		.tcr = 0,
		.ir = 0x06,
	},
	.dataLen = 0,
	.addr = 0,
	.isRead = 0,
};


static const flash_opDefinition_t opDef_write_disable = {
	.reg = {
		.ccr = MAKE_CCR_VALUE(S1, NO, NO, NO, 1, 0, 0, 0),
		.tcr = 0,
		.ir = 0x04,
	},
	.dataLen = 0,
	.addr = 0,
	.isRead = 0,
};


static const flash_opDefinition_t opDef_chip_erase = {
	.reg = {
		.ccr = MAKE_CCR_VALUE(S1, NO, NO, NO, 1, 0, 0, 0),
		.tcr = 0,
		.ir = 0x60,
	},
	.dataLen = 0,
	.addr = 0,
	.isRead = 0,
};


/* Definitions of octal-SPI Flash commands */


static const flash_opDefinition_t opDef_octa_read_status = {
	.reg = {
		.ccr = MAKE_CCR_VALUE(D8, D8, NO, S8, 2, 4, 0, 1),
		.tcr = 4,
		.ir = 0x05fa,
	},
	.dataLen = 2,
	.addr = 0,
	.isRead = 1,
};


static const flash_opDefinition_t opDef_octa_write_enable = {
	.reg = {
		.ccr = MAKE_CCR_VALUE(D8, NO, NO, NO, 2, 0, 0, 0),
		.tcr = 0,
		.ir = 0x06f9,
	},
	.dataLen = 0,
	.addr = 0,
	.isRead = 0,
};


static const flash_opDefinition_t opDef_octa_write_disable = {
	.reg = {
		.ccr = MAKE_CCR_VALUE(D8, NO, NO, NO, 2, 0, 0, 0),
		.tcr = 0,
		.ir = 0x04fb,
	},
	.dataLen = 0,
	.addr = 0,
	.isRead = 0,
};


static const flash_opDefinition_t opDef_octa_chip_erase = {
	.reg = {
		.ccr = MAKE_CCR_VALUE(D8, NO, NO, NO, 2, 0, 0, 0),
		.tcr = 0,
		.ir = 0x609f,
	},
	.dataLen = 0,
	.addr = 0,
	.isRead = 0,
};


static const u32 opModeToCCR[operation_io_types] = {
	[operation_io_111] = MAKE_CCR_VALUE(S1, S1, S1, S1, 1, 1, 1, 0),
	[operation_io_112] = MAKE_CCR_VALUE(S1, S1, S1, S2, 1, 1, 1, 0),
	[operation_io_122] = MAKE_CCR_VALUE(S1, S2, S2, S2, 1, 1, 1, 0),
	[operation_io_114] = MAKE_CCR_VALUE(S1, S1, S1, S4, 1, 1, 1, 0),
	[operation_io_144] = MAKE_CCR_VALUE(S1, S4, S4, S4, 1, 1, 1, 0),
	[operation_io_222] = MAKE_CCR_VALUE(S2, S2, S2, S2, 1, 1, 1, 0),
	[operation_io_444] = MAKE_CCR_VALUE(S4, S4, S4, S4, 1, 1, 1, 0),
	[operation_io_444d] = MAKE_CCR_VALUE(D4, D4, D4, D4, 1, 1, 1, 1),
	[operation_io_888] = MAKE_CCR_VALUE(S8, S8, S8, S8, 1, 1, 1, 0),
	[operation_io_888d] = MAKE_CCR_VALUE(D8, D8, D8, D8, 1, 1, 1, 1),
};


static u32 flashdrv_makeCCRValue(
		u8 opMode,
		u8 opcodeType,
		u8 addrBytes,
		u8 modeBytes,
		u8 hasData)
{
	u32 ccr = opModeToCCR[opMode];
	if (opcodeType != flash_opcode_8b) {
		ccr |= 1 << 4; /* 2-byte instruction */
	}

	if (addrBytes != 0) {
		ccr |= (addrBytes - 1) << 12;
	}
	else {
		ccr &= ~(0x7 << 8); /* No address */
	}

	if (modeBytes != 0) {
		ccr |= (modeBytes - 1) << 20;
	}
	else {
		ccr &= ~(0x7 << 16); /* No mode bytes */
	}

	if (hasData == 0) {
		ccr &= ~(0x7 << 24); /* No data bytes */
	}

	return ccr;
}


static u32 flashdrv_makeIRValue(u8 opcodeType, u16 opcode)
{
	switch (opcodeType) {
		case flash_opcode_8b_repeat:
			return ((u32)opcode << 8) | opcode;

		case flash_opcode_8b_inverse:
			return ((u32)opcode << 8) | (~opcode & 0xff);

		default:
			return opcode;
	}
}


static u32 flashdrv_changeCtrlMode(unsigned int minor, u32 new_mode)
{
	const struct flash_ctrlParams *p = &controllerParams[minor];
	struct flash_memParams *mp = &memParams[minor];
	u32 prev_mode, v;

	v = *(p->ctrl + xspi_cr);
	prev_mode = (v & XSPI_CR_MODE_MASK);
	if (prev_mode == new_mode) {
		return prev_mode;
	}

	if ((prev_mode == XSPI_CR_MODE_MEMORY) || (prev_mode == XSPI_CR_MODE_AUTOPOLL)) {
		hal_cpuDataMemoryBarrier();
		*(p->ctrl + xspi_cr) = v | (1 << 1); /* Abort operation in progress */
		while ((*(p->ctrl + xspi_sr) & XSPI_SR_BUSY) != 0) {
			/* Wait for controller to become ready */
		}
	}

	v &= ~XSPI_CR_MODE_MASK;
	if (new_mode == XSPI_CR_MODE_MEMORY) {
		*(p->ctrl + xspi_cr) = v;
		hal_cpuDataMemoryBarrier();
		while ((*(p->ctrl + xspi_sr) & XSPI_SR_BUSY) != 0) {
			/* Wait for controller to become ready */
		}

		*(p->ctrl + xspi_ccr) = mp->read.ccr;
		*(p->ctrl + xspi_tcr) = mp->read.tcr;
		*(p->ctrl + xspi_ir) = mp->read.ir;
		hal_cpuDataMemoryBarrier();
	}

	v |= new_mode;
	*(p->ctrl + xspi_cr) = v;
	while ((*(p->ctrl + xspi_sr) & XSPI_SR_BUSY) != 0) {
		/* Wait for controller to become ready */
	}

	/* Clear any flags that may have been set (e.g. in autopoll or memory-mapped mode) */
	*(p->ctrl + xspi_fcr) = XSPI_SR_TCF | XSPI_SR_SMF;
	return prev_mode;
}


static inline int flashdrv_opHasAddr(const flash_opDefinition_t *opDef)
{
	return ((opDef->reg.ccr >> 8) & 0x7) != 0;
}


static void flashdrv_performOp(unsigned int minor, const flash_opDefinition_t *opDef, unsigned char *data)
{
	const struct flash_ctrlParams *p = &controllerParams[minor];
	int i = 0, j = 0;
	u32 status;

	flashdrv_changeCtrlMode(minor, (opDef->isRead != 0) ? XSPI_CR_MODE_IREAD : XSPI_CR_MODE_IWRITE);
	if (opDef->dataLen != 0) {
		*(p->ctrl + xspi_dlr) = opDef->dataLen - 1;
	}

	*(p->ctrl + xspi_ccr) = opDef->reg.ccr;
	*(p->ctrl + xspi_tcr) = opDef->reg.tcr;
	/* If indirect read mode and no address, this write triggers the operation */
	*(p->ctrl + xspi_ir) = opDef->reg.ir;
	if (flashdrv_opHasAddr(opDef)) {
		/* If indirect read mode with address, this write triggers the operation */
		*(p->ctrl + xspi_ar) = opDef->addr;
	}

	while (i < opDef->dataLen) {
		do {
			status = *(p->ctrl + xspi_sr);
		} while ((status & (XSPI_SR_FTF | XSPI_SR_TCF)) == 0);

		if (opDef->isRead != 0) {
			j = FIFO_SIZE - ((status >> 8) & 0x7f);
			for (; j < FIFO_SIZE && (i < opDef->dataLen); j++, i++) {
				/* This controller allows byte and halfword reads from the XSPI_DR register */
				data[i] = *(volatile u8 *)(p->ctrl + xspi_dr);
			}
		}
		else {
			j = (status >> 8) & 0x7f;
			/* In indirect write mode, writing data triggers the operation */
			for (; j < FIFO_SIZE && (i < opDef->dataLen); j++, i++) {
				/* This controller allows byte and halfword writes to the XSPI_DR register */
				*(volatile u8 *)(p->ctrl + xspi_dr) = data[i];
			}
		}
	}

	while ((*(p->ctrl + xspi_sr) & XSPI_SR_TCF) == 0) {
		/* Wait for transfer completion */
	}

	*(p->ctrl + xspi_fcr) = XSPI_SR_TCF;
}


static int flashdrv_waitForWriteCompletion(unsigned int minor, const flash_opDefinition_t *opDef, u32 timeout)
{
	int ret = 0;
	const struct flash_ctrlParams *p = &controllerParams[minor];
	time_t time_start = hal_timerGet();

	flashdrv_changeCtrlMode(minor, XSPI_CR_MODE_AUTOPOLL);
	*(p->ctrl + xspi_psmkr) = 0x01; /* Check bit 0 */
	*(p->ctrl + xspi_psmar) = 0x00; /* Check until it's cleared */
	*(p->ctrl + xspi_pir) = 0x10;
	*(p->ctrl + xspi_dlr) = opDef->dataLen - 1;
	*(p->ctrl + xspi_ccr) = opDef->reg.ccr;
	*(p->ctrl + xspi_tcr) = opDef->reg.tcr;
	/* If command has no address, writing XSPI_IR starts the operation */
	*(p->ctrl + xspi_ir) = opDef->reg.ir;
	if (flashdrv_opHasAddr(opDef)) {
		/* If command has address, writing XSPI_AR starts the operation */
		*(p->ctrl + xspi_ar) = opDef->addr;
	}

	while (1) {
		if ((*(p->ctrl + xspi_sr) & XSPI_SR_SMF) != 0) {
			break;
		}

		if (hal_timerGet() - time_start > timeout) {
			*(p->ctrl + xspi_cr) |= (1 << 1); /* Abort auto-polling */
			ret = -ETIME;
			break;
		}
	}
	*(p->ctrl + xspi_fcr) = XSPI_SR_SMF;
	return ret;
}


static int flashdrv_writeEnable(unsigned int minor, int enable)
{
	/* Depending on command status may be 1 or 2 bytes long.
	 * We only care about the first byte, but need to allocate a large enough buffer. */
	u8 status[2];
	const flash_opDefinition_t *opDef = enable ? memParams[minor].wrEn : memParams[minor].wrDis;
	unsigned retries = 10;
	enable = (enable != 0) ? 1 : 0;

	/* Set flag and verify until it's set - required according to Macronix datasheet */
	do {
		if (retries == 0) {
			return -EIO;
		}

		retries--;
		flashdrv_performOp(minor, opDef, NULL);
		flashdrv_performOp(minor, memParams[minor].status, status);
	} while (((status[0] >> 1) & 1) != enable);

	return 0;
}


/* Perform a write-like operation (program or erase) */
static ssize_t flashdrv_performWriteOp(
		unsigned int minor,
		const flash_opDefinition_t *opDef,
		const void *data,
		u32 timeout)
{
	if (flashdrv_writeEnable(minor, 1) < 0) {
		return -EIO;
	}

	flashdrv_performOp(minor, opDef, (void *)data);
	return flashdrv_waitForWriteCompletion(minor, memParams[minor].status, timeout);
}


/* Puts controller into a mode where SFDP data can be accessed within the address space.
 * Returns pointer to SFDP data. */
static const u32 *flashdrv_mountSfdp(int minor)
{
	const struct flash_ctrlParams *p = &controllerParams[minor];
	struct flash_memParams *mp = &memParams[minor];

	mp->read.ccr = MAKE_CCR_VALUE(S1, S1, NO, S1, 1, 3, 0, 0);
	mp->read.tcr = 8;   /* 8 dummy cycles, no sample shifting */
	mp->read.ir = 0x5a; /* Set read command to 0x5a (READ SERIAL FLASH DISCOVERY PARAMETER) */
	flashdrv_changeCtrlMode(minor, XSPI_CR_MODE_MEMORY);
	return p->start;
}


static int flashdrv_detectGeneric(int minor, flash_opParameters_t *res, unsigned char *device_id)
{
	(void)device_id;
	return flashdrv_parseSfdp(flashdrv_mountSfdp(minor), res, 0);
}


static u8 flashdrv_modeCyclesToBits(u8 readIoType, u8 cycles)
{
	switch (readIoType) {
		case operation_io_888: /* Fall-through */
		case operation_io_888d:
			return cycles * 8;

		case operation_io_144: /* Fall-through */
		case operation_io_444: /* Fall-through */
		case operation_io_444d:
			return cycles * 4;

		case operation_io_122: /* Fall-through */
		case operation_io_222:
			return cycles * 2;

		default:
			return cycles;
	}
}


static void flashdrv_fillOperations(struct flash_memParams *mp)
{
	flash_opParameters_t *fp = &mp->params;
	u32 readModeBytes = 0, v;

	if (fp->readModeCyc != 0) {
		v = flashdrv_modeCyclesToBits(fp->readIoType, fp->readModeCyc);
		if ((v % 8) != 0 || ((v / 8) > 4)) {
			/* This controller only supports mode bytes (not bits). If number of bits is % 8 != 0, treat them
			 * as if they were dummy cycles.
			 * This is not entirely correct, but in this case it's likely that the data in SFDP is inaccurate anyway
			 * (manufacturer counted some dummy cycles as "mode cycles" and Flash doesn't care about mode data). */
			fp->readDummy += fp->readModeCyc;
			fp->readModeCyc = 0;
		}
		else {
			readModeBytes = v / 8;
		}
	}

	mp->read.ccr = flashdrv_makeCCRValue(
			fp->readIoType, fp->opcodeType, (fp->addrMode == ADDRMODE_3B) ? 3 : 4, readModeBytes, 1);
	mp->read.tcr = fp->readDummy;
	mp->read.ir = flashdrv_makeIRValue(fp->opcodeType, fp->readOpcode);

	mp->write.ccr = flashdrv_makeCCRValue(
			fp->writeIoType, fp->opcodeType, (fp->addrMode == ADDRMODE_3B) ? 3 : 4, 0, 1);
	mp->write.tcr = fp->writeDummy;
	mp->write.ir = flashdrv_makeIRValue(fp->opcodeType, fp->writeOpcode);

	mp->erase.ccr = flashdrv_makeCCRValue(
			fp->writeIoType, fp->opcodeType, (fp->addrMode == ADDRMODE_3B) ? 3 : 4, 0, 0);
	mp->erase.tcr = 0;
	mp->erase.ir = flashdrv_makeIRValue(fp->opcodeType, fp->eraseOpcode);
}


static int flashdrv_initGeneric(int minor)
{
	struct flash_memParams *mp = &memParams[minor];
	int ret;

	if (mp->params.addrMode == ADDRMODE_4BO) {
		ret = flashdrv_performWriteOp(minor, &opDef_enter_4byte, NULL, 1000);
		if (ret < 0) {
			return ret;
		}
	}

	flashdrv_fillOperations(mp);
	return 0;
}


static int flashdrv_detectMacronixOcta(int minor, flash_opParameters_t *res, unsigned char *device_id)
{
	(void)device_id;
	int ret = flashdrv_parseSfdp(flashdrv_mountSfdp(minor), res, 0);
	if (ret < 0) {
		return ret;
	}

	/* This chip's SFDP data is almost useless because it only includes 3-byte address versions of commands.
	 * We need to input the 4-byte versions of each command from the datasheet. */
	res->opcodeType = flash_opcode_8b_inverse;
	res->readIoType = operation_io_888d;
	res->readOpcode = 0xee; /* OCTA DTR Read */
	res->readDummy = 20;
	res->readModeCyc = 0;
	res->writeIoType = operation_io_888d;
	res->writeOpcode = 0x12; /* Page program 4B */
	res->writeDummy = 0;
	res->addrMode = ADDRMODE_4B;
	res->eraseOpcode = 0xdc;
	res->log_eraseSize = 16;
	return 0;
}


static int flashdrv_initMacronixOcta(int minor)
{
	static const flash_opDefinition_t opDef_writeWRCR2 = {
		.reg = {
			.ccr = MAKE_CCR_VALUE(S1, S1, NO, S1, 1, 4, 0, 0),
			.tcr = 0,
			.ir = 0x72,
		},
		.dataLen = 1,
		.addr = 0,
		.isRead = 0,
	};

	struct flash_memParams *mp = &memParams[minor];
	u8 value = 0x02; /* Value to put Flash into DTR mode */
	int ret;

	if (flashdrv_writeEnable(minor, 1) < 0) {
		return -EIO;
	}

	flashdrv_performOp(minor, &opDef_writeWRCR2, &value);
	ret = flashdrv_waitForWriteCompletion(minor, &opDef_octa_read_status, 1000);
	if (ret < 0) {
		return ret;
	}

	flashdrv_fillOperations(mp);
	mp->chipErase = &opDef_octa_chip_erase;
	mp->status = &opDef_octa_read_status;
	mp->wrEn = &opDef_octa_write_enable;
	mp->wrDis = &opDef_octa_write_disable;
	mp->memoryType = XSPI_DCR1_MTYP_MACRONIX;
	return 0;
}


static int flashdrv_detectFlashType(unsigned int minor, flash_opParameters_t *res)
{
	flashdrv_fillDefaultParams(res);
	unsigned char *device_id = memParams[minor].device_id;
	flashdrv_performOp(minor, &opDef_read_id, device_id);

	if ((device_id[0] == 0xc2) && (device_id[1] == 0x80)) {
		memParams[minor].name = "Macronix";
		memParams[minor].init_fn = flashdrv_initMacronixOcta;
		return flashdrv_detectMacronixOcta(minor, res, device_id);
	}

	memParams[minor].name = "generic";
	memParams[minor].init_fn = flashdrv_initGeneric;
	return flashdrv_detectGeneric(minor, res, device_id);
}


static int flashdrv_isValidAddress(unsigned int minor, u32 off, size_t size)
{
	size_t fsize = memParams[minor].size;
	if ((off < fsize) && ((off + size) <= fsize)) {
		return 1;
	}

	return 0;
}


static int flashdrv_isValidMinor(unsigned int minor)
{
	return minor < N_CONTROLLERS ? 1 : 0;
}


static void flashdrv_initPins(int p)
{
	unsigned int i;
	for (i = 0; i < MAX_PINS; i++) {
		if (mgrParams.spiPort[p].pins[i].port == -1) {
			break;
		}

		_stm32_gpioConfig(
				mgrParams.spiPort[p].pins[i].port,
				mgrParams.spiPort[p].pins[i].pin,
				gpio_mode_af,
				mgrParams.spiPort[p].pin_af,
				gpio_otype_pp,
				gpio_ospeed_vhi,
				gpio_pupd_nopull);
	}
}


static void flashdrv_commonInit(void)
{
	/* After reset XSPI peripherals may be enabled because they were used by BootROM.
	 * We want to disable them _AND_ put them in reset.
	 * XSPIM configuration can be written ONLY if all XSPI peripherals are disabled
	 * either by reset or their XSPI_CR register bit 0 is 0.
	 * Disabling clocks for all XSPI peripherals is not enough. */
	_stm32_rccSetDevClock(dev_xspi1, 0);
	_stm32_rccSetDevClock(dev_xspi2, 0);
	_stm32_rccSetDevClock(dev_xspi3, 0);
	_stm32_rccSetDevClock(dev_xspim, 0);
	_stm32_rccDevReset(dev_rst_xspi1, 1);
	_stm32_rccDevReset(dev_rst_xspi2, 1);
	_stm32_rccDevReset(dev_rst_xspi3, 1);
	_stm32_rccDevReset(dev_rst_xspim, 1);

	/* Enable XSPIPHYCOMP and reset XSPIPHY (not sure if it does anything) */
	_stm32_rccSetDevClock(dev_xspiphycomp, 1);
	_stm32_rccDevReset(dev_rst_xspiphy1, 1);
	_stm32_rccDevReset(dev_rst_xspiphy1, 0);
	_stm32_rccDevReset(dev_rst_xspiphy2, 1);
	_stm32_rccDevReset(dev_rst_xspiphy2, 0);

	/* Take XSPIM out of reset and enable clock */
	_stm32_rccDevReset(dev_rst_xspim, 0);
	_stm32_rccSetDevClock(dev_xspim, 1);

	*(mgrParams.base) = mgrParams.config;
	hal_cpuDataMemoryBarrier();
	(void)*(mgrParams.base);
}


/* Below are functions for device's public interface */


static int flashdrv_sync(unsigned int minor)
{
	if (flashdrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	/* TODO: it may be faster to clear the whole DCache instead of doing it by address
	 * or we can disable caching of this region using MPU */
	hal_cpuInvCache(hal_cpuDCache, (addr_t)controllerParams[minor].start, memParams[minor].size);
	return EOK;
}


static ssize_t flashdrv_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	(void)timeout;
	if ((flashdrv_isValidMinor(minor) == 0) || (flashdrv_isValidAddress(minor, offs, len) == 0)) {
		return -EINVAL;
	}

	hal_memcpy(buff, controllerParams[minor].start + offs, len);
	return (ssize_t)len;
}


static ssize_t flashdrv_write_internal(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	flash_opDefinition_t opDef;
	const struct flash_memParams *mp;
	size_t remaining = len;
	u32 pageSize;
	u32 pageRemaining, unalignedEnd = 0;
	ssize_t ret = 0;
	u8 tmp[2];
	const u8 *data = buff;

	/* In DTR mode both offset and length must be aligned to 2 bytes */
	mp = &memParams[minor];
	pageSize = 1 << mp->params.log_pageSize;
	opDef.reg = mp->write;
	opDef.isRead = 0;
	if ((offs % 2) != 0) {
		tmp[0] = 0xff;
		tmp[1] = data[0];
		opDef.addr = offs - 1;
		opDef.dataLen = 2;
		ret = flashdrv_performWriteOp(minor, &opDef, tmp, 1000);
		if (ret < 0) {
			return ret;
		}

		offs++;
		data++;
		remaining--;
	}

	unalignedEnd = remaining % 2;
	remaining -= unalignedEnd;

	while (remaining > 0) {
		pageRemaining = pageSize - (offs & (pageSize - 1));
		if (pageRemaining > remaining) {
			pageRemaining = remaining;
		}

		opDef.addr = offs;
		opDef.dataLen = pageRemaining;
		ret = flashdrv_performWriteOp(minor, &opDef, data, 1000);
		if (ret < 0) {
			return ret;
		}

		offs += pageRemaining;
		data += pageRemaining;
		remaining -= pageRemaining;
	}

	if (unalignedEnd != 0) {
		tmp[0] = data[0];
		tmp[1] = 0xff;
		opDef.addr = offs;
		opDef.dataLen = 2;
		ret = flashdrv_performWriteOp(minor, &opDef, tmp, 1000);
		if (ret < 0) {
			return ret;
		}
	}

	return (ssize_t)len;
}


static ssize_t flashdrv_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	ssize_t ret;
	u32 prev_mode;
	if ((flashdrv_isValidMinor(minor) == 0) || (flashdrv_isValidAddress(minor, offs, len) == 0)) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	prev_mode = flashdrv_changeCtrlMode(minor, XSPI_CR_MODE_IWRITE);
	ret = flashdrv_write_internal(minor, offs, buff, len);
	flashdrv_changeCtrlMode(minor, prev_mode);
	return ret;
}


static ssize_t flashdrv_erase_internal(unsigned int minor, addr_t offs, size_t len)
{
	flash_opDefinition_t op;
	const struct flash_memParams *mp;
	u32 eraseSize;
	mp = &memParams[minor];
	int ret = 0;
	ssize_t len_ret = (ssize_t)len;
	if (len == (size_t)-1) {
		ret = flashdrv_performWriteOp(minor, mp->chipErase, NULL, mp->params.eraseChipTimeout);
		return (ret < 0) ? ret : (ssize_t)memParams[minor].size;
	}
	else {
		eraseSize = 1 << mp->params.log_eraseSize;
		if ((offs & (eraseSize - 1)) != 0 || (len & (eraseSize - 1)) != 0) {
			return -EINVAL;
		}

		op.reg = mp->erase;
		op.isRead = 0;
		op.dataLen = 0;
		for (; len != 0; offs += eraseSize, len -= eraseSize) {
			op.addr = offs;
			ret = flashdrv_performWriteOp(minor, &op, NULL, mp->params.eraseBlockTimeout);
			if (ret < 0) {
				return ret;
			}
		}

		return (ssize_t)len_ret;
	}
}


static ssize_t flashdrv_erase(unsigned int minor, addr_t offs, size_t len, unsigned int flags)
{
	ssize_t ret;
	u32 prev_mode;

	(void)flags;
	if (flashdrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	if ((len != (size_t)-1) && (flashdrv_isValidAddress(minor, offs, len) == 0)) {
		return -EINVAL;
	}

	prev_mode = flashdrv_changeCtrlMode(minor, XSPI_CR_MODE_IWRITE);
	ret = flashdrv_erase_internal(minor, offs, len);
	flashdrv_sync(minor);
	flashdrv_changeCtrlMode(minor, prev_mode);
	return ret;
}


static int flashdrv_done(unsigned int minor)
{
	if (flashdrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	return EOK;
}


static int flashdrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	size_t fSz, ctrlSz;
	addr_t fStart;

	if (flashdrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	fStart = (addr_t)controllerParams[minor].start;
	ctrlSz = controllerParams[minor].size;
	fSz = memParams[minor].size;
	*a = fStart;

	/* Check if region is located on flash */
	if ((addr + sz) >= fSz) {
		return -EINVAL;
	}

	/* Check if flash is mappable to map region */
	if (fStart <= memaddr && (fStart + ctrlSz) >= (memaddr + memsz)) {
		return dev_isMappable;
	}

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode) {
		return -EINVAL;
	}

	/* Data can be copied from device to map */
	return dev_isNotMappable;
}


static int flashdrv_init(unsigned int minor)
{
	int ret;
	u32 v;
	const struct flash_ctrlParams *p;
	struct flash_memParams *mp;
	flash_opParameters_t *fp;

	if (xspi_common.xspimDone == 0) {
		flashdrv_commonInit();
		xspi_common.xspimDone = 1;
	}

	if (flashdrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	p = &controllerParams[minor];
	mp = &memParams[minor];
	fp = &mp->params;

	mp->chipErase = &opDef_chip_erase;
	mp->status = &opDef_read_status;
	mp->wrEn = &opDef_write_enable;
	mp->wrDis = &opDef_write_disable;

	_stm32_rccSetIPClk(p->clksel.sel, p->clksel.val);
	_stm32_rccDevReset(p->rst, 0);
	_stm32_rccSetDevClock(p->dev, 1);
	flashdrv_initPins(p->spiPort);

	*(p->ctrl + xspi_cr) = 0; /* Disable controller */
	hal_cpuDataSyncBarrier();

	/* NOTE: we set the NOPREF_AXI bit, during testing clearing it resulted in data corruption in some situations */
	*(p->ctrl + xspi_cr) =
			(1 << 26) | /* Prefetch is disabled when the AXI transaction is signaled as not-prefetchable */
			(0 << 24) | /* Use Chip select 1 */
			(1 << 22) | /* Stop auto-polling on match */
			(3 << 8) |  /* FIFO threshold = 4 bytes */
			(1 << 3);   /* Enable timeout for memory-mapped mode */

	*(p->ctrl + xspi_lptr) = 64; /* Timeout for memory-mapped mode */

	while ((*(p->ctrl + xspi_sr) & XSPI_SR_BUSY) != 0) {
		/* Wait for controller to become ready */
	}

	mp->memoryType = XSPI_DCR1_MTYP_STANDARD;
	*(p->ctrl + xspi_dcr1) =
			mp->memoryType | /* Standard memory mode */
			(23 << 16) |     /* 16 MB size */
			(0 << 8) |       /* Chip-select high for at least 1 cycle */
			(0 << 1);        /* Free-running clock disable */
	*(p->ctrl + xspi_dcr2) =
			(0 << 16) |                     /* Wrapped reads not supported */
			((p->divider_slow - 1) & 0xff); /* Clock prescaler */
	*(p->ctrl + xspi_dcr3) = 0;
	*(p->ctrl + xspi_dcr4) = 0;

	/* Disable writing in memory mapped mode */
	*(p->ctrl + xspi_wccr) = 0;
	*(p->ctrl + xspi_wtcr) = 0;
	*(p->ctrl + xspi_wir) = 0;

	*(p->ctrl + xspi_cr) |= 1; /* Enable controller */
	while ((*(p->ctrl + xspi_sr) & XSPI_SR_BUSY) != 0) {
		/* Wait for controller to become ready */
	}

	/* Read Flash memory details (JEDEC ID and SFDP) */
	ret = flashdrv_detectFlashType(minor, fp);
	if (ret < 0) {
		return ret;
	}

	/* Initialize Flash memory based on details read */
	flashdrv_changeCtrlMode(minor, XSPI_CR_MODE_IWRITE);
	if (mp->init_fn != NULL) {
		ret = mp->init_fn(minor);
		if (ret < 0) {
			return ret;
		}
	}

	/* Configure higher clocks */
	flashdrv_changeCtrlMode(minor, XSPI_CR_MODE_IWRITE);
	*(p->ctrl + xspi_cr) &= ~1;
	v = *(p->ctrl + xspi_dcr2);
	v &= ~0xff;
	v |= (p->divider - 1) & 0xff;
	*(p->ctrl + xspi_dcr2) = v;
	while ((*(p->ctrl + xspi_sr) & XSPI_SR_BUSY) != 0) {
		/* Wait for controller to become ready */
	}

	*(p->ctrl + xspi_cr) |= 1;

	if (fp->log_chipSize <= 31) {
		mp->size = 1 << fp->log_chipSize;
	}
	else {
		fp->log_chipSize = 31;
		mp->size = 1 << 31;
	}

	/* Limit size of the device to what's accessible */
	if (mp->size > controllerParams[minor].size) {
		mp->size = controllerParams[minor].size;
	}

	if ((fp->addrMode == ADDRMODE_3B) && (mp->size > (1 << 24))) {
		/* If flash was not fully recognized, it may be larger than 16 MB, but we don't know
		 * how to put it into 4-byte addressing mode - so it's limited to 16 MB */
		mp->size = (1 << 24);
	}

	/* Set new memory size into DCR1 */
	v = *(p->ctrl + xspi_dcr1);
	v &= ~((0x1f << 16) | (0x7 << 24));
	v |= ((fp->log_chipSize - 1) << 16) | mp->memoryType;
	*(p->ctrl + xspi_dcr1) = v;

	flashdrv_changeCtrlMode(minor, XSPI_CR_MODE_MEMORY);
	flashdrv_sync(minor);

	lib_printf("\ndev/flash: Configured %s %dMB NOR flash(%d.%d)",
			mp->name,
			mp->size >> 20,
			DEV_STORAGE,
			minor);

	return EOK;
}


__attribute__((constructor)) static void flashdrv_reg(void)
{
	static const dev_ops_t opsFlashSTM32_XSPI = {
		.read = flashdrv_read,
		.write = flashdrv_write,
		.erase = flashdrv_erase,
		.sync = flashdrv_sync,
		.map = flashdrv_map,
	};

	static const dev_t devFlashSTM32_XSPI = {
		.name = "flash-stm32xspi",
		.init = flashdrv_init,
		.done = flashdrv_done,
		.ops = &opsFlashSTM32_XSPI,
	};

	devs_register(DEV_STORAGE, N_CONTROLLERS, &devFlashSTM32_XSPI);
}
