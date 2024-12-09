/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * partition table tool
 *
 * Copyright 2024 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <devices/devs.h>
#include <hal/hal.h>
#include <lib/lib.h>

#include <phfs/phfs.h>

#define CSI_RESET "\033[0m"
#define CSI_BOLD  "\033[1m"

#define PTABLE_HEADER_FORMAT "%2s %-10s %10s %10s %10s %10s   %-8s\n"
#define PTABLE_ENTRY_FORMAT  "%2u %-10s %10u %10u %10u %10u   %-12s\n"


static struct {
	ptable_t ptable[1024];
	size_t memsz;
	size_t blksz;
} ptable_common;


static void cmd_ptableInfo(void)
{
	lib_printf("print partition table, usage: ptable <dev> [<offset>]");
}


static int partPrint(ptable_t *p)
{
	const char *type;
	unsigned int i = p->count;
	unsigned int j = 0;

	lib_printf(
		"\n" CSI_BOLD PTABLE_HEADER_FORMAT CSI_RESET,
		"#", "Name", "Start", "End", "Blocks", "Size", "Type");

	while (i-- != 0) {
		ptable_part_t *entry = &p->parts[i];
		type = ptable_typeName(entry->type);
		lib_printf(
				PTABLE_ENTRY_FORMAT,
				++j, entry->name, entry->offset, entry->offset + entry->size,
				entry->size / ptable_common.blksz, entry->size,
				(type != NULL) ? type : "???");
	}

	return 0;
}


static int partRead(handler_t h, addr_t offs)
{
	u32 partCount;
	size_t ptableSize;

	int res = phfs_read(h, offs, &partCount, sizeof(partCount));
	if (res < 0) {
		log_error("\nCan't read data");
		return res;
	}

	if (partCount == 0) {
		log_error("\nNo partitions at offset 0x%x", offs);
		return 0;
	}

	ptableSize = ptable_size(partCount);
	if (ptableSize > sizeof(ptable_common.ptable)) {
		log_error("\nIncorrect partition table at offset 0x%x", offs);
		return -1;
	}

	res = phfs_read(h, offs, ptable_common.ptable, ptableSize);
	if (res < 0) {
		log_error("\nCan't read data");
		return res;
	}

	res = ptable_deserialize(ptable_common.ptable, ptable_common.memsz, ptable_common.blksz);
	if (res < 0) {
		log_error("\nIncorrect partition table at offset 0x%x", offs);
		return res;
	}


	return (int)partCount;
}


static int cmd_ptable(int argc, char *argv[])
{
	int res;
	addr_t offs = 0;
	char *endptr = NULL;
	handler_t h;

	unsigned int major;
	unsigned int minor;
	unsigned int prot;

	if ((argc != 2) && (argc != 3)) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	if (argc == 3) {
		offs = lib_strtoul(argv[2], &endptr, 0);
		if (argv[2] == endptr) {
			log_error("\n%s: Wrong arguments", argv[0]);
			return -EINVAL;
		}
	}

	if (phfs_devGet(argv[1], &major, &minor, &prot) != EOK) {
		lib_printf("\n%s: Invalid phfs name provided: %s\n", argv[0], argv[1]);
		return CMD_EXIT_FAILURE;
	}

	if (prot != phfs_prot_raw) {
		lib_printf("\n%s: Device %s does not use raw protocol\n", argv[0], argv[1]);
		return CMD_EXIT_FAILURE;
	}

	if ((devs_control(major, minor, DEV_CONTROL_GETPROP_TOTALSZ, &ptable_common.memsz) != EOK) ||
		(devs_control(major, minor, DEV_CONTROL_GETPROP_BLOCKSZ, &ptable_common.blksz) != EOK)) {
		lib_printf("\n%s: Unable to get %s device properties: %s\n", argv[0], argv[1]);
		return CMD_EXIT_FAILURE;
	}

	if (argc != 3) {
		/* by default ptable is located in the last sector of raw device */
		offs = ptable_common.memsz - ptable_common.blksz;
	}

	lib_printf("\nDevice size: %zu", ptable_common.memsz);
	lib_printf("\nBlock size:  %zu\n", ptable_common.blksz);

	if (phfs_open(argv[1], NULL, PHFS_OPEN_RAWONLY, &h) < 0) {
		lib_printf("\n%s: Invalid phfs name provided: %s\n", argv[0], argv[1]);
		return CMD_EXIT_FAILURE;
	}

	res = partRead(h, offs);

	(void)phfs_close(h);

	if (res <= 0) {
		log_error("\n%s: Missing partition table at offset %zu", argv[0], offs);
		return CMD_EXIT_FAILURE;
	}

	if (res > 0) {
		partPrint(ptable_common.ptable);
		lib_printf("\nPartition table at offset: %zu\n", offs);
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t ptable_cmd __attribute__((section("commands"), used)) = {
	.name = "ptable", .run = cmd_ptable, .info = cmd_ptableInfo
};
