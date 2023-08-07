/*
 * Phoenix-RTOS
 *
 * MEMory read/write tool
 *
 * Copyright 2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/lib.h>
#include <syspage.h>
#include "cmd.h"


static void cmd_memInfo(void)
{
	lib_printf("Reads or writes values from/to a range of memory addresses");
}


static void cmd_memUsage(void)
{
	lib_printf(
		"Usage: mem [options].. address\n"
		"\t-F        Disable validation of memory (map) regions\n"
		"\t-a align  Memory alignment (default=%u)\n"
		"\t-s size   Size of write/read (1,2,4,8)\n"
		"\t-c count  Number of repetitions with address step of (size)\n"
		"\t-w value  Write (value) to memory address\n"
		"\t-r        Read memory from address\n",
		(unsigned int)sizeof(addr_t));
}


static int region_validator(addr_t addr, size_t size, unsigned int attr)
{
	unsigned int mapAttr = 0u;

	if (syspage_mapRangeCheck(addr, addr + size, &mapAttr) == 0) {
		return 0;
	}

	return ((mapAttr & attr) == attr) ? 1 : 0;
}


static int memr(addr_t addr, size_t size, int forceValid)
{
	lib_printf("read 0x%p = ", (void *)addr);

	if ((forceValid != 0) || (region_validator(addr, size, mAttrRead) != 0)) {
		hal_cpuDataMemoryBarrier();
		switch (size) {
			case sizeof(u8):
				lib_printf("0x%02x\n", *(u8 *)addr);
				break;
			case sizeof(u16):
				lib_printf("0x%04x\n", *(u16 *)addr);
				break;
			case sizeof(u32):
				lib_printf("0x%08x\n", *(u32 *)addr);
				break;
			case sizeof(u64):
				lib_printf("0x%016llx\n", *(u64 *)addr);
				break;
			default:
				return -EINVAL;
		}
	}
	else {
		/* not an error */
		lib_printf("not readable\n");
	}

	return 0;
}


static int memw(addr_t addr, size_t size, u64 val, int forceValid)
{
	lib_printf("write 0x%p = ", (void *)addr);

	if ((forceValid != 0) || (region_validator(addr, size, mAttrWrite) != 0)) {
		switch (size) {
			case sizeof(u8):
				*(u8 *)addr = (u8)val;
				lib_printf("0x%02x\n", (u8)val);
				break;
			case sizeof(u16):
				*(u16 *)addr = (u16)val;
				lib_printf("0x%04x\n", (u16)val);
				break;
			case sizeof(u32):
				*(u32 *)addr = (u32)val;
				lib_printf("0x%08x\n", (u32)val);
				break;
			case sizeof(u64):
				*(u64 *)addr = (u64)val;
				lib_printf("0x%016llx\n", (u64)val);
				break;
			default:
				return -EINVAL;
		}
		hal_cpuDataMemoryBarrier();
	}
	else {
		/* not an error */
		lib_printf("not writeable\n");
	}

	return 0;
}


static int cmd_memMain(int argc, char **argv)
{
	u64 writeValue = 0uLL;
	char *end;
	addr_t addr = 0uLL;
	size_t count = 1uLL;
	size_t size = sizeof(addr_t);
	size_t align = sizeof(addr_t);
	int forceValid = 0;
	int read = 0;
	int write = 0;

	lib_printf("\n");

	for (;;) {
		int opt = lib_getopt(argc, argv, "a:c:Fhrs:w:");
		if (opt < 0) {
			break;
		}
		switch (opt) {
			case 'a':
				align = lib_strtoul(optarg, &end, 0);
				if ((optarg == end) || (*end != '\0')) {
					align = 0uLL;
				}
				break;

			case 'c':
				count = lib_strtoul(optarg, &end, 0);
				if ((optarg == end) || (*end != '\0')) {
					count = 0uLL;
				}
				break;

			case 'F':
				forceValid = 1;
				break;

			case 'r':
				read = 1;
				break;

			case 's':
				size = lib_strtoul(optarg, &end, 0);
				if ((optarg == end) || (*end != '\0')) {
					size = 0uLL;
				}
				break;

			case 'w':
				/*
				 * FIXME: missing functionality on 64bit cpu targets,
				 * change to lib_strtoull, when function is implemented.
				 */
				writeValue = (u64)lib_strtoul(optarg, &end, 0);
				if ((optarg != end) && (*end == '\0')) {
					write = 1;
				}
				break;

			case 'h':
				/* fall-through */
			default:
				cmd_memUsage();
				return CMD_EXIT_FAILURE;
		}
	}

	do {
		if (optind == argc) {
			lib_printf("No address provided\n");
		}
		else if ((align == 0uLL) || ((align & (align - 1uLL)) != 0uLL)) {
			lib_printf("Invalid alignment\n");
		}
		else if ((size == 0uLL) || ((size & (size - 1uLL)) != 0uLL)) {
			lib_printf("Invalid size\n");
		}
		else if (size > align) {
			lib_printf("Size grater than alignment\n");
		}
		else if (count == 0uLL) {
			lib_printf("Invalid count of repetition\n");
		}
		else if ((read ^ write) == 0) {
			lib_printf("Specify either read or write mode\n");
		}
		else {
			break;
		}

		cmd_memUsage();
		return CMD_EXIT_FAILURE;
	} while (0);

	/*
	 * FIXME: missing functionality on 64bit cpu targets,
	 * change to lib_strtoull, when function is implemented.
	 */
	addr = lib_strtoul(argv[optind], &end, 0);
	if ((argv[optind] == end) || (*end != '\0')) {
		lib_printf("Invalid address\n");
		cmd_memUsage();
		return CMD_EXIT_FAILURE;
	}

	addr &= ~(align - 1uLL);

	if (read != 0) {
		for (; count != 0uLL; --count) {
			if (memr(addr, size, forceValid) < 0) {
				lib_printf("Invalid read size\n");
				return CMD_EXIT_FAILURE;
			}
			addr += size;
		}
	}
	else if (write != 0) {
		for (; count != 0uLL; --count) {
			if (memw(addr, size, writeValue, forceValid) < 0) {
				lib_printf("Invalid write size\n");
				return CMD_EXIT_FAILURE;
			}
			addr += size;
		}
	}
	else {
	}

	return CMD_EXIT_SUCCESS;
}


__attribute__((constructor)) static void cmd_memReg(void)
{
	const static cmd_t app_cmd = { .name = "mem", .run = cmd_memMain, .info = cmd_memInfo };
	cmd_reg(&app_cmd);
}
