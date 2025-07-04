/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * iMXRT 117x basic peripherals control functions
 *
 * Copyright 2017, 2019-2023 Phoenix Systems
 * Author: Aleksander Kaminski, Jan Sikorski, Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "imxrt.h"
#include "../../cpu.h"
#include "../otp.h"
#include "board_config.h"

#include <hal/hal.h>
#include <lib/lib.h>

/* 1500 ms is the sum of the minimum sensible watchdog timeout (500 ms) and time for WICT interrupt
to fire before watchdog times out (1000 ms) */
#if WATCHDOG_PLO && (WATCHDOG_PLO_TIMEOUT_MS < 1500 || WATCHDOG_PLO_TIMEOUT_MS > 128000)
#error "Watchdog timeout out of bounds!"
#endif

/* clang-format off */

enum { stk_ctrl = 0, stk_load, stk_val, stk_calib };


enum { aipstz_mpr = 0, aipstz_opacr = 16, aipstz_opacr1, aipstz_opacr2, aipstz_opacr3, aipstz_opacr4 };


enum { gpio_dr = 0, gpio_gdir, gpio_psr, gpio_icr1, gpio_icr2, gpio_imr, gpio_isr, gpio_edge_sel, gpio_dr_set,
	gpio_dr_clear, gpio_dr_toggle };


enum { src_scr = 0, src_srmr, src_sbmr1, src_sbmr2, src_srsr,
	src_gpr1, src_gpr2, src_gpr3, src_gpr4, src_gpr5,
	src_gpr6, src_gpr7, src_gpr8, src_gpr9, src_gpr10,
	src_gpr11, src_gpr12, src_gpr13, src_gpr14, src_gpr15,
	src_gpr16, src_gpr17, src_gpr18, src_gpr19, src_gpr20 };


enum { wdog_wcr = 0, wdog_wsr, wdog_wrsr, wdog_wicr, wdog_wmcr };


enum { rtwdog_cs = 0, rtwdog_cnt, rtwdog_toval, rtwdog_win };


/* ANADIG & ANATOP complex */
enum {
	/* OSC */
	osc_48m_ctrl = 0x04, osc_24m_ctrl = 0x08, osc_400m_ctrl0 = 0x10, osc_400m_ctrl1 = 0x14, osc_400m_ctrl2 = 0x18,
	osc_16m_ctrl = 0x30,

	/* PLL */
	arm_pll_ctrl = 0x80, sys_pll3_ctrl = 0x84, sys_pll3_update = 0x88, sys_pll3_pfd = 0x8c,
	sys_pll2_ctrl = 0x90, sys_pll2_update = 0x94, sys_pll2_ss = 0x98, sys_pll2_pfd = 0x9c,
	sys_pll2_mfd = 0xa8, sys_pll1_ss = 0xac, sys_pll1_ctrl = 0xb0, sys_pll1_denominator = 0xb4,
	sys_pll1_numerator = 0xb8, sys_pll1_div_select = 0xbc, pll_audio_ctrl = 0xc0, pll_audio_ss = 0xc4,
	pll_audio_denominator = 0xc8, pll_audio_numerator = 0xcc, pll_audio_div_select = 0xd0, pll_video_ctrl = 0xd4,
	pll_video_ss = 0xd8, pll_video_denominator = 0xdc, pll_video_numerator = 0xe0, pll_video_div_select = 0xe4,

	/* PMU */
	pmu_ldo_pll = 0x140, pmu_ldo_lpsr_ana = 0x144, pmu_ldo_lpsr_dig2 = 0x148, pmu_ldo_lpsr_dig = 0x14c,
	pmu_ref_ctrl = 0x15c,

	/* ANATOP AI */
	vddsoc_ai_ctrl = 0x208, vddsoc_ai_wdata = 0x20c, vddsoc_ai_rdata = 0x210,
	vddsoc2pll_ai_ctrl_1g = 0x214, vddsoc2pll_ai_wdata_1g = 0x218, vddsoc2pll_ai_rdata_1g = 0x21c,
	vddsoc2pll_ai_ctrl_audio = 0x220, vddsoc2pll_ai_wdata_audio = 0x224, vddsoc2pll_ai_rdata_audio = 0x228,
	vddsoc2pll_ai_ctrl_video = 0x22c, vddsoc2pll_ai_wdata_video = 0x230, vddsoc2pll_ai_rdata_video = 0x234
};


/* Fractional PLL (AI interface) */
enum {
	frac_pll_ctrl0 = 0x00, frac_pll_ctrl0_set = 0x04, frac_pll_ctrl0_clr = 0x08, frac_pll_ctrl0_tog = 0x0c,
	frac_pll_ss = 0x10, frac_pll_ss_set = 0x14, frac_pll_ss_clr = 0x18, frac_pll_ss_tog = 0x1c,
	frac_pll_num = 0x20, frac_pll_num_set = 0x24, frac_pll_num_clr = 0x28, frac_pll_num_tog = 0x2c,
	frac_pll_denom = 0x30, frac_pll_denom_set = 0x34, frac_pll_denom_clr = 0x38, frac_pll_denom_tog = 0x3c
};

struct {
	volatile u32 *aips[4];
	volatile u32 *stk;
	volatile u32 *src;
	volatile u16 *wdog1;
	volatile u16 *wdog2;
	volatile u32 *wdog3;
	volatile u32 *wdog4;
	volatile u32 *iomuxc_snvs;
	volatile u32 *iomuxc_lpsr;
	volatile u32 *iomuxc_lpsr_gpr;
	volatile u32 *iomuxc_gpr;
	volatile u32 *iomuxc;
	volatile u32 *gpio[13];
	volatile u32 *ccm;
	volatile u32 *anadig_pll;

	u32 resetFlags;
	u32 cpuclk;
	u32 cm4state;
} imxrt_common;

/* clang-format on */

unsigned int hal_getBootReason(void)
{
	return imxrt_common.resetFlags;
}

/* IOMUX */


__attribute__((section(".noxip"))) static volatile u32 *_imxrt_IOmuxGetReg(int mux)
{
	if ((mux < pctl_mux_gpio_emc_b1_00) || (mux > pctl_mux_gpio_lpsr_15)) {
		return NULL;
	}

	if (mux < pctl_mux_wakeup) {
		return imxrt_common.iomuxc + 4 + mux - pctl_mux_gpio_emc_b1_00;
	}

	if (mux < pctl_mux_gpio_lpsr_00) {
		return imxrt_common.iomuxc_snvs + mux - pctl_mux_wakeup;
	}

	return imxrt_common.iomuxc_lpsr + mux - pctl_mux_gpio_lpsr_00;
}


__attribute__((section(".noxip"))) int _imxrt_setIOmux(int mux, char sion, char mode)
{
	volatile u32 *reg;

	reg = _imxrt_IOmuxGetReg(mux);
	if (reg == NULL) {
		return -1;
	}

	(*reg) = (!!sion << 4) | (mode & 0xf);
	hal_cpuDataMemoryBarrier();

	return 0;
}


__attribute__((section(".noxip"))) static volatile u32 *_imxrt_IOpadGetReg(int pad)
{
	if ((pad < pctl_pad_gpio_emc_b1_00) || (pad > pctl_pad_gpio_lpsr_15)) {
		return NULL;
	}

	if (pad < pctl_pad_test_mode) {
		return imxrt_common.iomuxc + pad + 149 - pctl_pad_gpio_emc_b1_00;
	}

	if (pad < pctl_pad_gpio_lpsr_00) {
		return imxrt_common.iomuxc_snvs + pad + 13 - pctl_pad_test_mode;
	}

	return imxrt_common.iomuxc_lpsr + pad + 16 - pctl_pad_gpio_lpsr_00;
}


__attribute__((section(".noxip"))) int _imxrt_setIOpad(int pad, char sre, char dse, char pue, char pus, char ode, char apc)
{
	u32 t;
	volatile u32 *reg;
	char pull;

	reg = _imxrt_IOpadGetReg(pad);
	if (reg == NULL) {
		return -1;
	}

	if ((pad <= pctl_pad_gpio_emc_b2_20) || ((pad >= pctl_pad_gpio_sd_b1_00) && (pad <= pctl_pad_gpio_disp_b1_11))) {
		/* Fields have slightly diffrent meaning... */
		if (pue == 0) {
			pull = 3;
		}
		else if (pus != 0) {
			pull = 1;
		}
		else {
			pull = 2;
		}

		t = *reg & ~0x1e;
		t |= (!!dse << 1) | (pull << 2) | (!!ode << 4);
	}
	else {
		t = *reg & ~0x1f;
		t |= (!!sre) | (!!dse << 1) | (!!pue << 2) | (!!pus << 3);

		if (pad <= pctl_pad_gpio_disp_b2_15) {
			t &= ~(1 << 4);
			t |= !!ode << 4;
		}
		else if ((pad >= pctl_pad_wakeup) && (pad <= pctl_pad_gpio_snvs_09)) {
			t &= ~(1 << 6);
			t |= !!ode << 6;
		}
		else if (pad >= pctl_pad_gpio_lpsr_00) {
			t &= ~(1 << 5);
			t |= !!ode << 5;
		}
		else {
			/* MISRA */
			/* pctl_pad_test_mode, pctl_pad_por_b, pctl_pad_onoff - no ode field */
		}
	}

	/* APC field is not documented. Leave it alone for now. */
	//t &= ~(0xf << 28);
	//t |= (apc & 0xf) << 28;

	(*reg) = t;
	hal_cpuDataMemoryBarrier();

	return 0;
}


__attribute__((section(".noxip"))) static volatile u32 *_imxrt_IOiselGetReg(int isel, u32 *mask)
{
	if ((isel < pctl_isel_flexcan1_rx) || (isel > pctl_isel_sai4_txsync)) {
		return NULL;
	}

	switch (isel) {
		case pctl_isel_flexcan1_rx:
		case pctl_isel_ccm_enet_qos_ref_clk:
		case pctl_isel_enet_ipg_clk_rmii:
		case pctl_isel_enet_1g_ipg_clk_rmii:
		case pctl_isel_enet_1g_mac0_mdio:
		case pctl_isel_enet_1g_mac0_rxclk:
		case pctl_isel_enet_1g_mac0_rxdata_0:
		case pctl_isel_enet_1g_mac0_rxdata_1:
		case pctl_isel_enet_1g_mac0_rxdata_2:
		case pctl_isel_enet_1g_mac0_rxdata_3:
		case pctl_isel_enet_1g_mac0_rxen:
		case enet_qos_phy_rxer:
		case pctl_isel_flexspi1_dqs_fa:
		case pctl_isel_lpuart1_rxd:
		case pctl_isel_lpuart1_txd:
		case pctl_isel_qtimer1_tmr0:
		case pctl_isel_qtimer1_tmr1:
		case pctl_isel_qtimer2_tmr0:
		case pctl_isel_qtimer2_tmr1:
		case pctl_isel_qtimer3_tmr0:
		case pctl_isel_qtimer3_tmr1:
		case pctl_isel_qtimer4_tmr0:
		case pctl_isel_qtimer4_tmr1:
		case pctl_isel_sdio_slv_clk_sd:
		case pctl_isel_sdio_slv_cmd_di:
		case pctl_isel_sdio_slv_dat0_do:
		case pctl_isel_slv_dat1_irq:
		case pctl_isel_sdio_slv_dat2_rw:
		case pctl_isel_sdio_slv_dat3_cs:
		case pctl_isel_spdif_in1:
		case pctl_isel_can3_canrx:
		case pctl_isel_lpuart12_rxd:
		case pctl_isel_lpuart12_txd:
			(*mask) = 0x3;
			break;

		default:
			(*mask) = 0x1;
			break;
	}

	if (isel >= pctl_isel_can3_canrx) {
		return imxrt_common.iomuxc_lpsr + 32 + isel - pctl_isel_can3_canrx;
	}

	return imxrt_common.iomuxc + 294 + isel - pctl_isel_flexcan1_rx;
}


__attribute__((section(".noxip"))) int _imxrt_setIOisel(int isel, char daisy)
{
	volatile u32 *reg;
	u32 mask;

	reg = _imxrt_IOiselGetReg(isel, &mask);
	if (reg == NULL) {
		return -1;
	}

	(*reg) = daisy & mask;
	hal_cpuDataMemoryBarrier();

	return 0;
}


/* GPIO */


static volatile u32 *_imxrt_gpioGetReg(unsigned int d)
{
	return (d < sizeof(imxrt_common.gpio) / sizeof(imxrt_common.gpio[0])) ? imxrt_common.gpio[d] : NULL;
}


int _imxrt_gpioConfig(unsigned int d, u8 pin, u8 dir)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);
	u32 register clr;

	if ((reg == NULL) || (pin > 31u)) {
		return -1;
	}

	clr = *(reg + gpio_gdir) & ~(1uL << pin);
	dir = (dir != 0u) ? 1u : 0u;
	*(reg + gpio_gdir) = clr | (dir << pin);

	return 0;
}


int _imxrt_gpioSet(unsigned int d, u8 pin, u8 val)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);
	u32 register clr;

	if ((reg == NULL) || (pin > 31u)) {
		return -1;
	}

	clr = *(reg + gpio_dr) & ~(1uL << pin);
	val = (val != 0u) ? 1u : 0u;
	*(reg + gpio_dr) = clr | (val << pin);

	return 0;
}


int _imxrt_gpioSetPort(unsigned int d, u32 val)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);

	if (reg == NULL) {
		return -1;
	}

	*(reg + gpio_dr) = val;

	return 0;
}


int _imxrt_gpioGet(unsigned int d, u8 pin, u8 *val)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);

	if ((reg == NULL) || (pin > 31)) {
		return -1;
	}

	*val = ((*(reg + gpio_psr) & (1uL << pin)) != 0u) ? 1u : 0u;

	return 0;
}


int _imxrt_gpioGetPort(unsigned int d, u32 *val)
{
	volatile u32 *reg = _imxrt_gpioGetReg(d);

	if (reg == NULL) {
		return -1;
	}

	*val = *(reg + gpio_psr);

	return 0;
}


/* CCM */

__attribute__((section(".noxip"))) int _imxrt_getDevClock(int clock, int *div, int *mux, int *mfd, int *mfn, int *state)
{
	unsigned int t;
	volatile u32 *reg = imxrt_common.ccm + (clock * 0x20);

	if ((clock < pctl_clk_m7) || (clock > pctl_clk_clko2)) {
		return -1;
	}

	t = *reg;

	*div = t & 0xff;
	*mux = (t >> 8) & 0x7;
	*mfd = (t >> 16) & 0xf;
	*mfn = (t >> 20) & 0xf;
	*state = !(t & (1 << 24));

	return 0;
}

__attribute__((section(".noxip"))) int _imxrt_setDevClock(int clock, int div, int mux, int mfd, int mfn, int state)
{
	unsigned int t;
	volatile u32 *reg = imxrt_common.ccm + (clock * 0x20);

	if ((clock < pctl_clk_m7) || (clock > pctl_clk_clko2)) {
		return -1;
	}

	t = *reg & ~0x01ff07ffu;
	*reg = t | (!state << 24) | ((mfn & 0xf) << 20) | ((mfd & 0xf) << 16) | ((mux & 0x7) << 8) | (div & 0xff);

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();

	return 0;
}


__attribute__((section(".noxip"))) int _imxrt_setDirectLPCG(int clock, int state)
{
	u32 t;
	volatile u32 *reg;

	if ((clock < pctl_lpcg_m7) || (clock > pctl_lpcg_uniq_edt_i)) {
		return -1;
	}

	reg = imxrt_common.ccm + 0x1800 + clock * 0x8;

	t = *reg & ~1u;
	*reg = t | (state & 1);

	hal_cpuDataMemoryBarrier();
	hal_cpuInstrBarrier();

	return 0;
}


__attribute__((section(".noxip"))) int _imxrt_setLevelLPCG(int clock, int level)
{
	volatile u32 *reg;

	if ((clock < pctl_lpcg_m7) || (clock > pctl_lpcg_uniq_edt_i)) {
		return -1;
	}

	if ((level < 0) || (level > 4)) {
		return -1;
	}

	reg = imxrt_common.ccm + 0x1801 + clock * 0x8;
	*reg = (level << 28) | (level << 24) | (level << 20) | (level << 16) | level;

	hal_cpuDataMemoryBarrier();
	hal_cpuInstrBarrier();

	return 0;
}


static void _imxrt_delay(u32 ticks)
{
	/* TODO: use better method e.g. count cycles, must not use IRQs and peripheral timers! */
	while (ticks-- != 0u) {
		/* clang-format off */
		__asm__ volatile ("nop");
		/* clang-format on */
	}
}


__attribute__((unused)) static u32 _imxrt_vddsoc2PllAiRead(int ai_ctrl_addr, int ai_reg)
{
	u32 pre_tog_done, tog_done, t;

	pre_tog_done = *(imxrt_common.anadig_pll + ai_ctrl_addr) & (1uL << 9u);

	t = *(imxrt_common.anadig_pll + ai_ctrl_addr) & ~((1uL << 16u) | 0xff);
	t |= (1uL << 16u) | (ai_reg & 0xff);

	/* Write AI_CTRL, RWB=1 */
	*(imxrt_common.anadig_pll + ai_ctrl_addr) = t;

	/* Toggle AI_CTRL */
	*(imxrt_common.anadig_pll + ai_ctrl_addr) ^= (1uL << 8u);

	/* Wait for TOGGLE_DONE */
	do {
		tog_done = *(imxrt_common.anadig_pll + ai_ctrl_addr) & (1uL << 9u);
	} while (tog_done == pre_tog_done);

	/* Read AI_RDATA */
	return *(imxrt_common.anadig_pll + (ai_ctrl_addr + 0x8));
}


static void _imxrt_vddsoc2PllAiWrite(int ai_ctrl_addr, int ai_reg, u32 data)
{
	u32 pre_tog_done, tog_done, t;

	pre_tog_done = *(imxrt_common.anadig_pll + ai_ctrl_addr) & (1uL << 9u);

	t = *(imxrt_common.anadig_pll + ai_ctrl_addr) & ~((1uL << 16u) | 0xff);
	t |= ai_reg & 0xff;

	/* Write AI_CTRL, RWB=0 */
	*(imxrt_common.anadig_pll + ai_ctrl_addr) = t;

	/* Write AI_WDATA */
	*(imxrt_common.anadig_pll + (ai_ctrl_addr + 0x4)) = data;

	/* Toggle AI_CTRL */
	*(imxrt_common.anadig_pll + ai_ctrl_addr) ^= (1uL << 8u);

	/* Wait for TOGGLE_DONE */
	do {
		tog_done = *(imxrt_common.anadig_pll + ai_ctrl_addr) & (1uL << 9u);
	} while (tog_done == pre_tog_done);
}


static void _imxrt_setPllBypass(u8 clk_pll, u8 enable)
{
	/* clang-format off */
	switch (clk_pll) {
		case clk_pllarm:
			*(imxrt_common.anadig_pll + arm_pll_ctrl) = (enable != 0) ?
				(*(imxrt_common.anadig_pll + arm_pll_ctrl) | (1uL << 17u)) :
				(*(imxrt_common.anadig_pll + arm_pll_ctrl) & ~(1uL << 17u));
			break;
		case clk_pllsys1:
			_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_1g, (enable != 0) ? frac_pll_ctrl0_set : frac_pll_ctrl0_clr, (1uL << 16u));
			break;
		case clk_pllsys2:
			*(imxrt_common.anadig_pll + sys_pll2_ctrl) = (enable != 0) ?
				(*(imxrt_common.anadig_pll + sys_pll2_ctrl) | (1uL << 16u)) :
				(*(imxrt_common.anadig_pll + sys_pll2_ctrl) & ~(1uL << 16u));
			break;
		case clk_pllsys3:
			*(imxrt_common.anadig_pll + sys_pll3_ctrl) = (enable != 0) ?
				(*(imxrt_common.anadig_pll + sys_pll3_ctrl) | (1uL << 16u)) :
				(*(imxrt_common.anadig_pll + sys_pll3_ctrl) & ~(1uL << 16u));
			break;
		case clk_pllaudio:
			_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_audio, (enable != 0) ? frac_pll_ctrl0_set : frac_pll_ctrl0_clr, (1uL << 16u));
			break;
		case clk_pllvideo:
			_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_video, (enable != 0) ? frac_pll_ctrl0_set : frac_pll_ctrl0_clr, (1uL << 16u));
			break;
		default:
			break;
	}
	/* clang-format on */
}


#ifdef BYPASS_ANADIG_LDO
static void _imxrt_pmuBypassAnaLdo(void)
{
	/* HP mode */
	*(imxrt_common.anadig_pll + pmu_ldo_lpsr_ana) &= ~1u;
	hal_cpuDataMemoryBarrier();

	_imxrt_delay(1000u * 1000u);

	/* Tracking mode */
	*(imxrt_common.anadig_pll + pmu_ldo_lpsr_ana) |= 1uL << 19u;
	hal_cpuDataMemoryBarrier();

	_imxrt_delay(1000u * 1000u);

	/* Enable bypass mode */
	*(imxrt_common.anadig_pll + pmu_ldo_lpsr_ana) |= 1uL << 5u;
	hal_cpuDataMemoryBarrier();

	_imxrt_delay(1000u * 1000u);

	/* Disable ana_lpsr regulator */
	*(imxrt_common.anadig_pll + pmu_ldo_lpsr_ana) |= 1uL << 2u;
	hal_cpuDataMemoryBarrier();

	_imxrt_delay(1000u * 1000u);
}
#endif


#ifdef BYPASS_ANADIG_LDO
static void _imxrt_pmuBypassDigLdo(void)
{
	/* Tracking mode */
	*(imxrt_common.anadig_pll + pmu_ldo_lpsr_dig) |= 1uL << 17u;
	hal_cpuDataMemoryBarrier();

	_imxrt_delay(1000u * 1000u);

	/* Set bypass mode */
	*(imxrt_common.anadig_pll + pmu_ldo_lpsr_dig) |= 1uL << 18u;
	hal_cpuDataMemoryBarrier();

	_imxrt_delay(1000u * 1000u);

	/* Disable dig_lpsr regulator */
	*(imxrt_common.anadig_pll + pmu_ldo_lpsr_dig) |= 1uL << 2u;
	hal_cpuDataMemoryBarrier();

	_imxrt_delay(1000u * 1000u);
}
#endif


static void _imxrt_pmuEnablePllLdo(void)
{
	u32 val;

	/* Set address of PHY_LDO_CTRL0 */
	*(imxrt_common.anadig_pll + vddsoc_ai_ctrl) |= (1uL << 16u);

	val = *(imxrt_common.anadig_pll + vddsoc_ai_ctrl) & ~(0xffu);
	*(imxrt_common.anadig_pll + vddsoc_ai_ctrl) = val | (0u); /* PHY_LDO_CTRL0 = 0 */

	/* Toggle ldo PLL AI */
	*(imxrt_common.anadig_pll + pmu_ldo_pll) ^= 1uL << 16u;
	/* Read data */
	val = *(imxrt_common.anadig_pll + vddsoc_ai_rdata);

	if (val == ((0x10uL << 4u) | (1uL << 2u) | 1u)) {
		/* Already set PHY_LDO_CTRL0 LDO */
		return;
	}

	*(imxrt_common.anadig_pll + vddsoc_ai_ctrl) &= ~(1uL << 16u);

	val = *(imxrt_common.anadig_pll + vddsoc_ai_ctrl) & ~(0xffu);
	*(imxrt_common.anadig_pll + vddsoc_ai_ctrl) = val | (0u); /* PHY_LDO_CTRL0 = 0 */

	/* Write data */
	*(imxrt_common.anadig_pll + vddsoc_ai_wdata) = (0x10uL << 4u) | (1uL << 2u) | 1u;
	/* Toggle ldo PLL AI */
	*(imxrt_common.anadig_pll + pmu_ldo_pll) ^= 1uL << 16u;
	hal_cpuDataMemoryBarrier();

	_imxrt_delay(300u * 1000u);

	/* Enable Voltage Reference for PLLs before those PLLs were enabled */
	*(imxrt_common.anadig_pll + pmu_ref_ctrl) = 1uL << 4u;
}


static u32 _imxrt_deinitArmPll(void)
{
	u32 reg = *(imxrt_common.anadig_pll + arm_pll_ctrl) & ~(1uL << 29u);

	/* Disable and gate clock if not already */
	if ((reg & ((1uL << 13u) | (1uL << 14u))) != 0u) {
		/* Power down the PLL, disable clock */
		reg &= ~((1uL << 13u) | (1uL << 14u));
		/* Gate the clock */
		reg |= 1uL << 30u;
		*(imxrt_common.anadig_pll + arm_pll_ctrl) = reg;

		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
	}

	return reg;
}


static int _imxrt_initArmPll(u8 loopDivider, u8 postDivider)
{
	u32 reg;

	/*
	 * Fin = XTALOSC = 24MHz
	 * Fout = Fin * (loopDivider / (2 * postDivider))
	 */

	if ((loopDivider < 104u) || (208u < loopDivider)) {
		return -1;
	}

	reg = _imxrt_deinitArmPll();

	/* Set the configuration. */
	reg &= ~((3uL << 15u) | 0xffu);
	reg |= ((u32)(loopDivider & 0xffu) | (((u32)postDivider & 3uL) << 15u)) | (1uL << 30u) | (1uL << 13u);
	*(imxrt_common.anadig_pll + arm_pll_ctrl) = reg;

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();

	_imxrt_delay(300u * 1000u);

	/* Wait for stable PLL */
	while ((*(imxrt_common.anadig_pll + arm_pll_ctrl) & (1uL << 29u)) == 0u) {
	}

	/* Enable the clock. */
	reg |= 1uL << 14u;

	/* Ungate the clock */
	reg &= ~(1uL << 30u);

	*(imxrt_common.anadig_pll + arm_pll_ctrl) = reg;

	return 0;
}


static void _imxrt_initSysPll3(void)
{
	u32 reg;

	/* check if configuration is the same, then only enable clock */
	if ((*(imxrt_common.anadig_pll + sys_pll3_ctrl) & (1uL << 21u)) != 0u) {
		/* if clock disable -> enable it */
		if ((*(imxrt_common.anadig_pll + sys_pll3_ctrl) & (1uL << 13u)) == 0u) {
			*(imxrt_common.anadig_pll + sys_pll3_ctrl) |= (1uL << 13u);
		}

		/* if clock is gated -> ungate */
		if ((*(imxrt_common.anadig_pll + sys_pll3_ctrl) & (1uL << 30u)) != 0u) {
			*(imxrt_common.anadig_pll + sys_pll3_ctrl) &= ~(1uL << 30u);
		}

		return;
	}

	/* Gate all PFDs */
	*(imxrt_common.anadig_pll + sys_pll3_pfd) |= (1uL << 31u) | (1uL << 23u) | (1uL << 15u) | (1uL << 7u);

	/* Enable, but gate clock */
	reg = (1uL << 4u) | (1uL << 30u);
	*(imxrt_common.anadig_pll + sys_pll3_pfd) = reg;
	hal_cpuDataMemoryBarrier();

	_imxrt_delay(300u * 1000u);

	/* Power off and hold ring off */
	reg |= (1uL << 21u) | (1uL << 11u);
	*(imxrt_common.anadig_pll + sys_pll3_pfd) = reg;
	hal_cpuDataMemoryBarrier();

	_imxrt_delay(300u * 1000u);

	/* Deassert hold ring off */
	reg &= ~(1uL << 11u);
	*(imxrt_common.anadig_pll + sys_pll3_pfd) = reg;

	/* Wait for stable PLL */
	while ((*(imxrt_common.anadig_pll + sys_pll3_ctrl) & (1uL << 29u)) == 0u) {
	}

	/* Enable system pll3 and div2 clocks */
	reg |= (1uL << 13u) | (1uL << 3u);

	/* un-gate sys pll3 */
	reg &= ~(1uL << 30u);
	*(imxrt_common.anadig_pll + sys_pll3_pfd) = reg;
}


int _imxrt_setPfdPllFracClock(u8 pfd, u8 clk_pll, u8 frac)
{
	volatile u32 *ctrl;
	volatile u32 *update;
	volatile u32 stable;
	u32 fracval;
	u8 gatedpfd;

	if ((pfd > 3u) || (frac > 35u)) {
		return -1;
	}

	switch (clk_pll) {
		case clk_pllsys2:
			if (frac < 13u) {
				return -1;
			}
			ctrl = imxrt_common.anadig_pll + sys_pll2_pfd;
			update = imxrt_common.anadig_pll + sys_pll2_update;
			break;

		case clk_pllsys3:
			if (((pfd == 3u) && (frac < 12u)) || ((pfd < 3u) && (frac < 13u))) {
				return -1;
			}
			ctrl = imxrt_common.anadig_pll + sys_pll3_pfd;
			update = imxrt_common.anadig_pll + sys_pll3_update;
			break;

		default:
			return -1;
	}

	fracval = ((*ctrl) & (0x3fuL << (8u * pfd))) >> (8u * pfd);
	gatedpfd = ((*ctrl) & (0x80uL << (8u * pfd))) >> (8u * pfd);

	if ((fracval == (u32)frac) && (gatedpfd == 0u)) {
		return 0;
	}

	stable = *ctrl & (0x40uL << (8u * pfd));
	*ctrl |= 0x80uL << (8u * pfd);

	*ctrl &= ~(0x3fuL << (8u * pfd));
	*ctrl |= (u32)(frac & 0x3fuL) << (8u * pfd);

	*update ^= 2uL << pfd;
	*ctrl &= ~(0x80uL << (8u * pfd));

	while ((*ctrl & (0x40uL << (8u * pfd))) == stable) {
	}

	return 0;
}


__attribute__((unused)) static void _imxrt_deinitSysPll1(void)
{
	/* Disable PLL1 and div2, div5 */
	*(imxrt_common.anadig_pll + sys_pll1_ctrl) &= ~((1uL << 26u) | (1uL << 25u) | (1uL << 13u));

	/* Gate PLL1 */
	*(imxrt_common.anadig_pll + sys_pll1_ctrl) |= 1uL << 14u;

	/* Disable PLL1 */
	_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_1g, frac_pll_ctrl0_clr, (1uL << 15u));

	/* Power down PLL1 */
	_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_1g, frac_pll_ctrl0_clr, (1uL << 14u));

	/* Disable Spread Spectrum */
	_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_1g, frac_pll_ss_clr, (1uL << 15u));

	/* Disable Internal LDO */
	_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_1g, frac_pll_ctrl0_clr, (1uL << 22u));
}


__attribute__((unused)) static void _imxrt_initSysPll1(void)
{
	_imxrt_pmuEnablePllLdo();

	_imxrt_setPllBypass(clk_pllsys1, 1);

	/* Enable SYS_PLL1 clk output */
	*(imxrt_common.anadig_pll + sys_pll1_ctrl) |= (1uL << 13u);

	/* Configure Fractional PLL: div, num, denom */
	_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_1g, frac_pll_ctrl0_set, 41u);
	_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_1g, frac_pll_denom, 0x0fffffff);
	_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_1g, frac_pll_num, 0x0aaaaaaa);
	/* Disable SS */
	_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_1g, frac_pll_ss_clr, (1uL << 15));

	/* Enable Internal LDO */
	_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_1g, frac_pll_ctrl0_set, (1uL << 22u));
	_imxrt_delay(100u * 1000u);

	/* POWERUP */
	_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_1g, frac_pll_ctrl0_set, (1uL << 14u) | (1uL << 13u));

	/* assert HOLD_RING_OFF */
	_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_1g, frac_pll_ctrl0_set, (1uL << 13u));
	/* Wait until PLL lock time is halfway through */
	/* Lock time is 11250 ref cycles */
	_imxrt_delay(5625u);
	/* de-assert HOLD_RING_OFF */
	_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_1g, frac_pll_ctrl0_clr, (1uL << 13u));

	/* Wait till PLL lock time is complete */
	while ((*(imxrt_common.anadig_pll + sys_pll1_ctrl) & (1uL << 29u)) != (1uL << 29u)) {
	}

	/* Enable PLL1 */
	_imxrt_vddsoc2PllAiWrite(vddsoc2pll_ai_ctrl_1g, frac_pll_ctrl0_set, (1uL << 15u));

	/* Disable PLL gate */
	*(imxrt_common.anadig_pll + sys_pll1_ctrl) &= ~(1uL << 14u);

#ifdef CLOCK_SYS_PLL1DIV2_ENABLE
	*(imxrt_common.anadig_pll + sys_pll1_ctrl) |= (1uL << 25);
#endif

#ifdef CLOCK_SYS_PLL1DIV5_ENABLE
	*(imxrt_common.anadig_pll + sys_pll1_ctrl) |= (1uL << 26);
#endif

	_imxrt_setPllBypass(clk_pllsys1, 0);
}


static void _imxrt_initClockTree(void)
{
	unsigned n;
	static const struct {
		u8 root;
		u8 mux;
		u8 div;
		u8 isOn;
	} clktree[] = {
		{ pctl_clk_m7, mux_clkroot_m7_armpllout, 1, 1 },
		{ pctl_clk_m4, mux_clkroot_m4_syspll3pfd3, 1, 1 },
		{ pctl_clk_bus, mux_clkroot_bus_syspll3out, 2, 1 },
		{ pctl_clk_bus_lpsr, mux_clkroot_bus_lpsr_syspll3out, 3, 1 },
		/* NOTE: not using "pctl_clk_semc" is disabled by bootrom */
		{ pctl_clk_cssys, mux_clkroot_cssys_osc24mout, 1, 1 },
		{ pctl_clk_cstrace, mux_clkroot_cstrace_syspll2out, 4, 1 },
		{ pctl_clk_m4_systick, mux_clkroot_m4_systick_osc24mout, 1, 1 },
		{ pctl_clk_m7_systick, mux_clkroot_m7_systick_osc24mout, 2, 1 },
		{ pctl_clk_adc1, mux_clkroot_adc1_osc24mout, 1, 1 },
		{ pctl_clk_adc2, mux_clkroot_adc2_osc24mout, 1, 1 },
		{ pctl_clk_acmp, mux_clkroot_acmp_osc24mout, 1, 1 },
		{ pctl_clk_flexio1, mux_clkroot_flexio1_osc24mout, 1, 1 },
		{ pctl_clk_flexio2, mux_clkroot_flexio2_osc24mout, 1, 1 },
		{ pctl_clk_gpt1, mux_clkroot_gpt1_osc24mout, 1, 1 },
		{ pctl_clk_gpt2, mux_clkroot_gpt2_osc24mout, 1, 1 },
		{ pctl_clk_gpt3, mux_clkroot_gpt3_osc24mout, 1, 1 },
		{ pctl_clk_gpt4, mux_clkroot_gpt4_osc24mout, 1, 1 },
		{ pctl_clk_gpt5, mux_clkroot_gpt5_osc24mout, 1, 1 },
		{ pctl_clk_gpt6, mux_clkroot_gpt6_osc24mout, 1, 1 },
		/* NOTE: "pctl_clk_flexspi1" is changed by imxrt-flash driver */
		/* NOTE: "pctl_clk_flexspi2" is changed by imxrt-flash driver */
		{ pctl_clk_can1, mux_clkroot_can1_osc24mout, 1, 1 },
		{ pctl_clk_can2, mux_clkroot_can2_osc24mout, 1, 1 },
		{ pctl_clk_can3, mux_clkroot_can3_osc24mout, 1, 1 },
		{ pctl_clk_lpuart1, mux_clkroot_lpuart1_osc24mout, 2, 1 },
		{ pctl_clk_lpuart2, mux_clkroot_lpuart2_osc24mout, 2, 1 },
		{ pctl_clk_lpuart3, mux_clkroot_lpuart3_osc24mout, 1, 1 },
		{ pctl_clk_lpuart4, mux_clkroot_lpuart4_osc24mout, 1, 1 },
		{ pctl_clk_lpuart5, mux_clkroot_lpuart5_osc24mout, 1, 1 },
		{ pctl_clk_lpuart6, mux_clkroot_lpuart6_osc24mout, 1, 1 },
		{ pctl_clk_lpuart7, mux_clkroot_lpuart7_osc24mout, 1, 1 },
		{ pctl_clk_lpuart8, mux_clkroot_lpuart8_osc24mout, 1, 1 },
		{ pctl_clk_lpuart9, mux_clkroot_lpuart9_osc24mout, 1, 1 },
		{ pctl_clk_lpuart10, mux_clkroot_lpuart10_osc24mout, 1, 1 },
		{ pctl_clk_lpuart11, mux_clkroot_lpuart11_osc24mout, 1, 1 },
		{ pctl_clk_lpuart12, mux_clkroot_lpuart12_osc24mout, 1, 1 },
		{ pctl_clk_lpi2c1, mux_clkroot_lpi2c1_osc24mout, 1, 1 },
		{ pctl_clk_lpi2c2, mux_clkroot_lpi2c2_osc24mout, 1, 1 },
		{ pctl_clk_lpi2c3, mux_clkroot_lpi2c3_osc24mout, 1, 1 },
		{ pctl_clk_lpi2c4, mux_clkroot_lpi2c4_osc24mout, 1, 1 },
		{ pctl_clk_lpi2c5, mux_clkroot_lpi2c5_osc24mout, 1, 1 },
		{ pctl_clk_lpi2c6, mux_clkroot_lpi2c6_osc24mout, 1, 1 },
		{ pctl_clk_lpspi1, mux_clkroot_lpspi1_osc24mout, 1, 1 },
		{ pctl_clk_lpspi2, mux_clkroot_lpspi2_osc24mout, 1, 1 },
		{ pctl_clk_lpspi3, mux_clkroot_lpspi3_osc24mout, 1, 1 },
		{ pctl_clk_lpspi4, mux_clkroot_lpspi4_osc24mout, 1, 1 },
		{ pctl_clk_lpspi5, mux_clkroot_lpspi5_osc24mout, 1, 1 },
		{ pctl_clk_lpspi6, mux_clkroot_lpspi6_osc24mout, 1, 1 },
		{ pctl_clk_emv1, mux_clkroot_emv1_osc24mout, 1, 1 },
		{ pctl_clk_emv2, mux_clkroot_emv2_osc24mout, 1, 1 },
		{ pctl_clk_enet1, mux_clkroot_enet1_osc24mout, 1, 1 },
		{ pctl_clk_enet2, mux_clkroot_enet2_osc24mout, 1, 1 },
		{ pctl_clk_enet_qos, mux_clkroot_enet_qos_osc24mout, 1, 1 },
		{ pctl_clk_enet_25m, mux_clkroot_enet_25m_osc24mout, 1, 1 },
		{ pctl_clk_enet_timer1, mux_clkroot_enet_timer1_osc24mout, 1, 1 },
		{ pctl_clk_enet_timer2, mux_clkroot_enet_timer2_osc24mout, 1, 1 },
		{ pctl_clk_enet_timer3, mux_clkroot_enet_timer3_osc24mout, 1, 1 },
		{ pctl_clk_usdhc1, mux_clkroot_usdhc1_osc24mout, 1, 1 },
		{ pctl_clk_usdhc2, mux_clkroot_usdhc2_osc24mout, 1, 1 },
		{ pctl_clk_asrc, mux_clkroot_asrc_osc24mout, 1, 1 },
		{ pctl_clk_mqs, mux_clkroot_mqs_osc24mout, 1, 1 },
		{ pctl_clk_mic, mux_clkroot_mic_osc24mout, 1, 1 },
		{ pctl_clk_spdif, mux_clkroot_spdif_osc24mout, 1, 1 },
		{ pctl_clk_sai1, mux_clkroot_sai1_osc24mout, 1, 1 },
		{ pctl_clk_sai2, mux_clkroot_sai2_osc24mout, 1, 1 },
		{ pctl_clk_sai3, mux_clkroot_sai3_osc24mout, 1, 1 },
		{ pctl_clk_sai4, mux_clkroot_sai4_osc24mout, 1, 1 },
		/* NOTE: "pctl_clk_gpu2d" not using video peripheral - clock turned off */
		{ pctl_clk_gpu2d, mux_clkroot_gpu2d_videopllout, 2, 0 },
		{ pctl_clk_lcdif, mux_clkroot_lcdif_osc24mout, 1, 1 },
		{ pctl_clk_lcdifv2, mux_clkroot_lcdifv2_osc24mout, 1, 1 },
		{ pctl_clk_mipi_ref, mux_clkroot_mipi_ref_osc24mout, 1, 1 },
		{ pctl_clk_mipi_esc, mux_clkroot_mipi_esc_osc24mout, 1, 1 },
		{ pctl_clk_csi2, mux_clkroot_csi2_osc24mout, 1, 1 },
		{ pctl_clk_csi2_esc, mux_clkroot_csi2_esc_osc24mout, 1, 1 },
		{ pctl_clk_csi2_ui, mux_clkroot_csi2_ui_osc24mout, 1, 1 },
		{ pctl_clk_csi, mux_clkroot_csi_osc24mout, 1, 1 },
		{ pctl_clk_clko1, mux_clkroot_cko1_osc24mout, 1, 1 },
		{ pctl_clk_clko2, mux_clkroot_cko2_osc24mout, 1, 1 },
	};

	for (n = 0; n < sizeof(clktree) / sizeof(clktree[0]); n++) {
		/* NOTE: fraction divider is not used */
		_imxrt_setDevClock(clktree[n].root, clktree[n].div - 1, clktree[n].mux, 0, 0, clktree[n].isOn);
	}
}


static void _imxrt_initClocks(void)
{
#ifdef BYPASS_ANADIG_LDO
	_imxrt_pmuBypassAnaLdo();
	_imxrt_pmuBypassDigLdo();
#endif

	/* Initialize 24 MHz osc */
	*(imxrt_common.anadig_pll + osc_16m_ctrl) |= (1uL << 8u) | (1uL << 1u);

	/* Init 400 MHz RC osc */
	*(imxrt_common.anadig_pll + osc_400m_ctrl1) &= ~1u;
	*(imxrt_common.anadig_pll + osc_400m_ctrl2) |= 1u;
	*(imxrt_common.anadig_pll + osc_400m_ctrl1) |= 1uL << 1u;

	/* Init 48 MHz RC osc */
	*(imxrt_common.anadig_pll + osc_48m_ctrl) |= 1uL << 1u;

	/* Enables 24MHz clock source from 48MHz RC osc */
	*(imxrt_common.anadig_pll + osc_48m_ctrl) |= 1uL << 24u;

	/* Configure 24 MHz RC osc */
	*(imxrt_common.anadig_pll + osc_24m_ctrl) |= (1uL << 4u) | (1uL << 2u);
	hal_cpuDataSyncBarrier();

	while ((*(imxrt_common.anadig_pll + osc_24m_ctrl) & (1uL << 30u)) == 0u) {
	}

	/* Make sure main clocks are not using ARM PLL, PLL1, PLL2, PLL3 and XTAL */
	_imxrt_setDevClock(pctl_clk_m7, 0, mux_clkroot_m7_oscrc48mdiv2, 0, 0, 1);
	_imxrt_setDevClock(pctl_clk_m7_systick, 239, mux_clkroot_m7_oscrc48mdiv2, 0, 0, 1);
	_imxrt_setDevClock(pctl_clk_bus, 0, mux_clkroot_bus_oscrc48mdiv2, 0, 0, 1);
	_imxrt_setDevClock(pctl_clk_bus_lpsr, 0, mux_clkroot_bus_lpsr_oscrc48mdiv2, 0, 0, 1);

	/* bootrom already configured M4 core to RC OSC if M4 core is present */
	/* _imxrt_setDevClock(pctl_clk_m4, 0, mux_clkroot_m4_oscrc48mdiv2, 0, 0, 1); */
	/* _imxrt_setDevClock(pctl_clk_m4_systick, 239, mux_clkroot_m4_systick_oscrc48mdiv2, 0, 0, 1); */

	/* Power up ARM PLL, PLL3 slices */
	_imxrt_pmuEnablePllLdo();

	/* FIXME: Improve target platform CPU speed selector */

#ifdef PLATFORM_CONSUMER_MARKET
	/* commercial-qualified devices up to 1GHz */

	/* Initialize ARM PLL to 996 MHz */
	_imxrt_initArmPll(166, 0);
	imxrt_common.cpuclk = 996000000u;
#else
	/* industrial-qualified devices up to 800MHz */

	/* Initialize ARM PLL to 798 MHz */
	_imxrt_initArmPll(133, 0);
	imxrt_common.cpuclk = 798000000u;

	/* Initialize ARM PLL to 696 MHz */
	/* _imxrt_initArmPll(116, 0); */
	/* imxrt_common.cpuclk = 696000000u; */
#endif

#ifdef CLOCK_SYS_PLL1_ENABLE
	_imxrt_initSysPll1();
#else
	/* Bypass and deinitialize SYS_PLL1 */
	_imxrt_setPllBypass(clk_pllsys1, 1);
	_imxrt_deinitSysPll1();
#endif
	/* TODO: Init PLL2 fixed 528 MHz */
	/* _imxrt_initSysPll2(); */

	_imxrt_setPfdPllFracClock(0, clk_pllsys2, 27);
	_imxrt_setPfdPllFracClock(1, clk_pllsys2, 16);
	_imxrt_setPfdPllFracClock(2, clk_pllsys2, 24);
	_imxrt_setPfdPllFracClock(3, clk_pllsys2, 32);

	/* Init PLL3 fixed 480MHz */
	_imxrt_initSysPll3();

	_imxrt_setPfdPllFracClock(0, clk_pllsys3, 13);
	_imxrt_setPfdPllFracClock(1, clk_pllsys3, 17);
	_imxrt_setPfdPllFracClock(2, clk_pllsys3, 32);
	_imxrt_setPfdPllFracClock(3, clk_pllsys3, 24);

	/* Module clock root configurations */
	_imxrt_initClockTree();
}


void _imxrt_wdgReload(void)
{
	if ((*(imxrt_common.wdog1 + wdog_wcr) & (1u << 2)) != 0u) {
		*(imxrt_common.wdog1 + wdog_wsr) = 0x5555;
		hal_cpuDataMemoryBarrier();
		*(imxrt_common.wdog1 + wdog_wsr) = 0xaaaa;
	}
}


static int wdog1_irqHandler(unsigned int n, void *data)
{
	if ((*(imxrt_common.wdog1 + wdog_wicr) & (1u << 14)) != 0u) {
		*(imxrt_common.wdog1 + wdog_wicr) |= (1u << 14);
		_imxrt_wdgReload();
	}

	return 0;
}


void _imxrt_wdgAutoReload(int enable)
{
	if (enable != 0) {
		if ((*(imxrt_common.wdog1 + wdog_wcr) & (1u << 2)) != 0u) {
			/* Enable Watchdog Timer Interrupt and set WICT interrupt to 1s before watchdog timeout.
			Must be done in a single write - both fields are locked after one write */
			*(imxrt_common.wdog1 + wdog_wicr) = (*(imxrt_common.wdog1 + wdog_wicr) & ~0xffu) | (1u << 15) | 2u;
			hal_interruptsSet(wdog1_irq, wdog1_irqHandler, NULL);
		}
	}
	else {
		/* Watchdog Timer Interrupt Enable bit cannot be unset once set,
		so the interrupt must be masked when disabling auto mode */
		hal_interruptsSet(wdog1_irq, NULL, NULL);
	}
}


/* CM4 */


int _imxrt_setVtorCM4(int dwpLock, int dwp, addr_t vtor)
{
	u32 tmp;

	/* is CM4 running already ? */
	if ((imxrt_common.cm4state & 1u) != 0u) {
		return -1;
	}

	tmp = *(imxrt_common.iomuxc_lpsr_gpr + 0u);
	tmp |= *(imxrt_common.iomuxc_lpsr_gpr + 1u);

	/* is DWP locked or CM7 forbidden ? */
	if ((tmp & (0xduL << 28u)) != 0u) {
		return -1;
	}

	tmp = ((((u32)dwpLock & 3u) << 30u) | (((u32)dwp & 3u) << 28u));

	*(imxrt_common.iomuxc_lpsr_gpr + 0u) = (tmp | (vtor & 0xfff8u));
	*(imxrt_common.iomuxc_lpsr_gpr + 1u) = (tmp | ((vtor >> 16) & 0xffffu));

	hal_cpuDataMemoryBarrier();

	imxrt_common.cm4state |= 2u;

	return 0;
}


void _imxrt_runCM4(void)
{
	/* CM7 is allowed to reset system, CM4 is disallowed */
	*(imxrt_common.src + src_srmr) |= ((3uL << 10u) | (3uL << 6u));
	hal_cpuDataMemoryBarrier();

	/* Release CM4 reset */
	*(imxrt_common.src + src_scr) |= 1u;
	hal_cpuDataSyncBarrier();

	imxrt_common.cm4state |= 1u;
}


u32 _imxrt_getStateCM4(void)
{
	return imxrt_common.cm4state;
}


int _imxrt_wdgInfo(imxrt_wdgInfo_t *info)
{
	u32 val;
	u32 tmp;
	int res;

	tmp = *(imxrt_common.wdog1 + wdog_wcr);
	info->enabled = ((tmp & (1u << 2)) != 0u) ? 1u : 0u;
	info->timeoutMs = ((tmp >> 8) * 500u) + 500u;

	/* Get WDOG_ENABLE fuse */

	res = otp_read(0x9A0, &val);
	if (res < 0) {
		return res;
	}

	info->platformEnabled = (val & (1u << 15)) ? 1 : 0;

	/* Get WDOG Timeout Select fuse */

	res = otp_read(0x9B0, &val);
	if (res < 0) {
		return res;
	}

	info->defaultTimeoutMs = ((val & 0x7u) <= 3u) ? ((8u << (3u - (val & 0x7u))) * 1000u) : 0u;
	return 0;
}


void _imxrt_done(void)
{
	_imxrt_wdgAutoReload(0);
}


void _imxrt_init(void)
{
	int i;
	u32 tmp;
	volatile u32 *reg;

	imxrt_common.aips[0] = (void *)0x40000000;
	imxrt_common.aips[1] = (void *)0x40400000;
	imxrt_common.aips[2] = (void *)0x40800000;
	imxrt_common.aips[3] = (void *)0x40c00000;
	imxrt_common.stk = (void *)0xe000e010;
	imxrt_common.wdog1 = (void *)0x40030000;
	imxrt_common.wdog2 = (void *)0x40034000;
	imxrt_common.wdog3 = (void *)0x40038000;
	imxrt_common.wdog4 = (void *)0x40c10000;
	imxrt_common.src = (void *)0x40c04000;
	imxrt_common.iomuxc_snvs = (void *)0x40c94000;
	imxrt_common.iomuxc_lpsr = (void *)0x40c08000;
	imxrt_common.iomuxc_lpsr_gpr = (void *)0x40c0c000;
	imxrt_common.iomuxc_gpr = (void *)0x400e4000;
	imxrt_common.iomuxc = (void *)0x400e8000;
	imxrt_common.ccm = (void *)0x40cc0000;
	imxrt_common.anadig_pll = (void *)0x40c84000;

	imxrt_common.gpio[0] = (void *)0x4012c000;
	imxrt_common.gpio[1] = (void *)0x40130000;
	imxrt_common.gpio[2] = (void *)0x40134000;
	imxrt_common.gpio[3] = (void *)0x40138000;
	imxrt_common.gpio[4] = (void *)0x4013c000;
	imxrt_common.gpio[5] = (void *)0x40140000;
	imxrt_common.gpio[6] = (void *)0x40c5c000;
	imxrt_common.gpio[7] = (void *)0x40c60000;
	imxrt_common.gpio[8] = (void *)0x40c64000;
	imxrt_common.gpio[9] = (void *)0x40c68000;
	imxrt_common.gpio[10] = (void *)0x40c6c000;
	imxrt_common.gpio[11] = (void *)0x40c70000;
	imxrt_common.gpio[12] = (void *)0x40ca0000;

	imxrt_common.cm4state = (*(imxrt_common.src + src_scr) & 1u);

	_imxrt_initClocks();

	/* Disable WDOG1 and WDOG2's Power Down Counter */
	*(imxrt_common.wdog1 + wdog_wmcr) &= ~1u;
	*(imxrt_common.wdog2 + wdog_wmcr) &= ~1u;

	/* WDOG1 and WDOG2 can't be disabled once enabled */

	/* Enabling the watchdog and setting the timeout are separate actions controlled by WATCHDOG_PLO and
	WATCHDOG_PLO_TIMEOUT_MS. This way it is possible to change the timeout if the watchdog was already
	enabled by bootrom, without enabling it in the other case. */
#if defined(WATCHDOG_PLO_TIMEOUT_MS)
	/* Set the timeout (always possible) */
	tmp = *(imxrt_common.wdog1 + wdog_wcr) & ~(0xffu << 8);
	*(imxrt_common.wdog1 + wdog_wcr) = tmp | (((WATCHDOG_PLO_TIMEOUT_MS - 500u) / 500u) << 8);
	hal_cpuDataMemoryBarrier();
#endif
#if WATCHDOG_PLO
	/* Enable the watchdog */
	*(imxrt_common.wdog1 + wdog_wcr) |= (1u << 2);
	hal_cpuDataMemoryBarrier();
#endif
#if defined(WATCHDOG_PLO_TIMEOUT_MS)
	/* Reload the watchdog with a new timeout value in case it was already enabled by
	bootrom and was running with a different timeout */
	_imxrt_wdgReload();
#endif

	if ((*(imxrt_common.wdog3 + rtwdog_cs) & (1u << 7)) != 0u) {
		/* Unlock WDOG3 */
		*(imxrt_common.wdog3 + rtwdog_cnt) = 0xd928c520; /* Update key */
		hal_cpuDataMemoryBarrier();
		while ((*(imxrt_common.wdog3 + rtwdog_cs) & (1u << 11u)) == 0u) {
		}

		/* Disable WDOG3, but allow later reconfiguration without reset */
		*(imxrt_common.wdog3 + rtwdog_toval) = 0xffffu;
		*(imxrt_common.wdog3 + rtwdog_cs) = (*(imxrt_common.wdog3 + rtwdog_cs) & ~(1u << 7)) | (1u << 5);

		/* Wait until new config takes effect */
		while ((*(imxrt_common.wdog3 + rtwdog_cs) & (1u << 10)) == 0u) {
		}

		/* Wait until registers are locked (in case low power mode will be used promptly) */
		while ((*(imxrt_common.wdog3 + rtwdog_cs) & (1u << 11)) != 0u) {
		}
	}

	if ((*(imxrt_common.wdog4 + rtwdog_cs) & (1u << 7)) != 0u) {
		/* Unlock WDOG4 */
		*(imxrt_common.wdog4 + rtwdog_cnt) = 0xd928c520; /* Update key */
		hal_cpuDataMemoryBarrier();
		while ((*(imxrt_common.wdog4 + rtwdog_cs) & (1u << 11u)) == 0u) {
		}

		/* Disable WDOG4, but allow later reconfiguration without reset */
		*(imxrt_common.wdog4 + rtwdog_toval) = 0xffffu;
		*(imxrt_common.wdog4 + rtwdog_cs) = (*(imxrt_common.wdog4 + rtwdog_cs) & ~(1u << 7)) | (1u << 5);

		/* Wait until new config takes effect */
		while ((*(imxrt_common.wdog4 + rtwdog_cs) & (1u << 10)) == 0u) {
		}

		/* Wait until registers are locked (in case low power mode will be used promptly) */
		while ((*(imxrt_common.wdog4 + rtwdog_cs) & (1u << 11)) != 0u) {
		}
	}

	/* Disable Systick which might be enabled by bootrom */
	if ((*(imxrt_common.stk + stk_ctrl) & 1) != 0) {
		*(imxrt_common.stk + stk_ctrl) &= ~1;
	}

	/* HACK - temporary fix for crash when booting from FlexSPI */
	_imxrt_setDirectLPCG(pctl_lpcg_semc, 0);

	/* Disable USB cache (set by bootrom) */
	*(imxrt_common.iomuxc_gpr + 28) &= ~(1 << 13);
	hal_cpuDataMemoryBarrier();

	/* Store reset flags and then clean them */
	imxrt_common.resetFlags = *(imxrt_common.src + src_srsr);
	*(imxrt_common.src + src_srsr) = 0xffffffffu;
	hal_cpuDataSyncBarrier();

	/* Reconfigure all IO pads as slow slew-rate and low drive strength */
	for (i = pctl_pad_gpio_emc_b1_00; i <= pctl_pad_gpio_disp_b2_15; ++i) {
		reg = _imxrt_IOpadGetReg(i);
		if (reg == NULL) {
			continue;
		}

		*reg |= 1 << 1;
		hal_cpuDataMemoryBarrier();
	}

	for (; i <= pctl_pad_gpio_lpsr_15; ++i) {
		reg = _imxrt_IOpadGetReg(i);
		if (reg == NULL) {
			continue;
		}

		*reg &= ~0x3;
		hal_cpuDataMemoryBarrier();
	}
}
