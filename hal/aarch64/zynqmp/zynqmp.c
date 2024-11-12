/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Zynq-7000 basic peripherals control functions
 * based on: Zynq-7000 SoC TRM (Technical Reference Manual UG585 v1.12.2)
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "zynqmp.h"
#include "zynqmp_regs.h"
#include "config.h"

#include <board_config.h>

#define MAX_WAITING_COUNTER 10000000

#define CSU_BASE_ADDRESS      0xffca0000
#define IOU_SLCR_BASE_ADDRESS 0xff180000
#define CRF_APB_BASE_ADDRESS  0xfd1a0000
#define CRL_APB_BASE_ADDRESS  0xff5e0000
#define DDRC_BASE_ADDRESS     0xfd070000
#define DDR_PHY_BASE_ADDRESS  0xfd080000
#define APU_BASE_ADDRESS      0xfd5c0000
#define RPU_BASE_ADDRESS      0xff9a0000
#define LPD_SLCR_BASE_ADDRESS 0xff410000
#define FPD_SLCR_BASE_ADDRESS 0xfd610000


/* PLO entrypoint */
extern void _start(void);

struct {
	volatile u32 *csu;
	volatile u32 *iou_slcr;
	volatile u32 *apu;
	volatile u32 *crf_apb;
	volatile u32 *crl_apb;
	volatile u32 *ddrc;
	volatile u32 *ddr_phy;
	u32 resetFlags;
} zynq_common;


int _zynqmp_devReset(int dev, int reset)
{
	volatile u32 *reg;
	u32 bit;
	switch (dev) {
			/* clang-format off */
			case ctl_reset_lpd_gem0: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou0; bit = (1 << 0); break;
			case ctl_reset_lpd_gem1: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou0; bit = (1 << 1); break;
			case ctl_reset_lpd_gem2: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou0; bit = (1 << 2); break;
			case ctl_reset_lpd_gem3: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou0; bit = (1 << 3); break;
			case ctl_reset_lpd_qspi: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 0); break;
			case ctl_reset_lpd_uart0: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 1); break;
			case ctl_reset_lpd_uart1: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 2); break;
			case ctl_reset_lpd_spi0: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 3); break;
			case ctl_reset_lpd_spi1: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 4); break;
			case ctl_reset_lpd_sdio0: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 5); break;
			case ctl_reset_lpd_sdio1: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 6); break;
			case ctl_reset_lpd_can0: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 7); break;
			case ctl_reset_lpd_can1: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 8); break;
			case ctl_reset_lpd_i2c0: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 9); break;
			case ctl_reset_lpd_i2c1: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 10); break;
			case ctl_reset_lpd_ttc0: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 11); break;
			case ctl_reset_lpd_ttc1: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 12); break;
			case ctl_reset_lpd_ttc2: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 13); break;
			case ctl_reset_lpd_ttc3: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 14); break;
			case ctl_reset_lpd_swdt: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 15); break;
			case ctl_reset_lpd_nand: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 16); break;
			case ctl_reset_lpd_lpd_dma: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 17); break;
			case ctl_reset_lpd_gpio: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 18); break;
			case ctl_reset_lpd_iou_cc: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 19); break;
			case ctl_reset_lpd_timestamp: reg = zynq_common.crl_apb + crl_apb_rst_lpd_iou2; bit = (1 << 20); break;
			case ctl_reset_lpd_rpu_r50: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 0); break;
			case ctl_reset_lpd_rpu_r51: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 1); break;
			case ctl_reset_lpd_rpu_amba: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 2); break;
			case ctl_reset_lpd_ocm: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 3); break;
			case ctl_reset_lpd_rpu_pge: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 4); break;
			case ctl_reset_lpd_usb0_corereset: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 6); break;
			case ctl_reset_lpd_usb1_corereset: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 7); break;
			case ctl_reset_lpd_usb0_hiberreset: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 8); break;
			case ctl_reset_lpd_usb1_hiberreset: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 9); break;
			case ctl_reset_lpd_usb0_apb: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 10); break;
			case ctl_reset_lpd_usb1_apb: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 11); break;
			case ctl_reset_lpd_ipi: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 14); break;
			case ctl_reset_lpd_apm: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 15); break;
			case ctl_reset_lpd_rtc: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 16); break;
			case ctl_reset_lpd_sysmon: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 17); break;
			case ctl_reset_lpd_s_axi_lpd: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 19); break;
			case ctl_reset_lpd_lpd_swdt: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 20); break;
			case ctl_reset_lpd_fpd: reg = zynq_common.crl_apb + crl_apb_rst_lpd_top; bit = (1 << 23); break;
			case ctl_reset_lpd_dbg_fpd: reg = zynq_common.crl_apb + crl_apb_rst_lpd_dbg; bit = (1 << 0); break;
			case ctl_reset_lpd_dbg_lpd: reg = zynq_common.crl_apb + crl_apb_rst_lpd_dbg; bit = (1 << 1); break;
			case ctl_reset_lpd_rpu_dbg0: reg = zynq_common.crl_apb + crl_apb_rst_lpd_dbg; bit = (1 << 4); break;
			case ctl_reset_lpd_rpu_dbg1: reg = zynq_common.crl_apb + crl_apb_rst_lpd_dbg; bit = (1 << 5); break;
			case ctl_reset_lpd_dbg_ack: reg = zynq_common.crl_apb + crl_apb_rst_lpd_dbg; bit = (1 << 15); break;
			case ctl_reset_fpd_sata: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 1); break;
			case ctl_reset_fpd_gt: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 2); break;
			case ctl_reset_fpd_gpu: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 3); break;
			case ctl_reset_fpd_gpu_pp0: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 4); break;
			case ctl_reset_fpd_gpu_pp1: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 5); break;
			case ctl_reset_fpd_fpd_dma: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 6); break;
			case ctl_reset_fpd_s_axi_hpc_0_fpd: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 7); break;
			case ctl_reset_fpd_s_axi_hpc_1_fpd: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 8); break;
			case ctl_reset_fpd_s_axi_hp_0_fpd: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 9); break;
			case ctl_reset_fpd_s_axi_hp_1_fpd: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 10); break;
			case ctl_reset_fpd_s_axi_hpc_2_fpd: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 11); break;
			case ctl_reset_fpd_s_axi_hpc_3_fpd: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 12); break;
			case ctl_reset_fpd_swdt: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 15); break;
			case ctl_reset_fpd_dp: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 16); break;
			case ctl_reset_fpd_pcie_ctrl: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 17); break;
			case ctl_reset_fpd_pcie_bridge: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 18); break;
			case ctl_reset_fpd_pcie_cfg: reg = zynq_common.crf_apb + crf_apb_rst_fpd_top; bit = (1 << 19); break;
			case ctl_reset_fpd_acpu0: reg = zynq_common.crf_apb + crf_apb_rst_fpd_apu; bit = (1 << 0); break;
			case ctl_reset_fpd_acpu1: reg = zynq_common.crf_apb + crf_apb_rst_fpd_apu; bit = (1 << 1); break;
			case ctl_reset_fpd_acpu2: reg = zynq_common.crf_apb + crf_apb_rst_fpd_apu; bit = (1 << 2); break;
			case ctl_reset_fpd_acpu3: reg = zynq_common.crf_apb + crf_apb_rst_fpd_apu; bit = (1 << 3); break;
			case ctl_reset_fpd_apu_l2: reg = zynq_common.crf_apb + crf_apb_rst_fpd_apu; bit = (1 << 8); break;
			case ctl_reset_fpd_acpu0_pwron: reg = zynq_common.crf_apb + crf_apb_rst_fpd_apu; bit = (1 << 10); break;
			case ctl_reset_fpd_acpu1_pwron: reg = zynq_common.crf_apb + crf_apb_rst_fpd_apu; bit = (1 << 11); break;
			case ctl_reset_fpd_acpu2_pwron: reg = zynq_common.crf_apb + crf_apb_rst_fpd_apu; bit = (1 << 12); break;
			case ctl_reset_fpd_acpu3_pwron: reg = zynq_common.crf_apb + crf_apb_rst_fpd_apu; bit = (1 << 13); break;
			case ctl_reset_fpd_ddr_apm: reg = zynq_common.crf_apb + crf_apb_rst_ddr_ss; bit = (1 << 2); break;
			case ctl_reset_fpd_ddr_reserved: reg = zynq_common.crf_apb + crf_apb_rst_ddr_ss; bit = (1 << 3); break;
			default: return -1;
			/* clang-format on */
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


static u32 _zynqmp_sysPllSrcToValue(unsigned v)
{
	return (v < ctl_clock_sys_pll_src_ps) ? (4 + v) : 0;
}


static int _zynqmp_setSysPll(const ctl_clock_t *clk)
{
	/* How system PLL switching works:
	 * * Select source for the PLL using src_pll
	 * * Select source for the bypass using src_bypass
	 * * Select PLL multiplier, divider and if you want to use fractional multiplier
	 * 	 * NOTE: source * multiplier must be within range for the VCO (1500 ~ 3000 MHz, see FPSPLLVCOMIN, FPSPLLVCOMAX)
	 * * Program APLL_CFG based on selected multiplier
	 * * Activate PLL bypass
	 * * Reset-cycle PLL
	 * * Check for PLL stability
	 * * Deactivate PLL bypass
	 */

	if ((clk->sys_pll.pll_mul < 25) || (clk->sys_pll.pll_mul > 125)) {
		return -1;
	}

	volatile u32 *base;
	volatile u32 *statusReg;
	u32 lockBit;

	switch (clk->dev) {
		case ctl_clock_dev_rpll:
			base = zynq_common.crl_apb + crl_apb_rpll_ctrl;
			lockBit = (1 << 0);
			statusReg = zynq_common.crl_apb + crl_apb_pll_status;
			break;

		case ctl_clock_dev_iopll:
			base = zynq_common.crl_apb + crl_apb_iopll_ctrl;
			lockBit = (1 << 1);
			statusReg = zynq_common.crl_apb + crl_apb_pll_status;
			break;

		case ctl_clock_dev_apll:
			base = zynq_common.crf_apb + crf_apb_apll_cfg;
			lockBit = (1 << 0);
			statusReg = zynq_common.crf_apb + crf_apb_pll_status;
			break;

		case ctl_clock_dev_dpll:
			base = zynq_common.crf_apb + crf_apb_dpll_cfg;
			lockBit = (1 << 1);
			statusReg = zynq_common.crf_apb + crf_apb_pll_status;
			break;

		case ctl_clock_dev_vpll:
			base = zynq_common.crf_apb + crf_apb_vpll_cfg;
			lockBit = (1 << 2);
			statusReg = zynq_common.crf_apb + crf_apb_pll_status;
			break;

		default:
			return -1;
	}

	*base =
			(_zynqmp_sysPllSrcToValue(clk->sys_pll.src_bypass) << 24) |
			(_zynqmp_sysPllSrcToValue(clk->sys_pll.src_pll) << 20) |
			((clk->sys_pll.pll_div2 != 0) ? (1 << 16) : 0) |
			((clk->sys_pll.pll_mul & 0x7f) << 8) |
			((clk->sys_pll.src_bypass != 0) ? (1 << 3) : 0);

	if (clk->sys_pll.frac != 0) {
		*(base + 2) = (1 << 31) | clk->sys_pll.frac;
		*(base + 1) = _zynqmp_sysPllCfgForMulFractional[(unsigned int)clk->sys_pll.pll_mul];
	}
	else {
		*(base + 2) &= ~(1 << 31);
		*(base + 1) = _zynqmp_sysPllCfgForMulInteger[(unsigned int)clk->sys_pll.pll_mul];
	}

	if (clk->sys_pll.src_bypass == 0) {
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


static int _zynqmp_sysPllSrcFromValue(u32 v)
{
	return ((v & 0x4) == 0) ? ctl_clock_sys_pll_src_ps : (v & 0x3);
}


static int _zynqmp_getSysPll(ctl_clock_t *clk)
{
	volatile u32 *base;
	u32 val;

	switch (clk->dev) {
		case ctl_clock_dev_rpll:
			base = zynq_common.crl_apb + crl_apb_rpll_ctrl;
			break;

		case ctl_clock_dev_iopll:
			base = zynq_common.crl_apb + crl_apb_iopll_ctrl;
			break;

		case ctl_clock_dev_apll:
			base = zynq_common.crf_apb + crf_apb_apll_cfg;
			break;

		case ctl_clock_dev_dpll:
			base = zynq_common.crf_apb + crf_apb_dpll_cfg;
			break;

		case ctl_clock_dev_vpll:
			base = zynq_common.crf_apb + crf_apb_vpll_cfg;
			break;

		default:
			return -1;
	}

	val = *base;

	clk->sys_pll.src_bypass = _zynqmp_sysPllSrcFromValue((val >> 24) & 0x7);
	clk->sys_pll.src_pll = _zynqmp_sysPllSrcFromValue((val >> 20) & 0x7);
	clk->sys_pll.pll_div2 = (val >> 16) & 0x1;
	clk->sys_pll.pll_mul = (val >> 8) & 0x7f;
	clk->sys_pll.src_bypass = (val >> 3) & 0x1;

	val = *(base + 2);
	clk->sys_pll.frac = ((val >> 31) != 0) ? (val & 0xffff) : 0;
	return 0;
}


static int _zynqmp_setBasicGenerator(volatile u32 *reg, const ctl_clock_t *clk)
{
	u32 val = clk->basic_gen.src;
	if (clk->dev == ctl_clock_dev_lpd_timestamp) {
		val &= 0x7;
	}
	else {
		val &= 0x3;
	}

	val |= ((clk->basic_gen.div0 & 0x3f) << 8) | ((clk->basic_gen.div1 & 0x3f) << 16) | (clk->basic_gen.active << 24);
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
	clk->basic_gen.src = val & 0x7;
	clk->basic_gen.div0 = (val >> 8) & 0x3f;
	clk->basic_gen.div1 = (val >> 16) & 0x3f;
	clk->basic_gen.active = val >> 24;
	return 0;
}


int _zynqmp_setCtlClock(const ctl_clock_t *clk)
{
	if ((clk->dev >= ctl_clock_dev_rpll) && (clk->dev <= ctl_clock_dev_vpll)) {
		return _zynqmp_setSysPll(clk);
	}
	else if ((clk->dev >= ctl_clock_dev_rpll_to_fpd) && (clk->dev <= ctl_clock_dev_vpll_to_lpd)) {
		/* TODO: */
	}
	else if ((clk->dev >= ctl_clock_dev_lpd_usb3_dual) && (clk->dev <= ctl_clock_dev_lpd_timestamp)) {
		unsigned regOffset = (clk->dev - ctl_clock_dev_lpd_usb3_dual) + crl_apb_usb3_dual_ref_ctrl;
		return _zynqmp_setBasicGenerator(zynq_common.crl_apb + regOffset, clk);
	}
	else if ((clk->dev >= ctl_clock_dev_fpd_acpu) && (clk->dev <= ctl_clock_dev_fpd_dbg_tstmp)) {
		unsigned regOffset = (clk->dev - ctl_clock_dev_fpd_acpu) + crf_apb_acpu_ctrl;
		return _zynqmp_setBasicGenerator(zynq_common.crf_apb + regOffset, clk);
	}

	return -1;
}


int _zynqmp_getCtlClock(ctl_clock_t *clk)
{
	if ((clk->dev >= ctl_clock_dev_rpll) && (clk->dev <= ctl_clock_dev_vpll)) {
		return _zynqmp_getSysPll(clk);
	}
	else if ((clk->dev >= ctl_clock_dev_rpll_to_fpd) && (clk->dev <= ctl_clock_dev_vpll_to_lpd)) {
		/* TODO: */
	}
	else if ((clk->dev >= ctl_clock_dev_lpd_usb3_dual) && (clk->dev <= ctl_clock_dev_lpd_timestamp)) {
		unsigned regOffset = (clk->dev - ctl_clock_dev_lpd_usb3_dual) + crl_apb_usb3_dual_ref_ctrl;
		return _zynqmp_getBasicGenerator(zynq_common.crl_apb + regOffset, clk);
	}
	else if ((clk->dev >= ctl_clock_dev_fpd_acpu) && (clk->dev <= ctl_clock_dev_fpd_dbg_tstmp)) {
		unsigned regOffset = (clk->dev - ctl_clock_dev_fpd_acpu) + crf_apb_acpu_ctrl;
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
	return 0;
}


static void _zynqmp_ddrInit(void)
{
	/* TODO */
}


void _zynqmp_setupPLL(void)
{
	ctl_clock_t clk;

	/* Use PS_REF_CLK as input (external, assumed 33.33 MHz on ZCU102 board)
	 * Don't use fractional PLL
	 */
	clk.sys_pll.src_pll = ctl_clock_sys_pll_src_ps;
	clk.sys_pll.frac = 0;

	/* Set up PLL for IO - (33.33 * 60) / 2 = 1000 MHz */
	clk.dev = ctl_clock_dev_iopll;
	clk.sys_pll.pll_mul = 60;
	clk.sys_pll.pll_div2 = 1;
	clk.sys_pll.src_bypass = 0;
	_zynqmp_setCtlClock(&clk);

	/* Set up PLL for RPU - (33.33 * 72) / 2 = 1200 MHz */
	clk.dev = ctl_clock_dev_rpll;
	clk.sys_pll.pll_mul = 72;
	_zynqmp_setCtlClock(&clk);

	/* Set up PLL for APU and DRAM - (33.33 * 45) / 1 = 1.5 GHz */
	clk.dev = ctl_clock_dev_apll;
	clk.sys_pll.pll_mul = 45;
	clk.sys_pll.pll_div2 = 0;
	clk.sys_pll.src_bypass = 0;
	_zynqmp_setCtlClock(&clk);

	clk.dev = ctl_clock_dev_dpll;
	_zynqmp_setCtlClock(&clk);

	/* Bypass PLL for video - not used for now */
	clk.dev = ctl_clock_dev_vpll;
	clk.sys_pll.src_bypass = 1;
	_zynqmp_setCtlClock(&clk);
}


static void _zynqmp_clcksInit(void)
{
	ctl_clock_t clk;

	/* ACPU clock generator - active full-speed and half-speed clocks, no divider, source APLL */
	clk.dev = ctl_clock_dev_fpd_acpu;
	clk.basic_gen.src = 0;
	clk.basic_gen.div0 = 0;
	clk.basic_gen.active = 0x3;
	_zynqmp_setCtlClock(&clk);

	/* TODO: clocks for other peripherals */
}


void _zynqmp_softRst(void)
{
	/* Equivalent to PS_SRST_B signal */
	*(zynq_common.crl_apb + crl_apb_reset_ctrl) |= (1 << 4);
}


void _zynqmp_startApuCore(int idx, addr_t resetVector)
{
	/* NOTE: this function does nothing under QEMU */
	if (idx >= 4) {
		return;
	}

	*(zynq_common.apu + apu_config_0) |= 1 << idx; /* AArch64 mode */
	*(zynq_common.apu + apu_rvbaraddr0l + idx * 2) = resetVector & 0xffffffff;
	*(zynq_common.apu + apu_rvbaraddr0h + idx * 2) = (resetVector >> 32) & 0xffffffff;
	hal_cpuDataSyncBarrier();
	*(zynq_common.crf_apb + crf_apb_rst_fpd_apu) &= ~(1 << idx); /* Take core out of reset */
}


unsigned int hal_getBootReason(void)
{
	return zynq_common.resetFlags;
}


void _zynqmp_init(void)
{
	int i;
	zynq_common.csu = (void *)CSU_BASE_ADDRESS;
	zynq_common.ddrc = (void *)DDRC_BASE_ADDRESS;
	zynq_common.ddr_phy = (void *)DDR_PHY_BASE_ADDRESS;
	zynq_common.iou_slcr = (void *)IOU_SLCR_BASE_ADDRESS;
	zynq_common.apu = (void *)APU_BASE_ADDRESS;
	zynq_common.crf_apb = (void *)CRF_APB_BASE_ADDRESS;
	zynq_common.crl_apb = (void *)CRL_APB_BASE_ADDRESS;

	/* Read and clear reset flags */
	zynq_common.resetFlags = *(zynq_common.crl_apb + crl_apb_reset_reason) & 0x7f;
	*(zynq_common.crl_apb + crl_apb_reset_reason) = zynq_common.resetFlags;

	_zynqmp_setupPLL();

	_zynqmp_clcksInit();

	_zynqmp_ddrInit();

	for (i = 1; i < 4; i++) {
		_zynqmp_startApuCore(i, (addr_t)_start);
	}
}
