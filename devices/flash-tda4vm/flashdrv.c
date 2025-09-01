/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * TDA4VM Flash driver
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

#include <hal/armv7r/tda4vm/tda4vm.h>
#include <hal/armv7r/tda4vm/tda4vm_pins.h>

#include "flash_params.h"

/* TODO:
 * * Add detection and support for mode bits
 * * For better performance (or XIP) consider enabling CPU caches over Flash address range
 * 	 * If CPU data cache is enabled, it needs to be invalidated after erase operations
 * * Read typical erase times from SFDP and configure timeouts based on that
 */

enum {
	ospi_reg_config = (0x0 / 4),
	ospi_reg_dev_instr_rd_config = (0x4 / 4),
	ospi_reg_dev_instr_wr_config = (0x8 / 4),
	ospi_reg_dev_delay = (0xC / 4),
	ospi_reg_rd_data_capture = (0x10 / 4),
	ospi_reg_dev_size_config = (0x14 / 4),
	ospi_reg_sram_partition_cfg = (0x18 / 4),
	ospi_reg_ind_ahb_addr_trigger = (0x1C / 4),
	ospi_reg_dma_periph_config = (0x20 / 4),
	ospi_reg_remap_addr = (0x24 / 4),
	ospi_reg_mode_bit_config = (0x28 / 4),
	ospi_reg_sram_fill = (0x2C / 4),
	ospi_reg_tx_thresh = (0x30 / 4),
	ospi_reg_rx_thresh = (0x34 / 4),
	ospi_reg_write_completion_ctrl = (0x38 / 4),
	ospi_reg_no_of_polls_bef_exp = (0x3C / 4),
	ospi_reg_irq_status = (0x40 / 4),
	ospi_reg_irq_mask = (0x44 / 4),
	ospi_reg_lower_wr_prot = (0x50 / 4),
	ospi_reg_upper_wr_prot = (0x54 / 4),
	ospi_reg_wr_prot_ctrl = (0x58 / 4),
	ospi_reg_indirect_read_xfer_ctrl = (0x60 / 4),
	ospi_reg_indirect_read_xfer_watermark = (0x64 / 4),
	ospi_reg_indirect_read_xfer_start = (0x68 / 4),
	ospi_reg_indirect_read_xfer_num_bytes = (0x6C / 4),
	ospi_reg_indirect_write_xfer_ctrl = (0x70 / 4),
	ospi_reg_indirect_write_xfer_watermark = (0x74 / 4),
	ospi_reg_indirect_write_xfer_start = (0x78 / 4),
	ospi_reg_indirect_write_xfer_num_bytes = (0x7C / 4),
	ospi_reg_indirect_trigger_addr_range = (0x80 / 4),
	ospi_reg_flash_command_ctrl_mem = (0x8C / 4),
	ospi_reg_flash_cmd_ctrl = (0x90 / 4),
	ospi_reg_flash_cmd_addr = (0x94 / 4),
	ospi_reg_flash_rd_data_lower = (0xA0 / 4),
	ospi_reg_flash_rd_data_upper = (0xA4 / 4),
	ospi_reg_flash_wr_data_lower = (0xA8 / 4),
	ospi_reg_flash_wr_data_upper = (0xAC / 4),
	ospi_reg_polling_flash_status = (0xB0 / 4),
	ospi_reg_phy_configuration = (0xB4 / 4),
	ospi_reg_phy_master_control = (0xB8 / 4),
	ospi_reg_dll_observable_lower = (0xBC / 4),
	ospi_reg_dll_observable_upper = (0xC0 / 4),
	ospi_reg_opcode_ext_lower = (0xE0 / 4),
	ospi_reg_opcode_ext_upper = (0xE4 / 4),
	ospi_reg_module_id = (0xFC / 4),
};

#define DAT_PINS      8
#define N_CONTROLLERS 2

#define OSPI0_REG1_BASE     ((void *)0x50000000)
#define OSPI0_REG1_MAX_SIZE 0x8000000
#define OSPI0_CTRL_BASE     ((void *)0x47040000)

#define OSPI1_REG1_BASE     ((void *)0x58000000)
#define OSPI1_REG1_MAX_SIZE 0x8000000
#define OSPI1_CTRL_BASE     ((void *)0x47050000)

#define ERASE_BLOCK_TIMEOUT_MS 1000
#define ERASE_CHIP_TIMEOUT_MS  60000

typedef struct
{
	u8 opcode;
	u8 readBytes;
	u8 writeBytes;
	u8 addrBytes;
	u32 addr;
	u8 dummyCycles;
} flash_opDefinition_t;


typedef struct {
	s16 pin;
	s16 mux;
} flash_pin_t;


typedef struct {
	void *start;
	u32 size;
	struct {
		int num;
		u8 setting;
		u8 pll;
		u8 hsdiv;
	} clksel;
	u8 divider;
	volatile u32 *ctrl;
	struct
	{
		flash_pin_t clk;
		flash_pin_t lbclko;
		flash_pin_t dqs;
		flash_pin_t cs;
		flash_pin_t dat[DAT_PINS]; /* value < 0 signals end of list */
	} pins;
} flash_ctrlParams_t;


static const flash_ctrlParams_t controllerParams[N_CONTROLLERS] = {
	{
		.start = OSPI0_REG1_BASE,
		.size = OSPI0_REG1_MAX_SIZE,
		.clksel = { clksel_mcu_ospi0, 0, clk_mcu_per_pll1, 4 },
		.divider = 4,
		.ctrl = OSPI0_CTRL_BASE,
		.pins = {
			.clk = { OSPI0_CLK, OSPI0_PIN_MUX },
			.lbclko = { OSPI0_LBCLKO, OSPI0_PIN_MUX },
			.dqs = { OSPI0_DQS, OSPI0_PIN_MUX },
			.cs = { OSPI0_CS, OSPI0_PIN_MUX },
			.dat = {
				{ OSPI0_D0, OSPI0_PIN_MUX },
				{ OSPI0_D1, OSPI0_PIN_MUX },
				{ OSPI0_D2, OSPI0_PIN_MUX },
				{ OSPI0_D3, OSPI0_PIN_MUX },
				{ OSPI0_D4, OSPI0_PIN_MUX },
				{ OSPI0_D5, OSPI0_PIN_MUX },
				{ OSPI0_D6, OSPI0_PIN_MUX },
				{ OSPI0_D7, OSPI0_PIN_MUX },
			},
		},
	},
	{
		.start = OSPI1_REG1_BASE,
		.size = OSPI1_REG1_MAX_SIZE,
		.clksel = { clksel_mcu_ospi1, 0, clk_mcu_per_pll1, 4 },
		.divider = 4,
		.ctrl = OSPI1_CTRL_BASE,
		.pins = {
			.clk = { OSPI1_CLK, OSPI1_PIN_MUX },
			.lbclko = { OSPI1_LBCLKO, OSPI1_PIN_MUX },
			.dqs = { OSPI1_DQS, OSPI1_PIN_MUX },
			.cs = { OSPI1_CS, OSPI1_PIN_MUX },
			.dat = {
				{ OSPI1_D0, OSPI1_PIN_MUX },
				{ OSPI1_D1, OSPI1_PIN_MUX },
				{ OSPI1_D2, OSPI1_PIN_MUX },
				{ OSPI1_D3, OSPI1_PIN_MUX },
				{ -1, -1 },
			},
		},
	},
};


static struct {
	unsigned char device_id[6];
	int (*init_fn)(int minor);
	u32 size;
	flash_opParameters_t params;
	const char *name;
} flashParams[N_CONTROLLERS];


/* Definitions of Flash commands */


static const flash_opDefinition_t opDef_read_id = {
	.opcode = 0x9f,
	.readBytes = 6,
	.writeBytes = 0,
	.addrBytes = 0,
	.addr = 0,
	.dummyCycles = 0,
};


static const flash_opDefinition_t opDef_enter_4byte = {
	.opcode = 0xb7,
	.readBytes = 0,
	.writeBytes = 0,
	.addrBytes = 0,
	.addr = 0,
	.dummyCycles = 0,
};


static const flash_opDefinition_t opDef_read_status = {
	.opcode = 0x05,
	.readBytes = 1,
	.writeBytes = 0,
	.addrBytes = 0,
	.addr = 0,
	.dummyCycles = 0,
};


static const flash_opDefinition_t opDef_write_enable = {
	.opcode = 0x06,
	.readBytes = 0,
	.writeBytes = 0,
	.addrBytes = 0,
	.addr = 0,
	.dummyCycles = 0,
};


static const flash_opDefinition_t opDef_write_disable = {
	.opcode = 0x04,
	.readBytes = 0,
	.writeBytes = 0,
	.addrBytes = 0,
	.addr = 0,
	.dummyCycles = 0,
};


static const flash_opDefinition_t opDef_chip_erase = {
	.opcode = 0x60,
	.readBytes = 0,
	.writeBytes = 0,
	.addrBytes = 0,
	.addr = 0,
	.dummyCycles = 0,
};


/* Performs a simple command (up to 8 bytes read/written) */
static void flashdrv_performSimpleOp(unsigned int minor, const flash_opDefinition_t *opDef, unsigned char *data)
{
	const flash_ctrlParams_t *p = &controllerParams[minor];
	u32 data_32[2];
	u32 val = (u32)opDef->opcode << 24;
	if (opDef->readBytes > 0) {
		val |= (1 << 23);
		val |= (opDef->readBytes - 1) << 20;
	}

	if (opDef->addrBytes > 0) {
		val |= (1 << 19);
		val |= (opDef->addrBytes - 1) << 16;
		*(p->ctrl + ospi_reg_flash_cmd_addr) = opDef->addr;
	}

	if (opDef->writeBytes > 0) {
		val |= (1 << 15);
		val |= (opDef->writeBytes - 1) << 12;
		data_32[0] = 0;
		data_32[1] = 0;
		hal_memcpy(data_32, data, opDef->writeBytes);
		*(p->ctrl + ospi_reg_flash_wr_data_lower) = data_32[0];
		*(p->ctrl + ospi_reg_flash_wr_data_upper) = data_32[1];
	}

	val |= (opDef->dummyCycles & 0x1f) << 7;

	/* Write setup without start bit */
	*(p->ctrl + ospi_reg_flash_cmd_ctrl) = val;

	/* Write start bit */
	*(p->ctrl + ospi_reg_flash_cmd_ctrl) = val | 1;

	while ((*(p->ctrl + ospi_reg_flash_cmd_ctrl) & (1 << 1)) != 0) {
		/* Wait for command completion */
	}

	if (opDef->readBytes > 0) {
		data_32[0] = *(p->ctrl + ospi_reg_flash_rd_data_lower);
		data_32[1] = *(p->ctrl + ospi_reg_flash_rd_data_upper);
		hal_memcpy(data, data_32, opDef->readBytes);
	}
}


static int flashdrv_waitForWriteCompletion(unsigned int minor, u32 timeout)
{
	unsigned char status = 0;
	time_t time_start = hal_timerGet();
	while (1) {
		flashdrv_performSimpleOp(minor, &opDef_read_status, &status);
		if ((status & 1) == 0) {
			return 0;
		}

		if (hal_timerGet() - time_start > timeout) {
			return -ETIME;
		}
	}
}


static int flashdrv_writeEnable(unsigned int minor, int enable)
{
	if (enable) {
		flashdrv_performSimpleOp(minor, &opDef_write_enable, NULL);
	}
	else {
		flashdrv_performSimpleOp(minor, &opDef_write_disable, NULL);
	}

	return 0;
}


static int flashdrv_modifyRegister(unsigned int minor, u8 readOp, u8 writeOp, u8 clear, u8 set)
{
	u8 value;
	const flash_opDefinition_t opDef_read = {
		.opcode = readOp,
		.readBytes = 1,
		.writeBytes = 0,
		.addrBytes = 0,
		.addr = 0,
		.dummyCycles = 0,
	};

	const flash_opDefinition_t opDef_write = {
		.opcode = writeOp,
		.readBytes = 0,
		.writeBytes = 1,
		.addrBytes = 0,
		.addr = 0,
		.dummyCycles = 0,
	};

	flashdrv_performSimpleOp(minor, &opDef_read, &value);
	value &= ~clear;
	value |= set;
	flashdrv_performSimpleOp(minor, &opDef_write, &value);

	return 0;
}


static int flashdrv_detectMicron(int minor, flash_opParameters_t *res, unsigned char *device_id)
{
	const flash_ctrlParams_t *p = &controllerParams[minor];
	/* Memory capacity is BCD-encoded */
	if (((device_id[2] >> 4) >= 10) || ((device_id[2] & 0xf) >= 10)) {
		return -EINVAL;
	}

	res->log_chipSize = ((device_id[2] >> 4) * 10) + (device_id[2] & 0xf) + 6;

	/* Set read command to 0x5a (READ SERIAL FLASH DISCOVERY PARAMETER) with 8 dummy cycles */
	*(p->ctrl + ospi_reg_dev_instr_rd_config) = (8 << 24) | 0x5a;
	*(p->ctrl + ospi_reg_dev_size_config) = (16 << 16) | (0x100 << 4) | 0x2;
	hal_cpuDataSyncBarrier();

	/* Now SFDP data is available in address space */
	if (flashdrv_parseSfdp(p->start, res, 1) < 0) {
		return -EINVAL;
	}

	/* TODO: Micron apparently sets dummy cycle value to 1 less than actual value?
	 * Needs more investigation */
	res->readDummy += 1;
	res->writeIoType = res->readIoType;
	res->writeDummy = 0;
	switch (res->writeIoType) {
		case OPERATION_IO_111:
		case OPERATION_IO_222:
		case OPERATION_IO_444:
			res->writeOpcode = 0x2;
			break;
		case OPERATION_IO_112:
			res->writeOpcode = 0xa2;
			break;
		case OPERATION_IO_122:
			res->writeOpcode = 0xd2;
			break;
		case OPERATION_IO_114:
			res->writeOpcode = 0x32;
			break;
		case OPERATION_IO_144:
			res->writeOpcode = 0x38;
			break;
	}

	return 0;
}


static int flashdrv_initMicron(int minor)
{
	flash_opParameters_t *fp = &flashParams[minor].params;
	if (fp->addrMode == ADDRMODE_4BO) {
		flashdrv_writeEnable(minor, 1);
		flashdrv_performSimpleOp(minor, &opDef_enter_4byte, NULL);
	}

	/* Set Enhanced Volatile Configuration Register */
	if (fp->readIoType == OPERATION_IO_222) {
		flashdrv_writeEnable(minor, 1);
		flashdrv_modifyRegister(minor, 0x65, 0x61, 0x40, 0x0);
	}
	else if (fp->readIoType == OPERATION_IO_444) {
		flashdrv_writeEnable(minor, 1);
		flashdrv_modifyRegister(minor, 0x65, 0x61, 0x80, 0x0);
	}

	/* Note: if mode was switched to quad-I/O commands,
	 * no further commands can be issued until ospi_reg_dev_size_config is written */
	return 0;
}


static int flashdrv_detectGeneric(int minor, flash_opParameters_t *res, unsigned char *device_id)
{
	const flash_ctrlParams_t *p = &controllerParams[minor];
	/* Set read command to 0x5a (READ SERIAL FLASH DISCOVERY PARAMETER) with 8 dummy cycles */
	*(p->ctrl + ospi_reg_dev_instr_rd_config) = (8 << 24) | 0x5a;
	*(p->ctrl + ospi_reg_dev_size_config) = (16 << 16) | (0x100 << 4) | 0x2;
	hal_cpuDataSyncBarrier();

	/* Now SFDP data is available in address space */
	if (flashdrv_parseSfdp(p->start, res, 0) < 0) {
		return -EINVAL;
	}

	return 0;
}


static int flashdrv_initGeneric(int minor)
{
	flash_opParameters_t *fp = &flashParams[minor].params;
	if (fp->addrMode == ADDRMODE_4BO) {
		flashdrv_writeEnable(minor, 1);
		flashdrv_performSimpleOp(minor, &opDef_enter_4byte, NULL);
	}

	return 0;
}


static int flashdrv_detectFlashType(unsigned int minor, flash_opParameters_t *res)
{
	flashdrv_fillDefaultParams(res);
	unsigned char *device_id = flashParams[minor].device_id;
	flashdrv_performSimpleOp(minor, &opDef_read_id, device_id);

	if ((device_id[0] == 0x20) && ((device_id[1] == 0xBA) || (device_id[1] == 0xBB))) {
		flashParams[minor].name = "Micron";
		flashParams[minor].init_fn = flashdrv_initMicron;
		return flashdrv_detectMicron(minor, res, device_id);
	}

	flashParams[minor].name = "generic";
	flashParams[minor].init_fn = flashdrv_initGeneric;
	return flashdrv_detectGeneric(minor, res, device_id);
}


static int flashdrv_isValidAddress(unsigned int minor, u32 off, size_t size)
{
	size_t fsize = flashParams[minor].size;
	if ((off < fsize) && ((off + size) <= fsize)) {
		return 1;
	}

	return 0;
}


static int flashdrv_isValidMinor(unsigned int minor)
{
	return minor < N_CONTROLLERS ? 1 : 0;
}


static void flashdrv_initPins(const flash_ctrlParams_t *p)
{
	tda4vm_pinConfig_t pin_cfg;

	/* Configure pins */
	pin_cfg.flags = TDA4VM_GPIO_PULL_DISABLE;
	pin_cfg.mux = p->pins.clk.mux;
	tda4vm_setPinConfig(p->pins.clk.pin, &pin_cfg);

	pin_cfg.mux = p->pins.cs.mux;
	tda4vm_setPinConfig(p->pins.cs.pin, &pin_cfg);

	pin_cfg.flags = TDA4VM_GPIO_PULL_DISABLE | TDA4VM_GPIO_RX_EN;
	pin_cfg.mux = p->pins.lbclko.mux;
	tda4vm_setPinConfig(p->pins.lbclko.pin, &pin_cfg);

	pin_cfg.mux = p->pins.dqs.mux;
	tda4vm_setPinConfig(p->pins.dqs.pin, &pin_cfg);

	for (unsigned i = 0; i < DAT_PINS; i++) {
		if (p->pins.dat[i].pin < 0) {
			break;
		}

		pin_cfg.mux = p->pins.dat[i].mux;
		tda4vm_setPinConfig(p->pins.dat[i].pin, &pin_cfg);
	}
}


static inline u32 flashdrv_makeInstructionRegister(u8 ioType, u8 opcode, u8 dummy, int isWrite)
{
	u32 res = opcode;
	switch (ioType) {
		case OPERATION_IO_222:
			res |= isWrite ? 0 : (1 << 8);
			/* Fall-through */
		case OPERATION_IO_122:
			res |= 1 << 12;
			/* Fall-through */
		case OPERATION_IO_112:
			res |= 1 << 16;
			break;

		case OPERATION_IO_444:
			res |= isWrite ? 0 : (2 << 8);
			/* Fall-through */
		case OPERATION_IO_144:
			res |= 2 << 12;
			/* Fall-through */
		case OPERATION_IO_114:
			res |= 2 << 16;
			break;

		default:
			break;
	}

	res |= (u32)dummy << 24;
	return res;
}


/* Below are functions for device's public interface */


static ssize_t flashdrv_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	(void)timeout;
	if ((flashdrv_isValidMinor(minor) == 0) || (flashdrv_isValidAddress(minor, offs, len) == 0)) {
		return -EINVAL;
	}

	hal_memcpy(buff, controllerParams[minor].start + offs, len);
	return (ssize_t)len;
}


static ssize_t flashdrv_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	if ((flashdrv_isValidMinor(minor) == 0) || (flashdrv_isValidAddress(minor, offs, len) == 0)) {
		return -EINVAL;
	}

	hal_memcpy(controllerParams[minor].start + offs, buff, len);
	return (ssize_t)len;
}


static ssize_t flashdrv_erase(unsigned int minor, addr_t offs, size_t len, unsigned int flags)
{
	flash_opDefinition_t op;
	u32 eraseSize;
	if (flashdrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	if (len == (size_t)-1) {
		flashdrv_writeEnable(minor, 1);
		flashdrv_performSimpleOp(minor, &opDef_chip_erase, NULL);
		if (flashdrv_waitForWriteCompletion(minor, ERASE_CHIP_TIMEOUT_MS) < 0) {
			return -ETIME;
		}

		return (ssize_t)flashParams[minor].size;
	}
	else if (flashdrv_isValidAddress(minor, offs, len) == 0) {
		return -EINVAL;
	}
	else {
		eraseSize = 1 << flashParams[minor].params.log_eraseSize;
		if ((offs & (eraseSize - 1)) != 0 || (len & (eraseSize - 1)) != 0) {
			return -EINVAL;
		}

		op.addrBytes = (flashParams[minor].params.addrMode == ADDRMODE_3B) ? 3 : 4;
		op.opcode = flashParams[minor].params.eraseOpcode;
		op.dummyCycles = 0;
		op.readBytes = 0;
		op.writeBytes = 0;
		for (; len != offs; offs += eraseSize) {
			flashdrv_writeEnable(minor, 1);
			op.addr = offs;
			flashdrv_performSimpleOp(minor, &op, NULL);
			if (flashdrv_waitForWriteCompletion(minor, ERASE_BLOCK_TIMEOUT_MS) < 0) {
				return -ETIME;
			}
		}

		return (ssize_t)len;
	}
}


static int flashdrv_done(unsigned int minor)
{
	if (flashdrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	return EOK;
}


static int flashdrv_sync(unsigned int minor)
{
	if (flashdrv_isValidMinor(minor) == 0)
		return -EINVAL;

	/* Nothing to do */
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
	fSz = flashParams[minor].size;
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
	u32 v, dev_size_config;
	const flash_ctrlParams_t *p;
	flash_opParameters_t *fp;

	if (flashdrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	p = &controllerParams[minor];
	fp = &flashParams[minor].params;
	flashdrv_initPins(p);

	*(p->ctrl + ospi_reg_config) &= ~1;                 /* Disable controller */
	tda4vm_setClksel(p->clksel.num, p->clksel.setting); /* Set reference clock source to selected */
	*(p->ctrl + ospi_reg_config) |= 0xf << 19;          /* Set maximum clock divider */
	*(p->ctrl + ospi_reg_config) |= 1;                  /* Enable controller */
	hal_cpuDataSyncBarrier();

	/* Read Flash memory details (JEDEC ID and SFDP)
	 * Some chips require SFDP to be read at lower speed. */
	ret = flashdrv_detectFlashType(minor, fp);
	if (ret < 0) {
		return ret;
	}

	while ((*(p->ctrl + ospi_reg_config) >> 31) == 0) {
		/* Wait until interface is idle */
	}

	/* Configure higher clocks. NOTE: this must be done before Flash initialization */
	*(p->ctrl + ospi_reg_config) &= ~1; /* Disable controller */
	hal_cpuDataSyncBarrier();
	v = *(p->ctrl + ospi_reg_config) & ~(0xf << 19); /* Set up clock divider */
	*(p->ctrl + ospi_reg_config) = v | (u32)(((p->divider / 2) - 1) & 0xf) << 19;
	*(p->ctrl + ospi_reg_config) |= 1; /* Enable controller */
	hal_cpuDataSyncBarrier();

	while ((*(p->ctrl + ospi_reg_config) >> 31) == 0) {
		/* Wait until interface is idle */
	}

	if (flashParams[minor].init_fn != NULL) {
		ret = flashParams[minor].init_fn(minor);
		if (ret < 0) {
			return ret;
		}
	}

	dev_size_config = (16 << 16) | (0x100 << 4); /* Block size: 64 KB, Page size: 256 B */
	dev_size_config = (fp->log_eraseSize << 16) | ((1 << fp->log_pageSize) << 4);
	if (fp->log_chipSize <= 31) {
		flashParams[minor].size = 1 << fp->log_chipSize;
	}
	else {
		flashParams[minor].size = 1 << 31;
	}

	*(p->ctrl + ospi_reg_dev_instr_rd_config) =
			flashdrv_makeInstructionRegister(fp->readIoType, fp->readOpcode, fp->readDummy, 0);

	*(p->ctrl + ospi_reg_dev_instr_wr_config) =
			flashdrv_makeInstructionRegister(fp->writeIoType, fp->writeOpcode, fp->writeDummy, 1);

	/* Limit size of the device to what's accessible */
	if (flashParams[minor].size > controllerParams[minor].size) {
		flashParams[minor].size = controllerParams[minor].size;
	}

	if ((fp->addrMode == ADDRMODE_3B) && (flashParams[minor].size > (1 << 24))) {
		/* If flash was not fully recognized, it may be larger than 16 MB, but we don't know
		 * how to put it into 4-byte addressing mode - so it's limited to 16 MB */
		flashParams[minor].size = (1 << 24);
	}

	if (flashParams[minor].size <= (1 << 27)) {
		dev_size_config |= (0 << 21);
	}
	else if (flashParams[minor].size <= (1 << 28)) {
		dev_size_config |= (1 << 21);
	}
	else if (flashParams[minor].size <= (1 << 29)) {
		dev_size_config |= (2 << 21);
	}
	else {
		dev_size_config |= (3 << 21);
	}

	dev_size_config |= (fp->addrMode == ADDRMODE_3B) ? 0x2 : 0x3;
	*(p->ctrl + ospi_reg_dev_size_config) = dev_size_config;

	lib_printf("\ndev/flash: Configured %s %dMB NOR flash(%d.%d)",
			flashParams[minor].name,
			flashParams[minor].size >> 20,
			DEV_STORAGE,
			minor);

	return EOK;
}


__attribute__((constructor)) static void flashdrv_reg(void)
{
	static const dev_ops_t opsFlashTDA4VM = {
		.read = flashdrv_read,
		.write = flashdrv_write,
		.erase = flashdrv_erase,
		.sync = flashdrv_sync,
		.map = flashdrv_map,
	};

	static const dev_t devFlashTDA4VM = {
		.name = "flash-tda4vm",
		.init = flashdrv_init,
		.done = flashdrv_done,
		.ops = &opsFlashTDA4VM,
	};

	devs_register(DEV_STORAGE, N_CONTROLLERS, &devFlashTDA4VM);
}
