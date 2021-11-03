/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * iMXRT basic peripherals control functions
 *
 * Copyright 2017, 2019, 2020 Phoenix Systems
 * Author: Aleksander Kaminski, Jan Sikorski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "imxrt.h"
#include "../../cpu.h"

enum { stk_ctrl = 0, stk_load, stk_val, stk_calib };


enum { aipstz_mpr = 0, aipstz_opacr = 16, aipstz_opacr1, aipstz_opacr2, aipstz_opacr3, aipstz_opacr4 };


enum { src_scr = 0, src_sbmr1, src_srsr, src_sbmr2 = 7, src_gpr1, src_gpr2, src_gpr3, src_gpr4,
	src_gpr5, src_gpr6, src_gpr7, src_gpr8, src_gpr9, src_gpr10 };

enum { wdog_wcr = 0, wdog_wsr, wdog_wrsr, wdog_wicr, wdog_wmcr };


enum { rtwdog_cs = 0, rtwdog_cnt, rtwdog_total, rtwdog_win };


struct {
	volatile u32 *aips[4];
	volatile u32 *stk;
	volatile u32 *src;
	volatile u16 *wdog1;
	volatile u16 *wdog2;
	volatile u32 *wdog3;
	volatile u32 *iomux_snvs;
	volatile u32 *iomux_lpsr;
	volatile u32 *iomuxc_gpr;
	volatile u32 *iomuxc;
	volatile u32 *ccm;

	u32 cpuclk;
} imxrt_common;



/* IOMUX */


static volatile u32 *_imxrt_IOmuxGetReg(int mux)
{
	if (mux < pctl_mux_gpio_emc_b1_00 || mux > pctl_mux_gpio_lpsr_15)
		return NULL;

	if (mux < pctl_mux_wakeup)
		return imxrt_common.iomuxc + 4 + mux - pctl_mux_gpio_emc_b1_00;

	if (mux < pctl_mux_gpio_lpsr_00)
		return imxrt_common.iomux_snvs + mux - pctl_mux_wakeup;

	return imxrt_common.iomux_lpsr + mux - pctl_mux_gpio_lpsr_00;
}


int _imxrt_setIOmux(int mux, char sion, char mode)
{
	volatile u32 *reg;

	if ((reg = _imxrt_IOmuxGetReg(mux)) == NULL)
		return -1;

	(*reg) = (!!sion << 4) | (mode & 0xf);
	imxrt_dataBarrier();

	return 0;
}


static volatile u32 *_imxrt_IOpadGetReg(int pad)
{
	if (pad < pctl_pad_gpio_emc_b1_00 || pad > pctl_pad_gpio_lpsr_15)
		return NULL;

	if (pad < pctl_pad_test_mode)
		return imxrt_common.iomuxc + pad + 149 - pctl_pad_gpio_emc_b1_00;

	if (pad < pctl_pad_gpio_lpsr_00)
		return imxrt_common.iomux_snvs + pad + 14 - pctl_pad_test_mode;

	return imxrt_common.iomux_lpsr + pad + 16 - pctl_pad_gpio_lpsr_00;
}


int _imxrt_setIOpad(int pad, char sre, char dse, char pue, char pus, char ode, char apc)
{
	u32 t;
	volatile u32 *reg;
	char pull;

	if ((reg = _imxrt_IOpadGetReg(pad)) == NULL)
		return -1;

	if (pad >= pctl_pad_gpio_emc_b1_00 && pad <= pctl_pad_gpio_disp_b2_15) {
		/* Fields have slightly diffrent meaning... */
		if (!pue)
			pull = 3;
		else if (pus)
			pull = 1;
		else
			pull = 2;

		t = *reg & ~0x1e;
		t |= (!!dse << 1) | (pull << 2) | (!!ode << 4);
	}
	else {
		t = *reg & ~0x1f;
		t |= (!!sre) | (!!dse << 1) | (!!pue << 2) | (!!pus << 3);

		if (pad >= pctl_pad_test_mode && pad <= pctl_pad_gpio_snvs_09) {
			t &= ~(1 << 6);
			t |= !!ode << 6;
		}
		else {
			t &= ~(1 << 5);
			t |= !!ode << 5;
		}
	}

	/* APC field is not documented. Leave it alone for now. */
	//t &= ~(0xf << 28);
	//t |= (apc & 0xf) << 28;

	(*reg) = t;
	imxrt_dataBarrier();

	return 0;
}


static volatile u32 *_imxrt_IOiselGetReg(int isel, u32 *mask)
{
	if (isel < pctl_isel_flexcan1_rx || isel > pctl_isel_sai4_txsync)
		return NULL;

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

	if (isel >= pctl_isel_can3_canrx)
		return imxrt_common.iomux_lpsr + 32 + isel - pctl_isel_can3_canrx;

	return imxrt_common.iomuxc + 294 + isel - pctl_isel_flexcan1_rx;
}


int _imxrt_setIOisel(int isel, char daisy)
{
	volatile u32 *reg;
	u32 mask;

	if ((reg = _imxrt_IOiselGetReg(isel, &mask)) == NULL)
		return -1;

	(*reg) = daisy & mask;
	imxrt_dataBarrier();

	return 0;
}


/* CCM */

int _imxrt_setDevClock(int clock, int div, int mux, int mfd, int mfn, int state)
{
	unsigned int t;
	volatile u32 *reg = imxrt_common.ccm + (clock * 0x20);

	if (clock < pctl_clk_cm7 || clock > pctl_clk_ccm_clko2)
		return -1;

	t = *reg & ~0x01ff07ffu;
	*reg = t | (!state << 24) | ((mfn & 0xf) << 20) | ((mfd & 0xf) << 16) | ((mux & 0x7) << 8) | (div & 0xff);

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();

	return 0;
}


int _imxrt_setDirectLPCG(int clock, int state)
{
	u32 t;
	volatile u32 *reg;

	if (clock < pctl_lpcg_m7 || clock > pctl_lpcg_uniq_edt_i)
		return -1;

	reg = imxrt_common.ccm + 0x1800 + clock * 0x8;

	t = *reg & ~1u;
	*reg = t | (state & 1);

	imxrt_dataBarrier();
	imxrt_dataInstrBarrier();

	return 0;
}


int _imxrt_setLevelLPCG(int clock, int level)
{
	volatile u32 *reg;

	if (clock < pctl_lpcg_m7 || clock > pctl_lpcg_uniq_edt_i)
		return -1;

	if (level < 0 || level > 4)
		return -1;

	reg = imxrt_common.ccm + 0x1801 + clock * 0x8;
	*reg = (level << 28) | (level << 24) | (level << 20) | (level << 16) | level;

	imxrt_dataBarrier();
	imxrt_dataInstrBarrier();

	return 0;
}


void _imxrt_init(void)
{
	int i;
	volatile u32 *reg;

	imxrt_common.aips[0] = (void *)0x40000000;
	imxrt_common.aips[1] = (void *)0x40400000;
	imxrt_common.aips[2] = (void *)0x40800000;
	imxrt_common.aips[3] = (void *)0x40c00000;
	imxrt_common.stk = (void *)0xe000e010;
	imxrt_common.wdog1 = (void *)0x40030000;
	imxrt_common.wdog2 = (void *)0x40034000;
	imxrt_common.wdog3 = (void *)0x40038000;
	imxrt_common.src = (void *)0x40c04000;
	imxrt_common.iomux_snvs = (void *)0x40c94000;
	imxrt_common.iomux_lpsr = (void *)0x40c08000;
	imxrt_common.iomuxc_gpr = (void *)0x400e4000;
	imxrt_common.iomuxc = (void *)0x400e8000;
	imxrt_common.ccm = (void *)0x40cc0000;

	imxrt_common.cpuclk = 640000000;

	/* Disable watchdogs */
	if (*(imxrt_common.wdog1 + wdog_wcr) & (1 << 2))
		*(imxrt_common.wdog1 + wdog_wcr) &= ~(1 << 2);
	if (*(imxrt_common.wdog2 + wdog_wcr) & (1 << 2))
		*(imxrt_common.wdog2 + wdog_wcr) &= ~(1 << 2);

	*(imxrt_common.wdog3 + rtwdog_cnt) = 0xd928c520; /* Update key */
	*(imxrt_common.wdog3 + rtwdog_total) = 0xffff;
	*(imxrt_common.wdog3 + rtwdog_cs) |= 1 << 5;
	*(imxrt_common.wdog3 + rtwdog_cs) &= ~(1 << 7);

	/* Disable Systick which might be enabled by bootrom */
	if (*(imxrt_common.stk + stk_ctrl) & 1)
		*(imxrt_common.stk + stk_ctrl) &= ~1;

	/* HACK - temporary fix for crash when booting from FlexSPI */
	_imxrt_setDirectLPCG(pctl_lpcg_semc, 0);

	/* Disable USB cache (set by bootrom) */
	*(imxrt_common.iomuxc_gpr + 28) &= ~(1 << 13);
	imxrt_dataBarrier();

	/* Clear SRSR */
	*(imxrt_common.src + src_srsr) = 0xffffffffu;

	/* Reconfigure all IO pads as slow slew-rate and low drive strength */
	for (i = pctl_pad_gpio_emc_b1_00; i <= pctl_pad_gpio_disp_b2_15; ++i) {
		if ((reg = _imxrt_IOpadGetReg(i)) == NULL)
			continue;

		*reg |= 1 << 1;
		imxrt_dataBarrier();
	}

	for (; i <= pctl_pad_gpio_lpsr_15; ++i) {
		if ((reg = _imxrt_IOpadGetReg(i)) == NULL)
			continue;

		*reg &= ~0x3;
		imxrt_dataBarrier();
	}

	/* Enable UsageFault, BusFault and MemManage exceptions */
	*(imxrt_common.scb + scb_shcsr) |= (1 << 16) | (1 << 17) | (1 << 18);
	imxrt_dataBarrier();
}
