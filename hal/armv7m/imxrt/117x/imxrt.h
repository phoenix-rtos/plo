/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * iMXRT basic peripherals control functions
 *
 * Copyright 2017, 2020 Phoenix Systems
 * Author: Aleksander Kaminski, Pawel Pisarczyk, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_IMXRT_H_
#define _HAL_IMXRT_H_


/* iMXRT peripherals */

#include "../types.h"


/* clang-format off */

enum { gpio1 = 0, gpio2, gpio3, gpio4, gpio5, gpio6, gpio7, gpio8, gpio9, gpio10, gpio11, gpio12, gpio13 };


/* CCM - Root Clocks */
enum { pctl_clk_m7 = 0, pctl_clk_m4, pctl_clk_bus, pctl_clk_bus_lpsr, pctl_clk_semc, pctl_clk_cssys,
	pctl_clk_cstrace, pctl_clk_m4_systick, pctl_clk_m7_systick, pctl_clk_adc1, pctl_clk_adc2, pctl_clk_acmp,
	pctl_clk_flexio1, pctl_clk_flexio2, pctl_clk_gpt1, pctl_clk_gpt2, pctl_clk_gpt3, pctl_clk_gpt4, pctl_clk_gpt5,
	pctl_clk_gpt6, pctl_clk_flexspi1, pctl_clk_flexspi2, pctl_clk_can1, pctl_clk_can2, pctl_clk_can3, pctl_clk_lpuart1,
	pctl_clk_lpuart2, pctl_clk_lpuart3, pctl_clk_lpuart4, pctl_clk_lpuart5, pctl_clk_lpuart6, pctl_clk_lpuart7,
	pctl_clk_lpuart8, pctl_clk_lpuart9, pctl_clk_lpuart10, pctl_clk_lpuart11, pctl_clk_lpuart12, pctl_clk_lpi2c1,
	pctl_clk_lpi2c2, pctl_clk_lpi2c3, pctl_clk_lpi2c4, pctl_clk_lpi2c5, pctl_clk_lpi2c6, pctl_clk_lpspi1, pctl_clk_lpspi2,
	pctl_clk_lpspi3, pctl_clk_lpspi4, pctl_clk_lpspi5, pctl_clk_lpspi6, pctl_clk_emv1, pctl_clk_emv2, pctl_clk_enet1,
	pctl_clk_enet2, pctl_clk_enet_qos, pctl_clk_enet_25m, pctl_clk_enet_timer1, pctl_clk_enet_timer2, pctl_clk_enet_timer3,
	pctl_clk_usdhc1, pctl_clk_usdhc2, pctl_clk_asrc, pctl_clk_mqs, pctl_clk_mic, pctl_clk_spdif, pctl_clk_sai1,
	pctl_clk_sai2, pctl_clk_sai3, pctl_clk_sai4, pctl_clk_gpu2d, pctl_clk_lcdif, pctl_clk_lcdifv2, pctl_clk_mipi_ref,
	pctl_clk_mipi_esc, pctl_clk_csi2, pctl_clk_csi2_esc, pctl_clk_csi2_ui, pctl_clk_csi, pctl_clk_clko1,
	pctl_clk_clko2 };


/* CCM - Low Power Clock Gates */
enum { pctl_lpcg_m7 = 0, pctl_lpcg_m4, pctl_lpcg_sim_m7, pctl_lpcg_sim_m, pctl_lpcg_sim_disp, pctl_lpcg_sim_per,
	pctl_lpcg_sim_lpsr, pctl_lpcg_anadig, pctl_lpcg_dcdc, pctl_lpcg_src, pctl_lpcg_ccm, pctl_lpcg_gpc, pctl_lpcg_ssarc,
	pctl_lpcg_sim_r, pctl_lpcg_wdog1, pctl_lpcg_wdog2, pctl_lpcg_wdog3, pctl_lpcg_wdog4, pctl_lpcg_ewm0, pctl_lpcg_sema,
	pctl_lpcg_mu_a, pctl_lpcg_mu_b, pctl_lpcg_edma, pctl_lpcg_edma_lpsr, pctl_lpcg_romcp, pctl_lpcg_ocram,
	pctl_lpcg_flexram, pctl_lpcg_lmem, pctl_lpcg_flexspi1, pctl_lpcg_flexspi2, pctl_lpcg_rdc, pctl_lpcg_m7_xrdc,
	pctl_lpcg_m4_xrdc, pctl_lpcg_semc, pctl_lpcg_xecc, pctl_lpcg_iee, pctl_lpcg_puf, pctl_lpcg_ocotp, pctl_lpcg_snvs_hp,
	pctl_lpcg_snvs, pctl_lpcg_caam, pctl_lpcg_jtag_mux, pctl_lpcg_cstrace, pctl_lpcg_xbar1, pctl_lpcg_xbar2,
	pctl_lpcg_xbar3, pctl_lpcg_aoi1, pctl_lpcg_aoi2, pctl_lpcg_adc_etc, pctl_lpcg_iomuxc, pctl_lpcg_iomuxc_lpsr,
	pctl_lpcg_gpio, pctl_lpcg_kpp, pctl_lpcg_flexio1, pctl_lpcg_flexio2, pctl_lpcg_lpadc1, pctl_lpcg_lpadc2,
	pctl_lpcg_dac, pctl_lpcg_acmp1, pctl_lpcg_acmp2, pctl_lpcg_acmp3, pctl_lpcg_acmp4, pctl_lpcg_pit1, pctl_lpcg_pit2,
	pctl_lpcg_gpt1, pctl_lpcg_gpt2, pctl_lpcg_gpt3, pctl_lpcg_gpt4, pctl_lpcg_gpt5, pctl_lpcg_gpt6, pctl_lpcg_qtimer1,
	pctl_lpcg_qtimer2, pctl_lpcg_qtimer3, pctl_lpcg_qtimer4, pctl_lpcg_enc1, pctl_lpcg_enc2, pctl_lpcg_enc3,
	pctl_lpcg_enc4, pctl_lpcg_hrtimer, pctl_lpcg_pwm1, pctl_lpcg_pwm2, pctl_lpcg_pwm3, pctl_lpcg_pwm4, pctl_lpcg_can1,
	pctl_lpcg_can2, pctl_lpcg_can3, pctl_lpcg_lpuart1, pctl_lpcg_lpuart2, pctl_lpcg_lpuart3, pctl_lpcg_lpuart4,
	pctl_lpcg_lpuart5, pctl_lpcg_lpuart6, pctl_lpcg_lpuart7, pctl_lpcg_lpuart8, pctl_lpcg_lpuart9, pctl_lpcg_lpuart10,
	pctl_lpcg_lpuart11, pctl_lpcg_lpuart12, pctl_lpcg_lpi2c1, pctl_lpcg_lpi2c2, pctl_lpcg_lpi2c3, pctl_lpcg_lpi2c4,
	pctl_lpcg_lpi2c5, pctl_lpcg_lpi2c6, pctl_lpcg_lpspi1, pctl_lpcg_lpspi2, pctl_lpcg_lpspi3, pctl_lpcg_lpspi4,
	pctl_lpcg_lpspi5, pctl_lpcg_lpspi6, pctl_lpcg_sim1, pctl_lpcg_sim2, pctl_lpcg_enet, pctl_lpcg_enet_1g,
	pctl_lpcg_enet_qos, pctl_lpcg_usb, pctl_lpcg_cdog, pctl_lpcg_usdhc1, pctl_lpcg_usdhc2, pctl_lpcg_asrc,
	pctl_lpcg_mqs, pctl_lpcg_pdm, pctl_lpcg_spdif, pctl_lpcg_sai1, pctl_lpcg_sai2, pctl_lpcg_sai3, pctl_lpcg_sai4,
	pctl_lpcg_pxp, pctl_lpcg_gpu2d, pctl_lpcg_lcdif, pctl_lpcg_lcdifv2, pctl_lpcg_mipi_dsi, pctl_lpcg_mipi_csi,
	pctl_lpcg_csi, pctl_lpcg_dcic_mipi, pctl_lpcg_dcic_lcd, pctl_lpcg_video_mux, pctl_lpcg_uniq_edt_i };


/* Peripheral clock low power gate direct control */
enum { clk_state_off = 0, clk_state_run };


/* Peripheral clock run levels in clock domain */
enum { clk_level_off = 0, clk_level_run, clk_level_run_wait, clk_level_run_wait_stop, clk_level_any };


/* IOMUX - MUX */
enum {
	pctl_mux_gpio_emc_b1_00 = 0, pctl_mux_gpio_emc_b1_01, pctl_mux_gpio_emc_b1_02, pctl_mux_gpio_emc_b1_03,
	pctl_mux_gpio_emc_b1_04, pctl_mux_gpio_emc_b1_05, pctl_mux_gpio_emc_b1_06, pctl_mux_gpio_emc_b1_07,
	pctl_mux_gpio_emc_b1_08, pctl_mux_gpio_emc_b1_09, pctl_mux_gpio_emc_b1_10, pctl_mux_gpio_emc_b1_11,
	pctl_mux_gpio_emc_b1_12, pctl_mux_gpio_emc_b1_13, pctl_mux_gpio_emc_b1_14, pctl_mux_gpio_emc_b1_15,
	pctl_mux_gpio_emc_b1_16, pctl_mux_gpio_emc_b1_17, pctl_mux_gpio_emc_b1_18, pctl_mux_gpio_emc_b1_19,
	pctl_mux_gpio_emc_b1_20, pctl_mux_gpio_emc_b1_21, pctl_mux_gpio_emc_b1_22, pctl_mux_gpio_emc_b1_23,
	pctl_mux_gpio_emc_b1_24, pctl_mux_gpio_emc_b1_25, pctl_mux_gpio_emc_b1_26, pctl_mux_gpio_emc_b1_27,
	pctl_mux_gpio_emc_b1_28, pctl_mux_gpio_emc_b1_29, pctl_mux_gpio_emc_b1_30, pctl_mux_gpio_emc_b1_31,
	pctl_mux_gpio_emc_b1_32, pctl_mux_gpio_emc_b1_33, pctl_mux_gpio_emc_b1_34, pctl_mux_gpio_emc_b1_35,
	pctl_mux_gpio_emc_b1_36, pctl_mux_gpio_emc_b1_37, pctl_mux_gpio_emc_b1_38, pctl_mux_gpio_emc_b1_39,
	pctl_mux_gpio_emc_b1_40, pctl_mux_gpio_emc_b1_41, pctl_mux_gpio_emc_b2_00, pctl_mux_gpio_emc_b2_01,
	pctl_mux_gpio_emc_b2_02, pctl_mux_gpio_emc_b2_03, pctl_mux_gpio_emc_b2_04, pctl_mux_gpio_emc_b2_05,
	pctl_mux_gpio_emc_b2_06, pctl_mux_gpio_emc_b2_07, pctl_mux_gpio_emc_b2_08, pctl_mux_gpio_emc_b2_09,
	pctl_mux_gpio_emc_b2_10, pctl_mux_gpio_emc_b2_11, pctl_mux_gpio_emc_b2_12, pctl_mux_gpio_emc_b2_13,
	pctl_mux_gpio_emc_b2_14, pctl_mux_gpio_emc_b2_15, pctl_mux_gpio_emc_b2_16, pctl_mux_gpio_emc_b2_17,
	pctl_mux_gpio_emc_b2_18, pctl_mux_gpio_emc_b2_19, pctl_mux_gpio_emc_b2_20, pctl_mux_gpio_ad_00,
	pctl_mux_gpio_ad_01, pctl_mux_gpio_ad_02, pctl_mux_gpio_ad_03, pctl_mux_gpio_ad_04, pctl_mux_gpio_ad_05,
	pctl_mux_gpio_ad_06, pctl_mux_gpio_ad_07, pctl_mux_gpio_ad_08, pctl_mux_gpio_ad_09, pctl_mux_gpio_ad_10,
	pctl_mux_gpio_ad_11, pctl_mux_gpio_ad_12, pctl_mux_gpio_ad_13, pctl_mux_gpio_ad_14, pctl_mux_gpio_ad_15,
	pctl_mux_gpio_ad_16, pctl_mux_gpio_ad_17, pctl_mux_gpio_ad_18, pctl_mux_gpio_ad_19, pctl_mux_gpio_ad_20,
	pctl_mux_gpio_ad_21, pctl_mux_gpio_ad_22, pctl_mux_gpio_ad_23, pctl_mux_gpio_ad_24, pctl_mux_gpio_ad_25,
	pctl_mux_gpio_ad_26, pctl_mux_gpio_ad_27, pctl_mux_gpio_ad_28, pctl_mux_gpio_ad_29, pctl_mux_gpio_ad_30,
	pctl_mux_gpio_ad_31, pctl_mux_gpio_ad_32, pctl_mux_gpio_ad_33, pctl_mux_gpio_ad_34, pctl_mux_gpio_ad_35,
	pctl_mux_gpio_sd_b1_00, pctl_mux_gpio_sd_b1_01, pctl_mux_gpio_sd_b1_02, pctl_mux_gpio_sd_b1_03,
	pctl_mux_gpio_sd_b1_04, pctl_mux_gpio_sd_b1_05, pctl_mux_gpio_sd_b2_00, pctl_mux_gpio_sd_b2_01,
	pctl_mux_gpio_sd_b2_02, pctl_mux_gpio_sd_b2_03, pctl_mux_gpio_sd_b2_04, pctl_mux_gpio_sd_b2_05,
	pctl_mux_gpio_sd_b2_06, pctl_mux_gpio_sd_b2_07, pctl_mux_gpio_sd_b2_08, pctl_mux_gpio_sd_b2_09,
	pctl_mux_gpio_sd_b2_10, pctl_mux_gpio_sd_b2_11, pctl_mux_gpio_disp_b1_00, pctl_mux_gpio_disp_b1_01,
	pctl_mux_gpio_disp_b1_02, pctl_mux_gpio_disp_b1_03, pctl_mux_gpio_disp_b1_04, pctl_mux_gpio_disp_b1_05,
	pctl_mux_gpio_disp_b1_06, pctl_mux_gpio_disp_b1_07, pctl_mux_gpio_disp_b1_08, pctl_mux_gpio_disp_b1_09,
	pctl_mux_gpio_disp_b1_10, pctl_mux_gpio_disp_b1_11, pctl_mux_gpio_disp_b2_00, pctl_mux_gpio_disp_b2_01,
	pctl_mux_gpio_disp_b2_02, pctl_mux_gpio_disp_b2_03, pctl_mux_gpio_disp_b2_04, pctl_mux_gpio_disp_b2_05,
	pctl_mux_gpio_disp_b2_06, pctl_mux_gpio_disp_b2_07, pctl_mux_gpio_disp_b2_08, pctl_mux_gpio_disp_b2_09,
	pctl_mux_gpio_disp_b2_10, pctl_mux_gpio_disp_b2_11, pctl_mux_gpio_disp_b2_12, pctl_mux_gpio_disp_b2_13,
	pctl_mux_gpio_disp_b2_14, pctl_mux_gpio_disp_b2_15,

	/* SNVS */
	pctl_mux_wakeup, pctl_mux_pmic_on_req, pctl_mux_pmic_stby_req, pctl_mux_gpio_snvs_00, pctl_mux_gpio_snvs_01,
	pctl_mux_gpio_snvs_02, pctl_mux_gpio_snvs_03, pctl_mux_gpio_snvs_04, pctl_mux_gpio_snvs_05, pctl_mux_gpio_snvs_06,
	pctl_mux_gpio_snvs_07, pctl_mux_gpio_snvs_08, pctl_mux_gpio_snvs_09,

	/* LPSR */
	pctl_mux_gpio_lpsr_00, pctl_mux_gpio_lpsr_01, pctl_mux_gpio_lpsr_02, pctl_mux_gpio_lpsr_03, pctl_mux_gpio_lpsr_04,
	pctl_mux_gpio_lpsr_05, pctl_mux_gpio_lpsr_06, pctl_mux_gpio_lpsr_07, pctl_mux_gpio_lpsr_08, pctl_mux_gpio_lpsr_09,
	pctl_mux_gpio_lpsr_10, pctl_mux_gpio_lpsr_11, pctl_mux_gpio_lpsr_12, pctl_mux_gpio_lpsr_13, pctl_mux_gpio_lpsr_14,
	pctl_mux_gpio_lpsr_15
};


/* IOMUX - PAD */
enum {
	pctl_pad_gpio_emc_b1_00 = 0, pctl_pad_gpio_emc_b1_01, pctl_pad_gpio_emc_b1_02, pctl_pad_gpio_emc_b1_03,
	pctl_pad_gpio_emc_b1_04, pctl_pad_gpio_emc_b1_05, pctl_pad_gpio_emc_b1_06, pctl_pad_gpio_emc_b1_07,
	pctl_pad_gpio_emc_b1_08, pctl_pad_gpio_emc_b1_09, pctl_pad_gpio_emc_b1_10, pctl_pad_gpio_emc_b1_11,
	pctl_pad_gpio_emc_b1_12, pctl_pad_gpio_emc_b1_13, pctl_pad_gpio_emc_b1_14, pctl_pad_gpio_emc_b1_15,
	pctl_pad_gpio_emc_b1_16, pctl_pad_gpio_emc_b1_17, pctl_pad_gpio_emc_b1_18, pctl_pad_gpio_emc_b1_19,
	pctl_pad_gpio_emc_b1_20, pctl_pad_gpio_emc_b1_21, pctl_pad_gpio_emc_b1_22, pctl_pad_gpio_emc_b1_23,
	pctl_pad_gpio_emc_b1_24, pctl_pad_gpio_emc_b1_25, pctl_pad_gpio_emc_b1_26, pctl_pad_gpio_emc_b1_27,
	pctl_pad_gpio_emc_b1_28, pctl_pad_gpio_emc_b1_29, pctl_pad_gpio_emc_b1_30, pctl_pad_gpio_emc_b1_31,
	pctl_pad_gpio_emc_b1_32, pctl_pad_gpio_emc_b1_33, pctl_pad_gpio_emc_b1_34, pctl_pad_gpio_emc_b1_35,
	pctl_pad_gpio_emc_b1_36, pctl_pad_gpio_emc_b1_37, pctl_pad_gpio_emc_b1_38, pctl_pad_gpio_emc_b1_39,
	pctl_pad_gpio_emc_b1_40, pctl_pad_gpio_emc_b1_41, pctl_pad_gpio_emc_b2_00, pctl_pad_gpio_emc_b2_01,
	pctl_pad_gpio_emc_b2_02, pctl_pad_gpio_emc_b2_03, pctl_pad_gpio_emc_b2_04, pctl_pad_gpio_emc_b2_05,
	pctl_pad_gpio_emc_b2_06, pctl_pad_gpio_emc_b2_07, pctl_pad_gpio_emc_b2_08, pctl_pad_gpio_emc_b2_09,
	pctl_pad_gpio_emc_b2_10, pctl_pad_gpio_emc_b2_11, pctl_pad_gpio_emc_b2_12, pctl_pad_gpio_emc_b2_13,
	pctl_pad_gpio_emc_b2_14, pctl_pad_gpio_emc_b2_15, pctl_pad_gpio_emc_b2_16, pctl_pad_gpio_emc_b2_17,
	pctl_pad_gpio_emc_b2_18, pctl_pad_gpio_emc_b2_19, pctl_pad_gpio_emc_b2_20, pctl_pad_gpio_ad_00,
	pctl_pad_gpio_ad_01, pctl_pad_gpio_ad_02, pctl_pad_gpio_ad_03, pctl_pad_gpio_ad_04, pctl_pad_gpio_ad_05,
	pctl_pad_gpio_ad_06, pctl_pad_gpio_ad_07, pctl_pad_gpio_ad_08, pctl_pad_gpio_ad_09, pctl_pad_gpio_ad_10,
	pctl_pad_gpio_ad_11, pctl_pad_gpio_ad_12, pctl_pad_gpio_ad_13, pctl_pad_gpio_ad_14, pctl_pad_gpio_ad_15,
	pctl_pad_gpio_ad_16, pctl_pad_gpio_ad_17, pctl_pad_gpio_ad_18, pctl_pad_gpio_ad_19, pctl_pad_gpio_ad_20,
	pctl_pad_gpio_ad_21, pctl_pad_gpio_ad_22, pctl_pad_gpio_ad_23, pctl_pad_gpio_ad_24, pctl_pad_gpio_ad_25,
	pctl_pad_gpio_ad_26, pctl_pad_gpio_ad_27, pctl_pad_gpio_ad_28, pctl_pad_gpio_ad_29, pctl_pad_gpio_ad_30,
	pctl_pad_gpio_ad_31, pctl_pad_gpio_ad_32, pctl_pad_gpio_ad_33, pctl_pad_gpio_ad_34, pctl_pad_gpio_ad_35,
	pctl_pad_gpio_sd_b1_00, pctl_pad_gpio_sd_b1_01, pctl_pad_gpio_sd_b1_02, pctl_pad_gpio_sd_b1_03,
	pctl_pad_gpio_sd_b1_04, pctl_pad_gpio_sd_b1_05, pctl_pad_gpio_sd_b2_00, pctl_pad_gpio_sd_b2_01,
	pctl_pad_gpio_sd_b2_02, pctl_pad_gpio_sd_b2_03, pctl_pad_gpio_sd_b2_04, pctl_pad_gpio_sd_b2_05,
	pctl_pad_gpio_sd_b2_06, pctl_pad_gpio_sd_b2_07, pctl_pad_gpio_sd_b2_08, pctl_pad_gpio_sd_b2_09,
	pctl_pad_gpio_sd_b2_10, pctl_pad_gpio_sd_b2_11, pctl_pad_gpio_disp_b1_00, pctl_pad_gpio_disp_b1_01,
	pctl_pad_gpio_disp_b1_02, pctl_pad_gpio_disp_b1_03, pctl_pad_gpio_disp_b1_04, pctl_pad_gpio_disp_b1_05,
	pctl_pad_gpio_disp_b1_06, pctl_pad_gpio_disp_b1_07, pctl_pad_gpio_disp_b1_08, pctl_pad_gpio_disp_b1_09,
	pctl_pad_gpio_disp_b1_10, pctl_pad_gpio_disp_b1_11, pctl_pad_gpio_disp_b2_00, pctl_pad_gpio_disp_b2_01,
	pctl_pad_gpio_disp_b2_02, pctl_pad_gpio_disp_b2_03, pctl_pad_gpio_disp_b2_04, pctl_pad_gpio_disp_b2_05,
	pctl_pad_gpio_disp_b2_06, pctl_pad_gpio_disp_b2_07, pctl_pad_gpio_disp_b2_08, pctl_pad_gpio_disp_b2_09,
	pctl_pad_gpio_disp_b2_10, pctl_pad_gpio_disp_b2_11, pctl_pad_gpio_disp_b2_12, pctl_pad_gpio_disp_b2_13,
	pctl_pad_gpio_disp_b2_14, pctl_pad_gpio_disp_b2_15,

	/* SNVS */
	pctl_pad_test_mode, pctl_pad_por_b, pctl_pad_onoff, pctl_pad_wakeup, pctl_pad_pmic_on_req, pctl_pad_pmic_stby_req,
	pctl_pad_gpio_snvs_00, pctl_pad_gpio_snvs_01, pctl_pad_gpio_snvs_02, pctl_pad_gpio_snvs_03, pctl_pad_gpio_snvs_04,
	pctl_pad_gpio_snvs_05, pctl_pad_gpio_snvs_06, pctl_pad_gpio_snvs_07, pctl_pad_gpio_snvs_08, pctl_pad_gpio_snvs_09,

	/* LPSR */
	pctl_pad_gpio_lpsr_00, pctl_pad_gpio_lpsr_01, pctl_pad_gpio_lpsr_02, pctl_pad_gpio_lpsr_03, pctl_pad_gpio_lpsr_04,
	pctl_pad_gpio_lpsr_05, pctl_pad_gpio_lpsr_06, pctl_pad_gpio_lpsr_07, pctl_pad_gpio_lpsr_08, pctl_pad_gpio_lpsr_09,
	pctl_pad_gpio_lpsr_10, pctl_pad_gpio_lpsr_11, pctl_pad_gpio_lpsr_12, pctl_pad_gpio_lpsr_13, pctl_pad_gpio_lpsr_14,
	pctl_pad_gpio_lpsr_15
};


/* IOMUX - DAISY */
enum {
	pctl_isel_flexcan1_rx = 0, pctl_isel_flexcan2_rx,

	pctl_isel_ccm_enet_qos_ref_clk, pctl_isel_ccm_enet_qos_tx_clk, pctl_isel_enet_ipg_clk_rmii,
	pctl_isel_enet_mac0_mdio, pctl_isel_enet_mac0_rxdata_0, pctl_isel_enet_mac0_rxdata_1, pctl_isel_enet_mac0_rxen,
	pctl_isel_enet_mac0_rxerr, pctl_isel_enet_mac0_txclk,

	pctl_isel_enet_1g_ipg_clk_rmii, pctl_isel_enet_1g_mac0_mdio, pctl_isel_enet_1g_mac0_rxclk,
	pctl_isel_enet_1g_mac0_rxdata_0, pctl_isel_enet_1g_mac0_rxdata_1, pctl_isel_enet_1g_mac0_rxdata_2,
	pctl_isel_enet_1g_mac0_rxdata_3, pctl_isel_enet_1g_mac0_rxen, pctl_isel_enet_1g_mac0_rxerr,
	pctl_isel_enet_1g_mac0_txclk,

	pctl_isel_enet_qos_gmii_mdi, pctl_isel_enet_qos_phy_rxd_0, pctl_isel_enet_qos_phy_rxd_1,
	pctl_isel_enet_qos_phy_rxdv, enet_qos_phy_rxer,

	pctl_isel_flexpwm1_pwma_0, pctl_isel_flexpwm1_pwma_1, pctl_isel_flexpwm1_pwma_2, pctl_isel_flexpwm1_pwmb_0,
	pctl_isel_flexpwm1_pwmb_1, pctl_isel_flexpwm1_pwmb_2,

	pctl_isel_flexpwm2_pwma_0, pctl_isel_flexpwm2_pwma_1, pctl_isel_flexpwm2_pwma_2, pctl_isel_flexpwm2_pwmb_0,
	pctl_isel_flexpwm2_pwmb_1, pctl_isel_flexpwm2_pwmb_2,

	pctl_isel_flexpwm3_pwma_0,pctl_isel_flexpwm3_pwma_1, pctl_isel_flexpwm3_pwma_2, pctl_isel_flexpwm3_pwma_3,
	pctl_isel_flexpwm3_pwmb_0, pctl_isel_flexpwm3_pwmb_1, pctl_isel_flexpwm3_pwmb_2, pctl_isel_flexpwm3_pwmb_3,

	pctl_isel_flexspi1_dqs_fa, pctl_isel_flexspi1_fa_0, pctl_isel_flexspi1_fa_1, pctl_isel_flexspi1_fa_2,
	pctl_isel_flexspi1_fa_3, pctl_isel_flexspi1_fb_0, pctl_isel_flexspi1_fb_1, pctl_isel_flexspi1_fb_2,
	pctl_isel_flexspi1_fb_3, pctl_isel_flexspi1_sck_fa, pctl_isel_flexspi1_sck_fb,

	pctl_isel_flexspi2_fa_0, pctl_isel_flexspi2_fa_1, pctl_isel_flexspi2_fa_2, pctl_isel_flexspi2_fa_3,
	pctl_isel_flexspi2_sck_fa,

	pctl_isel_gpt3_capin1, pctl_isel_gpt3_capin2, pctl_isel_gpt3_clkin,

	pctl_isel_kpp_col_6, pctl_isel_kpp_col_7, pctl_isel_kpp_row_6, pctl_isel_kpp_row_7,

	pctl_isel_lpi2c1_scl, pctl_isel_lpi2c1_sda,

	pctl_isel_lpi2c2_scl, pctl_isel_lpi2c2_sda,

	pctl_isel_lpi2c3_scl, pctl_isel_lpi2c3_sda,

	pctl_isel_lpi2c4_scl, pctl_isel_lpi2c4_sda,

	pctl_isel_lpspi1_pcs_0, pctl_isel_lpspi1_sck, pctl_isel_lpspi1_sdi, pctl_isel_lpspi1_sdo,

	pctl_isel_lpspi2_pcs_0, pctl_isel_lpspi2_pcs_1, pctl_isel_lpspi2_sck, pctl_isel_lpspi2_sdi, pctl_isel_lpspi2_sdo,

	pctl_isel_lpspi3_pcs_0, pctl_isel_lpspi3_pcs_1, pctl_isel_lpspi3_pcs_2, pctl_isel_lpspi3_pcs_3,
	pctl_isel_lpspi3_sck, pctl_isel_lpspi3_sdi, pctl_isel_lpspi3_sdo,

	pctl_isel_lpspi4_pcs_0, pctl_isel_lpspi4_sck, pctl_isel_lpspi4_sdi, pctl_isel_lpspi4_sdo,

	pctl_isel_lpuart1_rxd, pctl_isel_lpuart1_txd,

	pctl_isel_lpuart10_rxd, pctl_isel_lpuart10_txd,

	pctl_isel_lpuart7_rxd, pctl_isel_lpuart7_txd,

	pctl_isel_lpuart8_rxd, pctl_isel_lpuart8_txd,

	pctl_isel_qtimer1_tmr0, pctl_isel_qtimer1_tmr1, pctl_isel_qtimer1_tmr2,

	pctl_isel_qtimer2_tmr0, pctl_isel_qtimer2_tmr1, pctl_isel_qtimer2_tmr2,

	pctl_isel_qtimer3_tmr0, pctl_isel_qtimer3_tmr1, pctl_isel_qtimer3_tmr2,

	pctl_isel_qtimer4_tmr0, pctl_isel_qtimer4_tmr1, pctl_isel_qtimer4_tmr2,

	pctl_isel_sai1_mclk, pctl_isel_sai1_rxbclk, pctl_isel_sai1_rxdata_0, pctl_isel_sai1_rxsync,
	pctl_isel_sai1_txbclk, pctl_isel_sai1_txsync,

	pctl_isel_sdio_slv_clk_sd, pctl_isel_sdio_slv_cmd_di, pctl_isel_sdio_slv_dat0_do, pctl_isel_slv_dat1_irq,
	pctl_isel_sdio_slv_dat2_rw, pctl_isel_sdio_slv_dat3_cs,

	pctl_isel_emvsim1_sio, pctl_isel_emvsim1_ipp_simpd, pctl_isel_emvsim1_power_fail,

	pctl_isel_emvsim2_sio, pctl_isel_emvsim2_ipp_simpd, pctl_isel_emvsim2_power_fail,

	pctl_isel_spdif_in1,

	pctl_isel_usb_otg2_oc, pctl_isel_usb_otg_oc,

	pctl_isel_usbphy1_id, pctl_isel_usbphy2_id,

	pctl_isel_usdhc1_ipp_card_det, pctl_isel_usdhc1_ipp_wp_on, pctl_isel_usdhc2_ipp_card_det, pctl_isel_usdhc_ipp_wp_on,

	pctl_isel_xbar1_in_20, pctl_isel_xbar1_in_21, pctl_isel_xbar1_in_22, pctl_isel_xbar1_in_23,
	pctl_isel_xbar1_in_24, pctl_isel_xbar1_in_25, pctl_isel_xbar1_in_26, pctl_isel_xbar1_in_27,
	pctl_isel_xbar1_in_28, pctl_isel_xbar1_in_29, pctl_isel_xbar1_in_30, pctl_isel_xbar1_in_31,
	pctl_isel_xbar1_in_32, pctl_isel_xbar1_in_33, pctl_isel_xbar1_in_34, pctl_isel_xbar1_in_35,

	/* LPSR */
	pctl_isel_can3_canrx,

	pctl_isel_lpi2c5_scl, pctl_isel_lpi2c5_sda,

	pctl_isel_lpi2c6_scl, pctl_isel_lpi2c6_sda,

	pctl_isel_lpspi5_pcs_0, pctl_isel_lpspi5_sck, pctl_isel_lpspi5_sdi, pctl_isel_lpspi5_sdo,

	pctl_isel_lpuart11_rxd, pctl_isel_lpuart11_txd,

	pctl_isel_lpuart12_rxd, pctl_isel_lpuart12_txd,

	pctl_isel_mic_pdm_bitstream_0, pctl_isel_mic_pdm_bitstream_1, pctl_isel_mic_pdm_bitstream_2,
	pctl_isel_mic_pdm_bitstream_3,

	pctl_isel_nmi,

	pctl_isel_sai4_mclk, pctl_isel_sai4_rxbclk, pctl_isel_sai4_rxdata_0, pctl_isel_sai4_rxsync, pctl_isel_sai4_txbclk,
	pctl_isel_sai4_txsync
};


/* Interrupts numbers */
enum { cti0_err_irq = 17 + 16, cti1_err_irq, core_irq, lpuart1_irq, lpuart2_irq, lpuart3_irq, lpuart4_irq, lpuart5_irq,
	lpuart6_irq, lpuart7_irq, lpuart8_irq, lpuart9_irq, lpuart10_irq, lpuart11_irq, lpuart12_irq, lpi2c1_irq,
	lpi2c2_irq, lpi2c3_irq, lpi2c4_irq, lpi2c5_irq, lpi2c6_irq, lpspi1_irq, lpspi2_irq, lpspi3_irq, lpspi4_irq,
	lpspi5_irq, lpspi6_irq, can1_irq, can2_irq, can3_irq, flexram_irq, kpp_irq, tsc_dig_irq, /* Reserved #53 */
	lcdif1_irq = 54 + 16, lcdif2_irq, csi_irq, pxp_irq, mipi_csi_irq, mipi_dsi_irq, gpu2d_irq, gpio6_int0_irq,
	gpio6_int1_irq, /* Reserved #63..64 */ wdog2_irq = 65 + 16, snvs_hp_wrapper_irq, snvs_hp_wrapper_tz_irq,
	snvs_lp_wrapper_irq, caam_jq0_irq, caam_jq1_irq, caam_jq2_irq, caam_jq3_irq, caam_err_irq, caam_rtic_irq,
	/* Reserved #75 */ sai1_irq = 76 + 16, sai2_irq, sai3_0_irq, sai3_1_irq, sai4_0_irq, sai4_1_irq, spdif_irq,
	/* Reserved #83..87 */ adc1_irq = 88 + 16, adc2_irq, adc3_irq, dcdc_irq, /* Reserved #92..98 */ cm7_irq = 99 + 16,
	gpio1_int0_irq, gpio1_int1_irq, gpio2_int0_irq, gpio2_int1_irq, gpio3_int0_irq, gpio3_int1_irq, gpio4_int0_irq,
	gpio4_int1_irq, gpio5_int0_irq, gpio5_int1_irq, flexio1_irq, flexio2_irq, wdog1_irq, rtwdog_irq, ewm_irq, ccm_1_irq,
	ccm2_irq, gpc_irq, /* Reserved #118 */ gpt1_irq = 119 + 16, gpt2_irq, gpt3_irq, gpt4_irq, gpt5_irq, gpt6_irq,
	flexpwm1_0_irq, flexpwm1_1_irq, flexpwm1_2_irq, flexpwm1_3_irq, flexpwm1_err_irq, flexspi1_irq, flexspi2_irq,
	semc_irq, usdhc1_irq, usdhc2_irq, usb_otg2_irq, usb_otg1_irq, enet_irq, enet_1588_timer_irq, enet_1g_rxtx_irq,
	enet_1g_rxtxdone_irq, enet_1g_irq, enet_1g_1588_timer_irq, xbar1_0_irq, xbar1_1_irq, adc_etc1_0_irq, adc_etc1_1_irq,
	adc_etc1_2_irq, adc_etc1_3_irq, adc_etc1_err_irq, adc_etc2_0_irq, adc_etc2_1_irq, adc_etc2_2_irq, adc_etc2_3_irq,
	adc_etc2_err_irq, pit1_irq, pit2_irq, acmp1_irq, acmp2_irq, acmp3_irq, acmp4_irq, acmp5_irq, acmp6_irq, acmp7_irq,
	acmp_lpsr, enc1_irq, enc2_irq, enc3_irq, enc4_irq, enc5_irq, enc6_irq, qtimer1_irq, qtimer2_irq, qtimer3_irq,
	qtimer4_irq, qtimer5_irq, qtimer6_irq, flexpwm2_0_irq, flexpwm2_1_irq, flexpwm2_2_irq, flexpwm2_3_irq,
	flexpwm2_err_irq, flexpwm3_0_irq, flexpwm3_1_irq, flexpwm3_2_irq, flexpwm3_3_irq, flexpwm3_err_irq,flexpwm4_0_irq,
	flexpwm4_1_irq, flexpwm4_2_irq, flexpwm4_3_irq, flexpwm4_err_irq, flexpwm5_0_irq, flexpwm5_1_irq, flexpwm5_2_irq,
	flexpwm5_3_irq, flexpwm5_err_irq, flexpwm6_0_irq, flexpwm6_1_irq, flexpwm6_2_irq, flexpwm6_3_irq, flexpwm6_err_irq,
	mic_irq, mic_err_irq, sim1_irq, sim2_irq, mecc1_irq, mecc1_fatal_irq, mecc2_irq, mecc2_fatal_irq, xecc_flexspi1_irq,
	xecc_flexspi1_fatal_irq, xecc_flexspi2_irq, xecc_flexspi2_fatal_irq, xecc_semc_irq, xecc_semc_fatal_irq, enet_qos_irq,
	enet_pmt_irq };


/* Mux selector: M7 */
enum { mux_clkroot_m7_oscrc48mdiv2 = 0, mux_clkroot_m7_osc24mout, mux_clkroot_m7_oscrc400m, mux_clkroot_m7_oscrc16m,
	mux_clkroot_m7_armpllout, mux_clkroot_m7_syspll1out, mux_clkroot_m7_syspll3out, mux_clkroot_m7_videopllout };

/* Mux selector: M4 */
enum { mux_clkroot_m4_oscrc48mdiv2 = 0, mux_clkroot_m4_osc24mout, mux_clkroot_m4_oscrc400m, mux_clkroot_m4_oscrc16m,
	mux_clkroot_m4_syspll3pfd3, mux_clkroot_m4_syspll3out, mux_clkroot_m4_syspll2out, mux_clkroot_m4_syspll1div5 };

/* Mux selector: BUS */
enum { mux_clkroot_bus_oscrc48mdiv2 = 0, mux_clkroot_bus_osc24mout, mux_clkroot_bus_oscrc400m, mux_clkroot_bus_oscrc16m,
	mux_clkroot_bus_syspll3out, mux_clkroot_bus_syspll1div5, mux_clkroot_bus_syspll2out, mux_clkroot_bus_syspll2pfd3 };

/* Mux selector: BUS_LPSR */
enum { mux_clkroot_bus_lpsr_oscrc48mdiv2 = 0, mux_clkroot_bus_lpsr_osc24mout, mux_clkroot_bus_lpsr_oscrc400m, mux_clkroot_bus_lpsr_oscrc16m,
	mux_clkroot_bus_lpsr_syspll3pfd3, mux_clkroot_bus_lpsr_syspll3out, mux_clkroot_bus_lpsr_syspll2out, mux_clkroot_bus_lpsr_syspll1div5 };

/* Mux selector: SEMC */
enum { mux_clkroot_semc_oscrc48mdiv2 = 0, mux_clkroot_semc_osc24mout, mux_clkroot_semc_oscrc400m, mux_clkroot_semc_oscrc16m,
	mux_clkroot_semc_syspll1div5, mux_clkroot_semc_syspll2out, mux_clkroot_semc_syspll2pfd1, mux_clkroot_semc_syspll3pfd0 };

/* Mux selector: CSSYS */
enum { mux_clkroot_cssys_oscrc48mdiv2 = 0, mux_clkroot_cssys_osc24mout, mux_clkroot_cssys_oscrc400m, mux_clkroot_cssys_oscrc16m,
	mux_clkroot_cssys_syspll3div2, mux_clkroot_cssys_syspll1div5, mux_clkroot_cssys_syspll2out, mux_clkroot_cssys_syspll2pfd3 };

/* Mux selector: CSTRACE */
enum { mux_clkroot_cstrace_oscrc48mdiv2 = 0, mux_clkroot_cstrace_osc24mout, mux_clkroot_cstrace_oscrc400m, mux_clkroot_cstrace_oscrc16m,
	mux_clkroot_cstrace_syspll3div2, mux_clkroot_cstrace_syspll1div5, mux_clkroot_cstrace_syspll2pfd1, mux_clkroot_cstrace_syspll2out };

/* Mux selector: M4_SYSTICK */
enum { mux_clkroot_m4_systick_oscrc48mdiv2 = 0, mux_clkroot_m4_systick_osc24mout, mux_clkroot_m4_systick_oscrc400m, mux_clkroot_m4_systick_oscrc16m,
	mux_clkroot_m4_systick_syspll3pfd3, mux_clkroot_m4_systick_syspll3out, mux_clkroot_m4_systick_syspll2pfd0, mux_clkroot_m4_systick_syspll1div5 };

/* Mux selector: M7_SYSTICK */
enum { mux_clkroot_m7_systick_oscrc48mdiv2 = 0, mux_clkroot_m7_systick_osc24mout, mux_clkroot_m7_systick_oscrc400m, mux_clkroot_m7_systick_oscrc16m,
	mux_clkroot_m7_systick_syspll2out, mux_clkroot_m7_systick_syspll3div2, mux_clkroot_m7_systick_syspll1div5, mux_clkroot_m7_systick_syspll2pfd0 };

/* Mux selector: ADC1 */
enum { mux_clkroot_adc1_oscrc48mdiv2 = 0, mux_clkroot_adc1_osc24mout, mux_clkroot_adc1_oscrc400m, mux_clkroot_adc1_oscrc16m,
	mux_clkroot_adc1_syspll3div2, mux_clkroot_adc1_syspll1div5, mux_clkroot_adc1_syspll2out, mux_clkroot_adc1_syspll2pfd3 };

/* Mux selector: ADC2 */
enum { mux_clkroot_adc2_oscrc48mdiv2 = 0, mux_clkroot_adc2_osc24mout, mux_clkroot_adc2_oscrc400m, mux_clkroot_adc2_oscrc16m,
	mux_clkroot_adc2_syspll3div2, mux_clkroot_adc2_syspll1div5, mux_clkroot_adc2_syspll2out, mux_clkroot_adc2_syspll2pfd3 };

/* Mux selector: ACMP */
enum { mux_clkroot_acmp_oscrc48mdiv2 = 0, mux_clkroot_acmp_osc24mout, mux_clkroot_acmp_oscrc400m, mux_clkroot_acmp_oscrc16m,
	mux_clkroot_acmp_syspll3out, mux_clkroot_acmp_syspll1div5, mux_clkroot_acmp_audiopllout, mux_clkroot_acmp_syspll2pfd3 };

/* Mux selector: FLEXIO1 */
enum { mux_clkroot_flexio1_oscrc48mdiv2 = 0, mux_clkroot_flexio1_osc24mout, mux_clkroot_flexio1_oscrc400m, mux_clkroot_flexio1_oscrc16m,
	mux_clkroot_flexio1_syspll3div2, mux_clkroot_flexio1_syspll1div5, mux_clkroot_flexio1_syspll2out, mux_clkroot_flexio1_syspll2pfd3 };

/* Mux selector: FLEXIO2 */
enum { mux_clkroot_flexio2_oscrc48mdiv2 = 0, mux_clkroot_flexio2_osc24mout, mux_clkroot_flexio2_oscrc400m, mux_clkroot_flexio2_oscrc16m,
	mux_clkroot_flexio2_syspll3div2, mux_clkroot_flexio2_syspll1div5, mux_clkroot_flexio2_syspll2out, mux_clkroot_flexio2_syspll2pfd3 };

/* Mux selector: GPT1 */
enum { mux_clkroot_gpt1_oscrc48mdiv2 = 0, mux_clkroot_gpt1_osc24mout, mux_clkroot_gpt1_oscrc400m, mux_clkroot_gpt1_oscrc16m,
	mux_clkroot_gpt1_syspll3div2, mux_clkroot_gpt1_syspll1div5, mux_clkroot_gpt1_syspll3pfd2, mux_clkroot_gpt1_syspll3pfd3 };

/* Mux selector: GPT2 */
enum { mux_clkroot_gpt2_oscrc48mdiv2 = 0, mux_clkroot_gpt2_osc24mout, mux_clkroot_gpt2_oscrc400m, mux_clkroot_gpt2_oscrc16m,
	mux_clkroot_gpt2_syspll3div2, mux_clkroot_gpt2_syspll1div5, mux_clkroot_gpt2_audiopllout, mux_clkroot_gpt2_videopllout };

/* Mux selector: GPT3 */
enum { mux_clkroot_gpt3_oscrc48mdiv2 = 0, mux_clkroot_gpt3_osc24mout, mux_clkroot_gpt3_oscrc400m, mux_clkroot_gpt3_oscrc16m,
	mux_clkroot_gpt3_syspll3div2, mux_clkroot_gpt3_syspll1div5, mux_clkroot_gpt3_audiopllout, mux_clkroot_gpt3_videopllout };

/* Mux selector: GPT4 */
enum { mux_clkroot_gpt4_oscrc48mdiv2 = 0, mux_clkroot_gpt4_osc24mout, mux_clkroot_gpt4_oscrc400m, mux_clkroot_gpt4_oscrc16m,
	mux_clkroot_gpt4_syspll3div2, mux_clkroot_gpt4_syspll1div5, mux_clkroot_gpt4_syspll3pfd2, mux_clkroot_gpt4_syspll3pfd3 };

/* Mux selector: GPT5 */
enum { mux_clkroot_gpt5_oscrc48mdiv2 = 0, mux_clkroot_gpt5_osc24mout, mux_clkroot_gpt5_oscrc400m, mux_clkroot_gpt5_oscrc16m,
	mux_clkroot_gpt5_syspll3div2, mux_clkroot_gpt5_syspll1div5, mux_clkroot_gpt5_syspll3pfd2, mux_clkroot_gpt5_syspll3pfd3 };

/* Mux selector: GPT6 */
enum { mux_clkroot_gpt6_oscrc48mdiv2 = 0, mux_clkroot_gpt6_osc24mout, mux_clkroot_gpt6_oscrc400m, mux_clkroot_gpt6_oscrc16m,
	mux_clkroot_gpt6_syspll3div2, mux_clkroot_gpt6_syspll1div5, mux_clkroot_gpt6_syspll3pfd2, mux_clkroot_gpt6_syspll3pfd3 };

/* Mux selector: FLEXSPI1 */
enum { mux_clkroot_flexspi1_oscrc48mdiv2 = 0, mux_clkroot_flexspi1_osc24mout, mux_clkroot_flexspi1_oscrc400m, mux_clkroot_flexspi1_oscrc16m,
	mux_clkroot_flexspi1_syspll3pfd0, mux_clkroot_flexspi1_syspll2out, mux_clkroot_flexspi1_syspll2pfd2, mux_clkroot_flexspi1_syspll3out };

/* Mux selector: FLEXSPI2 */
enum { mux_clkroot_flexspi2_oscrc48mdiv2 = 0, mux_clkroot_flexspi2_osc24mout, mux_clkroot_flexspi2_oscrc400m, mux_clkroot_flexspi2_oscrc16m,
	mux_clkroot_flexspi2_syspll3pfd0, mux_clkroot_flexspi2_syspll2out, mux_clkroot_flexspi2_syspll2pfd2, mux_clkroot_flexspi2_syspll3out };

/* Mux selector: CAN1 */
enum { mux_clkroot_can1_oscrc48mdiv2 = 0, mux_clkroot_can1_osc24mout, mux_clkroot_can1_oscrc400m, mux_clkroot_can1_oscrc16m,
	mux_clkroot_can1_syspll3div2, mux_clkroot_can1_syspll1div5, mux_clkroot_can1_syspll2out, mux_clkroot_can1_syspll2pfd3 };

/* Mux selector: CAN2 */
enum { mux_clkroot_can2_oscrc48mdiv2 = 0, mux_clkroot_can2_osc24mout, mux_clkroot_can2_oscrc400m, mux_clkroot_can2_oscrc16m,
	mux_clkroot_can2_syspll3div2, mux_clkroot_can2_syspll1div5, mux_clkroot_can2_syspll2out, mux_clkroot_can2_syspll2pfd3 };

/* Mux selector: CAN3 */
enum { mux_clkroot_can3_oscrc48mdiv2 = 0, mux_clkroot_can3_osc24mout, mux_clkroot_can3_oscrc400m, mux_clkroot_can3_oscrc16m,
	mux_clkroot_can3_syspll3pfd3, mux_clkroot_can3_syspll3out, mux_clkroot_can3_syspll2pfd3, mux_clkroot_can3_syspll1div5 };

/* Mux selector: LPUART1 */
enum { mux_clkroot_lpuart1_oscrc48mdiv2 = 0, mux_clkroot_lpuart1_osc24mout, mux_clkroot_lpuart1_oscrc400m, mux_clkroot_lpuart1_oscrc16m,
	mux_clkroot_lpuart1_syspll3div2, mux_clkroot_lpuart1_syspll1div5, mux_clkroot_lpuart1_syspll2out, mux_clkroot_lpuart1_syspll2pfd3 };

/* Mux selector: LPUART2 */
enum { mux_clkroot_lpuart2_oscrc48mdiv2 = 0, mux_clkroot_lpuart2_osc24mout, mux_clkroot_lpuart2_oscrc400m, mux_clkroot_lpuart2_oscrc16m,
	mux_clkroot_lpuart2_syspll3div2, mux_clkroot_lpuart2_syspll1div5, mux_clkroot_lpuart2_syspll2out, mux_clkroot_lpuart2_syspll2pfd3 };

/* Mux selector: LPUART3 */
enum { mux_clkroot_lpuart3_oscrc48mdiv2 = 0, mux_clkroot_lpuart3_osc24mout, mux_clkroot_lpuart3_oscrc400m, mux_clkroot_lpuart3_oscrc16m,
	mux_clkroot_lpuart3_syspll3div2, mux_clkroot_lpuart3_syspll1div5, mux_clkroot_lpuart3_syspll2out, mux_clkroot_lpuart3_syspll2pfd3 };

/* Mux selector: LPUART4 */
enum { mux_clkroot_lpuart4_oscrc48mdiv2 = 0, mux_clkroot_lpuart4_osc24mout, mux_clkroot_lpuart4_oscrc400m, mux_clkroot_lpuart4_oscrc16m,
	mux_clkroot_lpuart4_syspll3div2, mux_clkroot_lpuart4_syspll1div5, mux_clkroot_lpuart4_syspll2out, mux_clkroot_lpuart4_syspll2pfd3 };

/* Mux selector: LPUART5 */
enum { mux_clkroot_lpuart5_oscrc48mdiv2 = 0, mux_clkroot_lpuart5_osc24mout, mux_clkroot_lpuart5_oscrc400m, mux_clkroot_lpuart5_oscrc16m,
	mux_clkroot_lpuart5_syspll3div2, mux_clkroot_lpuart5_syspll1div5, mux_clkroot_lpuart5_syspll2out, mux_clkroot_lpuart5_syspll2pfd3 };

/* Mux selector: LPUART6 */
enum { mux_clkroot_lpuart6_oscrc48mdiv2 = 0, mux_clkroot_lpuart6_osc24mout, mux_clkroot_lpuart6_oscrc400m, mux_clkroot_lpuart6_oscrc16m,
	mux_clkroot_lpuart6_syspll3div2, mux_clkroot_lpuart6_syspll1div5, mux_clkroot_lpuart6_syspll2out, mux_clkroot_lpuart6_syspll2pfd3 };

/* Mux selector: LPUART7 */
enum { mux_clkroot_lpuart7_oscrc48mdiv2 = 0, mux_clkroot_lpuart7_osc24mout, mux_clkroot_lpuart7_oscrc400m, mux_clkroot_lpuart7_oscrc16m,
	mux_clkroot_lpuart7_syspll3div2, mux_clkroot_lpuart7_syspll1div5, mux_clkroot_lpuart7_syspll2out, mux_clkroot_lpuart7_syspll2pfd3 };

/* Mux selector: LPUART8 */
enum { mux_clkroot_lpuart8_oscrc48mdiv2 = 0, mux_clkroot_lpuart8_osc24mout, mux_clkroot_lpuart8_oscrc400m, mux_clkroot_lpuart8_oscrc16m,
	mux_clkroot_lpuart8_syspll3div2, mux_clkroot_lpuart8_syspll1div5, mux_clkroot_lpuart8_syspll2out, mux_clkroot_lpuart8_syspll2pfd3 };

/* Mux selector: LPUART9 */
enum { mux_clkroot_lpuart9_oscrc48mdiv2 = 0, mux_clkroot_lpuart9_osc24mout, mux_clkroot_lpuart9_oscrc400m, mux_clkroot_lpuart9_oscrc16m,
	mux_clkroot_lpuart9_syspll3div2, mux_clkroot_lpuart9_syspll1div5, mux_clkroot_lpuart9_syspll2out, mux_clkroot_lpuart9_syspll2pfd3 };

/* Mux selector: LPUART10 */
enum { mux_clkroot_lpuart10_oscrc48mdiv2 = 0, mux_clkroot_lpuart10_osc24mout, mux_clkroot_lpuart10_oscrc400m, mux_clkroot_lpuart10_oscrc16m,
	mux_clkroot_lpuart10_syspll3div2, mux_clkroot_lpuart10_syspll1div5, mux_clkroot_lpuart10_syspll2out, mux_clkroot_lpuart10_syspll2pfd3 };

/* Mux selector: LPUART11 */
enum { mux_clkroot_lpuart11_oscrc48mdiv2 = 0, mux_clkroot_lpuart11_osc24mout, mux_clkroot_lpuart11_oscrc400m, mux_clkroot_lpuart11_oscrc16m,
	mux_clkroot_lpuart11_syspll3pfd3, mux_clkroot_lpuart11_syspll3out, mux_clkroot_lpuart11_syspll2pfd3, mux_clkroot_lpuart11_syspll1div5 };

/* Mux selector: LPUART12 */
enum { mux_clkroot_lpuart12_oscrc48mdiv2 = 0, mux_clkroot_lpuart12_osc24mout, mux_clkroot_lpuart12_oscrc400m, mux_clkroot_lpuart12_oscrc16m,
	mux_clkroot_lpuart12_syspll3pfd3, mux_clkroot_lpuart12_syspll3out, mux_clkroot_lpuart12_syspll2pfd3, mux_clkroot_lpuart12_syspll1div5 };

/* Mux selector: LPI2C1 */
enum { mux_clkroot_lpi2c1_oscrc48mdiv2 = 0, mux_clkroot_lpi2c1_osc24mout, mux_clkroot_lpi2c1_oscrc400m, mux_clkroot_lpi2c1_oscrc16m,
	mux_clkroot_lpi2c1_syspll3div2, mux_clkroot_lpi2c1_syspll1div5, mux_clkroot_lpi2c1_syspll2out, mux_clkroot_lpi2c1_syspll2pfd3 };

/* Mux selector: LPI2C2 */
enum { mux_clkroot_lpi2c2_oscrc48mdiv2 = 0, mux_clkroot_lpi2c2_osc24mout, mux_clkroot_lpi2c2_oscrc400m, mux_clkroot_lpi2c2_oscrc16m,
	mux_clkroot_lpi2c2_syspll3div2, mux_clkroot_lpi2c2_syspll1div5, mux_clkroot_lpi2c2_syspll2out, mux_clkroot_lpi2c2_syspll2pfd3 };

/* Mux selector: LPI2C3 */
enum { mux_clkroot_lpi2c3_oscrc48mdiv2 = 0, mux_clkroot_lpi2c3_osc24mout, mux_clkroot_lpi2c3_oscrc400m, mux_clkroot_lpi2c3_oscrc16m,
	mux_clkroot_lpi2c3_syspll3div2, mux_clkroot_lpi2c3_syspll1div5, mux_clkroot_lpi2c3_syspll2out, mux_clkroot_lpi2c3_syspll2pfd3 };

/* Mux selector: LPI2C4 */
enum { mux_clkroot_lpi2c4_oscrc48mdiv2 = 0, mux_clkroot_lpi2c4_osc24mout, mux_clkroot_lpi2c4_oscrc400m, mux_clkroot_lpi2c4_oscrc16m,
	mux_clkroot_lpi2c4_syspll3div2, mux_clkroot_lpi2c4_syspll1div5, mux_clkroot_lpi2c4_syspll2out, mux_clkroot_lpi2c4_syspll2pfd3 };

/* Mux selector: LPI2C5 */
enum { mux_clkroot_lpi2c5_oscrc48mdiv2 = 0, mux_clkroot_lpi2c5_osc24mout, mux_clkroot_lpi2c5_oscrc400m, mux_clkroot_lpi2c5_oscrc16m,
	mux_clkroot_lpi2c5_syspll3pfd3, mux_clkroot_lpi2c5_syspll3out, mux_clkroot_lpi2c5_syspll2pfd3, mux_clkroot_lpi2c5_syspll1div5 };

/* Mux selector: LPI2C6 */
enum { mux_clkroot_lpi2c6_oscrc48mdiv2 = 0, mux_clkroot_lpi2c6_osc24mout, mux_clkroot_lpi2c6_oscrc400m, mux_clkroot_lpi2c6_oscrc16m,
	mux_clkroot_lpi2c6_syspll3pfd3, mux_clkroot_lpi2c6_syspll3out, mux_clkroot_lpi2c6_syspll2pfd3, mux_clkroot_lpi2c6_syspll1div5 };

/* Mux selector: LPSPI1 */
enum { mux_clkroot_lpspi1_oscrc48mdiv2 = 0, mux_clkroot_lpspi1_osc24mout, mux_clkroot_lpspi1_oscrc400m, mux_clkroot_lpspi1_oscrc16m,
	mux_clkroot_lpspi1_syspll3pfd2, mux_clkroot_lpspi1_syspll1div5, mux_clkroot_lpspi1_syspll2out, mux_clkroot_lpspi1_syspll2pfd3 };

/* Mux selector: LPSPI2 */
enum { mux_clkroot_lpspi2_oscrc48mdiv2 = 0, mux_clkroot_lpspi2_osc24mout, mux_clkroot_lpspi2_oscrc400m, mux_clkroot_lpspi2_oscrc16m,
	mux_clkroot_lpspi2_syspll3pfd2, mux_clkroot_lpspi2_syspll1div5, mux_clkroot_lpspi2_syspll2out, mux_clkroot_lpspi2_syspll2pfd3 };

/* Mux selector: LPSPI3 */
enum { mux_clkroot_lpspi3_oscrc48mdiv2 = 0, mux_clkroot_lpspi3_osc24mout, mux_clkroot_lpspi3_oscrc400m, mux_clkroot_lpspi3_oscrc16m,
	mux_clkroot_lpspi3_syspll3pfd2, mux_clkroot_lpspi3_syspll1div5, mux_clkroot_lpspi3_syspll2out, mux_clkroot_lpspi3_syspll2pfd3 };

/* Mux selector: LPSPI4 */
enum { mux_clkroot_lpspi4_oscrc48mdiv2 = 0, mux_clkroot_lpspi4_osc24mout, mux_clkroot_lpspi4_oscrc400m, mux_clkroot_lpspi4_oscrc16m,
	mux_clkroot_lpspi4_syspll3pfd2, mux_clkroot_lpspi4_syspll1div5, mux_clkroot_lpspi4_syspll2out, mux_clkroot_lpspi4_syspll2pfd3 };

/* Mux selector: LPSPI5 */
enum { mux_clkroot_lpspi5_oscrc48mdiv2 = 0, mux_clkroot_lpspi5_osc24mout, mux_clkroot_lpspi5_oscrc400m, mux_clkroot_lpspi5_oscrc16m,
	mux_clkroot_lpspi5_syspll3pfd3, mux_clkroot_lpspi5_syspll3out, mux_clkroot_lpspi5_syspll3pfd2, mux_clkroot_lpspi5_syspll1div5 };

/* Mux selector: LPSPI6 */
enum { mux_clkroot_lpspi6_oscrc48mdiv2 = 0, mux_clkroot_lpspi6_osc24mout, mux_clkroot_lpspi6_oscrc400m, mux_clkroot_lpspi6_oscrc16m,
	mux_clkroot_lpspi6_syspll3pfd3, mux_clkroot_lpspi6_syspll3out, mux_clkroot_lpspi6_syspll3pfd2, mux_clkroot_lpspi6_syspll1div5 };

/* Mux selector: EMV1 */
enum { mux_clkroot_emv1_oscrc48mdiv2 = 0, mux_clkroot_emv1_osc24mout, mux_clkroot_emv1_oscrc400m, mux_clkroot_emv1_oscrc16m,
	mux_clkroot_emv1_syspll3div2, mux_clkroot_emv1_syspll1div5, mux_clkroot_emv1_syspll2out, mux_clkroot_emv1_syspll2pfd3 };

/* Mux selector: EMV2 */
enum { mux_clkroot_emv2_oscrc48mdiv2 = 0, mux_clkroot_emv2_osc24mout, mux_clkroot_emv2_oscrc400m, mux_clkroot_emv2_oscrc16m,
	mux_clkroot_emv2_syspll3div2, mux_clkroot_emv2_syspll1div5, mux_clkroot_emv2_syspll2out, mux_clkroot_emv2_syspll2pfd3 };

/* Mux selector: ENET1 */
enum { mux_clkroot_enet1_oscrc48mdiv2 = 0, mux_clkroot_enet1_osc24mout, mux_clkroot_enet1_oscrc400m, mux_clkroot_enet1_oscrc16m,
	mux_clkroot_enet1_syspll1div2, mux_clkroot_enet1_audiopllout, mux_clkroot_enet1_syspll1div5, mux_clkroot_enet1_syspll2pfd1 };

/* Mux selector: ENET2 */
enum { mux_clkroot_enet2_oscrc48mdiv2 = 0, mux_clkroot_enet2_osc24mout, mux_clkroot_enet2_oscrc400m, mux_clkroot_enet2_oscrc16m,
	mux_clkroot_enet2_syspll1div2, mux_clkroot_enet2_audiopllout, mux_clkroot_enet2_syspll1div5, mux_clkroot_enet2_syspll2pfd1 };

/* Mux selector: ENET_QOS */
enum { mux_clkroot_enet_qos_oscrc48mdiv2 = 0, mux_clkroot_enet_qos_osc24mout, mux_clkroot_enet_qos_oscrc400m, mux_clkroot_enet_qos_oscrc16m,
	mux_clkroot_enet_qos_syspll1div2, mux_clkroot_enet_qos_audiopllout, mux_clkroot_enet_qos_syspll1div5, mux_clkroot_enet_qos_syspll2pfd1 };

/* Mux selector: ENET_25M */
enum { mux_clkroot_enet_25m_oscrc48mdiv2 = 0, mux_clkroot_enet_25m_osc24mout, mux_clkroot_enet_25m_oscrc400m, mux_clkroot_enet_25m_oscrc16m,
	mux_clkroot_enet_25m_syspll1div2, mux_clkroot_enet_25m_audiopllout, mux_clkroot_enet_25m_syspll1div5, mux_clkroot_enet_25m_syspll2pfd1 };

/* Mux selector: ENET_TIMER1 */
enum { mux_clkroot_enet_timer1_oscrc48mdiv2 = 0, mux_clkroot_enet_timer1_osc24mout, mux_clkroot_enet_timer1_oscrc400m, mux_clkroot_enet_timer1_oscrc16m,
	mux_clkroot_enet_timer1_syspll1div2, mux_clkroot_enet_timer1_audiopllout, mux_clkroot_enet_timer1_syspll1div5, mux_clkroot_enet_timer1_syspll2pfd1 };

/* Mux selector: ENET_TIMER2 */
enum { mux_clkroot_enet_timer2_oscrc48mdiv2 = 0, mux_clkroot_enet_timer2_osc24mout, mux_clkroot_enet_timer2_oscrc400m, mux_clkroot_enet_timer2_oscrc16m,
	mux_clkroot_enet_timer2_syspll1div2, mux_clkroot_enet_timer2_audiopllout, mux_clkroot_enet_timer2_syspll1div5, mux_clkroot_enet_timer2_syspll2pfd1 };

/* Mux selector: ENET_TIMER3 */
enum { mux_clkroot_enet_timer3_oscrc48mdiv2 = 0, mux_clkroot_enet_timer3_osc24mout, mux_clkroot_enet_timer3_oscrc400m, mux_clkroot_enet_timer3_oscrc16m,
	mux_clkroot_enet_timer3_syspll1div2, mux_clkroot_enet_timer3_audiopllout, mux_clkroot_enet_timer3_syspll1div5, mux_clkroot_enet_timer3_syspll2pfd1 };

/* Mux selector: USDHC1 */
enum { mux_clkroot_usdhc1_oscrc48mdiv2 = 0, mux_clkroot_usdhc1_osc24mout, mux_clkroot_usdhc1_oscrc400m, mux_clkroot_usdhc1_oscrc16m,
	mux_clkroot_usdhc1_syspll2pfd2, mux_clkroot_usdhc1_syspll2pfd0, mux_clkroot_usdhc1_syspll1div5, mux_clkroot_usdhc1_armpllout };

/* Mux selector: USDHC2 */
enum { mux_clkroot_usdhc2_oscrc48mdiv2 = 0, mux_clkroot_usdhc2_osc24mout, mux_clkroot_usdhc2_oscrc400m, mux_clkroot_usdhc2_oscrc16m,
	mux_clkroot_usdhc2_syspll2pfd2, mux_clkroot_usdhc2_syspll2pfd0, mux_clkroot_usdhc2_syspll1div5, mux_clkroot_usdhc2_armpllout };

/* Mux selector: ASRC */
enum { mux_clkroot_asrc_oscrc48mdiv2 = 0, mux_clkroot_asrc_osc24mout, mux_clkroot_asrc_oscrc400m, mux_clkroot_asrc_oscrc16m,
	mux_clkroot_asrc_syspll1div5, mux_clkroot_asrc_syspll3div2, mux_clkroot_asrc_audiopllout, mux_clkroot_asrc_syspll2pfd3 };

/* Mux selector: MQS */
enum { mux_clkroot_mqs_oscrc48mdiv2 = 0, mux_clkroot_mqs_osc24mout, mux_clkroot_mqs_oscrc400m, mux_clkroot_mqs_oscrc16m,
	mux_clkroot_mqs_syspll1div5, mux_clkroot_mqs_syspll3div2, mux_clkroot_mqs_audiopllout, mux_clkroot_mqs_syspll2pfd3 };

/* Mux selector: MIC */
enum { mux_clkroot_mic_oscrc48mdiv2 = 0, mux_clkroot_mic_osc24mout, mux_clkroot_mic_oscrc400m, mux_clkroot_mic_oscrc16m,
	mux_clkroot_mic_syspll3pfd3, mux_clkroot_mic_syspll3out, mux_clkroot_mic_audiopllout, mux_clkroot_mic_syspll1div5 };

/* Mux selector: SPDIF */
enum { mux_clkroot_spdif_oscrc48mdiv2 = 0, mux_clkroot_spdif_osc24mout, mux_clkroot_spdif_oscrc400m, mux_clkroot_spdif_oscrc16m,
	mux_clkroot_spdif_audiopllout, mux_clkroot_spdif_syspll3out, mux_clkroot_spdif_syspll3pfd2, mux_clkroot_spdif_syspll2pfd3 };

/* Mux selector: SAI1 */
enum { mux_clkroot_sai1_oscrc48mdiv2 = 0, mux_clkroot_sai1_osc24mout, mux_clkroot_sai1_oscrc400m, mux_clkroot_sai1_oscrc16m,
	mux_clkroot_sai1_audiopllout, mux_clkroot_sai1_syspll3pfd2, mux_clkroot_sai1_syspll1div5, mux_clkroot_sai1_syspll2pfd3 };

/* Mux selector: SAI2 */
enum { mux_clkroot_sai2_oscrc48mdiv2 = 0, mux_clkroot_sai2_osc24mout, mux_clkroot_sai2_oscrc400m, mux_clkroot_sai2_oscrc16m,
	mux_clkroot_sai2_audiopllout, mux_clkroot_sai2_syspll3pfd2, mux_clkroot_sai2_syspll1div5, mux_clkroot_sai2_syspll2pfd3 };

/* Mux selector: SAI3 */
enum { mux_clkroot_sai3_oscrc48mdiv2 = 0, mux_clkroot_sai3_osc24mout, mux_clkroot_sai3_oscrc400m, mux_clkroot_sai3_oscrc16m,
	mux_clkroot_sai3_audiopllout, mux_clkroot_sai3_syspll3pfd2, mux_clkroot_sai3_syspll1div5, mux_clkroot_sai3_syspll2pfd3 };

/* Mux selector: SAI4 */
enum { mux_clkroot_sai4_oscrc48mdiv2 = 0, mux_clkroot_sai4_osc24mout, mux_clkroot_sai4_oscrc400m, mux_clkroot_sai4_oscrc16m,
	mux_clkroot_sai4_syspll3pfd3, mux_clkroot_sai4_syspll3out, mux_clkroot_sai4_audiopllout, mux_clkroot_sai4_syspll1div5 };

/* Mux selector: GPU2D */
enum { mux_clkroot_gpu2d_oscrc48mdiv2 = 0, mux_clkroot_gpu2d_osc24mout, mux_clkroot_gpu2d_oscrc400m, mux_clkroot_gpu2d_oscrc16m,
	mux_clkroot_gpu2d_syspll2out, mux_clkroot_gpu2d_syspll2pfd1, mux_clkroot_gpu2d_syspll3out, mux_clkroot_gpu2d_videopllout };

/* Mux selector: LCDIF */
enum { mux_clkroot_lcdif_oscrc48mdiv2 = 0, mux_clkroot_lcdif_osc24mout, mux_clkroot_lcdif_oscrc400m, mux_clkroot_lcdif_oscrc16m,
	mux_clkroot_lcdif_syspll2out, mux_clkroot_lcdif_syspll2pfd2, mux_clkroot_lcdif_syspll3pfd0, mux_clkroot_lcdif_videopllout };

/* Mux selector: LCDIFV2 */
enum { mux_clkroot_lcdifv2_oscrc48mdiv2 = 0, mux_clkroot_lcdifv2_osc24mout, mux_clkroot_lcdifv2_oscrc400m, mux_clkroot_lcdifv2_oscrc16m,
	mux_clkroot_lcdifv2_syspll2out, mux_clkroot_lcdifv2_syspll2pfd2, mux_clkroot_lcdifv2_syspll3pfd0, mux_clkroot_lcdifv2_videopllout };

/* Mux selector: MIPI_REF */
enum { mux_clkroot_mipi_ref_oscrc48mdiv2 = 0, mux_clkroot_mipi_ref_osc24mout, mux_clkroot_mipi_ref_oscrc400m, mux_clkroot_mipi_ref_oscrc16m,
	mux_clkroot_mipi_ref_syspll2out, mux_clkroot_mipi_ref_syspll2pfd0, mux_clkroot_mipi_ref_syspll3pfd0, mux_clkroot_mipi_ref_videopllout };

/* Mux selector: MIPI_ESC */
enum { mux_clkroot_mipi_esc_oscrc48mdiv2 = 0, mux_clkroot_mipi_esc_osc24mout, mux_clkroot_mipi_esc_oscrc400m, mux_clkroot_mipi_esc_oscrc16m,
	mux_clkroot_mipi_esc_syspll2out, mux_clkroot_mipi_esc_syspll2pfd0, mux_clkroot_mipi_esc_syspll3pfd0, mux_clkroot_mipi_esc_videopllout };

/* Mux selector: CSI2 */
enum { mux_clkroot_csi2_oscrc48mdiv2 = 0, mux_clkroot_csi2_osc24mout, mux_clkroot_csi2_oscrc400m, mux_clkroot_csi2_oscrc16m,
	mux_clkroot_csi2_syspll2pfd2, mux_clkroot_csi2_syspll3out, mux_clkroot_csi2_syspll2pfd0, mux_clkroot_csi2_videopllout };

/* Mux selector: CSI2_ESC */
enum { mux_clkroot_csi2_esc_oscrc48mdiv2 = 0, mux_clkroot_csi2_esc_osc24mout, mux_clkroot_csi2_esc_oscrc400m, mux_clkroot_csi2_esc_oscrc16m,
	mux_clkroot_csi2_esc_syspll2pfd2, mux_clkroot_csi2_esc_syspll3out, mux_clkroot_csi2_esc_syspll2pfd0, mux_clkroot_csi2_esc_videopllout };

/* Mux selector: CSI2_UI */
enum { mux_clkroot_csi2_ui_oscrc48mdiv2 = 0, mux_clkroot_csi2_ui_osc24mout, mux_clkroot_csi2_ui_oscrc400m, mux_clkroot_csi2_ui_oscrc16m,
	mux_clkroot_csi2_ui_syspll2pfd2, mux_clkroot_csi2_ui_syspll3out, mux_clkroot_csi2_ui_syspll2pfd0, mux_clkroot_csi2_ui_videopllout };

/* Mux selector: CSI */
enum { mux_clkroot_csi_oscrc48mdiv2 = 0, mux_clkroot_csi_osc24mout, mux_clkroot_csi_oscrc400m, mux_clkroot_csi_oscrc16m,
	mux_clkroot_csi_syspll2pfd2, mux_clkroot_csi_syspll3out, mux_clkroot_csi_syspll3pfd1, mux_clkroot_csi_videopllout };

/* Mux selector: CKO1 */
enum { mux_clkroot_cko1_oscrc48mdiv2 = 0, mux_clkroot_cko1_osc24mout, mux_clkroot_cko1_oscrc400m, mux_clkroot_cko1_oscrc16m,
	mux_clkroot_cko1_syspll2pfd2, mux_clkroot_cko1_syspll2out, mux_clkroot_cko1_syspll3pfd1, mux_clkroot_cko1_syspll1div5 };

/* Mux selector: CKO2 */
enum { mux_clkroot_cko2_oscrc48mdiv2 = 0, mux_clkroot_cko2_osc24mout, mux_clkroot_cko2_oscrc400m, mux_clkroot_cko2_oscrc16m,
	mux_clkroot_cko2_syspll2pfd3, mux_clkroot_cko2_oscrc48m, mux_clkroot_cko2_syspll3pfd1, mux_clkroot_cko2_audiopllout };


/* PLL clock source */
enum { clk_pllarm = 0, clk_pllsys1, clk_pllsys2, clk_pllsys3, clk_pllaudio, clk_pllvideo };


/* clang-format on */


extern int _imxrt_setIOmux(int mux, char sion, char mode);


extern int _imxrt_setIOpad(int pad, char sre, char dse, char pue, char pus, char ode, char apc);


extern int _imxrt_setIOisel(int isel, char daisy);


extern int _imxrt_gpioConfig(unsigned int d, u8 pin, u8 dir);


extern int _imxrt_gpioSet(unsigned int d, u8 pin, u8 val);


extern int _imxrt_gpioSetPort(unsigned int d, u32 val);


extern int _imxrt_gpioGet(unsigned int d, u8 pin, u8 *val);


extern int _imxrt_gpioGetPort(unsigned int d, u32 *val);


extern int _imxrt_getDevClock(int clock, int *div, int *mux, int *mfd, int *mfn, int *state);


extern int _imxrt_setDevClock(int clock, int div, int mux, int mfd, int mfn, int state);


extern int _imxrt_setPfdPllFracClock(u8 pfd, u8 clk_pll, u8 frac);


extern void _imxrt_init(void);


extern int _imxrt_setDirectLPCG(int clock, int state);


extern int _imxrt_setLevelLPCG(int clock, int level);


/* CM4 core management */


extern u32 _imxrt_getStateCM4(void);


extern int _imxrt_setVtorCM4(int dwpLock, int dwp, addr_t vtor);


extern void _imxrt_runCM4(void);


#endif
