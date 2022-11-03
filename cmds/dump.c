/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Dump memory
 *
 * Copyright 2021-2022 Phoenix Systems
 * Author: Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <lib/lib.h>
#include <syspage.h>

#include <phfs/phfs.h>


static void cmd_dumpInfo(void)
{
	lib_consolePuts("dumps memory, usage: dump [-F|-r <phfs>] <addr> [<size>]");
}


static unsigned int region_validator(addr_t start, addr_t end, addr_t curr)
{
	unsigned int attr = 0;

	(void)start;
	(void)end;

	if (syspage_mapRangeCheck(curr, curr, &attr)) {
		attr = (attr & mAttrRead);
	}

	return attr != 0;
}


static void region_hexdump(addr_t start, addr_t end, addr_t offp, u8 align, unsigned int (*validator)(addr_t, addr_t, addr_t))
{
	unsigned int col;
	static const int cols = 16;
	addr_t ptr, offs = start;

	if (align != 0) {
		offp = (offp / cols) * cols;
		offs = (offs / cols) * cols;
	}

	while (offs < end) {
		lib_printf("0x%08x | ", (u32)offp);

		/* Print byte values */
		for (col = 0; col < cols; ++col) {
			ptr = offs + col;

			if ((ptr < start) || (ptr >= end)) {
				lib_consolePuts("   ");
				continue;
			}

			if ((validator != NULL) && (validator(start, end, ptr) == 0)) {
				lib_consolePuts("XX ");
			}
			else {
				lib_printf("%02x ", *(u8 *)ptr);
			}
		}
		lib_consolePuts("| ");

		/* Print "printable" representation */
		for (col = 0; col < cols; ++col) {
			ptr = offs + col;

			if ((ptr < start) || (ptr >= end)) {
				lib_consolePutc(' ');
				continue;
			}

			if ((validator != NULL) && (validator(start, end, ptr) == 0)) {
				lib_consolePutc('X');
			}
			else if (lib_isprint(*(u8 *)ptr)) {
				lib_printf("%c", *(u8 *)ptr);
			}
			else {
				lib_consolePutc('.');
			}
		}

		lib_consolePutc('\n');
		offp += cols;
		offs += cols;
	}
}


static void print_hline(void)
{
	unsigned int col;
	for (col = 0; col < 79; ++col) {
		lib_consolePutc('-');
	}
	lib_consolePutc('\n');
}


static int dump_memory(addr_t addr, size_t length, int validate)
{
	addr_t end = addr + length;

	lib_printf("\nMemory dump from 0x%x to 0x%x (%zu bytes):\n", (u32)addr, (u32)end, length);
	print_hline();

	region_hexdump(addr, end, addr, 1, validate ? region_validator : NULL);

	return EOK;
}


static int dump_phfs(const char *devname, addr_t addr, size_t length)
{
	u8 buf[256];
	handler_t h;
	size_t chunk, rsz = 0;

	int res = phfs_open(devname, NULL, PHFS_OPEN_RAWONLY, &h);
	if (res < 0) {
		lib_printf("\nUnable to open phfs device %s\n", devname);
		return res;
	}

	lib_printf("\nRead phfs %s device from 0x%x to 0x%x (%zu bytes):\n", devname, (u32)addr, (u32)addr + length, length);
	print_hline();

	do {
		chunk = (length - rsz) > sizeof(buf) ? sizeof(buf) : (length - rsz);
		res = phfs_read(h, addr + rsz, buf, chunk);
		if (res < 0) {
			lib_printf("\nCant read data\n", devname);
			return res;
		}
		region_hexdump((addr_t)buf, (addr_t)buf + res, addr + rsz, 0, NULL);
		rsz += res;
	} while (rsz < length);

	(void)phfs_close(h);

	return EOK;
}


static int cmd_dump(int argc, char *argv[])
{
	int validate = 1, memdump = 1, argn = 1;
	char *devname = NULL;
	char *endptr;
	addr_t start;
	size_t length = 0x100;

	while (argn < argc && argv[argn][0] == '-') {
		if (argv[argn][1] == '\0' || argv[argn][2] != '\0') {
			log_error("\n%s: Wrong arguments", argv[0]);
			return -EINVAL;
		}

		switch (argv[argn][1]) {
			case 'F':
				/* Force no validation */
				validate = 0;
				break;

			case 'r':
				/* Use phfs device to read */
				memdump = 0;
				if (argn + 1 < argc) {
					devname = argv[++argn];
					break;
				}
				/* fall-through */

			default:
				log_error("\n%s: Wrong arguments", argv[0]);
				return -EINVAL;
		}

		argn++;
	}

	if (argn >= argc) {
		log_error("\n%s: Wrong arguments", argv[0]);
		return -EINVAL;
	}

	start = lib_strtoul(argv[argn], &endptr, 16);
	if (*endptr != '\0') {
		log_error("\n%s: Wrong arguments", argv[0]);
		return -EINVAL;
	}

	argn++;
	if (argn < argc) {
		length = lib_strtoul(argv[argn], &endptr, 0);
		if ((*endptr != '\0') || (length == 0)) {
			log_error("\n%s: Wrong arguments", argv[0]);
			return -EINVAL;
		}
	}

	if (start + length < start) {
		length = (addr_t)(-1) - start;
	}

	if (memdump != 0) {
		dump_memory(start, length, validate);
	}
	else {
		dump_phfs(devname, start, length);
	}

	return EOK;
}


__attribute__((constructor)) static void cmd_dumpReg(void)
{
	const static cmd_t app_cmd = { .name = "dump", .run = cmd_dump, .info = cmd_dumpInfo };

	cmd_reg(&app_cmd);
}
