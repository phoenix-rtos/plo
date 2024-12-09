/*
 * Phoenix-RTOS
 *
 * Partition table
 *
 * Copyright 2020, 2023 Phoenix Systems
 * Author: Hubert Buczynski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _LIB_PTABLE_H_
#define _LIB_PTABLE_H_

#include <hal/hal.h>

/* Changelog:
 * version 2: Add checksum and version fields
 */
#define PTABLE_VERSION 2


/* Structure of partition table
 *  _________________________________________________________________________
 * |       28 B      |                 32 B * n                |     4 B     |
 * |-----------------|-----------------------------------------|-------------|
 * | ptable_t header | ptable_part_t 0 | ... | ptable_part_t n | magic bytes |
 *  -------------------------------------------------------------------------
 *
 *  NOTE: data in partition table should be stored in little endian
 */


/* Partition table magic signature */
static const u8 ptable_magic[] = { 0xde, 0xad, 0xfc, 0xbe };


/* Supported partition types */
/* clang-format off */
enum { ptable_raw = 0x51, ptable_jffs2 = 0x72, ptable_meterfs = 0x75, ptable_futurefs = 0x78 };
/* clang-format on */

static const u8 ptable_knownTypes[] = { ptable_raw, ptable_jffs2, ptable_meterfs, ptable_futurefs };


typedef struct {
	u8 name[8];      /* Partition name */
	u32 offset;      /* Partition offset (in bytes) */
	u32 size;        /* Partition size (in bytes) */
	u8 type;         /* Partition type */
	u8 reserved[11]; /* Reserved bytes */
	u32 crc;         /* Partition checksum */
} ptable_part_t;


typedef struct {
	u32 count;              /* Number of partitions */
	u8 version;             /* Ptable struct version */
	u8 reserved[19];        /* Reserved bytes */
	u32 crc;                /* Header checksum */
	ptable_part_t parts[0]; /* Partitions */
} ptable_t;


static inline const char *ptable_typeName(int type)
{
	switch (type) {
		case ptable_raw: return "raw";
		case ptable_jffs2: return "jffs2";
		case ptable_meterfs: return "meterfs";
		case ptable_futurefs: return "futurefs";
		default: return NULL;
	}
};


/* Returns partition table size provided partition count */
static inline u32 ptable_size(u32 count)
{
	return sizeof(ptable_t) + count * sizeof(ptable_part_t) + sizeof(ptable_magic);
}


/* Converts partition table to host endianness and verifies it */
extern int ptable_deserialize(ptable_t *ptable, u32 memsz, u32 blksz);


/* Verifies partition table and converts it to little endian */
extern int ptable_serialize(ptable_t *ptable, u32 memsz, u32 blksz);


#endif
