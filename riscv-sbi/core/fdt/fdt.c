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

#include "fdt-internal.h"
#include "string.h"


#define CEIL(x, y) (((x) + (y) - 1) & ~((y) - 1))


typedef struct {
	u32 magic;
	u32 totalsize;
	u32 off_dt_struct;
	u32 off_dt_strings;
	u32 off_mem_rsvmap;
	u32 version;
	u32 last_comp_version;
	u32 boot_cpuid_phys;
	u32 size_dt_strings;
	u32 size_dt_struct;
} __attribute__((packed, aligned(4))) fdt_header_t;


static struct {
	const void *fdt;
	const void *dt_struct;
	const void *dt_strings;
} fdt_common;


static const void *fdt_get_off_dt_struct(const void *fdt)
{
	fdt_header_t *header = (fdt_header_t *)fdt;

	return (u8 *)fdt + fdt32_to_cpu(header->off_dt_struct);
}


static const void *fdt_get_off_dt_strings(const void *fdt)
{
	fdt_header_t *header = (fdt_header_t *)fdt;

	return (u8 *)fdt + fdt32_to_cpu(header->off_dt_strings);
}


static u32 fdt_consumeProp(const u32 *pos)
{
	const fdt_prop_t *p = (const fdt_prop_t *)pos;

	return sizeof(*p) + CEIL(fdt32_to_cpu(p->len), sizeof(u32));
}


static const u32 *fdt_tryConsumeNode(const u32 *pos, ssize_t offset, int *depth)
{
	u8 *p;

	if (fdt32_to_cpu(*pos) == FDT_BEGIN_NODE) {
		if ((depth != NULL) && (offset == 0)) {
			(*depth)++;
		}
		pos++;
		p = (u8 *)pos;
		/* Consume node name */
		while (*p++ != '\0') { }
		pos = (void *)CEIL((addr_t)p, sizeof(u32));
	}

	return pos;
}


/* Returns the offset of the next node in the device tree */
ssize_t fdt_nextNode(ssize_t offset, int *depth)
{
	const u32 *nextPos = (u32 *)((u8 *)fdt_common.dt_struct + offset);
	const u32 *pos;
	u32 token;

	if ((size_t)offset >= (fdt32_to_cpu(((fdt_header_t *)fdt_common.fdt)->size_dt_struct)) || ((offset % sizeof(u32)) != 0)) {
		return FDT_EBADOFFS;
	}

	if ((offset != 0) && (fdt32_to_cpu(*nextPos) != FDT_BEGIN_NODE)) {
		return FDT_EBADOFFS;
	}

	nextPos = fdt_tryConsumeNode(nextPos, offset, depth);

	do {
		pos = nextPos;
		token = fdt32_to_cpu(*nextPos++);
		switch (token) {
			case FDT_BEGIN_NODE:
				(*depth)++;
				break;

			case FDT_END_NODE:
				if (*depth < 0) {
					return FDT_EBADSTRUCT;
				}
				(*depth)--;
				break;

			case FDT_END:
				return FDT_EEND;

			case FDT_PROP:
				nextPos = (u32 *)((addr_t)pos + fdt_consumeProp(pos));
				break;

			case FDT_NOP:
			default:
				break;
		}
	} while (token != FDT_BEGIN_NODE);

	return (ssize_t)((addr_t)pos - (addr_t)fdt_common.dt_struct);
}


const char *fdt_getNodeName(ssize_t offset)
{
	const u32 *pos = (u32 *)((u8 *)fdt_common.dt_struct + offset);
	if (fdt32_to_cpu(*pos) != FDT_BEGIN_NODE) {
		return NULL;
	}

	return (const char *)(pos + 1);
}


/* Returns offset of the property with given name in current node */
static ssize_t fdt_getPropertyByName(ssize_t offset, const char *name)
{
	const u32 *nextPos = (u32 *)((u8 *)fdt_common.dt_struct + offset);
	const u32 *pos;
	fdt_prop_t *prop;
	const char *propName;
	u32 token;

	nextPos = fdt_tryConsumeNode(nextPos, offset, NULL);

	for (;;) {
		pos = nextPos;
		token = fdt32_to_cpu(*nextPos++);
		switch (token) {
			case FDT_PROP:
				prop = (fdt_prop_t *)pos;
				propName = fdt_common.dt_strings + fdt32_to_cpu(prop->nameoff);
				if (sbi_strcmp(propName, name) == 0) {
					return (ssize_t)((addr_t)pos - (addr_t)fdt_common.dt_struct);
				}
				nextPos = (u32 *)((addr_t)pos + fdt_consumeProp(pos));
				break;

			case FDT_BEGIN_NODE:
			case FDT_END_NODE:
			case FDT_END:
				return FDT_ENOTFOUND;

			case FDT_NOP:
			default:
				break;
		}
	}
}


fdt_prop_t *fdt_getProperty(ssize_t offset, const char *name)
{
	ssize_t propOffset = fdt_getPropertyByName(offset, name);
	if (propOffset < 0) {
		return NULL;
	}

	return (fdt_prop_t *)((u8 *)fdt_common.dt_struct + propOffset);
}


ssize_t fdt_findNodeByCompatible(ssize_t offset, int *depth, const char *compatible)
{
	fdt_prop_t *prop;
	ssize_t propOffset;
	size_t propLen;
	const char *propData, *currStr;

	offset = fdt_nextNode(offset, depth);
	if (offset < 0) {
		return offset;
	}

	while (offset >= 0) {
		propOffset = fdt_getPropertyByName(offset, "compatible");
		if (propOffset >= 0) {
			prop = (fdt_prop_t *)((u8 *)fdt_common.dt_struct + propOffset);
			propData = (const char *)prop->data;
			propLen = fdt32_to_cpu(prop->len);

			currStr = propData;
			while (currStr < propData + propLen) {
				if (sbi_strcmp(currStr, compatible) == 0) {
					return offset;
				}
				currStr += sbi_strlen(currStr) + 1;
			}
		}
		offset = fdt_nextNode(offset, depth);
	}

	return FDT_ENOTFOUND;
}


ssize_t fdt_findNodeByPhandle(ssize_t offset, int *depth, u32 phandle)
{
	fdt_prop_t *prop;
	ssize_t propOffset;
	const u32 *propData;
	u32 propLen;

	offset = fdt_nextNode(offset, depth);
	if (offset < 0) {
		return offset;
	}

	while (offset >= 0) {
		propOffset = fdt_getPropertyByName(offset, "phandle");
		if (propOffset >= 0) {
			prop = (fdt_prop_t *)((u8 *)fdt_common.dt_struct + propOffset);
			propData = prop->data;
			propLen = fdt32_to_cpu(prop->len);

			if (propLen == sizeof(u32) && fdt32_to_cpu(*propData) == phandle) {
				return offset;
			}
		}
		offset = fdt_nextNode(offset, depth);
	}

	return FDT_ENOTFOUND;
}


int fdt_getCellInfo(ssize_t offset, fdt_cellInfo_t *info)
{
	ssize_t propOffset;
	fdt_prop_t *prop;
	const u32 *pos = (u32 *)((u8 *)fdt_common.dt_struct + offset);

	if (fdt32_to_cpu(*pos) != FDT_BEGIN_NODE) {
		return FDT_EBADOFFS;
	}

	propOffset = fdt_getPropertyByName(offset, "#address-cells");
	if (propOffset < 0) {
		return propOffset;
	}
	prop = (fdt_prop_t *)((u8 *)fdt_common.dt_struct + propOffset);
	info->addressCells = fdt32_to_cpu(*prop->data);

	propOffset = fdt_getPropertyByName(offset, "#size-cells");
	if (propOffset < 0) {
		return propOffset;
	}

	prop = (fdt_prop_t *)((u8 *)fdt_common.dt_struct + propOffset);
	info->sizeCells = fdt32_to_cpu(*prop->data);

	return FDT_EOK;
}


void fdt_init(const void *fdt)
{
	fdt_common.fdt = fdt;
	fdt_common.dt_struct = fdt_get_off_dt_struct(fdt);
	fdt_common.dt_strings = fdt_get_off_dt_strings(fdt);
}
