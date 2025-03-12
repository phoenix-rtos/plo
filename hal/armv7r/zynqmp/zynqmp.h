/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ZynqMP basic peripherals control functions
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _ZYNQMP_H_
#define _ZYNQMP_H_


#include "hal/armv7r/types.h"


#define ZYNQ_RESET_REASON_EXT_POR   (1 << 0)
#define ZYNQ_RESET_REASON_INT_POR   (1 << 1)
#define ZYNQ_RESET_REASON_PMU_SRST  (1 << 2)
#define ZYNQ_RESET_REASON_PSONLY    (1 << 3)
#define ZYNQ_RESET_REASON_EXT_SRST  (1 << 4)
#define ZYNQ_RESET_REASON_SOFT_SRST (1 << 5)
#define ZYNQ_RESET_REASON_DEBUG     (1 << 6)


/* clang-format off */
enum {
	mio_pin_00 = 0, mio_pin_01, mio_pin_02, mio_pin_03, mio_pin_04, mio_pin_05, mio_pin_06, mio_pin_07,
	mio_pin_08, mio_pin_09, mio_pin_10, mio_pin_11, mio_pin_12, mio_pin_13, mio_pin_14, mio_pin_15,
	mio_pin_16, mio_pin_17, mio_pin_18, mio_pin_19, mio_pin_20, mio_pin_21, mio_pin_22, mio_pin_23,
	mio_pin_24, mio_pin_25, mio_pin_26, mio_pin_27, mio_pin_28, mio_pin_29, mio_pin_30, mio_pin_31,
	mio_pin_32, mio_pin_33, mio_pin_34, mio_pin_35, mio_pin_36, mio_pin_37, mio_pin_38, mio_pin_39,
	mio_pin_40, mio_pin_41, mio_pin_42, mio_pin_43, mio_pin_44, mio_pin_45, mio_pin_46, mio_pin_47,
	mio_pin_48, mio_pin_49, mio_pin_50, mio_pin_51, mio_pin_52, mio_pin_53, mio_pin_54, mio_pin_55,
	mio_pin_56, mio_pin_57, mio_pin_58, mio_pin_59, mio_pin_60, mio_pin_61, mio_pin_62, mio_pin_63,
	mio_pin_64, mio_pin_65, mio_pin_66, mio_pin_67, mio_pin_68, mio_pin_69, mio_pin_70, mio_pin_71,
	mio_pin_72, mio_pin_73, mio_pin_74, mio_pin_75, mio_pin_76, mio_pin_77,
};
/* clang-format on */


#define MIO_DRIVE_2mA     (0x0)
#define MIO_DRIVE_4mA     (0x1)
#define MIO_DRIVE_8mA     (0x2)
#define MIO_DRIVE_12mA    (0x3)
#define MIO_SCHMITT_nCMOS (1 << 3)
#define MIO_PULL_UP_nDOWN (1 << 4)
#define MIO_PULL_ENABLE   (1 << 5)
#define MIO_SLOW_nFAST    (1 << 6)
#define MIO_TRI_ENABLE    (1 << 7)


typedef struct {
	unsigned pin;
	char l3;
	char l2;
	char l1;
	char l0;
	char config;
} ctl_mio_t;

enum ctl_clock_sys_pll_src {
	ctl_clock_sys_pll_src_ps = 0,
	ctl_clock_sys_pll_src_video = 4,
	ctl_clock_sys_pll_src_alt,
	ctl_clock_sys_pll_src_aux,
	ctl_clock_sys_pll_src_gt,
};

enum ctl_sys_pll {
	ctl_sys_pll_iopll = 0,
	ctl_sys_pll_rpll,
	ctl_sys_pll_apll,
	ctl_sys_pll_dpll,
	ctl_sys_pll_vpll,
};

enum ctl_clock_dev {
	ctl_clock_dev_iopll_to_fpd = 0x11,
	ctl_clock_dev_rpll_to_fpd,
	ctl_clock_dev_lpd_usb3_dual,
	ctl_clock_dev_lpd_gem0,
	ctl_clock_dev_lpd_gem1,
	ctl_clock_dev_lpd_gem2,
	ctl_clock_dev_lpd_gem3,
	ctl_clock_dev_lpd_usb0_bus,
	ctl_clock_dev_lpd_usb1_bus,
	ctl_clock_dev_lpd_qspi,
	ctl_clock_dev_lpd_sdio0,
	ctl_clock_dev_lpd_sdio1,
	ctl_clock_dev_lpd_uart0,
	ctl_clock_dev_lpd_uart1,
	ctl_clock_dev_lpd_spi0,
	ctl_clock_dev_lpd_spi1,
	ctl_clock_dev_lpd_can0,
	ctl_clock_dev_lpd_can1,
	ctl_clock_dev_lpd_cpu_r5 = 0x24,
	ctl_clock_dev_lpd_iou_switch = 0x27,
	ctl_clock_dev_lpd_csu_pll,
	ctl_clock_dev_lpd_pcap,
	ctl_clock_dev_lpd_lpd_switch,
	ctl_clock_dev_lpd_lpd_lsbus,
	ctl_clock_dev_lpd_dbg_lpd,
	ctl_clock_dev_lpd_nand,
	ctl_clock_dev_lpd_lpd_dma,
	ctl_clock_dev_lpd_pl0 = 0x30,
	ctl_clock_dev_lpd_pl1,
	ctl_clock_dev_lpd_pl2,
	ctl_clock_dev_lpd_pl3,
	ctl_clock_dev_lpd_gem_tsu = 0x40,
	ctl_clock_dev_lpd_dll,
	ctl_clock_dev_lpd_pssysmon,
	ctl_clock_dev_lpd_i2c0 = 0x48,
	ctl_clock_dev_lpd_i2c1,
	ctl_clock_dev_lpd_timestamp,
	ctl_clock_dev_apll_to_lpd = 0x50 + 0x12,
	ctl_clock_dev_dpll_to_lpd,
	ctl_clock_dev_vpll_to_lpd,
	ctl_clock_dev_fpd_acpu = 0x50 + 0x18,
	ctl_clock_dev_fpd_dbg_trace,
	ctl_clock_dev_fpd_dbg_fpd,
	ctl_clock_dev_fpd_dp_video = 0x50 + 0x1c,
	ctl_clock_dev_fpd_dp_audio,
	ctl_clock_dev_fpd_dp_stc = 0x50 + 0x1f,
	ctl_clock_dev_fpd_ddr,
	ctl_clock_dev_fpd_gpu,
	ctl_clock_dev_fpd_sata = 0x50 + 0x28,
	ctl_clock_dev_fpd_pcie = 0x50 + 0x2d,
	ctl_clock_dev_fpd_fpd_dma,
	ctl_clock_dev_fpd_dpdma,
	ctl_clock_dev_fpd_topsw_main,
	ctl_clock_dev_fpd_topsw_lsbus,
	ctl_clock_dev_fpd_dbg_tstmp = 0x50 + 0x3e,
};

enum ctl_reset_dev {
	ctl_reset_lpd_gem0,
	ctl_reset_lpd_gem1,
	ctl_reset_lpd_gem2,
	ctl_reset_lpd_gem3,
	ctl_reset_lpd_qspi,
	ctl_reset_lpd_uart0,
	ctl_reset_lpd_uart1,
	ctl_reset_lpd_spi0,
	ctl_reset_lpd_spi1,
	ctl_reset_lpd_sdio0,
	ctl_reset_lpd_sdio1,
	ctl_reset_lpd_can0,
	ctl_reset_lpd_can1,
	ctl_reset_lpd_i2c0,
	ctl_reset_lpd_i2c1,
	ctl_reset_lpd_ttc0,
	ctl_reset_lpd_ttc1,
	ctl_reset_lpd_ttc2,
	ctl_reset_lpd_ttc3,
	ctl_reset_lpd_swdt,
	ctl_reset_lpd_nand,
	ctl_reset_lpd_lpd_dma,
	ctl_reset_lpd_gpio,
	ctl_reset_lpd_iou_cc,
	ctl_reset_lpd_timestamp,
	ctl_reset_lpd_rpu_r50,
	ctl_reset_lpd_rpu_r51,
	ctl_reset_lpd_rpu_amba,
	ctl_reset_lpd_ocm,
	ctl_reset_lpd_rpu_pge,
	ctl_reset_lpd_usb0_corereset,
	ctl_reset_lpd_usb1_corereset,
	ctl_reset_lpd_usb0_hiberreset,
	ctl_reset_lpd_usb1_hiberreset,
	ctl_reset_lpd_usb0_apb,
	ctl_reset_lpd_usb1_apb,
	ctl_reset_lpd_ipi,
	ctl_reset_lpd_apm,
	ctl_reset_lpd_rtc,
	ctl_reset_lpd_sysmon,
	ctl_reset_lpd_s_axi_lpd,
	ctl_reset_lpd_lpd_swdt,
	ctl_reset_lpd_fpd,
	ctl_reset_lpd_dbg_fpd,
	ctl_reset_lpd_dbg_lpd,
	ctl_reset_lpd_rpu_dbg0,
	ctl_reset_lpd_rpu_dbg1,
	ctl_reset_lpd_dbg_ack,
	ctl_reset_fpd_sata,
	ctl_reset_fpd_gt,
	ctl_reset_fpd_gpu,
	ctl_reset_fpd_gpu_pp0,
	ctl_reset_fpd_gpu_pp1,
	ctl_reset_fpd_fpd_dma,
	ctl_reset_fpd_s_axi_hpc_0_fpd,
	ctl_reset_fpd_s_axi_hpc_1_fpd,
	ctl_reset_fpd_s_axi_hp_0_fpd,
	ctl_reset_fpd_s_axi_hp_1_fpd,
	ctl_reset_fpd_s_axi_hpc_2_fpd,
	ctl_reset_fpd_s_axi_hpc_3_fpd,
	ctl_reset_fpd_swdt,
	ctl_reset_fpd_dp,
	ctl_reset_fpd_pcie_ctrl,
	ctl_reset_fpd_pcie_bridge,
	ctl_reset_fpd_pcie_cfg,
	ctl_reset_fpd_acpu0,
	ctl_reset_fpd_acpu1,
	ctl_reset_fpd_acpu2,
	ctl_reset_fpd_acpu3,
	ctl_reset_fpd_apu_l2,
	ctl_reset_fpd_acpu0_pwron,
	ctl_reset_fpd_acpu1_pwron,
	ctl_reset_fpd_acpu2_pwron,
	ctl_reset_fpd_acpu3_pwron,
	ctl_reset_fpd_ddr_apm,
	ctl_reset_fpd_ddr_block,
};

typedef struct {
	int dev;
	char src_pll;    /* One of enum clock_sys_pll_src */
	char src_bypass; /* One of enum clock_sys_pll_src */
	char pll_mul;    /* 25 ~ 125 */
	char pll_div2;   /* T/F */
	u16 frac;
	char use_bypass; /* T/F */
} ctl_sys_pll_t;


typedef struct {
	int dev;
	char src;    /* 0, 2, 3 for most devices, 0, 2, 3, 4 for ctl_clock_dev_lpd_timestamp */
	char div0;   /* 0 ~ 63 */
	char div1;   /* 0 ~ 63 if supported by selected generator, otherwise 0 */
	char active; /* 0, 1 for most devices, some have additional active bits  */
} ctl_clock_t;


/* Put the selected device in or out of reset */
int _zynqmp_devReset(int dev, int reset);


/* Get current reset state of device */
int _zynqmp_getDevReset(int dev, int *state);


/* Function sets device's clock configuration                   */
extern int _zynqmp_setCtlClock(const ctl_clock_t *clk);


/* Function returns device's clock configuration                */
extern int _zynqmp_getCtlClock(ctl_clock_t *clk);


/* Function sets MIO's configuration                            */
extern int _zynqmp_setMIO(const ctl_mio_t *mio);


/* Function returns MIO's configuration                         */
extern int _zynqmp_getMIO(ctl_mio_t *mio);


/* Function loads bitstream data from specific memory address   */
extern int _zynq_loadPL(addr_t srcAddr, addr_t srcLen);


/* Processing System software reset control signal. */
extern void _zynqmp_softRst(void);


/* Sets the reset vector for selected A53 core and takes it out of reset */
extern void _zynqmp_startApuCore(int idx, u64 resetVector);


/* Function initializes plls, clocks, ddr and basic peripherals */
extern void _zynqmp_init(void);


#endif
