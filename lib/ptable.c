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

#include "lib.h"


#define le32toh(a) (a)
#define htole32(a) (a)


const char *ptable_typeName(int type)
{
	switch (type) {
		case ptable_raw: return "raw";
		case ptable_jffs2: return "jffs2";
		case ptable_meterfs: return "meterfs";
		default: return "unknown";
	}
}


static inline u32 ptable_crc32(const void *data, size_t len)
{
	return ~lib_crc32(data, len, 0xffffffff);
}


static int ptable_partVerify(const ptable_t *ptable, const ptable_part_t *part, u32 memsz, u32 blksz, int crcCheck)
{
	const ptable_part_t *p;
	size_t i;
	u32 size = le32toh(part->size);
	u32 offset = le32toh(part->offset);

	/* Verify partition checksum */
	if ((crcCheck != 0) && (le32toh(part->crc) != ptable_crc32(part, offsetof(ptable_part_t, crc)))) {
		return -1;
	}

	/* Verify offset and size */
	if (size == 0) {
		return -1;
	}

	if (size % blksz != 0) {
		return -1;
	}

	if (offset % blksz != 0) {
		return -1;
	}

	if (offset + size > memsz) {
		return -1;
	}

	/* Check for overflow */
	if (offset + size < offset) {
		return -1;
	}

	/* Verify partition type */
	switch (part->type) {
		case ptable_raw:
		case ptable_jffs2:
		case ptable_meterfs:
			break;

		default:
			return -1;
	}

	/* Verify partition name */
	for (i = 0; i < sizeof(part->name); i++) {
		if (!lib_isalnum(part->name[i])) {
			break;
		}
	}

	if ((i == 0) || (i >= sizeof(part->name)) || (part->name[i] != '\0')) {
		return -1;
	}

	/* Compare against previous partitions */
	for (p = ptable->parts; p != part; p++) {
		/* Check for range overlap */
		if ((offset <= le32toh(p->offset) + le32toh(p->size) - 1) && (offset + size - 1 >= le32toh(p->offset))) {
			return -1;
		}

		/* Check for name duplicate */
		if (hal_strcmp((const char *)part->name, (const char *)p->name) == 0) {
			return -1;
		}
	}

	return 0;
}


static int ptable_verify(const ptable_t *ptable, u32 memsz, u32 blksz)
{
	u32 size, i;
	int crcCheck = 1;
	u32 count = le32toh(ptable->count);

	if (ptable->version == 0 || ptable->version == 1 || ptable->version == 0xff) {
		/* Disable CRC check for legacy ptables */
		crcCheck = 0;
	}

	if (crcCheck != 0) {
		/* Verify header checksum */
		if (le32toh(ptable->crc) != ptable_crc32(ptable, offsetof(ptable_t, crc))) {
			return -1;
		}
	}

	/* Verify partition table size */
	size = ptable_size(count);
	if (size > blksz) {
		return -1;
	}

	/* Verify magic signature */
	if (hal_memcmp((const u8 *)ptable + size - sizeof(ptable_magic), ptable_magic, sizeof(ptable_magic)) != 0) {
		return -1;
	}

	/* Verify partitions */
	for (i = 0; i < count; i++) {
		if (ptable_partVerify(ptable, ptable->parts + i, memsz, blksz, crcCheck) < 0) {
			return -1;
		}
	}

	return 0;
}


int ptable_deserialize(ptable_t *ptable, u32 memsz, u32 blksz)
{
	int ret;
	u32 i;

	if (ptable == NULL) {
		return -1;
	}

	/* CRC must be verified with data in little endian */
	ret = ptable_verify(ptable, memsz, blksz);
	if (ret < 0) {
		return ret;
	}

	ptable->count = le32toh(ptable->count);
	ptable->crc = le32toh(ptable->crc);

	for (i = 0; i < ptable->count; i++) {
		ptable->parts[i].offset = le32toh(ptable->parts[i].offset);
		ptable->parts[i].size = le32toh(ptable->parts[i].size);
		ptable->parts[i].crc = le32toh(ptable->parts[i].crc);
	}


	return ret;
}


int ptable_serialize(ptable_t *ptable, u32 memsz, u32 blksz)
{
	u32 i;

	if (ptable == NULL) {
		return -1;
	}

	ptable->version = PTABLE_VERSION;

	/* Calculate checksums */
	ptable->crc = ptable_crc32(ptable, offsetof(ptable_t, crc));
	for (i = 0; i < ptable->count; i++) {
		ptable->parts[i].crc = ptable_crc32(ptable->parts + i, offsetof(ptable_part_t, crc));
	}

	/* Add magic signature */
	hal_memcpy((u8 *)ptable + ptable_size(ptable->count) - sizeof(ptable_magic), ptable_magic, sizeof(ptable_magic));

	if (ptable_verify(ptable, memsz, blksz) < 0) {
		return -1;
	}

	for (i = 0; i < ptable->count; i++) {
		ptable->parts[i].offset = htole32(ptable->parts[i].offset);
		ptable->parts[i].size = htole32(ptable->parts[i].size);
		ptable->parts[i].crc = htole32(ptable->parts[i].crc);
	}

	ptable->count = htole32(ptable->count);
	ptable->crc = htole32(ptable->crc);

	return 0;
}
