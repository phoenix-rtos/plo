/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * FDT internal definitions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _FDT_INTERNAL_H_
#define _FDT_INTERNAL_H_


#include "types.h"


#define FDT_MAGIC      0xd00dfeed
#define FDT_BEGIN_NODE 1
#define FDT_END_NODE   2
#define FDT_PROP       3
#define FDT_NOP        4
#define FDT_END        9


#define FDT_EOK        0
#define FDT_EBADOFFS   -1
#define FDT_EBADSTRUCT -2
#define FDT_EEND       -3
#define FDT_ENODEEND   -4
#define FDT_ENOTFOUND  -5


#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

#define fdt32_to_cpu(x) (__builtin_bswap32(x))
#define fdt64_to_cpu(x) (__builtin_bswap64(x))

#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

#define fdt32_to_cpu(x) (x)
#define fdt64_to_cpu(x) (x)

#else
#error "Unsupported byte order"
#endif


typedef struct {
	u32 token;
	u32 len;
	u32 nameoff;
	u32 data[];
} __attribute__((packed, aligned(4))) fdt_prop_t;


typedef struct {
	u32 addressCells;
	u32 sizeCells;
} __attribute__((packed, aligned(4))) fdt_cellInfo_t;


ssize_t fdt_nextNode(ssize_t offset, int *depth);


const char *fdt_getNodeName(ssize_t offset);


fdt_prop_t *fdt_getProperty(ssize_t offset, const char *name);


ssize_t fdt_findNodeByCompatible(ssize_t offset, int *depth, const char *compatible);


ssize_t fdt_findNodeByPhandle(ssize_t offset, int *depth, u32 phandle);


int fdt_getCellInfo(ssize_t offset, fdt_cellInfo_t *info);


#endif
