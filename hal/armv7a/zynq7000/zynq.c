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

#include "zynq.h"
#include "config.h"

#include <board_config.h>

#define MAX_WAITING_COUNTER 10000000

#define SCLR_BASE_ADDRESS   0xf8000000
#define DDRC_BASE_ADDRESS   0xf8006000
#define DEVCFG_BASE_ADDRESS 0xf8007000


/* SLCR (System Level Control Registers) */
enum {
	/* SLCR protection registers */
	slcr_scl = 0, slcr_lock, slcr_unlock, slcr_locksta,
	/* PLL configuration registers */
	slcr_arm_pll_ctrl = 0x40 ,slcr_ddr_pll_ctrl, slcr_io_pll_ctrl, slcr_pll_status, slcr_arm_pll_cfg, slcr_ddr_pll_cfg, slcr_io_pll_cfg,
	/* Clock control registers */
	slcr_arm_clk_ctrl = 0x48, slcr_ddr_clk_ctrl, slcr_dci_clk_ctrl, slcr_aper_clk_ctrl, slcr_usb0_clk_ctrl, slcr_usb1_clk_ctrl, slcr_gem0_rclk_ctrl,
	slcr_gem1_rclk_ctrl, slcr_gem0_clk_ctrl, slcr_gem1_clk_ctrl, slcr_smc_clk_ctrl, slcr_lqspi_clk_ctrl, slcr_sdio_clk_ctrl, slcr_uart_clk_ctrl,
	slcr_spi_clk_ctrl, slcr_can_clk_ctrl, slcr_can_mioclk_ctrl, slcr_dbg_clk_ctrl, slcr_pcap_clk_ctrl, slcr_topsw_clk_ctrl, slcr_fpga0_clk_ctrl,
	/* FPGA configuration registers */
	slcr_fpga0_thr_ctrl, slcr_fpga0_thr_cnt, slcr_fpga0_thr_sta, slcr_fpga1_clk_ctrl, slcr_fpga1_thr_ctrl, slcr_fpga1_thr_cnt, slcr_fpga1_thr_sta,
	slcr_fpga2_clk_ctrl, slcr_fpga2_thr_ctrl, slcr_fpga2_thr_cnt, slcr_fpga2_thr_sta, slcr_fpga3_clk_ctrl, slcr_fpga3_thr_ctrl, slcr_fpga3_thr_cnt,
	slcr_fpga3_thr_sta,
	/* Clock ratio register */
	slcr_clk_621_true = 0x71,
	/* Reset registers */
	slcr_pss_rst_ctrl = 0x80, slcr_ddr_rst_ctrl, slcr_topsw_rst_ctrl, slcr_dmac_rst_ctrl, slcr_usb_rst_ctrl, slcr_gem_rst_ctrl, slcr_sdio_rst_ctrl,
	slcr_spi_rst_ctrl, slcr_can_rst_ctrl, slcr_i2c_rst_ctrl, slcr_uart_rst_ctrl, slcr_gpio_rst_ctrl, slcr_lqspi_rst_ctrl, slcr_smc_rst_ctrl, slcr_ocm_rst_ctrl,
	slcr_fpga_rst_ctrl = 0x90, slcr_a9_cpu_rst_ctrl,
	/* APU watchdog register */
	slcr_rs_awdt_rst_ctrl = 0x93,
	slcr_reboot_status = 0x96, slcr_boot_mode,
	slcr_apu_control = 0xc0, slcr_wdt_clk_sel,
	slcr_tz_dma_ns = 0x110, slcr_tz_dma_irq_ns, slcr_tz_dma_periph_ns,
	slcr_pss_idcode = 0x14c,
	slcr_ddr_urgent = 0x180,
	slcr_ddr_cal_start = 0x183,
	slcr_ddr_ref_start = 0x185, slcr_ddr_cmd_sta, slcr_ddr_urgent_sel, slcr_ddr_dfi_status,
	/* MIO pins config registers */
	slcr_mio_pin_00 = 0x1c0 /* 53 registers */,
	slcr_mio_loopback = 0x201,
	slcr_mio_mst_tri0 = 0x203, slcr_mio_mst_tri1,
	slcr_sd0_wp_cd_sel = 0x20c, slcr_sd1_wp_cd_sel,
	slcr_lvl_shftr_en = 0x240,
	slcr_ocm_cfg = 0x244,
	/* GPIO config registers */
	slcr_gpiob_ctrl = 0x2c0, slcr_gpiob_cfg_cmos18, slcr_gpiob_cfg_cmos25, slcr_gpiob_cfg_cmos33,
	slcr_gpiob_cfg_hstl = 0x2c5, slcr_gpiob_drvr_bias_ctrl,
	/* DDR config registers */
	slcr_ddriob_addr0 = 0x2d0, slcr_ddriob_addr1, slcr_ddriob_data0, slcr_ddriob_data1, slcr_ddriob_diff0, slcr_ddriob_diff1, slcr_ddriob_clock, slcr_ddriob_drive_slew_addr,
	slcr_ddriob_drive_slew_data, slcr_ddriob_drive_slew_diff, slcr_ddriob_drive_slew_clock, slcr_ddriob_ddr_ctrl, slcr_ddriob_dci_ctrl, slcr_ddriob_dci_status,
};


/* DDRC (DDR Memory Controler Registers) */
enum {
	ddrc_ctrl = 0, ddrc_two_rank_cfg, ddrc_hpr_reg, ddrc_lpr_reg, ddrc_wr_reg, ddrc_dram_param_reg0, ddrc_dram_param_reg1, ddrc_dram_param_reg2, ddrc_dram_param_reg3,
	ddrc_dram_param_reg4, ddrc_dram_init_param, ddrc_dram_emr_reg, ddrc_dram_emr_mr_reg, ddrc_dram_burst8_rdwr, ddrc_dram_disable_dq, ddrc_dram_addr_map_bank,
	ddrc_dram_addr_map_col, ddrc_dram_addr_map_row, ddrc_dram_odt_reg, ddrc_phy_dbg_reg, ddrc_phy_cmd_timeout_rddata_cpt, ddrc_mode_sts_reg, ddrc_dll_calib,
	ddrc_odt_delay_hold, ddrc_ctrl_reg1, ddrc_ctrl_reg2, ddrc_ctrl_reg3, ddrc_ctrl_reg4,
	ddrc_ctrl_reg5 = 0x1e, ddrc_ctrl_reg6,
	ddrc_che_refresh_timer01 = 0x28, ddrc_che_t_zq, ddrc_che_t_zq_short_interval_reg, ddrc_deep_pwrdwn_reg, ddrc_reg_2c, ddrc_reg_2d, ddrc_dfi_timinig,
	ddrc_che_ecc_ctrl_reg_offs = 0x31, ddrc_che_corr_ecc_log_reg_offs, ddrc_che_corr_ecc_addr_reg_offs, ddrc_che_corr_ecc_data_31_0_reg_offs, ddrc_che_corr_ecc_data_63_32_reg_offs,
	ddrc_che_corr_ecc_data_71_64_reg_offs, ddrc_che_uncorr_ecc_log_reg_offs, ddrc_che_uncorr_ecc_addr_reg_offs, ddrc_che_uncorr_ecc_data_31_0_reg_offs, ddrc_che_uncorr_ecc_data_63_32_reg_offs,
	ddrc_che_uncorr_ecc_data_71_64_reg_offs, ddrc_che_ecc_stats_reg_offs, ddrc_ecc_scrub, ddrc_che_ecc_corr_bit_mask_31_0_reg_offs, ddrc_che_ecc_corr_bit_mask_63_32_reg_offs,
	ddrc_phy_rcvr_enable = 0x45, ddrc_phy_config0, ddrc_phy_config1, ddrc_phy_config2, ddrc_phy_config3,
	ddrc_phy_init_ratio0 = 0x4b, ddrc_phy_init_ratio1, ddrc_phy_init_ratio2, ddrc_phy_init_ratio3,
	ddrc_phy_rd_dqs_cfg0 = 0x50, ddrc_phy_rd_dqs_cfg1, ddrc_phy_rd_dqs_cfg2, ddrc_phy_rd_dqs_cfg3,
	ddrc_phy_wr_dqs_cfg0 = 0x55, ddrc_phy_wr_dqs_cfg1, ddrc_phy_wr_dqs_cfg2, ddrc_phy_wr_dqs_cfg3,
	ddrc_phy_we_cfg0 = 0x5a, ddrc_phy_we_cfg1, ddrc_phy_we_cfg2, ddrc_phy_we_cfg3,
	ddrc_wr_data_slv0 = 0x5f, ddrc_wr_data_slv1, ddrc_wr_data_slv2, ddrc_wr_data_slv3,
	ddrc_reg_64 = 0x64, ddrc_reg_65,
	ddrc_reg69_6a0 = 0x69, ddrc_reg69_6a1,
	ddrc_reg6c_6d2 = 0x6c, ddrc_reg6c_6d3, ddrc_reg6e_710, ddrc_reg6e_711, ddrc_reg6e_712, ddrc_reg6e_713,
	ddrc_phy_dll_sts0 = 0x73, ddrc_phy_dll_sts1, ddrc_phy_dll_sts2, ddrc_phy_dll_sts3,
	ddrc_dll_lock_sts = 0x78, ddrc_phy_ctrl_sts, ddrc_phy_ctrl_sts_reg2,
	ddrc_axi_id = 0x80, ddrc_page_mask, ddrc_axi_priority_wr_port0, ddrc_axi_priority_wr_port1, ddrc_axi_priority_wr_port2, ddrc_axi_priority_wr_port3, ddrc_axi_priority_rd_port0,
	ddrc_axi_priority_rd_port1, ddrc_axi_priority_rd_port2, ddrc_axi_priority_rd_port3,
	ddrc_excl_access_cfg0 = 0xa5, ddrc_excl_access_cfg1, ddrc_excl_access_cfg2, ddrc_excl_access_cfg3, ddrc_mode_reg_read, ddrc_lpddr_ctrl0, ddrc_lpddr_ctrl1, ddrc_lpddr_ctrl2, ddrc_lpddr_ctrl3
};


/* DCFG (Device Configuration Interface) */
enum {
	dcfg_ctrl = 0, dcfg_lock, dcfg_cfg, dcfg_int_sts, dcfg_int_mask, dcfg_status, dcfg_dma_src_addr, dcfg_dma_dest_addr, dcfg_dma_src_len, dcfg_dma_dest_len,
	dcfg_multiboot_addr = 0xb,
	dcfg_unlock = 0xd,
	dcfg_mctrl = 0x20,
	dcfg_adcif_cfg = 0x40, dcfg_adcif_int_sts, dcfg_adcif_int_mask, dcfg_adcif_msts, dcfg_adcif_cmdfifo, dcfg_adcif_rdfifo, dcfg_adcif_mctl
};


struct {
	volatile u32 *slcr;
	volatile u32 *ddr;
	volatile u32 *dcfg;
} zynq_common;


static void _zynq_slcrLock(void)
{
	*(zynq_common.slcr + slcr_lock) = 0x0000767b;
}


static void _zynq_slcrUnlock(void)
{
	*(zynq_common.slcr + slcr_unlock) = 0x0000df0d;
}


int _zynq_setAmbaClk(u32 dev, u32 state)
{
	/* Check max dev position in amba register */
	if (dev > 24)
		return -1;

	_zynq_slcrUnlock();
	*(zynq_common.slcr + slcr_aper_clk_ctrl) = (*(zynq_common.slcr + slcr_aper_clk_ctrl) & ~(1 << dev)) | (!!state << dev);
	_zynq_slcrLock();

	return 0;
}


int _zynq_getAmbaClk(u32 dev, u32 *state)
{
	/* Check max dev position in amba register */
	if (dev > 24)
		return -1;

	_zynq_slcrUnlock();
	*state = (*(zynq_common.slcr + slcr_aper_clk_ctrl) >> dev) & 0x1;
	_zynq_slcrLock();

	return 0;
}


int _zynq_setCtlClock(const ctl_clock_t *clk)
{
	u32 id = 0;

	_zynq_slcrUnlock();
	switch (clk->dev) {
		case ctrl_usb0_clk:
		case ctrl_usb1_clk:
			id = clk->dev - ctrl_usb0_clk;
			*(zynq_common.slcr + slcr_usb0_clk_ctrl + id) = (*(zynq_common.slcr + ctrl_usb0_clk + id) & ~0x00000070) | (clk->pll.srcsel & 0x7) << 4;
			break;

		case ctrl_gem0_rclk:
		case ctrl_gem1_rclk:
			id = clk->dev - ctrl_gem0_rclk;
			*(zynq_common.slcr + slcr_gem0_rclk_ctrl + id) = (*(zynq_common.slcr + ctrl_gem0_rclk + id) & ~0x00000011) | (!!clk->pll.clkact0) |
				((!!clk->pll.srcsel) << 4);
			break;

		case ctrl_gem0_clk:
		case ctrl_gem1_clk:
			id = clk->dev - ctrl_gem0_clk;
			*(zynq_common.slcr + slcr_gem0_clk_ctrl + id) = (*(zynq_common.slcr + slcr_gem0_clk_ctrl + id) & ~0x03f03f71) | (!!clk->pll.clkact0) |
				((clk->pll.srcsel & 0x7) << 4) | ((clk->pll.divisor0 & 0x3f) << 8) | ((clk->pll.divisor1 & 0x3f) << 20);
			break;

		case ctrl_smc_clk:
			*(zynq_common.slcr + slcr_smc_clk_ctrl) = (*(zynq_common.slcr + slcr_smc_clk_ctrl) & ~0x00003f31) | (!!clk->pll.clkact0) |
				((clk->pll.srcsel & 0x3) << 4) | ((clk->pll.divisor0 & 0x3f) << 8);
			break;

		case ctrl_lqspi_clk:
			*(zynq_common.slcr + slcr_lqspi_clk_ctrl) = (*(zynq_common.slcr + slcr_lqspi_clk_ctrl) & ~0x00003f31) | (!!clk->pll.clkact0) |
				((clk->pll.srcsel & 0x3) << 4) | ((clk->pll.divisor0 & 0x3f) << 8);
			break;

		case ctrl_sdio_clk:
			*(zynq_common.slcr + slcr_sdio_clk_ctrl) = (*(zynq_common.slcr + slcr_sdio_clk_ctrl) & ~0x00003f33) | (!!clk->pll.clkact0) | ((!!clk->pll.clkact1) << 1) |
				((clk->pll.srcsel & 0x3) << 4) | ((clk->pll.divisor0 & 0x3f) << 8);
			break;

		case ctrl_uart_clk:
			*(zynq_common.slcr + slcr_uart_clk_ctrl) = (*(zynq_common.slcr + slcr_uart_clk_ctrl) & ~0x00003f33) | (!!clk->pll.clkact0) |
				((!!clk->pll.clkact1) << 1) | ((clk->pll.srcsel & 0x3) << 4) | ((clk->pll.divisor0 & 0x3f) << 8);
			break;

		case ctrl_spi_clk:
			*(zynq_common.slcr + slcr_spi_clk_ctrl) = (*(zynq_common.slcr + slcr_spi_clk_ctrl) & ~0x00003f33) | (!!clk->pll.clkact0) |
				((!!clk->pll.clkact1) << 1) | ((clk->pll.srcsel & 0x3) << 4) | ((clk->pll.divisor0 & 0x3f) << 8);
			break;

		case ctrl_can_clk:
			*(zynq_common.slcr + slcr_can_clk_ctrl) = (*(zynq_common.slcr + slcr_can_clk_ctrl) & ~0x03f03f33) | (!!clk->pll.clkact0) |
				((!!clk->pll.clkact1) << 1) | ((clk->pll.srcsel & 0x3) << 4) | ((clk->pll.divisor0 & 0x3f) << 8) | ((clk->pll.divisor1 & 0x3f) << 20);
			break;

		case ctrl_can_mioclk:
			*(zynq_common.slcr + slcr_can_mioclk_ctrl) = (*(zynq_common.slcr + slcr_can_mioclk_ctrl) & ~0x007f007f) | (clk->mio.mux0 & 0x3f) | ((!!clk->mio.ref0) << 6) |
				((clk->mio.mux1 & 0x3f) << 16) | ((!!clk->mio.ref1) << 22);
			break;

		default:
			_zynq_slcrLock();
			return -1;
	}

	_zynq_slcrLock();

	return 0;
}


int _zynq_getCtlClock(ctl_clock_t *clk)
{
	u32 id;
	u32 val = 0;

	switch (clk->dev) {
		case ctrl_usb0_clk:
		case ctrl_usb1_clk:
			id = clk->dev - ctrl_usb0_clk;
			val = *(zynq_common.slcr + slcr_usb0_clk_ctrl + id);
			clk->pll.srcsel = (val >> 4) & 0x7;
			clk->pll.clkact0 = clk->pll.clkact1 = clk->pll.divisor0 = clk->pll.divisor1 = 0;
			break;

		case ctrl_gem0_rclk:
		case ctrl_gem1_rclk:
			id = clk->dev - ctrl_gem0_rclk;
			val = *(zynq_common.slcr + slcr_gem0_rclk_ctrl + id);
			clk->pll.clkact0 = val & 0x1;
			clk->pll.srcsel = (val >> 4) & 0x1;
			clk->pll.clkact1 = clk->pll.divisor0 = clk->pll.divisor1 = 0;
			return 0;

		case ctrl_gem0_clk:
		case ctrl_gem1_clk:
			id = clk->dev - ctrl_gem0_clk;
			val = *(zynq_common.slcr + slcr_gem0_clk_ctrl + id);
			clk->pll.clkact0 = val & 0x1;
			clk->pll.srcsel = (val >> 4) & 0x7;
			clk->pll.divisor0 = (val >> 8) & 0x3f;
			clk->pll.divisor1 = (val >> 20) & 0x3f;
			clk->pll.clkact1 = 0;
			break;

		case ctrl_smc_clk:
			val = *(zynq_common.slcr + slcr_smc_clk_ctrl);
			clk->pll.clkact0 = val & 0x1;
			clk->pll.srcsel = (val >> 4) & 0x3;
			clk->pll.divisor0 = (val >> 8) & 0x3f;
			clk->pll.clkact1 = clk->pll.divisor1 = 0;
			break;

		case ctrl_lqspi_clk:
			val = *(zynq_common.slcr + slcr_lqspi_clk_ctrl);
			clk->pll.clkact0 = val & 0x1;
			clk->pll.srcsel = (val >> 4) & 0x3;
			clk->pll.divisor0 = (val >> 8) & 0x3f;
			clk->pll.clkact1 = clk->pll.divisor1 = 0;
			break;

		case ctrl_sdio_clk:
			val = *(zynq_common.slcr + slcr_sdio_clk_ctrl);
			clk->pll.clkact0 = val & 0x1;
			clk->pll.clkact1 = (val >> 1) & 0x1;
			clk->pll.srcsel = (val >> 4) & 0x3;
			clk->pll.divisor0 = (val >> 8) & 0x3f;
			clk->pll.divisor1 = 0;
			break;

		case ctrl_uart_clk:
			val = *(zynq_common.slcr + slcr_uart_clk_ctrl);
			clk->pll.clkact0 = val & 0x1;
			clk->pll.clkact1 = (val >> 1) & 0x1;
			clk->pll.srcsel = (val >> 4) & 0x3;
			clk->pll.divisor0 = (val >> 8) & 0x3f;
			clk->pll.divisor1 = 0;
			break;

		case ctrl_spi_clk:
			val = *(zynq_common.slcr + slcr_spi_clk_ctrl);
			clk->pll.clkact0 = val & 0x1;
			clk->pll.clkact1 = (val >> 1) & 0x1;
			clk->pll.srcsel = (val >> 4) & 0x3;
			clk->pll.divisor0 = (val >> 8) & 0x3f;
			clk->pll.divisor1 = 0;
			break;

		case ctrl_can_clk:
			val = *(zynq_common.slcr + slcr_can_clk_ctrl);
			clk->pll.clkact0 = val & 0x1;
			clk->pll.clkact1 = (val >> 1) & 0x1;
			clk->pll.srcsel = (val >> 4) & 0x3;
			clk->pll.divisor0 = (val >> 8) & 0x3f;
			clk->pll.divisor1 = (val >> 20) & 0x3f;
			break;

		case ctrl_can_mioclk:
			val = *(zynq_common.slcr + slcr_can_mioclk_ctrl);
			clk->mio.mux0 = val & 0x3f;
			clk->mio.ref0 = (val >> 6) & 0x1;
			clk->mio.mux1 = (val >> 16) & 0x3f;
			clk->mio.ref1 = (val >> 22) & 0x1;
			break;

		default:
			return -1;
	}

	return 0;
}


int _zynq_setMIO(const ctl_mio_t *mio)
{
	u32 val = 0;

	if (mio->pin > 53)
		return -1;

	val = (!!mio->triEnable) | (!!mio->l0 << 1) | (!!mio->l1 << 2) | ((mio->l2 & 0x3) << 3) |
		((mio->l3 & 0x7) << 5) | (!!mio->speed << 8) | ((mio->ioType & 0x7) << 9) | (!!mio->pullup << 12) |
		(!!mio->disableRcvr << 13);

	_zynq_slcrUnlock();
	*(zynq_common.slcr + slcr_mio_pin_00 + mio->pin) = (*(zynq_common.slcr + slcr_mio_pin_00 + mio->pin) & ~0x00003fff) | val;
	_zynq_slcrLock();

	return 0;
}


int _zynq_getMIO(ctl_mio_t *mio)
{
	u32 val;

	if (mio->pin > 53)
		return -1;

	val = *(zynq_common.slcr + slcr_mio_pin_00 + mio->pin);

	mio->triEnable = val & 0x1;
	mio->l0 = (val >> 1) & 0x1;
	mio->l1 = (val >> 2) & 0x1;
	mio->l2 = (val >> 3) & 0x3;
	mio->l3 = (val >> 5) & 0x7;
	mio->speed = (val >> 8) & 0x1;
	mio->ioType = (val >> 9) & 0x7;
	mio->pullup = (val >> 12) & 0x1;
	mio->disableRcvr = (val >> 13) & 0x1;

	return 0;
}


int _zynq_setSDWpCd(char dev, unsigned char wpPin, unsigned char cdPin)
{
	if ((dev != 0) && (dev != 1)) {
		return -1;
	}

	if ((cdPin > 63) || (wpPin > 63)) {
		return -1;
	}

	_zynq_slcrUnlock();
	*(zynq_common.slcr + slcr_sd0_wp_cd_sel + dev) = ((u32)cdPin << 16) | (wpPin);
	_zynq_slcrLock();
	return 0;
}


int _zynq_getSDWpCd(char dev, unsigned char *wpPin, unsigned char *cdPin)
{
	u32 val = 0;
	if ((dev != 0) && (dev != 1)) {
		return -1;
	}

	val = *(zynq_common.slcr + slcr_sd0_wp_cd_sel + dev);
	*wpPin = val & 0x3f;
	*cdPin = (val >> 16) & 0x3f;
	return 0;
}


int _zynq_loadPL(u32 srcAddr, u32 srcLen)
{
	u32 cnt;
	u32 wordsCnt;

	if (srcAddr < ADDR_DDR || (srcAddr + srcLen) > (ADDR_DDR + SIZE_DDR))
		return -1;

	*(zynq_common.dcfg + dcfg_unlock) = 0x757bdf0d;

	/* Clear interrupts */
	*(zynq_common.dcfg + dcfg_int_sts) = 0xffffffff;

	/* Initialize PL - reset controller */
	*(zynq_common.dcfg + dcfg_ctrl) |= (1 << 30);
	*(zynq_common.dcfg + dcfg_ctrl) &= ~(1 << 30);

	while (*(zynq_common.dcfg + dcfg_status) & (1 << 4))
		;

	*(zynq_common.dcfg + dcfg_ctrl) |= (1 << 30);
	*(zynq_common.dcfg + dcfg_int_sts) |= (1 << 2);

	/* Ensure that PL is ready for programming; PCFG_INIT = 1 */
	while (!(*(zynq_common.dcfg + dcfg_status) & (1 << 4)))
		;

	/* DMA queue can't be full */
	cnt = MAX_WAITING_COUNTER;
	while ((*(zynq_common.dcfg + dcfg_status) & (1 << 31))) {
		if (!--cnt)
			return -1;
	}

	/* Disable PCAP loopback */
	*(zynq_common.dcfg + dcfg_mctrl) &= ~(1 << 4);

	/* Program the PCAP_2x clock divider in non-secure mode */
	*(zynq_common.dcfg + dcfg_ctrl) &= ~(1 << 25);

	/* Initialize DMA data transfer */
	wordsCnt = ((srcLen + 0x3) & ~0x3) / 4;

	*(zynq_common.dcfg + dcfg_dma_src_addr) = srcAddr;
	*(zynq_common.dcfg + dcfg_dma_dest_addr) = 0xffffffff;

	*(zynq_common.dcfg + dcfg_dma_src_len) = wordsCnt;
	*(zynq_common.dcfg + dcfg_dma_dest_len) = wordsCnt;

	/* Wait for DMA tranfer to be done */
	cnt = MAX_WAITING_COUNTER;
	while (!(*(zynq_common.dcfg + dcfg_int_sts) & (1 << 13))) {
		if (!--cnt)
			return -1;
	}

	/* Check the following errors:
	 * AXI_WERR_INT, AXI_RTO_INT, AXI_RERR_INT, RX_FIFO_OV_INT, DMA_CMD_ERR_INT, DMA_Q_OV_INT, P2D_LEN_ERR_INT, PCFG_HMAC_ERR_INT */
	if (*(zynq_common.dcfg + dcfg_int_sts) & 0x74c840)
		return -1;

	/* Wait for FPGA to be done */
	cnt = MAX_WAITING_COUNTER;
	while (!(*(zynq_common.dcfg + dcfg_int_sts) & (1 << 2))) {
		if (!--cnt)
			return -1;
	}

	/* Post-config */
	_zynq_slcrUnlock();

	/* Enable level shifter */
	*(zynq_common.slcr + slcr_lvl_shftr_en) |= 0xf;

	/* Deassert FPGA reset */
	*(zynq_common.slcr + slcr_fpga_rst_ctrl) = 0;

	_zynq_slcrLock();

	return 0;
}


static void _zynq_ddrInit(void)
{
	/* DDR Control register's value differs from reset value (0x00000200).
	 * It allows to detect running under emulator (qemu) which sets DDR controler's registers to 0. */
	if (*(zynq_common.ddr + ddrc_ctrl) != 0x00000200)
		return;


	/* DDR initialization */

	_zynq_slcrUnlock();
	/* reserved_INP_POWER = 0x0; INP_TYPE = 0x0; DCI_UPDATE_B = 0x0; TERM_EN = 0x0; DCI_TYPE = 0x0; IBUF_DISABLE_MODE = 0x0; TERM_DISABLE_MODE = 0x0
	 * OUTPUT_EN = 0x3; PULLUP_EN = 0x0 */
	*(zynq_common.slcr + slcr_ddriob_addr0) = (*(zynq_common.slcr + slcr_ddriob_addr0) & ~0x00000fff) | 0x00000600;
	*(zynq_common.slcr + slcr_ddriob_addr1) = (*(zynq_common.slcr + slcr_ddriob_addr1) & ~0x00000fff) | 0x00000600;

	/* reserved_INP_POWER = 0x0; INP_TYPE = 0x1; DCI_UPDATE_B = 0x0; TERM_EN = 0x1; DCI_TYPE = 0x3; IBUF_DISABLE_MODE = 0; TERM_DISABLE_MODE = 0;
	 * OUTPUT_EN = 0x3; PULLUP_EN = 0x0 */
	*(zynq_common.slcr + slcr_ddriob_data0) = (*(zynq_common.slcr + slcr_ddriob_data0) & ~0x00000fff) | 0x00000672;
	*(zynq_common.slcr + slcr_ddriob_data1) = (*(zynq_common.slcr + slcr_ddriob_data1) & ~0x00000fff) | 0x00000672;

	/* reserved_INP_POWER = 0x0; INP_TYPE = 0x2; DCI_UPDATE_B = 0x0; TERM_EN = 0x1; DCI_TYPE = 0x3; IBUF_DISABLE_MODE = 0; TERM_DISABLE_MODE = 0;
	 * OUTPUT_EN = 0x3; PULLUP_EN = 0x0 */
	*(zynq_common.slcr + slcr_ddriob_diff0) = (*(zynq_common.slcr + slcr_ddriob_diff0) & ~0x00000fff) | 0x00000674;
	*(zynq_common.slcr + slcr_ddriob_diff1) = (*(zynq_common.slcr + slcr_ddriob_diff1) & ~0x00000fff) | 0x00000674;

	/* reserved_INP_POWER = 0x0; INP_TYPE = 0x0; DCI_UPDATE_B = 0x0; TERM_EN = 0x0; DCI_TYPE = 0x0; IBUF_DISABLE_MODE = 0x0; TERM_DISABLE_MODE = 0x0;
	 * OUTPUT_EN = 0x3; PULLUP_EN = 0x0 */
	*(zynq_common.slcr + slcr_ddriob_clock) = (*(zynq_common.slcr + slcr_ddriob_clock) & ~0x00000fff) | 0x00000600;

	/* reserved_DRIVE_P = 0x1c; reserved_DRIVE_N = 0xc; reserved_SLEW_P = 0x3; reserved_SLEW_N = 0x3; reserved_GTL = 0x0; reserved_RTERM = 0x0 */
	*(zynq_common.slcr + slcr_ddriob_drive_slew_addr) = (*(zynq_common.slcr + slcr_ddriob_drive_slew_addr) & ~0xffffffff) | 0x0018c61c;

	/* reserved_DRIVE_P = 0x1c; reserved_DRIVE_N = 0xc; reserved_SLEW_P = 0x6; reserved_SLEW_N = 0x1f; reserved_GTL = 0x0; reserved_RTERM = 0x0 */
	*(zynq_common.slcr + slcr_ddriob_drive_slew_data) = (*(zynq_common.slcr + slcr_ddriob_drive_slew_data) & ~0xffffffff) | 0x00f9861c;
	*(zynq_common.slcr + slcr_ddriob_drive_slew_diff) = (*(zynq_common.slcr + slcr_ddriob_drive_slew_diff) & ~0xffffffff) | 0x00f9861c;
	*(zynq_common.slcr + slcr_ddriob_drive_slew_clock) = (*(zynq_common.slcr + slcr_ddriob_drive_slew_clock) & ~0xffffffff) | 0x00f9861c;

	/* VREF_INT_EN = 0x1; VREF_SEL = 0x4; VREF_EXT_EN = 0x0; reserved_VREF_PULLUP_EN = 0x0; REFIO_EN = 0x1; reserved_REFIO_TEST = 0x0;
	 * reserved_REFIO_PULLUP_EN = 0x0; reserved_DRST_B_PULLUP_EN = 0x0; reserved_CKE_PULLUP_EN = 0x0 */
	*(zynq_common.slcr + slcr_ddriob_ddr_ctrl) = (*(zynq_common.slcr + slcr_ddriob_ddr_ctrl) & ~0x00007fff) | 0x00000209;

	/* Assert reset */
	*(zynq_common.slcr + slcr_ddriob_dci_ctrl) = (*(zynq_common.slcr + slcr_ddriob_dci_ctrl) & ~0x00000001) | 0x00000001;

	/* Deassert reset; reserved_VRN_OUT = 0x1 */
	*(zynq_common.slcr + slcr_ddriob_dci_ctrl) = (*(zynq_common.slcr + slcr_ddriob_dci_ctrl) & ~0x00000021) | 0x00000020;

	/* RESET = 0x1; ENABLE = 0x1; reserved_VRP_TRI = 0x0; reserved_VRN_TRI = 0x0; reserved_VRP_OUT = 0x0; reserved_VRN_OUT = 0x1; NREF_OPT1 = 0x0;
	 * NREF_OPT2 = 0x0; NREF_OPT4 = 0x1; PREF_OPT1 = 0x0; PREF_OPT2 = 0x0; UPDATE_CONTROL = 0x0; reserved_INIT_COMPLETE = 0x0; reserved_TST_CLK = 0x0;
	 * reserved_TST_HLN = 0x0; reserved_TST_HLP = 0x0; reserved_TST_RST = 0x0; reserved_INT_DCI_EN = 0x0 */
	*(zynq_common.slcr + slcr_ddriob_dci_ctrl) = (*(zynq_common.slcr + slcr_ddriob_dci_ctrl) & ~0x07feffff) | 0x00000823;
	_zynq_slcrLock();


	/* reg_ddrc_rdwr_idle_gap = 0x1 */
	*(zynq_common.ddr + ddrc_ctrl) = (*(zynq_common.ddr + ddrc_ctrl) & ~0x0001ffff) | 0x00000080;

	/* reg_ddrc_t_rfc_nom_x32 = 0x82; reserved_reg_ddrc_active_ranks = 0x1 */
	*(zynq_common.ddr + ddrc_two_rank_cfg) = (*(zynq_common.ddr + ddrc_two_rank_cfg) & ~0x0007ffff) | 0x00001082;

	/* reg_ddrc_hpr_min_non_critical_x32 = 0xf; reg_ddrc_hpr_max_starve_x32 = 0xf; reg_ddrc_hpr_xact_run_length = 0xf*/
	*(zynq_common.ddr + ddrc_hpr_reg) = (*(zynq_common.ddr + ddrc_hpr_reg) & ~0x03ffffff) | 0x03c0780f;

	/* reg_ddrc_lpr_min_non_critical_x32 = 0x1; reg_ddrc_lpr_max_starve_x32 = 0x2; reg_ddrc_lpr_xact_run_length = 0x8 */
	*(zynq_common.ddr + ddrc_lpr_reg) = (*(zynq_common.ddr + ddrc_lpr_reg) & ~0x03ffffff) | 0x02001001;

	/* reg_ddrc_w_min_non_critical_x32 = 0x1; reg_ddrc_w_xact_run_length = 0x8; reg_ddrc_w_max_starve_x32 = 0x2 */
	*(zynq_common.ddr + ddrc_wr_reg) = (*(zynq_common.ddr + ddrc_wr_reg) & ~0x03ffffff) | 0x00014001;

	*(zynq_common.ddr + ddrc_dram_param_reg0) = (*(zynq_common.ddr + ddrc_dram_param_reg0) & ~0x001fffff) | DDRC_DRAM_PARAM_REG0;
	*(zynq_common.ddr + ddrc_dram_param_reg1) = (*(zynq_common.ddr + ddrc_dram_param_reg1) & ~0xf7ffffff) | DDRC_DRAM_PARAM_REG1;
	*(zynq_common.ddr + ddrc_dram_param_reg2) = (*(zynq_common.ddr + ddrc_dram_param_reg2) & ~0xffffffff) | DDRC_DRAM_PARAM_REG2;
	*(zynq_common.ddr + ddrc_dram_param_reg3) = (*(zynq_common.ddr + ddrc_dram_param_reg3) & ~0x7fdffffc) | DDRC_DRAM_PARAM_REG3;
	*(zynq_common.ddr + ddrc_dram_param_reg4) = (*(zynq_common.ddr + ddrc_dram_param_reg4) & ~0x0fffffc3) | DDRC_DRAM_PARAM_REG4;

	/* eg_ddrc_final_wait_x32 = 0x7; reg_ddrc_pre_ocd_x32 = 0x0; reg_ddrc_t_mrd = 0x4 */
	*(zynq_common.ddr + ddrc_dram_init_param) = (*(zynq_common.ddr + ddrc_dram_init_param) & ~0x00003fff) | 0x00002007;

	/* reg_ddrc_emr2 = 0x8; reg_ddrc_emr3 = 0x0 */
	*(zynq_common.ddr + ddrc_dram_emr_reg) = (*(zynq_common.ddr + ddrc_dram_emr_reg) & ~0xffffffff) | 0x00000008;

	/* reg_ddrc_mr = 0xb30; reg_ddrc_emr = 0x4 */
	*(zynq_common.ddr + ddrc_dram_emr_mr_reg) = (*(zynq_common.ddr + ddrc_dram_emr_mr_reg) & ~0xffffffff) | 0x00040b30;

	/* reg_ddrc_burst_rdwr = 0x4; reg_ddrc_pre_cke_x1024 = 0x16d; reg_ddrc_post_cke_x1024 = 0x1; reg_ddrc_burstchop = 0x0 */
	*(zynq_common.ddr + ddrc_dram_burst8_rdwr) = (*(zynq_common.ddr + ddrc_dram_burst8_rdwr) & ~0x13ff3fff) | 0x000116d4;

	/* reg_ddrc_force_low_pri_n = 0x0; reg_ddrc_dis_dq = 0x0*/
	*(zynq_common.ddr + ddrc_dram_disable_dq) = (*(zynq_common.ddr + ddrc_dram_disable_dq) & ~0x3) | 0x0;

	/* reg_ddrc_addrmap_bank_b0 = 0x7; reg_ddrc_addrmap_bank_b1 = 0x7; reg_ddrc_addrmap_bank_b2 = 0x7; reg_ddrc_addrmap_col_b5 = 0x0; reg_ddrc_addrmap_col_b6 = 0x0 */
	*(zynq_common.ddr + ddrc_dram_addr_map_bank) = (*(zynq_common.ddr + ddrc_dram_addr_map_bank) & ~0x000fffff) | 0x00000777;

	*(zynq_common.ddr + ddrc_dram_addr_map_col) = (*(zynq_common.ddr + ddrc_dram_addr_map_col) & ~0xffffffff) | DDRC_DRAM_ADDR_MAP_COL;
	*(zynq_common.ddr + ddrc_dram_addr_map_row) = (*(zynq_common.ddr + ddrc_dram_addr_map_row) & ~0x0fffffff) | DDRC_DRAM_ADDR_MAP_ROW;

	/* reg_phy_rd_local_odt = 0x0; reg_phy_wr_local_odt = 0x3; reg_phy_idle_local_odt = 0x3; reserved_reg_ddrc_rank0_wr_odt = 0x1; reserved_reg_ddrc_rank0_rd_odt = 0x0 */
	*(zynq_common.ddr + ddrc_dram_odt_reg) = (*(zynq_common.ddr + ddrc_dram_odt_reg) & ~0x0003f03f) | 0x0003c008;

	/* reg_phy_rd_cmd_to_data = 0x0; reg_phy_wr_cmd_to_data = 0x0; reg_phy_rdc_we_to_re_delay = 0x8; reg_phy_rdc_fifo_rst_disable = 0x0; reg_phy_use_fixed_re = 0x1;
	 * reg_phy_rdc_fifo_rst_err_cnt_clr = 0x0; reg_phy_dis_phy_ctrl_rstn = 0x0; reg_phy_clk_stall_level = 0x0; reg_phy_gatelvl_num_of_dq0 = 0x7; reg_phy_wrlvl_num_of_dq0 = 0x7 */
	*(zynq_common.ddr + ddrc_phy_cmd_timeout_rddata_cpt) = (*(zynq_common.ddr + ddrc_phy_cmd_timeout_rddata_cpt) & ~0xff0f8fff) | 0x77010800;

	/* reg_ddrc_dis_dll_calib = 0x0 */
	*(zynq_common.ddr + ddrc_dll_calib) = (*(zynq_common.ddr + ddrc_dll_calib) & ~0x00010000) | 0x0;

	/* reg_ddrc_rd_odt_delay = 0x3; reg_ddrc_wr_odt_delay = 0x0; reg_ddrc_rd_odt_hold = 0x0; reg_ddrc_wr_odt_hold = 0x5 */
	*(zynq_common.ddr + ddrc_odt_delay_hold) = (*(zynq_common.ddr + ddrc_odt_delay_hold) & ~0x0000ffff) | 0x00005003;

	/* reg_ddrc_pageclose = 0x0; reg_ddrc_lpr_num_entries = 0x1f; reg_ddrc_auto_pre_en = 0x0; reg_ddrc_refresh_update_level = 0x0; reg_ddrc_dis_wc = 0x0;
	 * reg_ddrc_dis_collision_page_opt = 0x0; reg_ddrc_selfref_en = 0x0 */
	*(zynq_common.ddr + ddrc_ctrl_reg1) = (*(zynq_common.ddr + ddrc_ctrl_reg1) & ~0x000017ff) | 0x0000003e;

	/* reg_ddrc_go2critical_hysteresis = 0x0; reg_arb_go2critical_en = 0x1 */
	*(zynq_common.ddr + ddrc_ctrl_reg2) = (*(zynq_common.ddr + ddrc_ctrl_reg2) & ~0x00021fe0) | 0x00020000;

	/* reg_ddrc_wrlvl_ww = 0x41; reg_ddrc_rdlvl_rr = 0x41; reg_ddrc_dfi_t_wlmrd = 0x28 */
	*(zynq_common.ddr + ddrc_ctrl_reg3) = (*(zynq_common.ddr + ddrc_ctrl_reg3) & ~0x03ffffff) | 0x00284141;

	/* dfi_t_ctrlupd_interval_min_x1024 = 0x10; dfi_t_ctrlupd_interval_max_x1024 = 0x16 */
	*(zynq_common.ddr + ddrc_ctrl_reg4) = (*(zynq_common.ddr + ddrc_ctrl_reg4) & ~0x0000ffff) | 0x00001610;

	/* reg_ddrc_dfi_t_ctrl_delay = 0x1; reg_ddrc_dfi_t_dram_clk_disable = 0x1; reg_ddrc_dfi_t_dram_clk_enable = 0x1; reg_ddrc_t_cksre = 0x6;
	 * reg_ddrc_t_cksrx = 0x6; reg_ddrc_t_ckesr = 0x4 */
	*(zynq_common.ddr + ddrc_ctrl_reg5) = (*(zynq_common.ddr + ddrc_ctrl_reg5) & ~0x03ffffff) | 0x00466111;

	/* reg_ddrc_t_ckpde = 0x2; reg_ddrc_t_ckpdx = 0x2; reg_ddrc_t_ckdpde = 0x2; reg_ddrc_t_ckdpdx = 0x2; reg_ddrc_t_ckcsx = 0x3 */
	*(zynq_common.ddr + ddrc_ctrl_reg6) = (*(zynq_common.ddr + ddrc_ctrl_reg6) & ~0x000fffff) | 0x00032222;

	/* reg_ddrc_dis_auto_zq = 0x0; reg_ddrc_ddr3 = 0x1; reg_ddrc_t_mod = 0x200; reg_ddrc_t_zq_long_nop = 0x200; reg_ddrc_t_zq_short_nop = 0x40 */
	*(zynq_common.ddr + ddrc_che_t_zq) = (*(zynq_common.ddr + ddrc_che_t_zq) & ~0xffffffff) | 0x10200802;

	/* t_zq_short_interval_x1024 = 0xcb73; dram_rstn_x1024 = 0x69 */
	*(zynq_common.ddr + ddrc_che_t_zq_short_interval_reg) = (*(zynq_common.ddr + ddrc_che_t_zq_short_interval_reg) & ~0x0fffffff) | 0x0690cb73;

	/* deeppowerdown_en = 0x0; deeppowerdown_to_x1024 = 0xff */
	*(zynq_common.ddr + ddrc_deep_pwrdwn_reg) = (*(zynq_common.ddr + ddrc_deep_pwrdwn_reg) & ~0x000001ff) | 0x000001fe;

	/* dfi_wrlvl_max_x1024 = 0xfff; dfi_rdlvl_max_x1024 = 0xfff; ddrc_reg_twrlvl_max_error = 0x0; ddrc_reg_trdlvl_max_error = 0x0; reg_ddrc_dfi_wr_level_en = 0x1;
	 * reg_ddrc_dfi_rd_dqs_gate_level = 0x1; reg_ddrc_dfi_rd_data_eye_train = 0x1 */
	*(zynq_common.ddr + ddrc_reg_2c) = (*(zynq_common.ddr + ddrc_reg_2c) & ~0x1fffffff) | 0x1cffffff;

	/* reg_ddrc_skip_ocd = 0x1 */
	*(zynq_common.ddr + ddrc_reg_2d) = (*(zynq_common.ddr + ddrc_reg_2d) & ~0x00000200) | 0x00000200;

	/* reg_ddrc_dfi_t_rddata_en = 0x6; reg_ddrc_dfi_t_ctrlup_min = 0x3; reg_ddrc_dfi_t_ctrlup_max = 0x40 */
	*(zynq_common.ddr + ddrc_dfi_timinig) = (*(zynq_common.ddr + ddrc_dfi_timinig) & ~0x01ffffff) | 0x00200066;

	/* Clear_Uncorrectable_DRAM_ECC_error = 0x0; Clear_Correctable_DRAM_ECC_error = 0x0 */
	*(zynq_common.ddr + ddrc_che_ecc_ctrl_reg_offs) = (*(zynq_common.ddr + ddrc_che_ecc_ctrl_reg_offs) & ~0x00000003) | 0x00000000;

	/* CORR_ECC_LOG_VALID = 0x0; ECC_CORRECTED_BIT_NUM = 0x0 */
	*(zynq_common.ddr + ddrc_che_corr_ecc_log_reg_offs) = (*(zynq_common.ddr + ddrc_che_corr_ecc_log_reg_offs) & ~0x000000FF) | 0x00000000;

	/* UNCORR_ECC_LOG_VALID = 0x0 */
	*(zynq_common.ddr + ddrc_che_uncorr_ecc_log_reg_offs) = (*(zynq_common.ddr + ddrc_che_uncorr_ecc_log_reg_offs) & ~0x00000001) | 0x00000000;

	/* STAT_NUM_CORR_ERR = 0x0; STAT_NUM_UNCORR_ERR = 0x0 */
	*(zynq_common.ddr + ddrc_che_ecc_stats_reg_offs) = (*(zynq_common.ddr + ddrc_che_ecc_stats_reg_offs) & ~0x0000ffff) | 0x00000000;

	/* reg_ddrc_ecc_mode = 0x0; reg_ddrc_dis_scrub = 0x1 */
	*(zynq_common.ddr + ddrc_ecc_scrub) = (*(zynq_common.ddr + ddrc_ecc_scrub) & ~0x0000000f) | 0x00000008;

	/* reg_phy_dif_on = 0x0; reg_phy_dif_off = 0x0 */
	*(zynq_common.ddr + ddrc_phy_rcvr_enable) = (*(zynq_common.ddr + ddrc_phy_rcvr_enable) & ~0x000000ff) | 0x00000000;

	*(zynq_common.ddr + ddrc_phy_config0) = (*(zynq_common.ddr + ddrc_phy_config0) & ~0x7fffffcf) | DDRC_PHY_CONFIG0;
	*(zynq_common.ddr + ddrc_phy_config1) = (*(zynq_common.ddr + ddrc_phy_config1) & ~0x7fffffcf) | DDRC_PHY_CONFIG1;
	*(zynq_common.ddr + ddrc_phy_config2) = (*(zynq_common.ddr + ddrc_phy_config2) & ~0x7fffffcf) | DDRC_PHY_CONFIG2;
	*(zynq_common.ddr + ddrc_phy_config3) = (*(zynq_common.ddr + ddrc_phy_config3) & ~0x7fffffcf) | DDRC_PHY_CONFIG3;

	*(zynq_common.ddr + ddrc_phy_init_ratio0) = (*(zynq_common.ddr + ddrc_phy_init_ratio0) & ~0x000fffff) | DDRC_PHY_INIT_RATIO0;
	*(zynq_common.ddr + ddrc_phy_init_ratio1) = (*(zynq_common.ddr + ddrc_phy_init_ratio1) & ~0x000fffff) | DDRC_PHY_INIT_RATIO1;
	*(zynq_common.ddr + ddrc_phy_init_ratio2) = (*(zynq_common.ddr + ddrc_phy_init_ratio2) & ~0x000fffff) | DDRC_PHY_INIT_RATIO2;
	*(zynq_common.ddr + ddrc_phy_init_ratio3) = (*(zynq_common.ddr + ddrc_phy_init_ratio3) & ~0x000fffff) | DDRC_PHY_INIT_RATIO3;

	*(zynq_common.ddr + ddrc_phy_rd_dqs_cfg0) = (*(zynq_common.ddr + ddrc_phy_rd_dqs_cfg0) & ~0x000fffff) | DDRC_PHY_RD_DQS_CFG0;
	*(zynq_common.ddr + ddrc_phy_rd_dqs_cfg1) = (*(zynq_common.ddr + ddrc_phy_rd_dqs_cfg1) & ~0x000fffff) | DDRC_PHY_RD_DQS_CFG1;
	*(zynq_common.ddr + ddrc_phy_rd_dqs_cfg2) = (*(zynq_common.ddr + ddrc_phy_rd_dqs_cfg2) & ~0x000fffff) | DDRC_PHY_RD_DQS_CFG2;
	*(zynq_common.ddr + ddrc_phy_rd_dqs_cfg3) = (*(zynq_common.ddr + ddrc_phy_rd_dqs_cfg3) & ~0x000fffff) | DDRC_PHY_RD_DQS_CFG3;

	*(zynq_common.ddr + ddrc_phy_wr_dqs_cfg0) = (*(zynq_common.ddr + ddrc_phy_wr_dqs_cfg0) & ~0x000fffff) | DDRC_PHY_WR_DQS_CFG0;
	*(zynq_common.ddr + ddrc_phy_wr_dqs_cfg1) = (*(zynq_common.ddr + ddrc_phy_wr_dqs_cfg1) & ~0x000fffff) | DDRC_PHY_WR_DQS_CFG1;
	*(zynq_common.ddr + ddrc_phy_wr_dqs_cfg2) = (*(zynq_common.ddr + ddrc_phy_wr_dqs_cfg2) & ~0x000fffff) | DDRC_PHY_WR_DQS_CFG2;
	*(zynq_common.ddr + ddrc_phy_wr_dqs_cfg3) = (*(zynq_common.ddr + ddrc_phy_wr_dqs_cfg3) & ~0x000fffff) | DDRC_PHY_WR_DQS_CFG3;

	*(zynq_common.ddr + ddrc_phy_we_cfg0) = (*(zynq_common.ddr + ddrc_phy_we_cfg0) & ~0x001fffff) | DDRC_PHY_WE_CFG0;
	*(zynq_common.ddr + ddrc_phy_we_cfg1) = (*(zynq_common.ddr + ddrc_phy_we_cfg1) & ~0x001fffff) | DDRC_PHY_WE_CFG1;
	*(zynq_common.ddr + ddrc_phy_we_cfg2) = (*(zynq_common.ddr + ddrc_phy_we_cfg2) & ~0x001fffff) | DDRC_PHY_WE_CFG2;
	*(zynq_common.ddr + ddrc_phy_we_cfg3) = (*(zynq_common.ddr + ddrc_phy_we_cfg3) & ~0x001fffff) | DDRC_PHY_WE_CFG3;

	*(zynq_common.ddr + ddrc_wr_data_slv0) = (*(zynq_common.ddr + ddrc_wr_data_slv0) & ~0x000fffff) | DDRC_WR_DATA_SLV0;
	*(zynq_common.ddr + ddrc_wr_data_slv1) = (*(zynq_common.ddr + ddrc_wr_data_slv1) & ~0x000fffff) | DDRC_WR_DATA_SLV1;
	*(zynq_common.ddr + ddrc_wr_data_slv2) = (*(zynq_common.ddr + ddrc_wr_data_slv2) & ~0x000fffff) | DDRC_WR_DATA_SLV2;
	*(zynq_common.ddr + ddrc_wr_data_slv3) = (*(zynq_common.ddr + ddrc_wr_data_slv3) & ~0x000fffff) | DDRC_WR_DATA_SLV3;

	/* reg_phy_bl2 = 0x0; reg_phy_at_spd_atpg = 0x0; reg_phy_bist_enable = 0x0; reg_phy_bist_force_err = 0x0; reg_phy_bist_mode = 0x0;
	 * reg_phy_invert_clkout = 0x1; reg_phy_sel_logic = 0x0; reg_phy_ctrl_slave_ratio = 0x100; reg_phy_ctrl_slave_force = 0x0; reg_phy_ctrl_slave_delay = 0x0;
	 * reg_phy_lpddr = 0x0; reg_phy_cmd_latency = 0x0 */
	*(zynq_common.ddr + ddrc_reg_64) = (*(zynq_common.ddr + ddrc_reg_64) & ~0x6ffffefe) | 0x00040080;

	/* reg_phy_wr_rl_delay = 0x2; reg_phy_rd_rl_delay = 0x4; reg_phy_dll_lock_diff = 0xf; reg_phy_use_wr_level = 0x1; reg_phy_use_rd_dqs_gate_level = 0x1;
	 * reg_phy_use_rd_data_eye_level = 0x1; reg_phy_dis_calib_rst = 0x0; reg_phy_ctrl_slave_delay = 0x0 */
	*(zynq_common.ddr + ddrc_reg_65) = (*(zynq_common.ddr + ddrc_reg_65) & ~0x000fffff) | 0x0001fc82;

	/* reg_arb_page_addr_mask = 0x0 */
	*(zynq_common.ddr + ddrc_page_mask) = (*(zynq_common.ddr + ddrc_page_mask) & ~0xffffffff) | 0x00000000;

	/* reg_arb_pri_wr_portn = 0x3ff; reg_arb_disable_aging_wr_portn = 0x0; reg_arb_disable_urgent_wr_portn = 0x0; reg_arb_dis_page_match_wr_portn = 0x0 */
	*(zynq_common.ddr + ddrc_axi_priority_wr_port0) = (*(zynq_common.ddr + ddrc_axi_priority_wr_port0) & ~0x000703ff) | 0x000003ff;
	*(zynq_common.ddr + ddrc_axi_priority_wr_port1) = (*(zynq_common.ddr + ddrc_axi_priority_wr_port1) & ~0x000703ff) | 0x000003ff;
	*(zynq_common.ddr + ddrc_axi_priority_wr_port2) = (*(zynq_common.ddr + ddrc_axi_priority_wr_port2) & ~0x000703ff) | 0x000003ff;
	*(zynq_common.ddr + ddrc_axi_priority_wr_port3) = (*(zynq_common.ddr + ddrc_axi_priority_wr_port3) & ~0x000703ff) | 0x000003ff;

	/* reg_arb_pri_rd_portn = 0x3ff; reg_arb_disable_aging_rd_portn = 0x0; reg_arb_disable_urgent_rd_portn = 0x0; reg_arb_dis_page_match_rd_portn = 0x0;
	 * reg_arb_set_hpr_rd_portn = 0x0 */
	*(zynq_common.ddr + ddrc_axi_priority_rd_port0) = (*(zynq_common.ddr + ddrc_axi_priority_rd_port0) & ~0x000f03ff) | 0x000003ff;
	*(zynq_common.ddr + ddrc_axi_priority_rd_port1) = (*(zynq_common.ddr + ddrc_axi_priority_rd_port1) & ~0x000f03ff) | 0x000003ff;
	*(zynq_common.ddr + ddrc_axi_priority_rd_port2) = (*(zynq_common.ddr + ddrc_axi_priority_rd_port2) & ~0x000f03ff) | 0x000003ff;
	*(zynq_common.ddr + ddrc_axi_priority_rd_port3) = (*(zynq_common.ddr + ddrc_axi_priority_rd_port3) & ~0x000f03ff) | 0x000003ff;

	/* reg_ddrc_lpddr2 = 0x0; reg_ddrc_derate_enable = 0x0; reg_ddrc_mr4_margin = 0x0 */
	*(zynq_common.ddr + ddrc_lpddr_ctrl0) = (*(zynq_common.ddr + ddrc_lpddr_ctrl0) & ~0x00000ff5) | 0x00000000;

	/* reg_ddrc_mr4_read_interval = 0x0 */
	*(zynq_common.ddr + ddrc_lpddr_ctrl1) = (*(zynq_common.ddr + ddrc_lpddr_ctrl1) & ~0xffffffff) | 0x00000000;

	/* reg_ddrc_min_stable_clock_x1 = 0x5; reg_ddrc_idle_after_reset_x32 = 0x12; reg_ddrc_t_mrw = 0x5 */
	*(zynq_common.ddr + ddrc_lpddr_ctrl2) = (*(zynq_common.ddr + ddrc_lpddr_ctrl2) & ~0x003fffff) | 0x00005125;

	/* reg_ddrc_max_auto_init_x1024 = 0xa8; reg_ddrc_dev_zqinit_x32 = 0x12 */
	*(zynq_common.ddr + ddrc_lpddr_ctrl3) = (*(zynq_common.ddr + ddrc_lpddr_ctrl3) & ~0x0003ffff) | 0x000012a8;


	/* POLL ON DCI STATUS */
	while (!(*(zynq_common.slcr + slcr_ddriob_dci_status) & 0x00002000))
		;

	/* Unlock DDR */
	/* reg_ddrc_soft_rstb = 0x1; reg_ddrc_powerdown_en = 0x0; reg_ddrc_data_bus_width = 0x0; reg_ddrc_burst8_refresh = 0x0; reg_ddrc_rdwr_idle_gap = 1;
	 * reg_ddrc_dis_rd_bypass = 0x0; reg_ddrc_dis_act_bypass = 0x0; reg_ddrc_dis_auto_refresh = 0x0 */
	*(zynq_common.ddr + ddrc_ctrl) = (*(zynq_common.ddr + ddrc_ctrl) & ~0x0001ffff) | 0x00000081;

	/* Check DDR status */
	while (!(*(zynq_common.ddr + ddrc_mode_sts_reg) & 0x00000007))
		;

	_zynq_slcrUnlock();

	/* IBUF_DISABLE_MODE = 0x1; TERM_DISABLE_MODE = 0x1 */
	*(zynq_common.slcr + slcr_ddriob_data0) = (*(zynq_common.slcr + slcr_ddriob_data0) & ~0x00000180) | 0x00000180;
	*(zynq_common.slcr + slcr_ddriob_data1) = (*(zynq_common.slcr + slcr_ddriob_data1) & ~0x00000180) | 0x00000180;
	*(zynq_common.slcr + slcr_ddriob_diff0) = (*(zynq_common.slcr + slcr_ddriob_diff0) & ~0x00000180) | 0x00000180;
	*(zynq_common.slcr + slcr_ddriob_diff1) = (*(zynq_common.slcr + slcr_ddriob_diff1) & ~0x00000180) | 0x00000180;

	_zynq_slcrLock();
}


static void _zynq_clcksInit(void)
{
	_zynq_slcrUnlock();

	/* Set divisers for DDR DCI, clock source: DDR PLL
	 * DDR DCI : 1000 MHz / 7 / 15 ~= 10 MHz
	 * CLKACT = 0x1; DIVISOR0 = 0xf; DIVISOR1 = 0x7 */
	*(zynq_common.slcr + slcr_dci_clk_ctrl) = (*(zynq_common.slcr + slcr_dci_clk_ctrl) & ~0x03f03f01) | 0x00700f01;

	/* Initialize PCAP interface, source clock: IO PLL
	 * PCAP   : 1000 MHz / 5 = 200 MHz
	 * CLKACT = 0x1; SRCSEL = 0x0; DIVISOR = 0x5 */
	*(zynq_common.slcr + slcr_pcap_clk_ctrl) = (*(zynq_common.slcr + slcr_pcap_clk_ctrl) & ~0x00003f31) | 0x00000501;

	/* FPGA clock, source clock: IO PLL
	 * FPGA   : 1000 MHz / 5 / 2 = 100 MHz
	 * SRCSEL = 0x0; DIVISOR0 = 0x5; DIVISOR1 = 0x2 */
	*(zynq_common.slcr + slcr_fpga0_clk_ctrl) = (*(zynq_common.slcr + slcr_fpga0_clk_ctrl) & ~0x03f03f30) | 0x00200500;

	/* Set 6:2:1 as CPU clock ratio */
	*(zynq_common.slcr + slcr_clk_621_true) = (*(zynq_common.slcr + slcr_clk_621_true) & ~0x00000001) | 0x00000001;

	/* Disable unused clocks
	 * DMA_CPU_2XCLKACT   = 0x0;
	 * USB0_CPU_1XCLKACT  = 0x0; USB1_CPU_1XCLKACT  = 0x0
	 * GEM0_CPU_1XCLKACT  = 0x0; GEM1_CPU_1XCLKACT  = 0x0
	 * SDI0_CPU_1XCLKACT  = 0x0; SDI1_CPU_1XCLKACT  = 0x0
	 * SPI0_CPU_1XCLKACT  = 0x0; SPI1_CPU_1XCLKACT  = 0x0
	 * CAN0_CPU_1XCLKACT  = 0x0; CAN1_CPU_1XCLKACT  = 0x0
	 * I2C0_CPU_1XCLKACT  = 0x0; I2C1_CPU_1XCLKACT  = 0x0
	 * UART0_CPU_1XCLKACT = 0x0; UART1_CPU_1XCLKACT = 0x0
	 * GPIO_CPU_1XCLKACT  = 0x0
	 * LQSPI_CPU_1XCLKACT = 0x0
	 * SMC_CPU_1XCLKACT   = 0x0                           */
	*(zynq_common.slcr + slcr_aper_clk_ctrl) = (*(zynq_common.slcr + slcr_aper_clk_ctrl) & ~0x01ffcccd) | 0x00000000;

	_zynq_slcrLock();
}


static void _zynq_ioPllInit(u16 fdiv)
{
	_zynq_slcrUnlock();

	/* PLL_RES = 0xc; PLL_CP = 0x2; LOCK_CNT = 0x145 */
	*(zynq_common.slcr + slcr_io_pll_cfg) = (*(zynq_common.slcr + slcr_io_pll_cfg) & ~0x003ffff0) | 0x001452c0;

	/* Set the feedback divisor for the PLL */
	*(zynq_common.slcr + slcr_io_pll_ctrl) = (*(zynq_common.slcr + slcr_io_pll_ctrl) & ~0x0007f000) | (fdiv << 12);

	/* By pass pll
	 * PLL_BYPASS_FORCE = 1 */
	*(zynq_common.slcr + slcr_io_pll_ctrl) = (*(zynq_common.slcr + slcr_io_pll_ctrl) & ~0x00000010) | 0x00000010;

	/* Assert reset
	 * PLL_RESET = 1 */
	*(zynq_common.slcr + slcr_io_pll_ctrl) = (*(zynq_common.slcr + slcr_io_pll_ctrl) & ~0x00000001) | 0x00000001;

	/* Deassert reset
	 * PLL_RESET = 0 */
	*(zynq_common.slcr + slcr_io_pll_ctrl) = (*(zynq_common.slcr + slcr_io_pll_ctrl) & ~0x00000001) | 0x00000000;

	/* Check pll status */
	while(!(*(zynq_common.slcr + slcr_pll_status) & 0x00000004))
		;

	/* Remove pll by pass
	 * PLL_BYPASS_FORCE = 1 */
	*(zynq_common.slcr + slcr_io_pll_ctrl) = (*(zynq_common.slcr + slcr_io_pll_ctrl) & ~0x00000010) | 0x00000000;


	_zynq_slcrLock();
}


static void _zynq_ddrPllInit(u16 fdiv)
{
	_zynq_slcrUnlock();

	/* PLL_RES = 0x2; PLL_CP = 0x2; LOCK_CNT = 0x12c */
	*(zynq_common.slcr + slcr_ddr_pll_cfg) = (*(zynq_common.slcr + slcr_ddr_pll_cfg) & ~0x003ffff0) | 0x0012c220;

	/* Set the feedback divisor for the PLL */
	*(zynq_common.slcr + slcr_ddr_pll_ctrl) = (*(zynq_common.slcr + slcr_ddr_pll_ctrl) & ~0x0007f000) | (fdiv << 12);

	/* By pass pll
	 * PLL_BYPASS_FORCE = 1 */
	*(zynq_common.slcr + slcr_ddr_pll_ctrl) = (*(zynq_common.slcr + slcr_ddr_pll_ctrl) & ~0x00000010) | 0x00000010;

	/* Assert reset
	 * PLL_RESET = 1 */
	*(zynq_common.slcr + slcr_ddr_pll_ctrl) = (*(zynq_common.slcr + slcr_ddr_pll_ctrl) & ~0x00000001) | 0x00000001;

	/* Deassert reset
	 * PLL_RESET = 0 */
	*(zynq_common.slcr + slcr_ddr_pll_ctrl) = (*(zynq_common.slcr + slcr_ddr_pll_ctrl) & ~0x00000001) | 0x00000000;

	/* Check pll status */
	while(!(*(zynq_common.slcr + slcr_pll_status) & 0x00000002))
		;

	/* Remove pll by pass
	 * PLL_BYPASS_FORCE = 1 */
	*(zynq_common.slcr + slcr_ddr_pll_ctrl) = (*(zynq_common.slcr + slcr_ddr_pll_ctrl) & ~0x00000010) | 0x00000000;

	*(zynq_common.slcr + slcr_ddr_clk_ctrl) = (*(zynq_common.slcr + slcr_ddr_clk_ctrl) & ~0xfff00003) | 0x0c200003;


	_zynq_slcrLock();
}


static void _zynq_armPllInit(u16 fdiv)
{
	_zynq_slcrUnlock();

	/* PLL_RES = 0x2; PLL_CP = 0x2; LOCK_CNT = 0xfa */
	*(zynq_common.slcr + slcr_arm_pll_cfg) = (*(zynq_common.slcr + slcr_arm_pll_cfg) & ~0x003ffff0) | 0x000fa220;

	/* Set the feedback divisor for the PLL */
	*(zynq_common.slcr + slcr_arm_pll_ctrl) = (*(zynq_common.slcr + slcr_arm_pll_ctrl) & ~0x0007f000) | (fdiv << 12);

	/* By pass pll
	 * PLL_BYPASS_FORCE = 1 */
	*(zynq_common.slcr + slcr_arm_pll_ctrl) = (*(zynq_common.slcr + slcr_arm_pll_ctrl) & ~0x00000010) | 0x00000010;

	/* Assert reset
	 * PLL_RESET = 1 */
	*(zynq_common.slcr + slcr_arm_pll_ctrl) = (*(zynq_common.slcr + slcr_arm_pll_ctrl) & ~0x00000001) | 0x00000001;

	/* Deassert reset
	 * PLL_RESET = 0 */
	*(zynq_common.slcr + slcr_arm_pll_ctrl) = (*(zynq_common.slcr + slcr_arm_pll_ctrl) & ~0x00000001) | 0x00000000;

	/* Check pll status */
	while(!(*(zynq_common.slcr + slcr_pll_status) & 0x00000001))
		;

	/* Remove pll by pass
	 * PLL_BYPASS_FORCE = 1 */
	*(zynq_common.slcr + slcr_arm_pll_ctrl) = (*(zynq_common.slcr + slcr_arm_pll_ctrl) & ~0x00000010) | 0x00000000;

	/* ARM PLL / 2 as a source fo CPU.
	 * SRCSEL = 0x0; DIVISOR = 0x2; CPU_6OR4XCLKACT = 0x1; CPU_3OR2XCLKACT = 0x1; CPU_2XCLKACT = 0x1; CPU_1XCLKACT = 0x1; CPU_PERI_CLKACT = 0x1 */
	*(zynq_common.slcr + slcr_arm_clk_ctrl) = (*(zynq_common.slcr + slcr_arm_clk_ctrl) & ~0x1f003f30) | 0x1f000200;

	_zynq_slcrLock();
}


void _zynq_softRst(void)
{
	_zynq_slcrUnlock();

	*(zynq_common.slcr + slcr_pss_rst_ctrl) |= 0x1;

	_zynq_slcrLock();
}


void _zynq_init(void)
{
	zynq_common.ddr = (void *)DDRC_BASE_ADDRESS;
	zynq_common.slcr = (void *)SCLR_BASE_ADDRESS;
	zynq_common.dcfg = (void *)DEVCFG_BASE_ADDRESS;

	/* PLL & Clocks initialization:
	 *  -  PS_CLK       : 33.33 MHz (depends on external source connected to the hardware)
	 *  -  ARM_PLL_FDIV : 40
	 *  -  ARM_PLL      : 33.33 MHz * 40 = 1333 MHz
	 *  -  ARM_PLL      : source used to generate the CPU clock
	 *  -  CPU_CLOCK    : 1333 MHz / 2 = 667 MHz                                       */
	_zynq_armPllInit(40);

	/*  -  DDR_PLL_FDIV : 32
	 *  -  DDR_PLL      : 33.33 MHz * 32 = 1067 MHz                                    */
	_zynq_ddrPllInit(32);

	/*  -  DDR_IO_FDIV  : 30
	 *  -  IO_PLL       : 33.33 MHz * 30 = 1000 MHz                                    */
	_zynq_ioPllInit(30);

	_zynq_clcksInit();

	_zynq_ddrInit();
}
