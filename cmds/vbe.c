/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * VESA VBE modesetting
 *
 * Copyright 2024 Phoenix Systems
 * Author: Adam Greloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <lib/lib.h>
#include <syspage.h>


#define PTR_TO_SEG(ptr)       ((addr_t)ptr >> 4)
#define SEGOFS_TO_PTR(segofs) ((addr_t)((segofs & 0xffff0000) >> 12) + (segofs & 0xffff))

#define SUPPORTS_LFB (0x80)


typedef struct {
	u16 width;
	u16 height;
	u16 bpp;
} video_mode_t;


/* Structure definitions follow "VESA BIOS EXTENSION (VBE) Core Functions
 * Standard Version: 3.0", Video Electronics Standards Association, 1998 */

typedef struct {
	char VbeSignature[4];
	u16 VbeVersion;
	u32 OemStringPtr;
	u32 Capabilities;
	u32 VideoModePtr;
	u16 TotalMemory;

	u16 OemSoftwareRev;
	u32 OemVendorNamePtr;
	u32 OemProductNamePtr;
	u32 OemProductRevPtr;
	char Reserved[222];

	char OemData[256];
} __attribute__((packed)) vbe_info_block_t;


typedef struct {
	/* Mandatory information for all VBE revisions */
	u16 ModeAttributes;
	u8 WinAAttributes;
	u8 WinBAttributes;
	u16 WinGranularity;
	u16 WinSize;
	u16 WinASegment;
	u16 WinBSegment;
	u32 WinFuncPtr;
	u16 BytesPerScanLine;

	/* Mandatory information for VBE 1.2 and above */
	u16 XResolution;
	u16 YResolution;
	u8 XCharSize;
	u8 YCharSize;
	u8 NumberOfPlanes;
	u8 BitsPerPixel;
	u8 NumberOfBanks;
	u8 MemoryModel;
	u8 BankSize;
	u8 NumberOfImagePages;
	u8 Reserved0;

	u8 RedMaskSize;
	u8 RedFieldPosition;
	u8 GreenMaskSize;
	u8 GreenFieldPosition;
	u8 BlueMaskSize;
	u8 BlueFieldPosition;
	u8 RsvdMaskSize;
	u8 RsvdFieldPosition;
	u8 DirectColorModeInfo;

	/* Mandatory information for VBE 2.0 and above */
	u32 PhysBasePtr;
	u8 Reserved1[212];
} __attribute__((packed)) vbe_mode_info_block_t;


static struct {
	video_mode_t preferred;
	video_mode_t fallback;
	vbe_info_block_t *info;
	vbe_mode_info_block_t *modeInfo;
} vbe_common = {
	.preferred = { 1280, 800, 32 },
	.fallback = { 1024, 768, 32 },

	/* ADDR_VBE_INFO, ADDR_VBE_MODE_INFO should be aligned to 0x10, so that they can
	 * be referenced from 8086 emulation mode using segment register only (es=(ADDR >> 4), di=0) */
	.info = (vbe_info_block_t *)ADDR_VBE_INFO,
	.modeInfo = (vbe_mode_info_block_t *)ADDR_VBE_MODE_INFO,
};


static int _vbe_infoGet(void)
{
	int ax;

	/* indicate support for VBE 2.0+ */
	hal_memset(vbe_common.info, 0, sizeof(vbe_info_block_t));
	hal_strcpy(vbe_common.info->VbeSignature, "VBE2");

	ax = 0x4f00;

	/* clang-format off */
	__asm__ volatile (
		"pushl $0x10;\n\t"
		"pushl $0x0;\n\t"
		"pushl %1;\n\t"
		"xorw %%di, %%di;\n\t"
		"call _interrupts_bios;\n\t"
		"addl $0xc, %%esp;\n\t"
		: "+a" (ax)
		: "r" (PTR_TO_SEG(vbe_common.info))
		: "edi", "memory", "cc");
	/* clang-format on */

	return ax == 0x4f ? 0 : -1;
}


static int _vbe_modeInfoGet(u16 mode)
{
	int ax;

	hal_memset(vbe_common.modeInfo, 0, sizeof(vbe_info_block_t));

	ax = 0x4f01;

	/* clang-format off */
	__asm__ volatile (
		"pushl $0x10;\n\t"
		"pushl $0x0;\n\t"
		"pushl %1;\n\t"
		"xorw %%di, %%di;\n\t"
		"call _interrupts_bios;\n\t"
		"addl $0xc, %%esp;\n\t"
		: "+a" (ax)
		: "r" (PTR_TO_SEG(vbe_common.modeInfo)), "cx" (mode)
		: "edi", "memory", "cc");
	/* clang-format on */

	return ax == 0x4f ? 0 : -1;
}


static int _vbe_modeSet(u16 mode)
{
	int ax = 0x4f02;

	mode |= (1 << 14);  /* enable linear framebuffer */
	mode &= ~(1 << 15); /* don't clear the screen */

	/* clang-format off */
	__asm__ volatile (
		"pushl $0x10;\n\t"
		"pushl $0x0;\n\t"
		"pushl $0x0;\n\t"
		"xorw %%di, %%di;\n\t"
		"call _interrupts_bios;\n\t"
		"addl $0xc, %%esp;\n\t"
		: "+a" (ax)
		: "b" (mode)
		: "edi", "memory", "cc");
	/* clang-format on */

	return ax == 0x4f ? 0 : -1;
}


static int _vbe_modeFind(graphmode_t *picked)
{
	int i, err;

	u16 *modeIds = (u16 *)SEGOFS_TO_PTR(vbe_common.info->VideoModePtr);

	int fallbackModeId = -1;
	graphmode_t fallback = { 0 };

	for (i = 0; modeIds[i] != 0xffff; i++) {
		err = _vbe_modeInfoGet(modeIds[i]);
		if (err < 0) {
			return -1;
		}

		if ((vbe_common.modeInfo->ModeAttributes & SUPPORTS_LFB) != 0) {
			if ((vbe_common.modeInfo->XResolution == vbe_common.preferred.width) && (vbe_common.modeInfo->YResolution == vbe_common.preferred.height) && (vbe_common.modeInfo->BitsPerPixel == vbe_common.preferred.bpp)) {
				picked->width = vbe_common.modeInfo->XResolution;
				picked->height = vbe_common.modeInfo->YResolution;
				picked->bpp = vbe_common.modeInfo->BitsPerPixel;
				picked->pitch = vbe_common.modeInfo->BytesPerScanLine;
				picked->framebuffer = vbe_common.modeInfo->PhysBasePtr;

				return modeIds[i];
			}

			if ((vbe_common.modeInfo->XResolution == vbe_common.fallback.width) && (vbe_common.modeInfo->YResolution == vbe_common.fallback.height) && (vbe_common.modeInfo->BitsPerPixel == vbe_common.fallback.bpp)) {
				fallbackModeId = modeIds[i];
				fallback.width = vbe_common.modeInfo->XResolution;
				fallback.height = vbe_common.modeInfo->YResolution;
				fallback.bpp = vbe_common.modeInfo->BitsPerPixel;
				fallback.pitch = vbe_common.modeInfo->BytesPerScanLine;
				fallback.framebuffer = vbe_common.modeInfo->PhysBasePtr;
			}
		}
	}

	hal_memcpy(picked, &fallback, sizeof(graphmode_t));
	return fallbackModeId;
}


static int vbe_videoModeParse(char *str, video_mode_t *video_mode)
{
	unsigned int val;
	char *ptr = str;

	if ((ptr == NULL) || (*ptr == '\0')) {
		return -EINVAL;
	}

	val = lib_strtoul(ptr, &ptr, 0);
	if (*ptr != 'x') {
		return -EINVAL;
	}
	video_mode->width = val;

	ptr++;
	val = lib_strtoul(ptr, &ptr, 0);
	if (*ptr != 'x') {
		return -EINVAL;
	}
	video_mode->height = val;

	ptr++;
	val = lib_strtoul(ptr, &ptr, 0);
	if (*ptr != '\0') {
		return -EINVAL;
	}
	video_mode->bpp = val;

	return 0;
}


static void cmd_usage(const char *name)
{
	lib_printf(
		"Usage: %s <options>\n"
		"\t-p W:H:BPP   preferred mode (default: %dx%dx%d)\n"
		"\t-f W:H:BPP   fallback mode (default: %dx%dx%d)\n"
		"\t-h           prints help\n",
		name, vbe_common.preferred.width, vbe_common.preferred.height, vbe_common.preferred.bpp,
		vbe_common.fallback.width, vbe_common.fallback.height, vbe_common.fallback.bpp);
}


static void cmd_vbeInfo(void)
{
	lib_printf("sets graphics mode using VESA VBE");
}


static int cmd_vbeMain(int argc, char *argv[])
{
	int opt, err;
	graphmode_t graphmode;

	lib_printf("\n");

	for (;;) {
		opt = lib_getopt(argc, argv, "p:f:h");
		if (opt == -1) {
			break;
		}

		if (opt == 'p') {
			vbe_videoModeParse(optarg, &vbe_common.preferred);
			continue;
		}

		if (opt == 'f') {
			vbe_videoModeParse(optarg, &vbe_common.fallback);
			continue;
		}

		cmd_usage(argv[0]);
		return CMD_EXIT_FAILURE;
	}

	do {
		err = _vbe_infoGet();
		if (err < 0) {
			break;
		}

		err = _vbe_modeFind(&graphmode);
		if (err < 0) {
			break;
		}

		syspage_graphmodeSet(graphmode);

		_vbe_modeSet(err);
	} while (0);

	if (err < 0) {
		lib_printf("\n%s: not supported, skipping", argv[0]);
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t vbe_cmd __attribute__((section("commands"), used)) = {
	.name = "vbe", .run = cmd_vbeMain, .info = cmd_vbeInfo
};
