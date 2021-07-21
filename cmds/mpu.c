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
	lib_printf("prints the use of MPU regions, usage: mpu [all]");
}


static int cmd_mpu(int argc, char *argv[])
{
	const char *name;
	unsigned int i, regCnt;
	const mpu_region_t *region;
	const mpu_common_t *const mpu_common = mpu_getCommon();

	if (argc == 1) {
		regCnt = mpu_common->regCnt;
	}
	else if (argc == 2) {
		if (hal_strcmp(argv[1], "all") != 0) {
			log_error("\n%s: Wrong arguments", argv[0]);
			return -EINVAL;
		}

		regCnt = mpu_common->regMax;
	}
	else {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	if (mpu_common->regMax != sizeof(((hal_syspage_t *)0)->mpu.table) / sizeof(((hal_syspage_t *)0)->mpu.table[0])) {
		log_error("\n%s: MPU hal is not initialized or unsupported type was detected", argv[0]);
		return -EFAULT;
	}

	lib_printf(CONSOLE_BOLD "\n%-9s %-7s %-4s %-11s %-11s %-3s %-3s %-9s %-4s %-2s %-2s %-2s\n" CONSOLE_NORMAL,
		"MAP NAME", "REGION", "SUB", "START", "END", "EN", "XN", "PERM P/U", "TEX", "S", "C", "B");

	for (i = 0; i < regCnt; i++) {
		region = &mpu_common->region[i];
		if ((name = syspage_mapName(mpu_common->mapId[i])) == NULL)
			name = "<none>";

		mpu_regionPrint(name, region->rbar, region->rasr);
	}

	lib_printf("\nConfigured %d of %d MPU regions based on %d map definitions.\n",
		mpu_common->regCnt, mpu_common->regMax, mpu_common->mapCnt);

	return EOK;
}


__attribute__((constructor)) static void cmd_mpuReg(void)
{
	const static cmd_t app_cmd = { .name = "mpu", .run = cmd_mpu, .info = cmd_mpuInfo };

	cmd_reg(&app_cmd);
}
