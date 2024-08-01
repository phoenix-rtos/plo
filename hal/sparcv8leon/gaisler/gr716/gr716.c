/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR716 specific functions
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "gr716.h"
#include "../../cpu.h"

#include <devices/gpio-gr716/gpio.h>


#define GRGPREG_BASE ((void *)0x8000D000)
#define PLL_BASE     ((void *)0x8010D000)
#define CGU_BASE0    ((void *)0x80006000)
#define CGU_BASE1    ((void *)0x80007000)

/*    Oscillator frequency   *  PLL Multiplier (for 400 MHz) */
/*---------------------------|-------------------------------*/
/*         25 MHz            *          0x3 - 16             */
/*         20 MHz            *          0x5 - 20             */
/*         12.5 MHz          *          0x4 - 32             */
/*         10 MHz            *          0x6 - 40             */
/*         5 MHz             *          0x7 - 80             */
/*---------------------------|-------------------------------*/

#define REF_CLK  20 * 1000 * 1000  /* Hz */
#define PLL_FREQ 400 * 1000 * 1000 /* Hz */
#define PLL_MUL  0x5
#define PLL_SEL  0x2

#define PLL_DUTY_SHFT 16
#define PLL_SEL_SHFT  8

/* System configuration registers */

enum {
	cfg_gp0 = 0,   /* Sys IO config GPIO 0-7      : 0x00 */
	cfg_gp1,       /* Sys IO config GPIO 8-15     : 0x04 */
	cfg_gp2,       /* Sys IO config GPIO 16-23    : 0x08 */
	cfg_gp3,       /* Sys IO config GPIO 24-31    : 0x0C */
	cfg_gp4,       /* Sys IO config GPIO 32-39    : 0x10 */
	cfg_gp5,       /* Sys IO config GPIO 40-47    : 0x14 */
	cfg_gp6,       /* Sys IO config GPIO 48-55    : 0x18 */
	cfg_gp7,       /* Sys IO config GPIO 56-63    : 0x1C */
	cfg_pullup0,   /* Pull-up config GPIO 0-31    : 0x20 */
	cfg_pullup1,   /* Pull-up config GPIO 32-63   : 0x24 */
	cfg_pulldn0,   /* Pull-down config GPIO 0-31  : 0x28 */
	cfg_pulldn1,   /* Pull-down config GPIO 32-63 : 0x2C */
	cfg_lvds,      /* LVDS config                 : 0x30 */
	cfg_prot = 16, /* Sys IO config protection    : 0x40 */
	cfg_eirq,      /* Sys IO config err interrupt : 0x44 */
	cfg_estat      /* Sys IO config err status    : 0x48 */
};

/* PLL */

enum { pll_cfg = 0, /* PLL config              : 0x00 */
	pll_sts,        /* PLL status              : 0x04 */
	pll_ref,        /* PLL reference clk       : 0x08 */
	spw_ref,        /* SpaceWire reference clk : 0x0C */
	mil_ref,        /* MIL1553B reference clk  : 0x10 */
	sys_ref,        /* SYS clk source and div  : 0x14 */
	sys_sel,        /* Select system clk src   : 0x18 */
	pll_ctrl,       /* Enable PLL interrupt    : 0x1C */
	pll_prot,       /* Clock cfg protection    : 0x20 */
	pll_tctrl,      /* PLL test clock enable   : 0x24 */
	pwm_0_ref,      /* PWM0 reference clk      : 0x28 */
	pwm_1_ref       /* PWM1 reference clk      : 0x2C */
};

/* Clock gating unit */

enum {
	cgu_unlock = 0, /* Unlock register        : 0x00 */
	cgu_clk_en,     /* Clock enable register  : 0x04 */
	cgu_core_reset, /* Core reset register    : 0x08 */
	cgu_override,   /* Override register - only primary CGU : 0x0C */
};


static struct {
	vu32 *grgpreg_base;
	vu32 *pll_base;
	vu32 *cgu_base0;
	vu32 *cgu_base1;
} gr716_common;


/* Set system clock divisor */
static void _gr716_setSysClk(u32 freq)
{
	u8 div = PLL_FREQ / freq;
	u8 duty = div / 2;
	*(gr716_common.pll_base + sys_ref) = (duty << PLL_DUTY_SHFT) |
		(PLL_SEL << PLL_SEL_SHFT) | div;
}


static void _gr716_pllSetDefault(void)
{
	*(gr716_common.pll_base + sys_sel) = 0x0;

	/* Set PLL multiplier to 20 (for 20 MHz input freq) */
	*(gr716_common.pll_base + pll_cfg) = PLL_MUL;

	/* Use SYS_CLK pin for source */
	*(gr716_common.pll_base + pll_ref) = 0;

	_gr716_setSysClk(SYSCLK_FREQ);

	/* Select new system clock source */
	*(gr716_common.pll_base + sys_sel) = 0x1;

	/* Wait for PLL to lock */
	while ((*(gr716_common.pll_base + pll_sts) & 0x1) == 0) { }
}


int gaisler_iomuxCfg(iomux_cfg_t *ioCfg)
{
	vu32 oldCfg;

	if (ioCfg->pin > 63) {
		return -1;
	}

	oldCfg = *(gr716_common.grgpreg_base + cfg_gp0 + (ioCfg->pin / 8));

	*(gr716_common.grgpreg_base + cfg_gp0 + (ioCfg->pin / 8)) =
		(oldCfg & ~(0xf << ((ioCfg->pin % 8) << 2))) | (ioCfg->opt << ((ioCfg->pin % 8) << 2));

	oldCfg = *(gr716_common.grgpreg_base + cfg_pullup0 + (ioCfg->pin / 32));

	*(gr716_common.grgpreg_base + cfg_pullup0 + (ioCfg->pin / 32)) =
		(oldCfg & ~(1 << (ioCfg->pin % 32))) | (ioCfg->pullup << (ioCfg->pin % 32));

	oldCfg = *(gr716_common.grgpreg_base + cfg_pulldn0 + (ioCfg->pin / 32));

	*(gr716_common.grgpreg_base + cfg_pulldn0 + (ioCfg->pin / 32)) =
		(oldCfg & ~(1 << (ioCfg->pin % 32))) | (ioCfg->pulldn << (ioCfg->pin % 32));

	return 0;
}

/* CGU setup - section 26.2 GR716 manual */

void _gr716_cguClkEnable(u32 cgu, u32 device)
{
	vu32 *cguBase = (cgu == cgu_primary) ? gr716_common.cgu_base0 : gr716_common.cgu_base1;
	u32 msk = 1 << device;

	*(cguBase + cgu_unlock) |= msk;
	*(cguBase + cgu_core_reset) |= msk;
	*(cguBase + cgu_clk_en) |= msk;
	*(cguBase + cgu_clk_en) &= ~msk;
	*(cguBase + cgu_core_reset) &= ~msk;
	*(cguBase + cgu_clk_en) |= msk;
	*(cguBase + cgu_unlock) &= ~msk;
}


void _gr716_cguClkDisable(u32 cgu, u32 device)
{
	vu32 *cguBase = (cgu == cgu_primary) ? gr716_common.cgu_base0 : gr716_common.cgu_base1;
	u32 msk = 1 << device;

	*(cguBase + cgu_unlock) |= msk;
	*(cguBase + cgu_clk_en) &= ~msk;
	*(cguBase + cgu_unlock) &= ~msk;
}


int _gr716_cguClkStatus(u32 cgu, u32 device)
{
	vu32 *cguBase = (cgu == cgu_primary) ? gr716_common.cgu_base0 : gr716_common.cgu_base1;
	u32 msk = 1 << device;

	return (*(cguBase + cgu_clk_en) & msk) ? 1 : 0;
}


void _gr716_init(void)
{
	gr716_common.grgpreg_base = GRGPREG_BASE;
	gr716_common.pll_base = PLL_BASE;
	gr716_common.cgu_base0 = CGU_BASE0;
	gr716_common.cgu_base1 = CGU_BASE1;

	_gr716_pllSetDefault();
}
