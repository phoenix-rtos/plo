/*
 * Phoenix-RTOS
 *
 * Operating system kernel
 *
 * STM32N6 basic peripherals control functions
 *
 * Copyright 2017, 2020, 2021, 2025 Phoenix Systems
 * Author: Aleksander Kaminski, Pawel Pisarczyk, Hubert Buczynski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_STM32N6_H_
#define _HAL_STM32N6_H_

#include "../types.h"

/* Device identifiers */
enum {
	dev_gpdma1 = 0x4,
	dev_adc12,
	dev_ramcfg = 0x2c,
	dev_mdf1 = 0x30,
	dev_adf1,
	dev_rng = 0x40,
	dev_hash,
	dev_cryp,
	dev_saes = 0x44,
	dev_pka = 0x48,
	dev_rifsc,
	dev_iac,
	dev_risaf = 0x4e,
	dev_gpioa = 0x60,
	dev_gpiob,
	dev_gpioc,
	dev_gpiod,
	dev_gpioe,
	dev_gpiof,
	dev_gpiog,
	dev_gpioh,
	dev_gpion = 0x6d,
	dev_gpioo,
	dev_gpiop,
	dev_gpioq,
	dev_pwr = 0x72,
	dev_crc,
	dev_hpdma1 = 0x80,
	dev_dma2d,
	dev_jpeg = 0x83,
	dev_fmc,
	dev_xspi1,
	dev_pssi,
	dev_sdmmc2,
	dev_sdmmc1,
	dev_xspi2 = 0x8c,
	dev_xspim,
	dev_mce1,
	dev_mce2,
	dev_mce3,
	dev_xspi3,
	dev_mce4,
	dev_gfxmmu,
	dev_gpu,
	dev_eth1mac = 0x96,
	dev_eth1tx,
	dev_eth1rx,
	dev_eth1,
	dev_otg1,
	dev_otgphy1,
	dev_otgphy2,
	dev_otg2,
	dev_npucache,
	dev_npu,
	dev_tim2 = 0xa0,
	dev_tim3,
	dev_tim4,
	dev_tim5,
	dev_tim6,
	dev_tim7,
	dev_tim12,
	dev_tim13,
	dev_tim14,
	dev_lptim1,
	dev_wwdg = 0xab,
	dev_tim10,
	dev_tim11,
	dev_spi2,
	dev_spi3,
	dev_spdifrx1,
	dev_usart2,
	dev_usart3,
	dev_uart4,
	dev_uart5,
	dev_i2c1,
	dev_i2c2,
	dev_i2c3,
	dev_i3c1,
	dev_i3c2,
	dev_uart7 = 0xbe,
	dev_uart8,
	dev_mdios = 0xc5,
	dev_fdcan = 0xc8,
	dev_ucpd1 = 0xd2,
	dev_tim1 = 0xe0,
	dev_tim8,
	dev_usart1 = 0xe4,
	dev_usart6,
	dev_uart9,
	dev_usart10,
	dev_spi1 = 0xec,
	dev_spi4,
	dev_tim18 = 0xef,
	dev_tim15,
	dev_tim16,
	dev_tim17,
	dev_tim9,
	dev_spi5,
	dev_sai1,
	dev_sai2,
	dev_dft = 0x102,
	dev_hdp = 0x122,
	dev_lpuart1,
	dev_spi6 = 0x125,
	dev_i2c4 = 0x127,
	dev_lptim2 = 0x129,
	dev_lptim3,
	dev_lptim4,
	dev_lptim5,
	dev_vrefbuf = 0x12f,
	dev_rtc,
	dev_rtcapb,
	dev_r2gret = 0x136,
	dev_r2gnpu,
	dev_serf = 0x13f,
	dev_syscfg = 0x140,
	dev_bsec,
	dev_dts,
	dev_busperfm = 0x144,
	dev_ltdc,
	dev_dcmipp,
	dev_gfxtim = 0x164,
	dev_venc,
	dev_csi,
};


enum ipclks {
	ipclk_adf1sel = 0,
	ipclk_adc12sel,
	ipclk_adcpre,
	ipclk_dcmippsel,
	ipclk_eth1ptpsel,
	ipclk_eth1ptpdiv,
	ipclk_eth1pwrdownack,
	ipclk_eth1clksel,
	ipclk_eth1sel,
	ipclk_eth1refclksel,
	ipclk_eth1gtxclksel,
	ipclk_fdcansel,
	ipclk_fmcsel,
	ipclk_dftsel,
	ipclk_i2c1sel,
	ipclk_i2c2sel,
	ipclk_i2c3sel,
	ipclk_i2c4sel,
	ipclk_i3c1sel,
	ipclk_i3c2sel,
	ipclk_ltdcsel,
	ipclk_mco1sel,
	ipclk_mco1pre,
	ipclk_mco2sel,
	ipclk_mco2pre,
	ipclk_mdf1sel,
	ipclk_xspi1sel,
	ipclk_xspi2sel,
	ipclk_xspi3sel,
	ipclk_otgphy1sel,
	ipclk_otgphy1ckrefsel,
	ipclk_otgphy2sel,
	ipclk_otgphy2ckrefsel,
	ipclk_persel,
	ipclk_pssisel,
	ipclk_rtcsel,
	ipclk_rtcpre,
	ipclk_sai1sel,
	ipclk_sai2sel,
	ipclk_sdmmc1sel,
	ipclk_sdmmc2sel,
	ipclk_spdifrx1sel,
	ipclk_spi1sel,
	ipclk_spi2sel,
	ipclk_spi3sel,
	ipclk_spi4sel,
	ipclk_spi5sel,
	ipclk_spi6sel,
	ipclk_lptim1sel,
	ipclk_lptim2sel,
	ipclk_lptim3sel,
	ipclk_lptim4sel,
	ipclk_lptim5sel,
	ipclk_usart1sel,
	ipclk_usart2sel,
	ipclk_usart3sel,
	ipclk_uart4sel,
	ipclk_uart5sel,
	ipclk_usart6sel,
	ipclk_uart7sel,
	ipclk_uart8sel,
	ipclk_uart9sel,
	ipclk_usart10sel,
	ipclk_lpuart1sel,
	ipclks_count,
};


enum pwr_supplies {
	pwr_supply_vddio4 = 0,
	pwr_supply_vddio5,
	pwr_supply_vddio2,
	pwr_supply_vddio3,
	pwr_supply_vusb33,
	pwr_supply_vdda,
	pwr_supplies_count,
};


enum pwr_supply_ops {
	pwr_supply_op_monitor_enable = 0,
	pwr_supply_op_valid,
	pwr_supply_op_ready,
	pwr_supply_op_range_sel,
	pwr_supply_op_range_sel_standby,
	pwr_supply_ops_count
};


enum gpio_modes {
	gpio_mode_gpi = 0,
	gpio_mode_gpo = 1,
	gpio_mode_af = 2,
	gpio_mode_analog = 3,
};


enum gpio_otypes {
	gpio_otype_pp = 0,
	gpio_otype_od = 1,
};


enum gpio_ospeeds {
	gpio_ospeed_low = 0,
	gpio_ospeed_med = 1,
	gpio_ospeed_hi = 2,
	gpio_ospeed_vhi = 3,
};


enum gpio_pupds {
	gpio_pupd_nopull = 0,
	gpio_pupd_pullup = 1,
	gpio_pupd_pulldn = 2,
};


/* Sets peripheral's bus clock */
extern int _stm32_rccSetDevClock(unsigned int dev, u32 status);


extern int _stm32_rccGetDevClock(unsigned int dev, u32 *status);


/* Sets independent peripheral clock configuration */
extern int _stm32_rccSetIPClk(unsigned int ipclk, u8 setting);


/* Get frequency of CPU clock in Hz */
extern u32 _stm32_rccGetCPUClock(void);


/* Get frequency of PER (common peripheral) clock in Hz */
extern u32 _stm32_rccGetPerClock(void);


extern void _stm32_rccClearResetFlags(void);


extern int _stm32_gpioConfig(unsigned int d, u8 pin, u8 mode, u8 af, u8 otype, u8 ospeed, u8 pupd);


extern int _stm32_gpioSet(unsigned int d, u8 pin, u8 val);


extern int _stm32_gpioSetPort(unsigned int d, u16 val);


extern int _stm32_gpioGet(unsigned int d, u8 pin, u8 *val);


extern int _stm32_gpioGetPort(unsigned int d, u16 *val);


/* Range = 0 - VOS low, 1 - VOS high */
extern void _stm32_pwrSetCPUVolt(u8 range);


/* Perform an operation (pwr_supply_op_*) on a selected power supply (pwr_supply_*) */
extern int _stm32_pwrSupplyOperation(unsigned int supply, unsigned int operation, int status);


extern void _stm32_rtcUnlockRegs(void);


extern void _stm32_rtcLockRegs(void);


extern int _stm32_systickInit(u32 interval);


extern void _stm32_systickDone(void);


extern void _stm32_wdgReload(void);


extern void _stm32_init(void);

#endif
