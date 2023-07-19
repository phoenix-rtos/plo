/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Leon3 CPU related routines
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

#define WATCHDOG_CTRL  0x78
#define DIS_LVDS_ADDR  0x8000d030
#define CGU_UNLOCK     0x80006000
#define CGU_EN         0x80006004
#define CGU_RESET      0x80006008
#define BO_VMON        0x8010c018
#define BOOTSTRAP_ADDR 0x80008000
#define BOOTSTRAP_SPIM 0x400BC003
#define TESTCFG_ADDR   0x8000E000
#define TESTCFG_DISR   0x3
#define SPIM_CTRL_ADDR 0xFFF00104
#define SPIM_EDAC_ADDR 0xFFF00114


void hal_cpuReboot(void)
{
	/* Reset to the built-in bootloader */
	hal_interruptsDisableAll();

	/* Disable watchdog boot sequence */
	*(vu32 *)(GPTIMER0_BASE + WATCHDOG_CTRL) = 0;

	/* Clear Brownout */
	*(vu32 *)(CGU_UNLOCK) = 0x8000000;
	*(vu32 *)(CGU_EN) = 0x8000000;
	*(vu32 *)(CGU_RESET) = 0;
	*(vu32 *)(CGU_UNLOCK) = 0;

	*(vu32 *)(BO_VMON) = 0x7F;
	*(vu32 *)(BO_VMON) = 0;

	/* Disable LVDS */
	*(vu32 *)(DIS_LVDS_ADDR) = 0x00888888;

	/* Disable watchdog reset */
	*(vu32 *)(TESTCFG_ADDR) = TESTCFG_DISR;

	/* Enable alt scaler and disable EDAC for SPI memory */
	*(vu32 *)(SPIM_CTRL_ADDR) = 0x4;
	*(vu32 *)(SPIM_EDAC_ADDR) = 0;

	/* Reboot to SPIM */
	*(vu32 *)(BOOTSTRAP_ADDR) = BOOTSTRAP_SPIM;

	/* clang-format off */
	__asm__ volatile (
		"jmp %%g0\n\t"
		"nop\n\t"
		:::
	);
	/* clang-format on */

	__builtin_unreachable();
}
