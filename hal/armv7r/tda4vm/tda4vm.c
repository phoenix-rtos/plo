/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * TDA4VM basic peripherals control functions
 *
 * Copyright 2025 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "tda4vm.h"
#include "config.h"
#include <hal/hal.h>

#include <board_config.h>

#include "tda4vm_intrtr_defs.h"
#include "tda4vm_regs.h"


static const struct {
	unsigned start;
	unsigned end;
	unsigned offset;
	volatile u32 *base_addr;
} tda4vm_intr_routers[intr_routers_count] = {
	[intr_router_wkup_gpiomux] = {
		.start = 124,
		.end = 140,
		.offset = 0,
		.base_addr = INTR_ROUTER_WKUP_GPIOMUX_BASE_ADDR,
	},
	[intr_router_main2mcu_lvl] = {
		.start = 160,
		.end = 224,
		.offset = 0,
		.base_addr = INTR_ROUTER_MAIN2MCU_LVL_BASE_ADDR,
	},
	[intr_router_main2mcu_pls] = {
		.start = 224,
		.end = 272,
		.offset = 0,
		.base_addr = INTR_ROUTER_MAIN2MCU_PLS_BASE_ADDR,
	},
	[intr_router_mcu_navss0] = {
		.start = 64,
		.end = 96,
		.offset = 0,
		.base_addr = INTR_ROUTER_MCU_NAVSS0_BASE_ADDR,
	},
	[intr_router_navss0] = {
		.start = 376,
		.end = 384,
		.offset = 400,
		.base_addr = INTR_ROUTER_NAVSS0_BASE_ADDR,
	},
};


/* Clock select registers */
static const struct {
	volatile u32 *reg;
} clksels[] = {
	[clksel_wkup_per] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_per_clksel },
	[clksel_wkup_usart] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_usart_clksel },
	[clksel_wkup_gpio] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_gpio_clksel },
	[clksel_wkup_main_pll0] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll0_clksel },
	[clksel_wkup_main_pll1] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll1_clksel },
	[clksel_wkup_main_pll2] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll2_clksel },
	[clksel_wkup_main_pll3] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll3_clksel },
	[clksel_wkup_main_pll4] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll4_clksel },
	[clksel_wkup_main_pll5] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll5_clksel },
	[clksel_wkup_main_pll6] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll6_clksel },
	[clksel_wkup_main_pll7] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll7_clksel },
	[clksel_wkup_main_pll8] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll8_clksel },
	[clksel_wkup_main_pll12] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll12_clksel },
	[clksel_wkup_main_pll13] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll13_clksel },
	[clksel_wkup_main_pll14] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll14_clksel },
	[clksel_wkup_main_pll15] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll15_clksel },
	[clksel_wkup_main_pll16] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll16_clksel },
	[clksel_wkup_main_pll17] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll17_clksel },
	[clksel_wkup_main_pll18] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll18_clksel },
	[clksel_wkup_main_pll19] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll19_clksel },
	[clksel_wkup_main_pll23] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll23_clksel },
	[clksel_wkup_main_pll24] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll24_clksel },
	[clksel_wkup_main_pll25] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll25_clksel },
	[clksel_wkup_mcu_spi0] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_mcu_spi0_clksel },
	[clksel_wkup_mcu_spi1] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_mcu_spi1_clksel },
	[clksel_mcu_efuse] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_efuse_clksel },
	[clksel_mcu_mcan0] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_mcan0_clksel },
	[clksel_mcu_mcan1] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_mcan1_clksel },
	[clksel_mcu_ospi0] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_ospi0_clksel },
	[clksel_mcu_ospi1] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_ospi1_clksel },
	[clksel_mcu_adc0] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_adc0_clksel },
	[clksel_mcu_adc1] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_adc1_clksel },
	[clksel_mcu_enet] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_enet_clksel },
	[clksel_mcu_r5_core0] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_r5_core0_clksel },
	[clksel_mcu_timer0] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_timer0_clksel },
	[clksel_mcu_timer1] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_timer1_clksel },
	[clksel_mcu_timer2] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_timer2_clksel },
	[clksel_mcu_timer3] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_timer3_clksel },
	[clksel_mcu_timer4] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_timer4_clksel },
	[clksel_mcu_timer5] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_timer5_clksel },
	[clksel_mcu_timer6] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_timer6_clksel },
	[clksel_mcu_timer7] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_timer7_clksel },
	[clksel_mcu_timer8] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_timer8_clksel },
	[clksel_mcu_timer9] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_timer9_clksel },
	[clksel_mcu_rti0] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_rti0_clksel },
	[clksel_mcu_rti1] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_rti1_clksel },
	[clksel_mcu_usart] = { CTRLMMR_MCU_BASE_ADDR + ctrlmmr_mcu_reg_usart_clksel },
	[clksel_gtc] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_gtc_clksel },
	[clksel_efuse] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_efuse_clksel },
	[clksel_icssg0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_icssg0_clksel },
	[clksel_icssg1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_icssg1_clksel },
	[clksel_pcie0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_pcie0_clksel },
	[clksel_pcie1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_pcie1_clksel },
	[clksel_pcie2] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_pcie2_clksel },
	[clksel_pcie3] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_pcie3_clksel },
	[clksel_cpsw] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_cpsw_clksel },
	[clksel_navss] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_navss_clksel },
	[clksel_emmc0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_emmc0_clksel },
	[clksel_emmc1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_emmc1_clksel },
	[clksel_emmc2] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_emmc2_clksel },
	[clksel_ufs0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_ufs0_clksel },
	[clksel_gpmc] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_gpmc_clksel },
	[clksel_usb0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_usb0_clksel },
	[clksel_usb1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_usb1_clksel },
	[clksel_timer0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer0_clksel },
	[clksel_timer1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer1_clksel },
	[clksel_timer2] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer2_clksel },
	[clksel_timer3] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer3_clksel },
	[clksel_timer4] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer4_clksel },
	[clksel_timer5] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer5_clksel },
	[clksel_timer6] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer6_clksel },
	[clksel_timer7] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer7_clksel },
	[clksel_timer8] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer8_clksel },
	[clksel_timer9] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer9_clksel },
	[clksel_timer10] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer10_clksel },
	[clksel_timer11] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer11_clksel },
	[clksel_timer12] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer12_clksel },
	[clksel_timer13] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer13_clksel },
	[clksel_timer14] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer14_clksel },
	[clksel_timer15] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer15_clksel },
	[clksel_timer16] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer16_clksel },
	[clksel_timer17] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer17_clksel },
	[clksel_timer18] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer18_clksel },
	[clksel_timer19] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_timer19_clksel },
	[clksel_spi0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_spi0_clksel },
	[clksel_spi1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_spi1_clksel },
	[clksel_spi2] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_spi2_clksel },
	[clksel_spi3] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_spi3_clksel },
	[clksel_spi5] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_spi5_clksel },
	[clksel_spi6] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_spi6_clksel },
	[clksel_spi7] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_spi7_clksel },
	[clksel_mcasp0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp0_clksel },
	[clksel_mcasp1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp1_clksel },
	[clksel_mcasp2] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp2_clksel },
	[clksel_mcasp3] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp3_clksel },
	[clksel_mcasp4] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp4_clksel },
	[clksel_mcasp5] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp5_clksel },
	[clksel_mcasp6] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp6_clksel },
	[clksel_mcasp7] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp7_clksel },
	[clksel_mcasp8] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp8_clksel },
	[clksel_mcasp9] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp9_clksel },
	[clksel_mcasp10] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp10_clksel },
	[clksel_mcasp11] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp11_clksel },
	[clksel_mcasp0_ah] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp0_ahclksel },
	[clksel_mcasp1_ah] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp1_ahclksel },
	[clksel_mcasp2_ah] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp2_ahclksel },
	[clksel_mcasp3_ah] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp3_ahclksel },
	[clksel_mcasp4_ah] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp4_ahclksel },
	[clksel_mcasp5_ah] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp5_ahclksel },
	[clksel_mcasp6_ah] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp6_ahclksel },
	[clksel_mcasp7_ah] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp7_ahclksel },
	[clksel_mcasp8_ah] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp8_ahclksel },
	[clksel_mcasp9_ah] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp9_ahclksel },
	[clksel_mcasp10_ah] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp10_ahclksel },
	[clksel_mcasp11_ah] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcasp11_ahclksel },
	[clksel_atl] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_atl_clksel },
	[clksel_dphy0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_dphy0_clksel },
	[clksel_edp_phy0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_edp_phy0_clksel },
	[clksel_wwd0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_wwd0_clksel },
	[clksel_wwd1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_wwd1_clksel },
	[clksel_wwd15] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_wwd15_clksel },
	[clksel_wwd16] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_wwd16_clksel },
	[clksel_wwd24] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_wwd24_clksel },
	[clksel_wwd25] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_wwd25_clksel },
	[clksel_wwd28] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_wwd28_clksel },
	[clksel_wwd29] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_wwd29_clksel },
	[clksel_wwd30] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_wwd30_clksel },
	[clksel_wwd31] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_wwd31_clksel },
	[clksel_serdes0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_serdes0_clksel },
	[clksel_serdes0_1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_serdes0_clk1sel },
	[clksel_serdes1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_serdes1_clksel },
	[clksel_serdes1_1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_serdes1_clk1sel },
	[clksel_serdes2] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_serdes2_clksel },
	[clksel_serdes2_1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_serdes2_clk1sel },
	[clksel_serdes3] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_serdes3_clksel },
	[clksel_serdes3_1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_serdes3_clk1sel },
	[clksel_mcan0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcan0_clksel },
	[clksel_mcan1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcan1_clksel },
	[clksel_mcan2] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcan2_clksel },
	[clksel_mcan3] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcan3_clksel },
	[clksel_mcan4] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcan4_clksel },
	[clksel_mcan5] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcan5_clksel },
	[clksel_mcan6] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcan6_clksel },
	[clksel_mcan7] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcan7_clksel },
	[clksel_mcan8] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcan8_clksel },
	[clksel_mcan9] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcan9_clksel },
	[clksel_mcan10] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcan10_clksel },
	[clksel_mcan11] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcan11_clksel },
	[clksel_mcan12] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcan12_clksel },
	[clksel_mcan13] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_mcan13_clksel },
	[clksel_pcie_refclk0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_pcie_refclk0_clksel },
	[clksel_pcie_refclk1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_pcie_refclk1_clksel },
	[clksel_pcie_refclk2] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_pcie_refclk2_clksel },
	[clksel_pcie_refclk3] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_pcie_refclk3_clksel },
	[clksel_dss_dispc0_1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_dss_dispc0_clksel1 },
	[clksel_dss_dispc0_2] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_dss_dispc0_clksel2 },
	[clksel_dss_dispc0_3] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_dss_dispc0_clksel3 },
	[clksel_wkup_mcu_obsclk0] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_mcu_obsclk_ctrl },
	[clksel_obsclk0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_obsclk0_ctrl },
};


static const struct {
	volatile u32 *reg;
} clkdivs[] = {
	[clkdiv_usart0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_usart0_clk_ctrl },
	[clkdiv_usart1] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_usart1_clk_ctrl },
	[clkdiv_usart2] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_usart2_clk_ctrl },
	[clkdiv_usart3] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_usart3_clk_ctrl },
	[clkdiv_usart4] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_usart4_clk_ctrl },
	[clkdiv_usart5] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_usart5_clk_ctrl },
	[clkdiv_usart6] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_usart6_clk_ctrl },
	[clkdiv_usart7] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_usart7_clk_ctrl },
	[clkdiv_usart8] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_usart8_clk_ctrl },
	[clkdiv_usart9] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_usart9_clk_ctrl },
	[clkdiv_wkup_mcu_obsclk0] = { CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_mcu_obsclk_ctrl },
	[clkdiv_obsclk0] = { CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_obsclk0_ctrl },
};


int tda4vm_routeInterrupt(unsigned router, unsigned intrSource, unsigned intrDestination)
{
	unsigned offset;

	if (router >= intr_routers_count) {
		return -1;
	}

	if ((intrDestination < tda4vm_intr_routers[router].start) || (intrDestination >= tda4vm_intr_routers[router].end)) {
		return -1;
	}

	offset = (intrDestination - tda4vm_intr_routers[router].start) + tda4vm_intr_routers[router].offset + 1;
	*(tda4vm_intr_routers[router].base_addr + offset) = (intrSource & 0x1ff) | (1 << 16);
	return 0;
}


static volatile u32 *tda4vm_getPLLBase(unsigned pll)
{
	if (pll >= clk_main_pll0) {
		return MAIN_PLL_BASE_ADDR + ((0x1000 / 4) * (pll - clk_main_pll0));
	}

	return MCU_PLL_BASE_ADDR + ((0x1000 / 4) * pll);
}


static int tda4vm_isPLLValid(unsigned pll)
{
	/* A few of the PLLs are missing on this platform... */
	return !((pll > clk_plls_count) || ((pll > clk_main_arm0_pll8) && (pll < clk_main_ddr_pll12)));
}


static void tda4vm_PLLDelay(void)
{
	/* Waits 1000 CPU cycles. In default configuration (1 GHz clock) this is 1 us.
	 * May wait longer if CPU PLL is bypassed, but this is not a problem. */
	u32 start = hal_getCycleCount();
	while ((hal_getCycleCount() - start) < 1000) {
		/* Wait for 1000 ticks to pass */
	}
}


int tda4vm_getPLL(unsigned pll, tda4vm_clk_pll_t *config)
{
	/* Lookup table from register value to divider */
	static const unsigned char tda4vm_deskewPllValToDivider[4] = { 4, 2, 1, 1 };
	volatile u32 *base;
	u32 ctrl, div, hw_config;
	if (!tda4vm_isPLLValid(pll)) {
		return -1;
	}

	base = tda4vm_getPLLBase(pll);
	hw_config = *(base + pll_reg_cfg);
	ctrl = *(base + pll_reg_ctrl);
	div = *(base + pll_reg_div_ctrl);
	if ((hw_config & 0x3) == 2) {
		/* Deskew PLL */
		config->pre_div = tda4vm_deskewPllValToDivider[div & 0x3];
		config->mult_int = tda4vm_deskewPllValToDivider[(div >> 12) & 0x3];
		config->mult_frac = 0;
		config->post_div1 = 1 << ((div >> 8) & 0x7);
		config->post_div2 = 1;
		/* Active when bit is 0 */
		config->is_enabled = (*(base + pll_reg_ctrl) & (1 << 4)) == 0 ? 1 : 0;
	}
	else {
		/* Fractional PLL */
		config->mult_int = *(base + pll_reg_freq_ctrl0) & ((1 << 12) - 1);
		if ((ctrl & 0x3) == 0x3) {
			/* Fractional mode active */
			config->mult_frac = *(base + pll_reg_freq_ctrl1) & ((1 << 24) - 1);
		}
		else {
			config->mult_frac = 0;
		}

		config->pre_div = div & (0x3f);
		config->post_div1 = (div >> 16) & 0x7;
		config->post_div2 = (div >> 24) & 0x7;
		/* Active when bit is 1 */
		config->is_enabled = (*(base + pll_reg_ctrl) & (1 << 15)) != 0 ? 1 : 0;
	}

	return 0;
}


/* Function must be used while PLL configuration is unlocked and bypass is enabled */
static void _tda4vm_setHSDIV(volatile u32 *base, unsigned hsdiv, unsigned divisor)
{
	u32 val;
	*(base + pll_reg_hsdiv_ctrl0 + hsdiv) &= ~(1 << 15); /* Disable clock output */
	tda4vm_PLLDelay();
	*(base + pll_reg_hsdiv_ctrl0 + hsdiv) |= (1 << 31); /* Put HSDIV into reset */
	val = *(base + pll_reg_hsdiv_ctrl0 + hsdiv);
	val &= ~0x7f;
	val |= divisor - 1;
	*(base + pll_reg_hsdiv_ctrl0 + hsdiv) = val;        /* Set new divider */
	*(base + pll_reg_hsdiv_ctrl0 + hsdiv) &= ~(1 << 8); /* Clear SYNC_DIS */
	tda4vm_PLLDelay();
	*(base + pll_reg_hsdiv_ctrl0 + hsdiv) &= ~(1 << 31); /* Take HSDIV out of reset */
	tda4vm_PLLDelay();
	*(base + pll_reg_hsdiv_ctrl0 + hsdiv) |= (1 << 15); /* Enable clock output */
}


int tda4vm_setPLL(unsigned pll, const tda4vm_clk_pll_t *config, const unsigned *hsdivs, unsigned int n_hsdivs)
{
	/* Lookup table from divider to register value */
	static const unsigned char tda4vm_deskewPllDividerToVal[5] = { 0b11, 0b10, 0b01, 0b11, 0b00 };
	volatile u32 *base;
	volatile u32 *pllctrl;
	u32 hw_config;
	unsigned i;
	if (!tda4vm_isPLLValid(pll) || (n_hsdivs > 16)) {
		return -1;
	}

	/* Verify arguments before any changes */
	for (i = 0; i < n_hsdivs; i++) {
		if ((hsdivs[i] > 128) || (hsdivs[i] == 0)) {
			return -1;
		}
	}

	base = tda4vm_getPLLBase(pll);
	/* Special treatment necessary for PLLs that run the CPUs */
	if (pll == clk_main_pll0) {
		pllctrl = PLLCTRL0_BASE_ADDR;
	}
	else if (pll == clk_mcu_r5fss_pll0) {
		pllctrl = WKUP_PLLCTRL0_BASE_ADDR;
	}
	else {
		pllctrl = NULL;
	}

	hw_config = *(base + pll_reg_cfg);

	if ((hw_config & 0x3) == 2) {
		/* Deskew PLL */
		if (config->is_enabled != 0) {
			if ((config->post_div2 != 1) ||
					(config->mult_int > 4) ||
					(config->pre_div > 4) ||
					(config->post_div1 > 128) ||
					(config->post_div1 == 0)) {
				return -1;
			}
		}

		/* Unlock configuration */
		*(base + pll_reg_lockkey0) = PLL_LOCKKEY_0;
		*(base + pll_reg_lockkey1) = PLL_LOCKKEY_1;
		*(base + pll_reg_ctrl) |= (1 << 31); /* Activate bypass */
		tda4vm_PLLDelay();
		*(base + pll_reg_ctrl) |= (1 << 4); /* Power down PLL */
		tda4vm_PLLDelay();
		if (config->is_enabled != 0) {
			*(base + pll_reg_div_ctrl) =
					((u32)tda4vm_deskewPllDividerToVal[config->mult_int] << 12) |
					((31 - __builtin_clz(config->post_div1)) << 8) |
					((u32)tda4vm_deskewPllDividerToVal[config->pre_div]);

			*(base + pll_reg_ctrl) &= ~(1 << 4); /* Power up PLL */
			tda4vm_PLLDelay();

			while ((*(base + pll_reg_stat) & 1) == 0) {
				/* Wait for PLL lock */
			}

			for (i = 0; i < n_hsdivs; i++) {
				/* Only set HSDIVs that are present in the system */
				if ((hw_config & ((1 << 16) << i)) != 0) {
					_tda4vm_setHSDIV(base, i, hsdivs[i]);
				}
			}

			tda4vm_PLLDelay();
			*(base + pll_reg_ctrl) &= ~(1 << 31); /* Deactivate bypass */
		}

		/* Lock configuration again */
		*(base + pll_reg_lockkey0) = 0;
		*(base + pll_reg_lockkey1) = 0;
	}
	else {
		/* Fractional PLL */
		if (config->is_enabled != 0) {
			/* According to docs "REFDIV[5:0] must be set to 000001" */
			if ((config->pre_div != 1) || (config->post_div1 < config->post_div2)) {
				return -1;
			}
		}

		/* Unlock configuration */
		*(base + pll_reg_lockkey0) = PLL_LOCKKEY_0;
		*(base + pll_reg_lockkey1) = PLL_LOCKKEY_1;
		if (pllctrl != NULL) {
			*(pllctrl + pllctrl_reg_pllctl) &= ~1; /* Put PLL control into bypass mode */
			*(pllctrl + pllctrl_reg_pllctl) &= ~(1 << 5);
		}

		*(base + pll_reg_ctrl) |= (1 << 31); /* Activate bypass */
		tda4vm_PLLDelay();
		*(base + pll_reg_ctrl) &= ~(1 << 15); /* Disable PLL */
		tda4vm_PLLDelay();
		if (config->is_enabled != 0) {
			if (pllctrl != NULL) {
				while ((*(pllctrl + pllctrl_reg_pllstat) & 1) != 0) {
					/* Wait until any previous operation completed */
				}

				*(pllctrl + pllctrl_reg_pllcmd) &= ~1;     /* Clear GOSET */
				*(pllctrl + pllctrl_reg_plldiv1) = 0x8000; /* Enable divider, divisor == 1*/
				*(pllctrl + pllctrl_reg_plldiv2) = 0x0;    /* Disable divider, divisor == 1*/
				*(pllctrl + pllctrl_reg_alnctl) = 0x3;
				*(pllctrl + pllctrl_reg_pllcmd) |= 1; /* Set GOSET - initiate divider change */
				while ((*(pllctrl + pllctrl_reg_pllstat) & 1) != 0) {
					/* Wait until operation completed */
				}
			}

			/* According to docs "A PLL must always operate in Fractional Mode" */
			*(base + pll_reg_ctrl) |= 0x3;
			*(base + pll_reg_ctrl) |= (1 << 4); /* CLK_POSTDIV_EN */
			*(base + pll_reg_freq_ctrl0) = config->mult_int & ((1 << 12) - 1);
			*(base + pll_reg_freq_ctrl1) = config->mult_frac & ((1 << 24) - 1);
			*(base + pll_reg_div_ctrl) =
					(config->pre_div & 0x3f) |
					((config->post_div1 & 0x7) << 16) |
					((config->post_div2 & 0x7) << 24);
			tda4vm_PLLDelay();
			*(base + pll_reg_ctrl) |= (1 << 15); /* Enable PLL */
			while ((*(base + pll_reg_stat) & 1) == 0) {
				/* Wait for PLL lock */
			}

			for (i = 0; i < n_hsdivs; i++) {
				/* Only set HSDIVs that are present in the system */
				if ((hw_config & ((1 << 16) << i)) != 0) {
					_tda4vm_setHSDIV(base, i, hsdivs[i]);
				}
			}

			tda4vm_PLLDelay();
			*(base + pll_reg_ctrl) &= ~(1 << 31); /* Deactivate bypass */
			if (pllctrl != NULL) {
				*(pllctrl + pllctrl_reg_pllctl) |= 1; /* Put PLL control into PLL mode */
			}
		}

		/* Lock configuration again */
		*(base + pll_reg_lockkey0) = 0;
		*(base + pll_reg_lockkey1) = 0;
	}

	return 0;
}


u64 tda4vm_getFrequency(unsigned pll, unsigned hsdiv)
{
	volatile u32 *base;
	tda4vm_clk_pll_t config;
	u32 hw_config, hsdiv_ctrl;
	u64 multiplier_24, final_freq_24; /* 30 bit integer : 24 bit fractional part format */
	u32 rounding;
	unsigned total_division, in_frequency;
	u32 clksel_val;
	if (hsdiv >= 16) {
		return 0;
	}

	if (tda4vm_getPLL(pll, &config) < 0) {
		return 0;
	}

	base = tda4vm_getPLLBase(pll);
	hw_config = *(base + pll_reg_cfg);
	if (((hw_config >> 16) & (1 << hsdiv)) == 0) {
		return 0;
	}

	if (pll < clk_main_pll0) {
		in_frequency = WKUP_HFOSC0_HZ;
	}
	else {
		clksel_val = *(CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_main_pll0_clksel + (pll - clk_main_pll0));
		if ((clksel_val & 1) == 0) {
			in_frequency = WKUP_HFOSC0_HZ;
		}
		else {
			in_frequency = HFOSC1_HZ;
		}
	}

	if (config.is_enabled == 0) {
		return in_frequency;
	}

	multiplier_24 = (u64)config.mult_int << 24;
	multiplier_24 |= config.mult_frac;

	total_division = config.pre_div * config.post_div1 * config.post_div2;
	if (total_division == 0) {
		return 0;
	}

	hsdiv_ctrl = *(base + pll_reg_hsdiv_ctrl0 + hsdiv);
	total_division *= (hsdiv_ctrl & 0x7f) + 1;
	final_freq_24 = multiplier_24 / total_division;
	final_freq_24 *= in_frequency;
	rounding = ((final_freq_24 & ((1 << 24) - 1)) >= (1 << 23)) ? 1 : 0;
	return (final_freq_24 >> 24) + rounding;
}


int tda4vm_setDebounceConfig(unsigned idx, unsigned period)
{
	volatile u32 *base = CTRLMMR_WKUP_BASE_ADDR;
	if ((idx == 0) || (idx > 6)) {
		return -1;
	}

	*(base + ctrlmmr_wkup_reg_dbounce_cfg1 - 1 + idx) = period & 0x3f;
	return 0;
}


int tda4vm_setPinConfig(unsigned pin, const tda4vm_pinConfig_t *config)
{
	volatile u32 *reg;
	if (pin >= PIN_MAIN_OFFS) {
		reg = CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_padconfig0 + pin - PIN_MAIN_OFFS;
	}
	else {
		reg = CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_padconfig0 + pin;
	}

	*reg = (config->flags & 0xffffc000) | ((config->debounce_idx & 0x3) << 11) | (config->mux & 0xf);

	return 0;
}


int tda4vm_getPinConfig(unsigned pin, tda4vm_pinConfig_t *config)
{
	volatile u32 *reg;
	u32 val;
	if (pin >= PIN_MAIN_OFFS) {
		reg = CTRL_MMR0_BASE_ADDR + ctrlmmr_reg_padconfig0 + pin - PIN_MAIN_OFFS;
	}
	else {
		reg = CTRLMMR_WKUP_BASE_ADDR + ctrlmmr_wkup_reg_padconfig0 + pin;
	}

	val = *reg;
	config->flags = (val & 0xffffc000);
	config->debounce_idx = ((val >> 11) & 0x3);
	config->mux = (val & 0xf);

	return 0;
}


static void tda4vm_PLLSetup(void)
{
	static const struct {
		tda4vm_clk_pll_t pll_conf;
		unsigned hsdivs[5];
		unsigned n_hsdivs;
	} pll_configs[3] = {
		{
			/* MCU_PLL0 -> multiplier 104.166666687, resulting clock == 2 GHz */
			.pll_conf = {
				.mult_int = 104,
				.mult_frac = 0x2aaaab,
				.pre_div = 1,
				.post_div1 = 1,
				.post_div2 = 1,
				.is_enabled = 1,
			},
			/* HSDIV outputs: 1 GHz, 2 GHz */
			.hsdivs = { 2, 1 },
			.n_hsdivs = 2,
		},
		{
			/* MCU_PLL1 -> multiplier 125, resulting clock == 2.4 GHz */
			.pll_conf = {
				.mult_int = 125,
				.mult_frac = 0,
				.pre_div = 1,
				.post_div1 = 1,
				.post_div2 = 1,
				.is_enabled = 1,
			},
			/* HSDIV outputs: 400, 60, 80, 96, 133.33 MHz */
			.hsdivs = { 6, 40, 30, 25, 18 },
			.n_hsdivs = 5,
		},
		{
			/* MCU_PLL2 -> multiplier 104.166666687, divider 2, resulting clock == 1 GHz */
			.pll_conf = {
				.mult_int = 104,
				.mult_frac = 0x2aaaab,
				.pre_div = 1,
				.post_div1 = 2,
				.post_div2 = 1,
				.is_enabled = 1,
			},
			/* HSDIV outputs: 1 GHz, 1 GHz, 1 GHz, 1 GHz, 166.67 MHz */
			.hsdivs = { 1, 1, 1, 1, 6 },
			.n_hsdivs = 5,
		},

	};
	unsigned i;

	for (i = 0; i < 3; i++) {
		tda4vm_setPLL(i, &pll_configs[i].pll_conf, pll_configs[i].hsdivs, pll_configs[i].n_hsdivs);
	}
}


__attribute__((noreturn)) void tda4vm_warmReset(void)
{
	volatile u32 *base = CTRLMMR_WKUP_BASE_ADDR;
	*(base + ctrlmmr_wkup_reg_mcu_warm_rst_ctrl) = 0x60000; /* Magic value to trigger reset */
	while (1) {
		/* Hang and wait for reset */
	}
}


int tda4vm_RATMapMemory(unsigned entry, addr_t cpuAddr, u64 physAddr, u32 logSize)
{
	volatile u32 *base = MCU_ARMSS_RAT_BASE_ADDR;
	u32 regions;

	regions = *(base + r5fss_rat_reg_config) & 0xff;
	if ((logSize > 32) || (entry >= regions)) {
		return -1;
	}

	/* Regions must be aligned to size on both sides */
	if (((cpuAddr & ((1 << logSize) - 1)) != 0) || ((physAddr & ((1uL << logSize) - 1)) != 0)) {
		return -1;
	}

	hal_cpuDataMemoryBarrier();
	*(base + r5fss_rat_reg_ctrl_0 + (entry * 4)) = 0; /* Disable translation */
	*(base + r5fss_rat_reg_base_0 + (entry * 4)) = cpuAddr;
	*(base + r5fss_rat_reg_trans_l_0 + (entry * 4)) = physAddr & 0xffffffff;
	*(base + r5fss_rat_reg_trans_u_0 + (entry * 4)) = (physAddr >> 32) & 0xffff;
	*(base + r5fss_rat_reg_ctrl_0 + (entry * 4)) = (1 << 31) | logSize; /* Enable and set size */
	hal_cpuDataMemoryBarrier();
	return 0;
}


void tda4vm_RATUnmapMemory(unsigned entry)
{
	volatile u32 *base = MCU_ARMSS_RAT_BASE_ADDR;
	u32 regions;

	regions = *(base + r5fss_rat_reg_config) & 0xff;
	if (entry >= regions) {
		return;
	}

	*(base + r5fss_rat_reg_ctrl_0 + (entry * 4)) = 0;
}


int tda4vm_setClksel(unsigned sel, unsigned val)
{
	if (sel >= clksels_count) {
		return -1;
	}

	if ((sel == clksel_wkup_mcu_obsclk0) || (sel == clksel_obsclk0)) {
		*clksels[sel].reg &= ~0x1f;
		*clksels[sel].reg |= val & 0x1f;
	}
	else {
		*clksels[sel].reg = val;
	}

	return 0;
}


int tda4vm_getClksel(unsigned sel)
{
	if (sel >= clksels_count) {
		return -1;
	}

	return (int)(*clksels[sel].reg & 0xff);
}


int tda4vm_setClkdiv(unsigned sel, unsigned val)
{
	if (sel >= clkdivs_count) {
		return -1;
	}

	if ((sel == clkdiv_wkup_mcu_obsclk0) || (sel == clkdiv_obsclk0)) {
		*clkdivs[sel].reg &= ~(0x1ff << 8);
		*clkdivs[sel].reg |= (val & 0xff) << 8;
	}
	else {
		*clkdivs[sel].reg = val & 0xff;
	}

	*clkdivs[sel].reg |= (1 << 16);
	return 0;
}


int tda4vm_getClkdiv(unsigned sel)
{
	u32 val;
	if (sel >= clkdivs_count) {
		return -1;
	}

	val = *clkdivs[sel].reg;
	if ((sel == clkdiv_wkup_mcu_obsclk0) || (sel == clkdiv_obsclk0)) {
		return (int)(val >> 8) & 0xff;
	}

	return (int)(val & 0xff);
}


const tda4vm_uart_info_t *tda4vm_getUartInfo(unsigned n)
{
	static const tda4vm_uart_info_t uarts[] = {
		{
			.base = MCU_UART0_BASE_ADDR,
			.clksel = clksel_mcu_usart,
			.clksel_val = 0, /* CLKSEL set to MCU_PLL1_HSDIV3_CLKOUT */
			.clkdiv = -1,
			.divisor = 1,
			.pll = clk_mcu_per_pll1,
			.hsdiv = 3,
			.irq = MCU_UART0_IRQ,
			.pins = {
				{ pin_mcu_ospi1_d2, 4, 1 },
				{ pin_wkup_gpio0_10, 2, 1 },
				{ pin_wkup_gpio0_12, 0, 1 },
				{ pin_mcu_ospi1_d1, 4, 0 },
				{ pin_wkup_gpio0_11, 2, 0 },
				{ pin_wkup_gpio0_13, 0, 0 },
			},
		},
	};

	if (n >= sizeof(uarts) / sizeof(uarts[0])) {
		return NULL;
	}

	return &uarts[n];
}


void _tda4vm_init(void)
{
	/* Set up memory for kernel code */
	hal_cpuMapATCM(ADDR_ATCM, 1);
	hal_cpuMapBTCM(ADDR_BTCM, 1);
	tda4vm_RATMapMemory(
			RAT_REGION_MSRAM_REMAP,
			ADDR_MSRAM_REMAP,
			ORIG_MSRAM_REMAP,
			31 - __builtin_clz(SIZE_MSRAM_REMAP));

	tda4vm_PLLSetup();
}
