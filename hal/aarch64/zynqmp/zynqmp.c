/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ZynqMP basic peripherals control functions
 * based on: Zynq UltraScale+ Technical Reference Manual UG1085 (v2.4)
 *
 * Copyright 2021, 2024 Phoenix Systems
 * Author: Hubert Buczynski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "zynqmp.h"
#include "zynqmp_regs.h"
#include "config.h"
#include <hal/hal.h>

#include <board_config.h>

#define CSU_BASE_ADDRESS        0xffca0000
#define IOU_SLCR_BASE_ADDRESS   0xff180000
#define CRF_APB_BASE_ADDRESS    0xfd1a0000
#define CRL_APB_BASE_ADDRESS    0xff5e0000
#define APU_BASE_ADDRESS        0xfd5c0000
#define RPU_BASE_ADDRESS        0xff9a0000
#define LPD_SLCR_BASE_ADDRESS   0xff410000
#define FPD_SLCR_BASE_ADDRESS   0xfd610000
#define CSUDMA_BASE_ADDRESS     0xffc80000
#define PMU_GLOBAL_BASE_ADDRESS 0xffd80000


/* PLO entrypoint */
extern void _start(void);

extern int _zynqmp_ddrInit(void);

static struct {
	volatile u32 *csu;
	volatile u32 *csudma;
	volatile u32 *pmu_global;
	volatile u32 *iou_slcr;
	volatile u32 *apu;
	volatile u32 *crf_apb;
	volatile u32 *crl_apb;
	u32 resetFlags;
} zynq_common;


static int _zynqmp_parseReset(int dev, volatile u32 **reg, u32 *bit)
{
	static const u32 lookup[] = {
		[ctl_reset_lpd_gem0] = crl_apb_rst_lpd_iou0 | (0 << 12),
		[ctl_reset_lpd_gem1] = crl_apb_rst_lpd_iou0 | (1 << 12),
		[ctl_reset_lpd_gem2] = crl_apb_rst_lpd_iou0 | (2 << 12),
		[ctl_reset_lpd_gem3] = crl_apb_rst_lpd_iou0 | (3 << 12),
		[ctl_reset_lpd_qspi] = crl_apb_rst_lpd_iou2 | (0 << 12),
		[ctl_reset_lpd_uart0] = crl_apb_rst_lpd_iou2 | (1 << 12),
		[ctl_reset_lpd_uart1] = crl_apb_rst_lpd_iou2 | (2 << 12),
		[ctl_reset_lpd_spi0] = crl_apb_rst_lpd_iou2 | (3 << 12),
		[ctl_reset_lpd_spi1] = crl_apb_rst_lpd_iou2 | (4 << 12),
		[ctl_reset_lpd_sdio0] = crl_apb_rst_lpd_iou2 | (5 << 12),
		[ctl_reset_lpd_sdio1] = crl_apb_rst_lpd_iou2 | (6 << 12),
		[ctl_reset_lpd_can0] = crl_apb_rst_lpd_iou2 | (7 << 12),
		[ctl_reset_lpd_can1] = crl_apb_rst_lpd_iou2 | (8 << 12),
		[ctl_reset_lpd_i2c0] = crl_apb_rst_lpd_iou2 | (9 << 12),
		[ctl_reset_lpd_i2c1] = crl_apb_rst_lpd_iou2 | (10 << 12),
		[ctl_reset_lpd_ttc0] = crl_apb_rst_lpd_iou2 | (11 << 12),
		[ctl_reset_lpd_ttc1] = crl_apb_rst_lpd_iou2 | (12 << 12),
		[ctl_reset_lpd_ttc2] = crl_apb_rst_lpd_iou2 | (13 << 12),
		[ctl_reset_lpd_ttc3] = crl_apb_rst_lpd_iou2 | (14 << 12),
		[ctl_reset_lpd_swdt] = crl_apb_rst_lpd_iou2 | (15 << 12),
		[ctl_reset_lpd_nand] = crl_apb_rst_lpd_iou2 | (16 << 12),
		[ctl_reset_lpd_lpd_dma] = crl_apb_rst_lpd_iou2 | (17 << 12),
		[ctl_reset_lpd_gpio] = crl_apb_rst_lpd_iou2 | (18 << 12),
		[ctl_reset_lpd_iou_cc] = crl_apb_rst_lpd_iou2 | (19 << 12),
		[ctl_reset_lpd_timestamp] = crl_apb_rst_lpd_iou2 | (20 << 12),
		[ctl_reset_lpd_rpu_r50] = crl_apb_rst_lpd_top | (0 << 12),
		[ctl_reset_lpd_rpu_r51] = crl_apb_rst_lpd_top | (1 << 12),
		[ctl_reset_lpd_rpu_amba] = crl_apb_rst_lpd_top | (2 << 12),
		[ctl_reset_lpd_ocm] = crl_apb_rst_lpd_top | (3 << 12),
		[ctl_reset_lpd_rpu_pge] = crl_apb_rst_lpd_top | (4 << 12),
		[ctl_reset_lpd_usb0_corereset] = crl_apb_rst_lpd_top | (6 << 12),
		[ctl_reset_lpd_usb1_corereset] = crl_apb_rst_lpd_top | (7 << 12),
		[ctl_reset_lpd_usb0_hiberreset] = crl_apb_rst_lpd_top | (8 << 12),
		[ctl_reset_lpd_usb1_hiberreset] = crl_apb_rst_lpd_top | (9 << 12),
		[ctl_reset_lpd_usb0_apb] = crl_apb_rst_lpd_top | (10 << 12),
		[ctl_reset_lpd_usb1_apb] = crl_apb_rst_lpd_top | (11 << 12),
		[ctl_reset_lpd_ipi] = crl_apb_rst_lpd_top | (14 << 12),
		[ctl_reset_lpd_apm] = crl_apb_rst_lpd_top | (15 << 12),
		[ctl_reset_lpd_rtc] = crl_apb_rst_lpd_top | (16 << 12),
		[ctl_reset_lpd_sysmon] = crl_apb_rst_lpd_top | (17 << 12),
		[ctl_reset_lpd_s_axi_lpd] = crl_apb_rst_lpd_top | (19 << 12),
		[ctl_reset_lpd_lpd_swdt] = crl_apb_rst_lpd_top | (20 << 12),
		[ctl_reset_lpd_fpd] = crl_apb_rst_lpd_top | (23 << 12),
		[ctl_reset_lpd_dbg_fpd] = crl_apb_rst_lpd_dbg | (0 << 12),
		[ctl_reset_lpd_dbg_lpd] = crl_apb_rst_lpd_dbg | (1 << 12),
		[ctl_reset_lpd_rpu_dbg0] = crl_apb_rst_lpd_dbg | (4 << 12),
		[ctl_reset_lpd_rpu_dbg1] = crl_apb_rst_lpd_dbg | (5 << 12),
		[ctl_reset_lpd_dbg_ack] = crl_apb_rst_lpd_dbg | (15 << 12),
		[ctl_reset_fpd_sata] = crf_apb_rst_fpd_top | (1 << 12),
		[ctl_reset_fpd_gt] = crf_apb_rst_fpd_top | (2 << 12),
		[ctl_reset_fpd_gpu] = crf_apb_rst_fpd_top | (3 << 12),
		[ctl_reset_fpd_gpu_pp0] = crf_apb_rst_fpd_top | (4 << 12),
		[ctl_reset_fpd_gpu_pp1] = crf_apb_rst_fpd_top | (5 << 12),
		[ctl_reset_fpd_fpd_dma] = crf_apb_rst_fpd_top | (6 << 12),
		[ctl_reset_fpd_s_axi_hpc_0_fpd] = crf_apb_rst_fpd_top | (7 << 12),
		[ctl_reset_fpd_s_axi_hpc_1_fpd] = crf_apb_rst_fpd_top | (8 << 12),
		[ctl_reset_fpd_s_axi_hp_0_fpd] = crf_apb_rst_fpd_top | (9 << 12),
		[ctl_reset_fpd_s_axi_hp_1_fpd] = crf_apb_rst_fpd_top | (10 << 12),
		[ctl_reset_fpd_s_axi_hpc_2_fpd] = crf_apb_rst_fpd_top | (11 << 12),
		[ctl_reset_fpd_s_axi_hpc_3_fpd] = crf_apb_rst_fpd_top | (12 << 12),
		[ctl_reset_fpd_swdt] = crf_apb_rst_fpd_top | (15 << 12),
		[ctl_reset_fpd_dp] = crf_apb_rst_fpd_top | (16 << 12),
		[ctl_reset_fpd_pcie_ctrl] = crf_apb_rst_fpd_top | (17 << 12),
		[ctl_reset_fpd_pcie_bridge] = crf_apb_rst_fpd_top | (18 << 12),
		[ctl_reset_fpd_pcie_cfg] = crf_apb_rst_fpd_top | (19 << 12),
		[ctl_reset_fpd_acpu0] = crf_apb_rst_fpd_apu | (0 << 12),
		[ctl_reset_fpd_acpu1] = crf_apb_rst_fpd_apu | (1 << 12),
		[ctl_reset_fpd_acpu2] = crf_apb_rst_fpd_apu | (2 << 12),
		[ctl_reset_fpd_acpu3] = crf_apb_rst_fpd_apu | (3 << 12),
		[ctl_reset_fpd_apu_l2] = crf_apb_rst_fpd_apu | (8 << 12),
		[ctl_reset_fpd_acpu0_pwron] = crf_apb_rst_fpd_apu | (10 << 12),
		[ctl_reset_fpd_acpu1_pwron] = crf_apb_rst_fpd_apu | (11 << 12),
		[ctl_reset_fpd_acpu2_pwron] = crf_apb_rst_fpd_apu | (12 << 12),
		[ctl_reset_fpd_acpu3_pwron] = crf_apb_rst_fpd_apu | (13 << 12),
		[ctl_reset_fpd_ddr_apm] = crf_apb_rst_ddr_ss | (2 << 12),
		[ctl_reset_fpd_ddr_block] = crf_apb_rst_ddr_ss | (3 << 12),
	};

	if ((dev < ctl_reset_lpd_gem0) || (dev > ctl_reset_fpd_ddr_block)) {
		return -1;
	}

	if (dev >= ctl_reset_fpd_sata) {
		*reg = zynq_common.crf_apb + (lookup[dev] & ((1 << 12) - 1));
	}
	else {
		*reg = zynq_common.crl_apb + (lookup[dev] & ((1 << 12) - 1));
	}

	*bit = (1u << (lookup[dev] >> 12));
	return 0;
}


int _zynqmp_devReset(int dev, int reset)
{
	volatile u32 *reg;
	u32 bit;

	if (_zynqmp_parseReset(dev, &reg, &bit) < 0) {
		return -1;
	}

	if (reset != 0) {
		*reg |= bit;
	}
	else {
		*reg &= ~bit;
	}

	hal_cpuDataSyncBarrier();
	return 0;
}


int _zynqmp_getDevReset(int dev, int *state)
{
	volatile u32 *reg;
	u32 bit;

	if (_zynqmp_parseReset(dev, &reg, &bit) < 0) {
		return -1;
	}

	*state = ((*reg & bit) != 0) ? 1 : 0;
	return 0;
}


int _zynqmp_setSysPll(const ctl_sys_pll_t *sys_pll)
{
	/* How system PLL switching works:
	 * * Select source for the PLL using src_pll
	 * * Select source for the bypass using src_bypass
	 * * Select PLL multiplier, divider and if you want to use fractional multiplier
	 * 	 * NOTE: source * multiplier must be within range for the VCO (1500 ~ 3000 MHz, see FPSPLLVCOMIN, FPSPLLVCOMAX)
	 * * Program *PLL_CFG based on selected multiplier
	 * * Activate PLL bypass
	 * * Reset-cycle PLL
	 * * Check for PLL stability
	 * * Deactivate PLL bypass
	 */

	/* clang-format off */
	static const u32 _zynqmp_sysPllCfgForMulInteger[] = {
							[25]  = 0x7e7d0c6a, [26]  = 0x7e7d0c6a, [27]  = 0x7e7d0c86, [28]  = 0x7e7d0c86, [29]  = 0x7e7d0c86, [30]  = 0x7e7d0c86, [31]  = 0x7e7d0cc1,
		[32]  = 0x7e7d0cc1, [33]  = 0x7e7d0c8a, [34]  = 0x7e7d0ca6, [35]  = 0x7e7d0ca6, [36]  = 0x7e7d0ca6, [37]  = 0x7e7d0ca6, [38]  = 0x7e79eca6, [39]  = 0x7e76cc6c,
		[40]  = 0x7e73ac6c, [41]  = 0x7e708c6c, [42]  = 0x7e6d6c6c, [43]  = 0x7e6a4c6c, [44]  = 0x7e6a4c6c, [45]  = 0x7e672c6c, [46]  = 0x7e640c6c, [47]  = 0x7e60ec6c,
		[48]  = 0x7e60ec6c, [49]  = 0x7e5dcc6c, [50]  = 0x7e5dcc6c, [51]  = 0x7e5aac62, [52]  = 0x7e578c62, [53]  = 0x7e578c62, [54]  = 0x7e546c62, [55]  = 0x7e546c62,
		[56]  = 0x7e514c62, [57]  = 0x7e514c62, [58]  = 0x7e4e2c62, [59]  = 0x7e4e2c62, [60]  = 0x7e4e2c62, [61]  = 0x7e4b0c62, [62]  = 0x7e4b0c62, [63]  = 0x7e4b0c62,
		[64]  = 0x7e4b0c62, [65]  = 0x7e4b0c62, [66]  = 0x7e4b0c62, [67]  = 0x7e4b0c62, [68]  = 0x7e4b0c62, [69]  = 0x7e4b0c62, [70]  = 0x7e4b0c62, [71]  = 0x7e4b0c62,
		[72]  = 0x7e4b0c62, [73]  = 0x7e4b0c62, [74]  = 0x7e4b0c62, [75]  = 0x7e4b0c62, [76]  = 0x7e4b0c62, [77]  = 0x7e4b0c62, [78]  = 0x7e4b0c62, [79]  = 0x7e4b0c62,
		[80]  = 0x7e4b0c62, [81]  = 0x7e4b0c62, [82]  = 0x7e4b0c62, [83]  = 0x7e4b0c82, [84]  = 0x7e4b0c82, [85]  = 0x7e4b0c82, [86]  = 0x7e4b0c82, [87]  = 0x7e4b0c82,
		[88]  = 0x7e4b0c82, [89]  = 0x7e4b0c82, [90]  = 0x7e4b0c82, [91]  = 0x7e4b0c82, [92]  = 0x7e4b0c82, [93]  = 0x7e4b0c82, [94]  = 0x7e4b0c82, [95]  = 0x7e4b0c82,
		[96]  = 0x7e4b0c82, [97]  = 0x7e4b0c82, [98]  = 0x7e4b0c82, [99]  = 0x7e4b0c82, [100] = 0x7e4b0c82, [101] = 0x7e4b0c82, [102] = 0x7e4b0c82, [103] = 0x7e4b0ca2,
		[104] = 0x7e4b0ca2, [105] = 0x7e4b0ca2, [106] = 0x7e4b0ca2, [107] = 0x7e4b0c64, [108] = 0x7e4b0c64, [109] = 0x7e4b0c64, [110] = 0x7e4b0c64, [111] = 0x7e4b0c64,
		[112] = 0x7e4b0c64, [113] = 0x7e4b0c64, [114] = 0x7e4b0c64, [115] = 0x7e4b0c64, [116] = 0x7e4b0c64, [117] = 0x7e4b0c64, [118] = 0x7e4b0c64, [119] = 0x7e4b0c64,
		[120] = 0x7e4b0c64, [121] = 0x7e4b0c64, [122] = 0x7e4b0c64, [123] = 0x7e4b0c64, [124] = 0x7e4b0c64, [125] = 0x7e4b0c64,
	};


	static const u32 _zynqmp_sysPllCfgForMulFractional[] = {
							[25]  = 0x7e7d0c65, [26]  = 0x7e7d0c65, [27]  = 0x7e7d0c69, [28]  = 0x7e7d0c69, [29]  = 0x7e7d0c69, [30]  = 0x7e7d0c69, [31]  = 0x7e7d0c6e,
		[32]  = 0x7e7d0c6e, [33]  = 0x7e7d0c6e, [34]  = 0x7e7d0c6e, [35]  = 0x7e7d0c6e, [36]  = 0x7e7d0c6e, [37]  = 0x7e7d0c6e, [38]  = 0x7e79ec6e, [39]  = 0x7e76cc6e,
		[40]  = 0x7e73ac6e, [41]  = 0x7e708c61, [42]  = 0x7e6d6c61, [43]  = 0x7e6a4c61, [44]  = 0x7e6a4c61, [45]  = 0x7e672c61, [46]  = 0x7e640c61, [47]  = 0x7e60ec61,
		[48]  = 0x7e60ec66, [49]  = 0x7e5dcc66, [50]  = 0x7e5dcc66, [51]  = 0x7e5aac66, [52]  = 0x7e578c66, [53]  = 0x7e578c66, [54]  = 0x7e546c66, [55]  = 0x7e546c66,
		[56]  = 0x7e514c66, [57]  = 0x7e514c66, [58]  = 0x7e4e2c66, [59]  = 0x7e4e2c66, [60]  = 0x7e4e2c66, [61]  = 0x7e4b0c66, [62]  = 0x7e4b0c66, [63]  = 0x7e4b0c66,
		[64]  = 0x7e4b0c66, [65]  = 0x7e4b0c66, [66]  = 0x7e4b0c6a, [67]  = 0x7e4b0c6a, [68]  = 0x7e4b0c6a, [69]  = 0x7e4b0c6a, [70]  = 0x7e4b0c6a, [71]  = 0x7e4b0c6a,
		[72]  = 0x7e4b0c6a, [73]  = 0x7e4b0c6a, [74]  = 0x7e4b0c6a, [75]  = 0x7e4b0c6a, [76]  = 0x7e4b0c6a, [77]  = 0x7e4b0c6a, [78]  = 0x7e4b0c6a, [79]  = 0x7e4b0c6a,
		[80]  = 0x7e4b0c6a, [81]  = 0x7e4b0c6a, [82]  = 0x7e4b0c66, [83]  = 0x7e4b0c66, [84]  = 0x7e4b0c66, [85]  = 0x7e4b0c66, [86]  = 0x7e4b0c66, [87]  = 0x7e4b0c66,
		[88]  = 0x7e4b0c6c, [89]  = 0x7e4b0c6c, [90]  = 0x7e4b0c6c, [91]  = 0x7e4b0c6c, [92]  = 0x7e4b0c6c, [93]  = 0x7e4b0c6c, [94]  = 0x7e4b0c6c, [95]  = 0x7e4b0c6c,
		[96]  = 0x7e4b0c6c, [97]  = 0x7e4b0c6c, [98]  = 0x7e4b0c6c, [99]  = 0x7e4b0c6c, [100] = 0x7e4b0c6c, [101] = 0x7e4b0c6c, [102] = 0x7e4b0c6c, [103] = 0x7e4b0c6c,
		[104] = 0x7e4b0c6c, [105] = 0x7e4b0c6c, [106] = 0x7e4b0c6c, [107] = 0x7e4b0c6c, [108] = 0x7e4b0c6c, [109] = 0x7e4b0c6c, [110] = 0x7e4b0c6c, [111] = 0x7e4b0c6c,
		[112] = 0x7e4b0c6c, [113] = 0x7e4b0c6c, [114] = 0x7e4b0c6c, [115] = 0x7e4b0c6c, [116] = 0x7e4b0c6c, [117] = 0x7e4b0c6c, [118] = 0x7e4b0c6c, [119] = 0x7e4b0c6c,
		[120] = 0x7e4b0c6c, [121] = 0x7e4b0c6c, [122] = 0x7e4b0c6c, [123] = 0x7e4b0c6c, [124] = 0x7e4b0c6c, [125] = 0x7e4b0c6c,
	};
	/* clang-format on */

	volatile u32 *base;
	volatile u32 *statusReg;
	u32 lockBit;

	if ((sys_pll->pll_mul < 25) || (sys_pll->pll_mul > 125)) {
		return -1;
	}

	switch (sys_pll->dev) {
		case ctl_sys_pll_iopll:
			base = zynq_common.crl_apb + crl_apb_iopll_ctrl;
			lockBit = (1 << 0);
			statusReg = zynq_common.crl_apb + crl_apb_pll_status;
			break;

		case ctl_sys_pll_rpll:
			base = zynq_common.crl_apb + crl_apb_rpll_ctrl;
			lockBit = (1 << 1);
			statusReg = zynq_common.crl_apb + crl_apb_pll_status;
			break;

		case ctl_sys_pll_apll:
			base = zynq_common.crf_apb + crf_apb_apll_ctrl;
			lockBit = (1 << 0);
			statusReg = zynq_common.crf_apb + crf_apb_pll_status;
			break;

		case ctl_sys_pll_dpll:
			base = zynq_common.crf_apb + crf_apb_dpll_ctrl;
			lockBit = (1 << 1);
			statusReg = zynq_common.crf_apb + crf_apb_pll_status;
			break;

		case ctl_sys_pll_vpll:
			base = zynq_common.crf_apb + crf_apb_vpll_ctrl;
			lockBit = (1 << 2);
			statusReg = zynq_common.crf_apb + crf_apb_pll_status;
			break;

		default:
			return -1;
	}

	*base =
			((sys_pll->src_bypass & 0x7) << 24) |
			((sys_pll->src_pll & 0x7) << 20) |
			((sys_pll->pll_div2 != 0) ? (1 << 16) : 0) |
			((sys_pll->pll_mul & 0x7f) << 8) |
			((sys_pll->use_bypass != 0) ? (1 << 3) : 0);

	if (sys_pll->frac != 0) {
		*(base + 2) = (1 << 31) | sys_pll->frac;
		*(base + 1) = _zynqmp_sysPllCfgForMulFractional[(unsigned int)sys_pll->pll_mul];
	}
	else {
		*(base + 2) &= ~(1 << 31);
		*(base + 1) = _zynqmp_sysPllCfgForMulInteger[(unsigned int)sys_pll->pll_mul];
	}

	if (sys_pll->use_bypass == 0) {
		/* User wants to use PLL - follow procedure for changing PLL settings */
		*base |= (1 << 3); /* Enable bypass */
		hal_cpuDataMemoryBarrier();
		*base |= (1 << 0); /* Reset PLL to latch new settings */
		hal_cpuDataMemoryBarrier();
		*base &= ~(1 << 0); /* Activate PLL */
		hal_cpuDataMemoryBarrier();
		while ((*statusReg & lockBit) == 0) {
			/* Wait for lock */
		}

		hal_cpuDataMemoryBarrier();
		*base &= ~(1 << 3); /* Disable bypass */
	}

	hal_cpuDataSyncBarrier();
	return 0;
}


int _zynqmp_getSysPll(ctl_sys_pll_t *sys_pll)
{
	volatile u32 *base;
	u32 val;

	switch (sys_pll->dev) {
		case ctl_sys_pll_rpll:
			base = zynq_common.crl_apb + crl_apb_rpll_ctrl;
			break;

		case ctl_sys_pll_iopll:
			base = zynq_common.crl_apb + crl_apb_iopll_ctrl;
			break;

		case ctl_sys_pll_apll:
			base = zynq_common.crf_apb + crf_apb_apll_cfg;
			break;

		case ctl_sys_pll_dpll:
			base = zynq_common.crf_apb + crf_apb_dpll_cfg;
			break;

		case ctl_sys_pll_vpll:
			base = zynq_common.crf_apb + crf_apb_vpll_cfg;
			break;

		default:
			return -1;
	}

	val = *base;

	sys_pll->src_bypass = (val >> 24) & 0x7;
	sys_pll->src_pll = (val >> 20) & 0x7;
	sys_pll->pll_div2 = (val >> 16) & 0x1;
	sys_pll->pll_mul = (val >> 8) & 0x7f;
	sys_pll->src_bypass = (val >> 3) & 0x1;

	val = *(base + 2);
	sys_pll->frac = ((val >> 31) != 0) ? (val & 0xffff) : 0;
	return 0;
}


static int _zynqmp_setBasicGenerator(volatile u32 *reg, const ctl_clock_t *clk)
{
	u32 val = clk->src;
	if (clk->dev == ctl_clock_dev_lpd_timestamp) {
		val &= 0x7;
	}
	else {
		val &= 0x3;
	}

	val |= ((clk->div0 & 0x3f) << 8) | ((clk->div1 & 0x3f) << 16) | (clk->active << 24);
	if (clk->dev == ctl_clock_dev_lpd_cpu_r5) {
		/* According to docs turning this bit off could lead to system hang - ensure it is on */
		val |= (1 << 24);
	}

	*reg = val;
	hal_cpuDataSyncBarrier();
	return 0;
}


static int _zynqmp_getBasicGenerator(volatile u32 *reg, ctl_clock_t *clk)
{
	u32 val = *reg;
	clk->src = val & 0x7;
	clk->div0 = (val >> 8) & 0x3f;
	clk->div1 = (val >> 16) & 0x3f;
	clk->active = val >> 24;
	return 0;
}


int _zynqmp_setCtlClock(const ctl_clock_t *clk)
{
	if ((clk->dev >= ctl_clock_dev_iopll_to_fpd) && (clk->dev <= ctl_clock_dev_lpd_timestamp)) {
		unsigned regOffset = (clk->dev - ctl_clock_dev_iopll_to_fpd) + crl_apb_iopll_to_fpd_ctrl;
		return _zynqmp_setBasicGenerator(zynq_common.crl_apb + regOffset, clk);
	}
	else if ((clk->dev >= ctl_clock_dev_apll_to_lpd) && (clk->dev <= ctl_clock_dev_fpd_dbg_tstmp)) {
		unsigned regOffset = (clk->dev - ctl_clock_dev_apll_to_lpd) + crf_apb_apll_to_lpd_ctrl;
		return _zynqmp_setBasicGenerator(zynq_common.crf_apb + regOffset, clk);
	}

	return -1;
}


int _zynqmp_getCtlClock(ctl_clock_t *clk)
{
	if ((clk->dev >= ctl_clock_dev_iopll_to_fpd) && (clk->dev <= ctl_clock_dev_lpd_timestamp)) {
		unsigned regOffset = (clk->dev - ctl_clock_dev_iopll_to_fpd) + crl_apb_iopll_to_fpd_ctrl;
		return _zynqmp_getBasicGenerator(zynq_common.crl_apb + regOffset, clk);
	}
	else if ((clk->dev >= ctl_clock_dev_apll_to_lpd) && (clk->dev <= ctl_clock_dev_fpd_dbg_tstmp)) {
		unsigned regOffset = (clk->dev - ctl_clock_dev_apll_to_lpd) + crf_apb_apll_to_lpd_ctrl;
		return _zynqmp_getBasicGenerator(zynq_common.crf_apb + regOffset, clk);
	}

	return -1;
}


static void _zynqmp_setMIOMuxing(const ctl_mio_t *mio)
{
	u32 val = ((mio->l0 & 0x1) << 1) | ((mio->l1 & 0x1) << 2) | ((mio->l2 & 0x3) << 3) | ((mio->l3 & 0x7) << 5);
	*(zynq_common.iou_slcr + iou_slcr_mio_pin_0 + mio->pin) = (*(zynq_common.iou_slcr + iou_slcr_mio_pin_0 + mio->pin) & ~0xff) | val;
}


static void _zynqmp_setMIOTristate(const ctl_mio_t *mio)
{
	u32 reg = mio->pin / 32 + iou_slcr_mio_mst_tri0;
	u32 bit = mio->pin % 32;
	u32 mask = 1 << bit;

	if ((mio->config & MIO_TRI_ENABLE) != 0) {
		*(zynq_common.iou_slcr + reg) |= mask;
	}
	else {
		*(zynq_common.iou_slcr + reg) &= ~mask;
	}
}


static void _zynqmp_setMIOControl(const ctl_mio_t *mio)
{
	u32 reg = (mio->pin / 26) * (iou_slcr_bank1_ctrl0 - iou_slcr_bank0_ctrl0) + iou_slcr_bank0_ctrl0;
	u32 bit = mio->pin % 26;
	u32 mask = 1 << bit;
	int i;

	for (i = 0; i <= 6; i++) {
		if (i == 2) {
			/* ctrl2 registers don't exist, skip */
			continue;
		}

		if ((mio->config & (1 << i)) != 0) {
			*(zynq_common.iou_slcr + reg + i) |= mask;
		}
		else {
			*(zynq_common.iou_slcr + reg + i) &= ~mask;
		}
	}
}


int _zynqmp_setMIO(const ctl_mio_t *mio)
{
	if (mio->pin > 77) {
		return -1;
	}

	_zynqmp_setMIOMuxing(mio);
	_zynqmp_setMIOTristate(mio);
	_zynqmp_setMIOControl(mio);

	return 0;
}


static void _zynqmp_getMIOMuxing(ctl_mio_t *mio)
{
	u32 val = *(zynq_common.iou_slcr + iou_slcr_mio_pin_0 + mio->pin) & 0xff;
	mio->l0 = (val >> 1) & 0x1;
	mio->l1 = (val >> 2) & 0x1;
	mio->l2 = (val >> 3) & 0x3;
	mio->l3 = (val >> 5) & 0x7;
}


static void _zynqmp_getMIOTristate(ctl_mio_t *mio)
{
	u32 reg = mio->pin / 32 + iou_slcr_mio_mst_tri0;
	u32 bit = mio->pin % 32;
	if (*(zynq_common.iou_slcr + reg) & (1 << bit)) {
		mio->config |= MIO_TRI_ENABLE;
	}
}


static void _zynqmp_getMIOControl(ctl_mio_t *mio)
{
	u32 reg = (mio->pin / 26) * (iou_slcr_bank1_ctrl0 - iou_slcr_bank0_ctrl0) + iou_slcr_bank0_ctrl0;
	u32 bit = mio->pin % 26;
	u32 mask = 1 << bit;
	int i;

	for (i = 0; i <= 6; i++) {
		if (i == 2) {
			/* ctrl2 registers don't exist, skip */
			continue;
		}

		if ((*(zynq_common.iou_slcr + reg + i) & mask) != 0) {
			mio->config |= (1 << i);
		}
	}
}


int _zynqmp_getMIO(ctl_mio_t *mio)
{
	if (mio->pin > 77) {
		return -1;
	}

	mio->config = 0;
	_zynqmp_getMIOMuxing(mio);
	_zynqmp_getMIOTristate(mio);
	_zynqmp_getMIOControl(mio);

	return 0;
}


int _zynq_loadPL(addr_t srcAddr, addr_t srcLen)
{
	/* TODO */
	return -1;
}


void _zynqmp_pllInit(void)
{
	ctl_sys_pll_t sys_pll;
	ctl_clock_t clk;

	/* QSPI may be active at this point - put it into reset before changing PLL parameters */
	_zynqmp_devReset(ctl_reset_lpd_qspi, 1);

	/* Set PSSYSMON clock source to IOPLL temporarily to avoid glitches when switching PLL */
	clk.dev = ctl_clock_dev_lpd_pssysmon;
	clk.src = 2;
	clk.div0 = 35;
	clk.div1 = 1;
	clk.active = 1;
	_zynqmp_setCtlClock(&clk);

	/* Use PS_REF_CLK as input (external, assumed 33.33 MHz on ZCU102 board)
	 * Don't use fractional PLL
	 */
	sys_pll.src_pll = ctl_clock_sys_pll_src_ps;
	sys_pll.src_bypass = ctl_clock_sys_pll_src_ps;
	sys_pll.frac = 0;
	sys_pll.use_bypass = 0;

	/* Set up PLL for RPU - (33.33 * 72) / 2 = 1200 MHz */
	sys_pll.dev = ctl_sys_pll_rpll;
	sys_pll.pll_mul = 72;
	sys_pll.pll_div2 = 1;
	_zynqmp_setSysPll(&sys_pll);

	/* Set PSSYSMON clock source to RPLL temporarily to avoid glitches when switching PLL */
	clk.src = 0;
	_zynqmp_setCtlClock(&clk);

	/* Set up PLL for IO - (33.33 * 60) / 2 = 1000 MHz */
	sys_pll.dev = ctl_sys_pll_iopll;
	sys_pll.pll_mul = 60;
	sys_pll.pll_div2 = 1;
	_zynqmp_setSysPll(&sys_pll);

	/* Set up PLL for APU - (33.33 * 72) / 2 = 1200 MHz */
	sys_pll.dev = ctl_sys_pll_apll;
	sys_pll.pll_mul = 72;
	sys_pll.pll_div2 = 1;
	_zynqmp_setSysPll(&sys_pll);

	/* Set up PLL for DDR - (33.33 * 64) / 2 = 1066 MHz */
	sys_pll.dev = ctl_sys_pll_dpll;
	sys_pll.pll_mul = 64;
	sys_pll.pll_div2 = 1;
	_zynqmp_setSysPll(&sys_pll);

	/* Set up PLL for video - (33.33 * 90) / 2 = 1500 MHz */
	sys_pll.dev = ctl_sys_pll_vpll;
	sys_pll.pll_mul = 90;
	sys_pll.pll_div2 = 1;
	_zynqmp_setSysPll(&sys_pll);

	clk.src = 0;
	clk.div1 = 0;
	clk.active = 0;

	/* Set RPLL_TO_FPD frequency to 600 MHz */
	clk.dev = ctl_clock_dev_rpll_to_fpd;
	clk.div0 = 2;
	_zynqmp_setCtlClock(&clk);

	/* Set IOPLL_TO_FPD frequency to 500 MHz */
	clk.dev = ctl_clock_dev_iopll_to_fpd;
	clk.div0 = 2;
	_zynqmp_setCtlClock(&clk);

	/* Set APLL_TO_LPD frequency to 400 MHz */
	clk.dev = ctl_clock_dev_apll_to_lpd;
	clk.div0 = 3;
	_zynqmp_setCtlClock(&clk);

	/* Set DPLL_TO_LPD frequency to 533 MHz */
	clk.dev = ctl_clock_dev_dpll_to_lpd;
	clk.div0 = 2;
	_zynqmp_setCtlClock(&clk);

	/* Set VPLL_TO_LPD frequency to 500 MHz */
	clk.dev = ctl_clock_dev_vpll_to_lpd;
	clk.div0 = 3;
	_zynqmp_setCtlClock(&clk);
}


static void _zynqmp_clocksInit(void)
{
	int i;
	static const ctl_clock_t clks[] = {
		/* R5 CPU clock generator - RPLL / 2 => 600 MHz */
		{
			.dev = ctl_clock_dev_lpd_cpu_r5,
			.src = 0,
			.div0 = 2,
			.active = 0x1,
		},

		/* LPD In/Outbound Switches clock generator - IOPLL / 4 => 250 MHz */
		{
			.dev = ctl_clock_dev_lpd_iou_switch,
			.src = 2,
			.div0 = 4,
			.active = 0x1,
		},

		/* LPD Main Switch clock generator - IOPLL / 2 => 500 MHz */
		{
			.dev = ctl_clock_dev_lpd_lpd_switch,
			.src = 2,
			.div0 = 2,
			.active = 0x1,
		},

		/* LPD IOP In/Outbound Switches clock generator - IOPLL / 10 => 100 MHz */
		{
			.dev = ctl_clock_dev_lpd_lpd_lsbus,
			.src = 2,
			.div0 = 10,
			.active = 0x1,
		},

		/* LPD Debug clock generator - IOPLL / 4 => 250 MHz */
		{
			.dev = ctl_clock_dev_lpd_dbg_lpd,
			.src = 2,
			.div0 = 4,
			.active = 0x1,
		},

		/* LPD DMA clock generator - IOPLL / 2 => 500 MHz */
		{
			.dev = ctl_clock_dev_lpd_lpd_dma,
			.src = 2,
			.div0 = 2,
			.active = 0x1,
		},

		/* PS SYSMON clock generator - IOPLL / 10 => 100 MHz */
		{
			.dev = ctl_clock_dev_lpd_pssysmon,
			.src = 2,
			.div0 = 20,
			.div1 = 1,
			.active = 0x1,
		},

		{
			.dev = ctl_clock_dev_lpd_dll,
			.src = 0,
			.div0 = 0,
			.active = 0,
		},

		/* PCAP clock generator - IOPLL / 5 => 200 MHz */
		{
			.dev = ctl_clock_dev_lpd_pcap,
			.src = 0,
			.div0 = 5,
			.active = 0x1,
		},

		/* Timestamp clock generator - IOPLL / 10 => 100 MHz */
		{
			.dev = ctl_clock_dev_lpd_timestamp,
			.src = 0,
			.div0 = 10,
			.active = 0x1,
		},

		/* ACPU clock generator - active full-speed and half-speed clocks, APLL / 2 => 600 MHz */
		{
			.dev = ctl_clock_dev_fpd_acpu,
			.src = 0,
			.div0 = 2,
			.active = 0x3,
		},

		/* FPD Debug clock generator - IOPLL_TO_FPD / 2 => 250 MHz */
		{
			.dev = ctl_clock_dev_fpd_dbg_fpd,
			.src = 0,
			.div0 = 2,
			.active = 0x1,
		},

		/* DDR clock generator - DPLL / 2 => 533 MHz */
		{
			.dev = ctl_clock_dev_fpd_ddr,
			.src = 0,
			.div0 = 2,
			.active = 0x1,
		},

		/* FPD DMA clock generator - DPLL / 2 => 533 MHz */
		{
			.dev = ctl_clock_dev_fpd_fpd_dma,
			.src = 3,
			.div0 = 2,
			.active = 0x1,
		},

		/* FPD Main Switch clock generator - DPLL / 2 => 533 MHz */
		{
			.dev = ctl_clock_dev_fpd_topsw_main,
			.src = 3,
			.div0 = 2,
			.active = 0x1,
		},

		/* TOP_LSBUS clock generator - IOPLL_TO_FPD / 5 => 100 MHz */
		{
			.dev = ctl_clock_dev_fpd_topsw_lsbus,
			.src = 2,
			.div0 = 5,
			.active = 0x1,
		},

		/* FPD Debug Timestamp clock generator - IOPLL_TO_FPD / 2 => 250 MHz */
		{
			.dev = ctl_clock_dev_fpd_dbg_tstmp,
			.src = 0,
			.div0 = 2,
			.active = 0x1,
		},
	};

	for (i = 0; i < (sizeof(clks) / sizeof(clks[0])); i++) {
		_zynqmp_setCtlClock(&clks[i]);
	}

	/* Select LPD_APB_CLK (lpd_lsbus) as interface clock for all TTC units */
	*(zynq_common.iou_slcr + iou_slcr_iou_ttc_apb_clk) = 0;
}


void _zynqmp_softRst(void)
{
	/* Equivalent to PS_SRST_B signal */
	*(zynq_common.crl_apb + crl_apb_reset_ctrl) |= (1 << 4);
}


void _zynqmp_startApuCore(int idx, addr_t resetVector)
{
	if (idx >= 4) {
		return;
	}

	/* NOTE: Writing CONFIG_0 and RVBARADDR does nothing under QEMU */
	*(zynq_common.apu + apu_config_0) |= 1 << idx; /* AArch64 mode */
	*(zynq_common.apu + apu_rvbaraddr0l + (idx * 2)) = resetVector & 0xffffffff;
	*(zynq_common.apu + apu_rvbaraddr0h + (idx * 2)) = (resetVector >> 32) & 0xffffffff;
	hal_cpuDataSyncBarrier();
	/* Clear POR reset and system reset */
	*(zynq_common.crf_apb + crf_apb_rst_fpd_apu) &= ~(((1 << 10) | (1 << 0)) << idx);
}


unsigned int hal_getBootReason(void)
{
	return zynq_common.resetFlags;
}


void _zynqmp_init(void)
{
	int i, ret;
	zynq_common.csu = (void *)CSU_BASE_ADDRESS;
	zynq_common.csudma = (void *)CSUDMA_BASE_ADDRESS;
	zynq_common.pmu_global = (void *)PMU_GLOBAL_BASE_ADDRESS;
	zynq_common.iou_slcr = (void *)IOU_SLCR_BASE_ADDRESS;
	zynq_common.apu = (void *)APU_BASE_ADDRESS;
	zynq_common.crf_apb = (void *)CRF_APB_BASE_ADDRESS;
	zynq_common.crl_apb = (void *)CRL_APB_BASE_ADDRESS;

	/* Read and clear reset flags */
	zynq_common.resetFlags = *(zynq_common.crl_apb + crl_apb_reset_reason) & 0x7f;
	*(zynq_common.crl_apb + crl_apb_reset_reason) = zynq_common.resetFlags;

	_zynqmp_pllInit();

	_zynqmp_clocksInit();

	ret = _zynqmp_ddrInit();
	if (ret < 0) {
		_zynqmp_softRst();
	}

	for (i = 1; i < 4; i++) {
		_zynqmp_startApuCore(i, (addr_t)_start);
	}
}
