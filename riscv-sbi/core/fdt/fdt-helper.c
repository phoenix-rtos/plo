/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * FDT parsing
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "fdt.h"
#include "fdt-internal.h"
#include "string.h"


static ssize_t fdt_findNodeByName(ssize_t offset, int *depth, const char *targetName, size_t targetNameLen, int targetDepth)
{
	const char *name;

	for (;;) {
		offset = fdt_nextNode(offset, depth);
		if (offset < 0) {
			return offset;
		}

		name = fdt_getNodeName(offset);
		if ((*depth == targetDepth) && (sbi_strncmp(targetName, name, targetNameLen) == 0)) {
			return offset;
		}
	}
}


static ssize_t fdt_findInCurrentNode(ssize_t offset, int *depth, const char *targetName, size_t targetNameLen, int targetDepth)
{
	const char *name;

	do {
		offset = fdt_nextNode(offset, depth);
		if (offset < 0) {
			return offset;
		}

		name = fdt_getNodeName(offset);
		if ((*depth == targetDepth) && (sbi_strncmp(targetName, name, targetNameLen) == 0)) {
			return offset;
		}
	} while (*depth >= targetDepth);

	return FDT_ENOTFOUND;
}


int fdt_parseCpus(void)
{
	int cpus = 0;
	int depth = -1;

	ssize_t offset = fdt_findNodeByName(0, &depth, "cpus", 4, 1);
	if (offset < 0) {
		return offset;
	}

	for (;;) {
		offset = fdt_findInCurrentNode(offset, &depth, "cpu@", 4, 2);
		if (offset >= 0) {
			cpus++;
		}
		else if (offset == FDT_ENOTFOUND) {
			break;
		}
		else {
			return offset;
		}
	}

	return cpus;
}


/* Get #address-cells and #size-cells from node */
static ssize_t fdt_getPeripheralAddressCells(ssize_t offset, int *depth, fdt_cellInfo_t *info, const char *node)
{
	static const fdt_cellInfo_t defaultCells = { .addressCells = 2, .sizeCells = 1 };
	fdt_cellInfo_t cells;
	int err;

	/* Get soc node info */
	offset = fdt_findNodeByName(offset, depth, node, 4, 1);
	if (offset < 0) {
		return offset;
	}

	err = fdt_getCellInfo(offset, &cells);
	if ((err != FDT_EOK) && (err != FDT_ENOTFOUND)) {
		return err;
	}

	/* Make sth out of what we have */
	if (err == FDT_ENOTFOUND) {
		/* No info, use default */
		sbi_memcpy(info, &defaultCells, sizeof(fdt_cellInfo_t));
	}
	else {
		sbi_memcpy(info, &cells, sizeof(fdt_cellInfo_t));
	}

	return offset;
}


static void fdt_getReg(sbi_reg_t *reg, fdt_prop_t *prop, fdt_cellInfo_t *cells)
{
	addr_t base;
	size_t size;
	if (cells->addressCells == 2) {
		base = fdt32_to_cpu(prop->data[0]);
		base <<= 32;
		base |= fdt32_to_cpu(prop->data[1]);
	}
	else {
		base = fdt32_to_cpu(prop->data[0]);
	}

	if (cells->sizeCells == 2) {
		size = fdt32_to_cpu(prop->data[cells->addressCells]);
		size <<= 32;
		size |= fdt32_to_cpu(prop->data[cells->addressCells + 1]);
	}
	else {
		size = fdt32_to_cpu(prop->data[cells->addressCells]);
	}

	reg->base = base;
	reg->size = size;
}


static ssize_t fdt_getPeripheralReg(ssize_t offset, int *depth, sbi_reg_t *reg, const char *compatible)
{
	fdt_cellInfo_t cells;
	fdt_prop_t *prop;

	offset = fdt_getPeripheralAddressCells(offset, depth, &cells, "soc");
	if (offset < 0) {
		return offset;
	}

	offset = fdt_findNodeByCompatible(offset, depth, compatible);
	if (offset < 0) {
		return offset;
	}

	prop = fdt_getProperty(offset, "reg");
	if (prop == NULL) {
		return FDT_EBADSTRUCT;
	}

	fdt_getReg(reg, prop, &cells);

	return offset;
}


int fdt_getUartInfo(uart_info_t *uart, const char *compatible)
{
	ssize_t offset = 0, clockOffset = 0;
	int depth = -1, clockDepth = -1;
	u32 phandle;
	fdt_prop_t *prop;

	offset = fdt_getPeripheralReg(offset, &depth, &uart->reg, compatible);
	if (offset < 0) {
		return offset;
	}

	prop = fdt_getProperty(offset, "clock-frequency");
	if (prop == NULL) {
		prop = fdt_getProperty(offset, "clocks");
		if (prop == NULL) {
			return FDT_EBADSTRUCT;
		}

		phandle = fdt32_to_cpu(*prop->data);
		clockOffset = fdt_findNodeByPhandle(0, &clockDepth, phandle);
		if (clockOffset < 0) {
			return clockOffset;
		}

		prop = fdt_getProperty(clockOffset, "clock-frequency");
		if (prop == NULL) {
			return FDT_EBADSTRUCT;
		}
	}
	uart->freq = fdt32_to_cpu(*prop->data);

	prop = fdt_getProperty(offset, "current-speed");
	if (prop != NULL) {
		uart->baud = fdt32_to_cpu(*prop->data);
	}
	else {
		uart->baud = 115200;
	}

	return FDT_EOK;
}


int fdt_getClintInfo(clint_info_t *clint)
{
	ssize_t offset = 0;
	int depth = -1;

	offset = fdt_getPeripheralReg(offset, &depth, &clint->reg, "riscv,clint0");
	if (offset < 0) {
		return offset;
	}

	return FDT_EOK;
}
