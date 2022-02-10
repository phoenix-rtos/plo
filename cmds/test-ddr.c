/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Test DDR - code derived form phoenix-rtos-kernel/hal/armv7a/memtest.c
 *
 * Copyright 2015, 2018, 2021 Phoenix Systems
 * Author: Jakub Sejdak, Aleksander Kaminski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>


#define BANK_COUNT        8
#define BANK_SELECT_MASK  0x3800
#define BANK_SELECT_SHIFT 11
#define BANK_SET(x)       (((u32)(((u32)(x)) << BANK_SELECT_SHIFT)) & BANK_SELECT_MASK)
#define BANK_GET(x)       (((u32)(((u32)(x)) & BANK_SELECT_MASK)) >> BANK_SELECT_SHIFT)

#define COLUMN_COUNT        1024
#define COLUMN_SELECT_MASK  0x7fe
#define COLUMN_SELECT_SHIFT 1
#define COLUMN_SET(x)       (((u32)(((u32)(x)) << COLUMN_SELECT_SHIFT)) & COLUMN_SELECT_MASK)
#define COLUMN_GET(x)       (((u32)(((u32)(x)) & COLUMN_SELECT_MASK)) >> COLUMN_SELECT_SHIFT)

#define ROW_COUNT        8192
#define ROW_SELECT_MASK  0x7ffc000
#define ROW_SELECT_SHIFT 14
#define ROW_SET(x)       (((u32)(((u32)(x)) << ROW_SELECT_SHIFT)) & ROW_SELECT_MASK)
#define ROW_GET(x)       (((u32)(((u32)(x)) & ROW_SELECT_MASK)) >> ROW_SELECT_SHIFT)


/* DDR3 RAM Addressing layout
 * Physical address:
 *                   0|000 0000 0000 00|00 0|000 0000 0000 000|0
 *        chip select |       row      |bank|      column     | data path
 *            1b      |       13b      | 3b |       10b       | 1b
 */

static const u16 patterns[] = { 0x5555, ~0x5555, 0x3333, ~0x3333, 0x0f0f, ~0x0f0f, 0x00ff, ~0x00ff };


static void cmd_ddrInfo(void)
{
	lib_printf("perform test DDR, usage: test-ddr");
}


static int cmd_ddrByteAccessibility(u32 ddrAddr, u32 size)
{
	u32 i;
	u8 val;
	int errs = 0;
	volatile u8 *ddr = (u8 *)ddrAddr;

	/* write_value overflow here is intentional */
	for (i = 0, val = 0; i < size; ++i, ++val)
		ddr[i] = val;

	for (i = 0, val = 0; i < size; ++i, ++val) {
		if (ddr[i] != val)
			++errs;
	}

	return errs;
}


static int cmd_ddrWordAccessibility(u32 ddrAddr, u32 size)
{
	volatile u32 *ddr = (u32 *)ddrAddr;
	u32 i, val;
	int errs = 0;

	for (i = 0; i < (size >> 2); ++i) {
		val = i << 2;
		ddr[i] = val;
	}

	for (i = 0; i < (size >> 2); ++i) {
		val = i << 2;
		if (ddr[i] != val)
			++errs;
	}

	return errs;
}


int cmd_ddrAccessibility(u32 ddrAddr, u32 size)
{
	int errs;

	errs = cmd_ddrByteAccessibility(ddrAddr, size);
	errs += cmd_ddrWordAccessibility(ddrAddr, size);

	return errs;
}


u16 cmd_generateTestVector(int pattern, int column)
{
	int flipShift = (pattern - 8) >> 1;
	u16 vector = 0;
	int flip;

	if (pattern < sizeof(patterns) / sizeof(patterns[0]))
		return patterns[pattern];

	/* Starting from pattern 8, vector for each column has to be changed */
	if (pattern & 1)
		vector = ~vector;

	if (!flipShift) {
		if (column & 1)
			vector = ~vector;
	}
	else {
		flip = column >> flipShift;
		if (flip & 1)
			vector = ~vector;
	}

	return vector;
}


int cmd_ddrBitCrossTalk(u32 ddrAddr)
{
	int bank, row, column, pattern;
	volatile u16 *addr;
	int errs = 0;

	for (bank = 0; bank < BANK_COUNT; ++bank) {
		lib_printf("\n- cross talk: Testing bank #%d", bank);

		for (pattern = 0; pattern < 30; ++pattern) {
			/* Write test pattern vectors */
			for (row = 0; row < ROW_COUNT; ++row) {
				for (column = 0; column < COLUMN_COUNT; ++column) {
					addr = (u16 *)(ddrAddr | ROW_SET(row) | BANK_SET(bank) | COLUMN_SET(column));
					*addr = cmd_generateTestVector(pattern, column);
				}
			}

			/* Read and compare test vectors */
			for (row = 0; row < ROW_COUNT; ++row) {
				for (column = 0; column < COLUMN_COUNT; ++column) {
					addr = (u16 *)(ddrAddr | ROW_SET(row) | BANK_SET(bank) | COLUMN_SET(column));
					if (*addr != cmd_generateTestVector(pattern, column))
						++errs;
				}
			}
		}
	}

	return errs;
}

/* TODO: Don't hardcode BANK_COUNT, COLUMN_COUNT, ROW_COUNT and others.
*        They should be defined separately for each memory. */
int cmd_ddrBitChargeLeakage(u32 ddrAddr)
{
	int bank, row, column, i;
	volatile u16 *addr;
	volatile u16 val;
	int errs = 0;

	for (bank = 0; bank < BANK_COUNT; ++bank) {
		lib_printf("\n- charge leakage: Testing bank #%d", bank);

		for (row = 1; row < ROW_COUNT - 1; ++row) {
			/* Write pattern to tested row */
			for (column = 0; column < COLUMN_COUNT; ++column) {
				addr = (u16 *)(ddrAddr | ROW_SET(row) | BANK_SET(bank) | COLUMN_SET(column));
				*addr = 0xffff;
			}

			/* Read multiple times neighboring rows */
			for (i = 0; i < 10000; ++i) {
				/* Read previous row */
				addr = (u16 *)(ddrAddr | ROW_SET(row - 1) | BANK_SET(bank) | COLUMN_SET(0));
				val = *addr;

				/* Read next row */
				addr = (u16 *)(ddrAddr | ROW_SET(row + 1) | BANK_SET(bank) | COLUMN_SET(0));
				val = *addr;
			}

			/* Check if tested row has changed value */
			for (column = 0; column < COLUMN_COUNT; ++column) {
				addr = (u16 *)(ddrAddr | ROW_SET(row) | BANK_SET(bank) | COLUMN_SET(column));
				val = *addr;
				if (val != 0xffff)
					++errs;
			}
		}
	}

	return errs;
}


static void cmd_errPrint(int err)
{
	lib_printf(CONSOLE_NORMAL);

	if (err)
		log_error("\nFAILED  %d", err);
	else
		lib_printf(CONSOLE_GREEN "\nPASSED");

	lib_printf(CONSOLE_NORMAL);
}


static int cmd_ddr(int argc, char *argv[])
{
	int err;

	if (argc != 1) {
		log_error("\n%s: Command does not accept arguments", argv[0]);
		return -EINVAL;
	}

	lib_printf(CONSOLE_BOLD "\nTest are executing, please wait ...");
	lib_printf("\n1/3 Accessibility test" CONSOLE_NORMAL);
	err = cmd_ddrAccessibility(ADDR_DDR, SIZE_DDR);
	cmd_errPrint(err);

	lib_printf(CONSOLE_BOLD "\n2/3 Crosstalk test:" CONSOLE_NORMAL);
	err = cmd_ddrBitCrossTalk(ADDR_DDR);
	cmd_errPrint(err);

	lib_printf(CONSOLE_BOLD "\n3/3 Leakage test:" CONSOLE_NORMAL);
	err = cmd_ddrBitChargeLeakage(ADDR_DDR);
	cmd_errPrint(err);

	return EOK;
}


__attribute__((constructor)) static void cmd_ddrReg(void)
{
	const static cmd_t ddrCmd = { .name = "test-ddr", .run = cmd_ddr, .info = cmd_ddrInfo };

	cmd_reg(&ddrCmd);
}
