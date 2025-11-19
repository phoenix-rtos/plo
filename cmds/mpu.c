/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Print MPU regions
 *
 * Copyright 2021 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <syspage.h>


static void mpu_attrPrint(u32 attr, int enabled)
{
	static const char *strAP[] = { "na/na", "rw/na", "rw/ro", "rw/rw", "reserved", "ro/na", "ro/ro", "ro/ro" };
	static const char strYesNo[] = { 'n', 'y' };

	/* Enabled */
	lib_printf("%c   ", strYesNo[enabled != 0]);

	/* eXec Never */
	lib_printf("%c   ", strYesNo[(attr >> 12) & 1ul]);

	/* Access Permission */
	lib_printf("%-10s", strAP[(attr >> 8) & 0x7ul]);

	/* TEX */
	lib_printf("%c", '0' + ((attr >> 5) & 1ul));
	lib_printf("%c", '0' + ((attr >> 4) & 1ul));
	lib_printf("%c  ", '0' + ((attr >> 3) & 1ul));

	/* Shareable */
	lib_printf("%c  ", strYesNo[(attr >> 2) & 1ul]);

	/* Cacheable */
	lib_printf("%c  ", strYesNo[(attr >> 1) & 1ul]);

	/* Bufferable */
	lib_printf("%c\n", strYesNo[attr & 1ul]);
}


/* Describe content of rbar & rasr registers */
static void mpu_regionPrint(const char *name, u32 rbar, u32 rasr)
{
	unsigned int subregion;
	u32 srSize, srBase = rbar & (0x7fffffful << 5);
	u32 attr = (rasr >> 16) & 0x173f;
	u8 sizeBit = ((rasr >> 1) & 0x1fu) + 1;
	u8 srdMask = rasr >> 8;
	u8 region = rbar & 0xfu;

	if (srdMask != 0) {
		srSize = 1u << (sizeBit - 3);

		for (subregion = 0; subregion < 8; subregion++) {
			if ((srdMask & (1u << subregion)) == 0) {
				lib_printf("%-9s %d%-5s%s %d%-3s 0x%08x  0x%08x%2s",
						name, region, "", region > 9 ? "" : " ", subregion, "", srBase, srBase + srSize - 1, "");
				mpu_attrPrint(attr, (rasr & 1));
			}

			srBase += srSize;
		}
	}
	else {
		lib_printf("%-9s %d%-5s%s %-4s 0x%08x  0x%08x%2s",
				name, region, "", region > 9 ? "" : " ", "all", srBase, srBase + (1ul << sizeBit) - 1, "");
		mpu_attrPrint(attr, (rasr & 1));
	}
}


static void cmd_mpuInfo(void)
{
	lib_printf("prints the use of MPU regions, usage: mpu [prog name]");
}


static int cmd_mpu(int argc, char *argv[])
{
	const char *name;
	unsigned int i;
	const syspage_prog_t *prog = syspage_progsGet();
	const syspage_prog_t *firstProg = prog;

	// if (mpu_common->regMax != sizeof(((hal_syspage_t *)0)->mpu.table) / sizeof(((hal_syspage_t *)0)->mpu.table[0])) {
	// 	log_error("\n%s: MPU hal is not initialized or unsupported type was detected", argv[0]);
	// 	return CMD_EXIT_FAILURE;
	// }

	lib_printf(CONSOLE_BOLD "\n%-9s %-7s %-4s %-11s %-11s %-3s %-3s %-9s %-4s %-2s %-2s %-2s\n" CONSOLE_NORMAL,
			"MAP NAME", "REGION", "SUB", "START", "END", "EN", "XN", "PERM P/U", "TEX", "S", "C", "B");

	if (prog == NULL) {
		lib_printf("\nApps number: 0");
		return CMD_EXIT_FAILURE;
	}

	do {
		// TODO: filter progname if argc > 1
		name = (prog->argv[0] == 'X') ? prog->argv + 1 : prog->argv;
		lib_printf("%-16s 0x%08x%4s 0x%08x", name, prog->start, "", prog->end);
		for (i = 0; i < prog->hal.allocCnt; i++) {
			name = syspage_mapName(prog->hal.map[i]);
			if (name == NULL) {
				name = "<none>";
			}
			mpu_regionPrint(name, prog->hal.table[i].rbar, prog->hal.table[i].rasr);
		}
		lib_printf("\n");
		prog = prog->next;
	} while (prog != firstProg);

	// lib_printf("\nConfigured %d of %d MPU regions.\n",
	// 		mpu->allocCnt, mpu_common->regMax);

	return CMD_EXIT_SUCCESS;
}


static const cmd_t mpu_cmd __attribute__((section("commands"), used)) = {
	.name = "mpu", .run = cmd_mpu, .info = cmd_mpuInfo
};
