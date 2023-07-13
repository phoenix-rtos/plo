/*
 * Phoenix-RTOS
 *
 * Operating system kernel
 *
 * Low-level initialization for iMX6ULL processor
 *
 * Copyright 2018, 2021 Phoenix Systems
 * Author: Jan Sikorski, Aleksander Kaminski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _IMX6ULL_H_
#define _IMX6ULL_H_


#include "types.h"


/* clang-format off */
/* Devices clocks */
enum {
	/* CCM_CCGR0 */
	clk_aips_tz1 = 0, clk_aips_tz2, clk_apbhdma, clk_asrc, clk_dcp = clk_asrc + 2,
	clk_enet, clk_can1, clk_can1_serial, clk_can2, clk_can2_serial, clk_arm_dbg,
	clk_gpt2, clk_gpt2_serial, clk_uart2, clk_gpio2,

	/* CCM_CCGR1 */
	clk_ecspi1, clk_ecspi2, clk_ecspi3, clk_ecspi4, clk_adc2, clk_uart3,
	clk_epit1, clk_epit2, clk_adc1, clk_sim_s, clk_gpt, clk_gpt_serial,
	clk_uart4, clk_gpio1, clk_csu, clk_gpio5,

	/* CCM_CCGR2 */
	clk_esai = clk_gpio5 + 1, clk_csi, clk_iomuxc_snvs, clk_i2c1_serial,
	clk_i2c2_serial, clk_i2c3_serial, clk_ocotp_ctrl, clk_iomux_ipt_clk_io, clk_ipmux1,
	clk_ipmux2, clk_ipmux3, clk_ipsync_ip2apb_tzasc1_ipg,
	clk_gpio3 = clk_ipsync_ip2apb_tzasc1_ipg + 2, clk_lcd, clk_pxp,

	/* CCM_CCGR3 */
	clk_uart5 = clk_pxp + 2, clk_epdc, clk_uart6, clk_ca7_ccm_dap, clk_lcdif1_pix,
	clk_gpio4, clk_qspi, clk_wdog1, clk_a7_clkdiv_patch, clk_mmdc_core_aclk_fast_core_p0,
	clk_mmdc_core_ipg_clk_p0 = clk_mmdc_core_aclk_fast_core_p0 + 2, clk_mmdc_core_ipg_clk_p1,
	clk_ocram, clk_iomux_snvs_gpr,

	/* CCM_CCGR4 */
	clk_iomuxc = clk_iomux_snvs_gpr + 2, clk_iomuxc_gpr, clk_sim_cpu,
	clk_cxapbsyncbridge_slave, clk_tsc_dig, clk_p301_mx6qper1_bch, clk_pl301_mx6qper2_mainclk,
	clk_pwm1, clk_pwm2, clk_pwm3, clk_pwm4, clk_rawnand_u_bch_input_apb,
	clk_rawnand_u_gpmi_bch_input_bch, clk_rawnand_u_gpmi_bch_input_gpmi_io,
	clk_rawnand_u_gpmi_input_apb,

	/* CCM_CCGR5 */
	clk_rom, clk_sctr, clk_snvs_dryice, clk_sdma, clk_kpp, clk_wdog2,
	clk_spba, clk_spdif, clk_sim_main, clk_snvs_hp, clk_snvs_lp, clk_sai3,
	clk_uart1, clk_uart7, clk_sai1, clk_sai2,

	/* CCM_CCGR6 */
	clk_usboh3, clk_usdhc1, clk_usdhc2, clk_ipmux4 = clk_usdhc2 + 2,
	clk_eim_slow, clk_uart_debug, clk_uart8, clk_pwm8, clk_aips_tz3, clk_wdog3,
	clk_anadig, clk_i2c4_serial, clk_pwm5, clk_pwm6, clk_pwm7
};


/* IOMUX - DAISY */
enum {
	isel_anatop = 302, isel_usb_otg2id, isel_ccm_pmicrdy, isel_csi_d2, isel_csi_d3,
	isel_csi_d5, isel_csi_d0, isel_csi_d1, isel_csi_d4, isel_csi_d6, isel_csi_d7,
	isel_csi_d8, isel_csi_d9, isel_csi_d10, isel_csi_d11, isel_csi_d12,
	isel_csi_d13, isel_csi_d14, isel_csi_d15, isel_csi_d16, isel_csi_d17,
	isel_csi_d18, isel_csi_d19, isel_csi_d20, isel_csi_d21, isel_csi_d22,
	isel_csi_d23, isel_csi_hsync, isel_csi_pclk, isel_csi_vsync, isel_csi_field,
	isel_ecspi1_sclk, isel_ecspi1_miso, isel_ecspi1_mosi, isel_ecspi1_ss0,
	isel_ecspi2_sclk, isel_ecspi2_miso, isel_ecspi2_mosi, isel_ecspi2_ss0,
	isel_ecspi3_sclk, isel_ecspi3_miso, isel_ecspi3_mosi, isel_ecspi3_ss0,
	isel_ecspi4_sclk, isel_ecspi4_miso, isel_ecspi4_mosi, isel_ecspi4_ss0,
	isel_enet1_refclk1, isel_enet1_mac0mdio, isel_enet2_refclk2, isel_enet2_mac0mdio,
	isel_flexcan1_rx, isel_flexcan2_rx, isel_gpt1_cap1, isel_gpt1_cap2, isel_gpt1_clksel,
	isel_gpt2_cap1, isel_gpt2_cap2, isel_gpt2_clksel, isel_i2c1_scl, isel_i2c1_sda,
	isel_i2c2_scl, isel_i2c2_sda, isel_i2c3_scl, isel_i2c3_sda, isel_i2c4_scl,
	isel_i2c4_sda, isel_kpp_col0, isel_kpp_col1, isel_kpp_col2, isel_kpp_row0,
	isel_kpp_row1, isel_kpp_row2, isel_lcd_busy, isel_sai1_mclk, isel_sai1_rx,
	isel_sai1_txclk, isel_sai1_txsync, isel_sai2_mclk, isel_sai2_rx, isel_sai2_txclk,
	isel_sai2_txsync, isel_sai3_mclk, isel_sai3_rx, isel_sai3_txclk, isel_sai3_txsync,
	isel_sdma_ev0, isel_sdma_ev1, isel_spdif_in, isel_spdif_clk, isel_uart1_rts,
	isel_uart1_rx, isel_uart2_rts, isel_uart2_rx, isel_uart3_rts, isel_uart3_rx,
	isel_uart4_rts, isel_uart4_rx, isel_uart5_rts, isel_uart5_rx, isel_uart6_rts,
	isel_uart6_rx, isel_uart7_rts, isel_uart7_rx, isel_uart8_rts, isel_uart8_rx,
	isel_usb_otg2oc, isel_usb_otgoc, isel_usdhc1_cd, isel_usdhc1_wp, isel_usdhc2_clk,
	isel_usdhc2_cd, isel_usdhc2_cmd, isel_usdhc2_d0, isel_usdhc2_d1, isel_usdhc2_d2,
	isel_usdhc2_d3, isel_usdhc2_d4, isel_usdhc2_d5, isel_usdhc2_d6, isel_usdhc2_d7,
	isel_usdhc2_wp
};


/* IOMUX - MUX */
enum {
	mux_boot_mode0 = 0, mux_boot_mode1, mux_tamper0, mux_tamper1, mux_tamper2,
	mux_tamper3, mux_tamper4, mux_tamper5, mux_tamper6, mux_tamper7, mux_tamper8,
	mux_tamper9, mux_jtag_mod = 17, mux_jtag_tms, mux_jtag_tdo, mux_jtag_tdi,
	mux_jtag_tck, mux_jtag_trst, mux_gpio1_00, mux_gpio1_01, mux_gpio1_02,
	mux_gpio1_03, mux_gpio1_04, mux_gpio1_05, mux_gpio1_06, mux_gpio1_07,
	mux_gpio1_08, mux_gpio1_09, mux_uart1_tx, mux_uart1_rx, mux_uart1_cts,
	mux_uart1_rts, mux_uart2_tx, mux_uart2_rx, mux_uart2_cts, mux_uart2_rts,
	mux_uart3_tx, mux_uart3_rx, mux_uart3_cts, mux_uart3_rts, mux_uart4_tx,
	mux_uart4_rx, mux_uart5_tx, mux_uart5_rx, mux_enet1_rx0, mux_enet1_rx1,
	mux_enet1_rxen, mux_enet1_tx0, mux_enet1_tx1, mux_enet1_txen, mux_enet1_txclk,
	mux_enet1_rxer, mux_enet2_rx0, mux_enet2_rx1, mux_enet2_rxen, mux_enet2_tx0,
	mux_enet2_tx1, mux_enet2_txen, mux_enet2_txclk, mux_enet2_rxer, mux_lcd_clk,
	mux_lcd_en, mux_lcd_hsync, mux_lcd_vsync, mux_lcd_rst, mux_lcd_d0, mux_lcd_d1,
	mux_lcd_d2, mux_lcd_d3, mux_lcd_d4, mux_lcd_d5, mux_lcd_d6, mux_lcd_d7,
	mux_lcd_d8, mux_lcd_d9, mux_lcd_d10, mux_lcd_d11, mux_lcd_d12, mux_lcd_d13,
	mux_lcd_d14, mux_lcd_d15, mux_lcd_d16, mux_lcd_d17, mux_lcd_d18, mux_lcd_d19,
	mux_lcd_d20, mux_lcd_d21, mux_lcd_d22, mux_lcd_d23, mux_nand_re, mux_nand_we,
	mux_nand_d0, mux_nand_d1, mux_nand_d2, mux_nand_d3, mux_nand_d4, mux_nand_d5,
	mux_nand_d6, mux_nand_d7, mux_nand_ale, mux_nand_wp, mux_nand_rdy,
	mux_nand_ce0, mux_nand_ce1, mux_nand_cle, mux_nand_dqs, mux_sd1_cmd,
	mux_sd1_clk, mux_sd1_d0, mux_sd1_d1, mux_sd1_d2, mux_sd1_d3, mux_csi_mclk,
	mux_csi_pclk, mux_csi_vsync, mux_csi_hsync, mux_csi_d0, mux_csi_d1,
	mux_csi_d2, mux_csi_d3, mux_csi_d4, mux_csi_d5, mux_csi_d6, mux_csi_d7
};


/* IOMUX - PAD */
enum {
	pad_test_mode = 0, pad_por, pad_onoff, pad_pmic_on, pad_pmic_stby, pad_boot0,
	pad_boot1, pad_tamper0, pad_tamper1, pad_tamper2, pad_tamper3, pad_tamper4,
	pad_tamper5, pad_tamper6, pad_tamper7, pad_tamper8, pad_tamper9,

	pad_dram_addr0 = 129, pad_dram_addr1, pad_dram_addr2, pad_dram_addr3,
	pad_dram_addr4, pad_dram_addr5, pad_dram_addr6, pad_dram_addr7, pad_dram_addr8,
	pad_dram_addr9, pad_dram_addr10, pad_dram_addr11, pad_dram_addr12, pad_dram_addr13,
	pad_dram_addr14, pad_dram_addr15, pad_dram_dqm0, pad_dram_dqm1, pad_dram_rasb,
	pad_dram_cas_b, pad_dram_cs0_b, pad_dram_cs1_b, pad_dram_sdwe_b, pad_dram_odt0,
	pad_dram_odt1, pad_dram_sdba0, pad_dram_sdba1, pad_dram_sdba2, pad_dram_sdcke0,
	pad_dram_sdcke1, pad_dram_sdclk0p, pad_dram_sdqs0p, pad_dram_sdqs1p, pad_dram_reset,

	pad_jtag_mod = 180, pad_jtag_tms, pad_jtag_tdo, pad_jtag_tdi, pad_jtag_tck,
	pad_jtag_rst, pad_gpio1_00, pad_gpio1_01, pad_gpio1_02, pad_gpio1_03,
	pad_gpio1_04, pad_gpio1_05, pad_gpio1_06, pad_gpio1_07, pad_gpio1_08,
	pad_gpio1_09, pad_uart1_tx, pad_uart1_rx, pad_uart1_cts, pad_uart1_rts,
	pad_uart2_tx, pad_uart2_rx, pad_uart2_cts, pad_uart2_rts, pad_uart3_tx,
	pad_uart3_rx, pad_uart3_cts, pad_uart3_rts, pad_uart4_tx, pad_uart4_rx,
	pad_uart5_tx, pad_uart5_rx, pad_enet1_rx0, pad_enet1_rx1, pad_enet1_rxen,
	pad_enet1_tx0, pad_enet1_tx1, pad_enet1_txen, pad_enet1_txclk, pad_enet1_rxer,
	pad_enet2_rx0, pad_enet2_rx1, pad_enet2_rxen, pad_enet2_tx0, pad_enet2_tx1,
	pad_enet2_txen, pad_enet2_txclk, pad_enet2_rxer, pad_lcd_clk, pad_lcd_en,
	pad_lcd_hsync, pad_lcd_vsync, pad_lcd_rst, pad_lcd_d0, pad_lcd_d1, pad_lcd_d2,
	pad_lcd_d3, pad_lcd_d4, pad_lcd_d5, pad_lcd_d6, pad_lcd_d7, pad_lcd_d8,
	pad_lcd_d9, pad_lcd_d10, pad_lcd_d11, pad_lcd_d12, pad_lcd_d13, pad_lcd_d14,
	pad_lcd_d15, pad_lcd_d16, pad_lcd_d17, pad_lcd_d18, pad_lcd_d19, pad_lcd_d20,
	pad_lcd_d21, pad_lcd_d22, pad_lcd_d23, pad_nand_re, pad_nand_we, pad_nand_d0,
	pad_nand_d1, pad_nand_d2, pad_nand_d3, pad_nand_d4, pad_nand_d5, pad_nand_d6,
	pad_nand_d7, pad_nand_ale, pad_nand_wp, pad_nand_rdy, pad_nand_ce0, pad_nand_ce1,
	pad_nand_cle, pad_nand_dqs, pad_sd1_cmd, pad_sd1_clk, pad_sd1_d0, pad_sd1_d1,
	pad_sd1_d2, pad_sd1_d3, pad_csi_mclk, pad_csi_pclk, pad_csi_vsync, pad_csi_hsync,
	pad_csi_d0, pad_csi_d1, pad_csi_d2, pad_csi_d3, pad_csi_d4, pad_csi_d5,
	pad_csi_d6, pad_csi_d7
};
/* clang-format off */

#define CLK_SEL_QSPI1_PLL3      0
#define CLK_SEL_QSPI1_PLL2_PFD0 1
#define CLK_SEL_QSPI1_PLL2_PFD2 2
#define CLK_SEL_QSPI1_PLL2      3
#define CLK_SEL_QSPI1_PLL3_PFD3 4
#define CLK_SEL_QSPI1_PLL3_PFD2 5


extern void _imx6ull_softRst(void);


extern int imx6ull_setDevClock(int dev, unsigned int state);


extern int imx6ull_setIOisel(int isel, char daisy);


extern int imx6ull_setIOmux(int mux, char sion, char mode);


extern int imx6ull_setIOpad(int pad, char hys, char pus, char pue, char pke, char ode, char speed, char dse, char sre);


extern void imx6ull_setQSPIClockSource(unsigned char source, unsigned char divider);


extern void imx6ull_init(void);


#endif
