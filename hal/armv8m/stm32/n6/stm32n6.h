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

#ifndef EIO
#define EIO    5
#endif

#ifndef EAGAIN
#define EAGAIN 11
#endif

/* Device clocks */
enum {
	dev_aclkn = 0x0,
	dev_aclknc,
	dev_ahbm,
	dev_ahb1,
	dev_ahb2,
	dev_ahb3,
	dev_ahb4,
	dev_ahb5,
	dev_apb1,
	dev_apb2,
	dev_apb3,
	dev_apb4,
	dev_apb5,
	dev_dbg = 0x20,
	dev_mco1,
	dev_mco2,
	dev_xspiphycomp,
	dev_per = 0x26,
	dev_axisram3 = 0x40,
	dev_axisram4,
	dev_axisram5,
	dev_axisram6,
	dev_ahbsram1,
	dev_ahbsram2,
	dev_bkpsram,
	dev_axisram1,
	dev_axisram2,
	dev_flexram,
	dev_npucacheram,
	dev_vencram,
	dev_bootrom,
	dev_gpdma1 = 0x64,
	dev_adc12,
	dev_ramcfg = 0x8c,
	dev_mdf1 = 0x90,
	dev_adf1,
	dev_rng = 0xa0,
	dev_hash,
	dev_cryp,
	dev_saes = 0xa4,
	dev_pka = 0xa8,
	dev_rifsc,
	dev_iac,
	dev_risaf = 0xae,
	dev_gpioa = 0xc0,
	dev_gpiob,
	dev_gpioc,
	dev_gpiod,
	dev_gpioe,
	dev_gpiof,
	dev_gpiog,
	dev_gpioh,
	dev_gpion = 0xcd,
	dev_gpioo,
	dev_gpiop,
	dev_gpioq,
	dev_pwr = 0xd2,
	dev_crc,
	dev_hpdma1 = 0xe0,
	dev_dma2d,
	dev_jpeg = 0xe3,
	dev_fmc,
	dev_xspi1,
	dev_pssi,
	dev_sdmmc2,
	dev_sdmmc1,
	dev_xspi2 = 0xec,
	dev_xspim,
	dev_mce1,
	dev_mce2,
	dev_mce3,
	dev_xspi3,
	dev_mce4,
	dev_gfxmmu,
	dev_gpu,
	dev_eth1mac = 0xf6,
	dev_eth1tx,
	dev_eth1rx,
	dev_eth1,
	dev_otg1,
	dev_otgphy1,
	dev_otgphy2,
	dev_otg2,
	dev_npucache,
	dev_npu,
	dev_tim2,
	dev_tim3,
	dev_tim4,
	dev_tim5,
	dev_tim6,
	dev_tim7,
	dev_tim12,
	dev_tim13,
	dev_tim14,
	dev_lptim1,
	dev_wwdg = 0x10b,
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
	dev_uart7 = 0x11e,
	dev_uart8,
	dev_mdios = 0x125,
	dev_fdcan = 0x128,
	dev_ucpd1 = 0x132,
	dev_tim1 = 0x140,
	dev_tim8,
	dev_usart1 = 0x144,
	dev_usart6,
	dev_uart9,
	dev_usart10,
	dev_spi1 = 0x14c,
	dev_spi4,
	dev_tim18 = 0x14f,
	dev_tim15,
	dev_tim16,
	dev_tim17,
	dev_tim9,
	dev_spi5,
	dev_sai1,
	dev_sai2,
	dev_dft = 0x162,
	dev_hdp = 0x182,
	dev_lpuart1,
	dev_spi6 = 0x185,
	dev_i2c4 = 0x187,
	dev_lptim2 = 0x189,
	dev_lptim3,
	dev_lptim4,
	dev_lptim5,
	dev_vrefbuf = 0x18f,
	dev_rtc,
	dev_rtcapb,
	dev_r2gret = 0x196,
	dev_r2gnpu,
	dev_serf = 0x19f,
	dev_syscfg,
	dev_bsec,
	dev_dts,
	dev_busperfm = 0x1a4,
	dev_ltdc = 0x1c1,
	dev_dcmipp,
	dev_gfxtim = 0x1c4,
	dev_venc,
	dev_csi,
};


/* Device resets */
enum {
	dev_rst_aclkn = 0x0,
	dev_rst_ahbm = 0x2,
	dev_rst_ahb1,
	dev_rst_ahb2,
	dev_rst_ahb3,
	dev_rst_ahb4,
	dev_rst_ahb5,
	dev_rst_apb1,
	dev_rst_apb2,
	dev_rst_apb3,
	dev_rst_apb4,
	dev_rst_apb5,
	dev_rst_noc,
	dev_rst_dbg = 0x20,
	dev_rst_xspiphy1 = 0x24,
	dev_rst_xspiphy2,
	dev_rst_sdmmc1dll = 0x27,
	dev_rst_sdmmc2dll,
	dev_rst_axisram3 = 0x40,
	dev_rst_axisram4,
	dev_rst_axisram5,
	dev_rst_axisram6,
	dev_rst_ahbsram1,
	dev_rst_ahbsram2,
	dev_rst_axisram1 = 0x47,
	dev_rst_axisram2,
	dev_rst_flexram,
	dev_rst_npucacheram,
	dev_rst_vencram,
	dev_rst_bootrom,
	dev_rst_gpdma1 = 0x64,
	dev_rst_adc12,
	dev_rst_ramcfg = 0x8c,
	dev_rst_mdf1 = 0x90,
	dev_rst_adf1,
	dev_rst_rng = 0xa0,
	dev_rst_hash,
	dev_rst_cryp,
	dev_rst_saes = 0xa4,
	dev_rst_pka = 0xa8,
	dev_rst_iac = 0xaa,
	dev_rst_gpioa = 0xc0,
	dev_rst_gpiob,
	dev_rst_gpioc,
	dev_rst_gpiod,
	dev_rst_gpioe,
	dev_rst_gpiof,
	dev_rst_gpiog,
	dev_rst_gpioh,
	dev_rst_gpion = 0xcd,
	dev_rst_gpioo,
	dev_rst_gpiop,
	dev_rst_gpioq,
	dev_rst_pwr = 0xd2,
	dev_rst_crc,
	dev_rst_hpdma1 = 0xe0,
	dev_rst_dma2d,
	dev_rst_jpeg = 0xe3,
	dev_rst_fmc,
	dev_rst_xspi1,
	dev_rst_pssi,
	dev_rst_sdmmc2,
	dev_rst_sdmmc1,
	dev_rst_xspi2 = 0xec,
	dev_rst_xspim,
	dev_rst_xspi3 = 0xf1,
	dev_rst_mce4,
	dev_rst_gfxmmu,
	dev_rst_gpu,
	dev_rst_syscfgotghsphy1 = 0xf7,
	dev_rst_syscfgotghsphy2,
	dev_rst_eth1,
	dev_rst_otg1,
	dev_rst_otgphy1,
	dev_rst_otgphy2,
	dev_rst_otg2,
	dev_rst_npucache,
	dev_rst_npu,
	dev_rst_tim2,
	dev_rst_tim3,
	dev_rst_tim4,
	dev_rst_tim5,
	dev_rst_tim6,
	dev_rst_tim7,
	dev_rst_tim12,
	dev_rst_tim13,
	dev_rst_tim14,
	dev_rst_lptim1,
	dev_rst_wwdg = 0x10b,
	dev_rst_tim10,
	dev_rst_tim11,
	dev_rst_spi2,
	dev_rst_spi3,
	dev_rst_spdifrx1,
	dev_rst_usart2,
	dev_rst_usart3,
	dev_rst_uart4,
	dev_rst_uart5,
	dev_rst_i2c1,
	dev_rst_i2c2,
	dev_rst_i2c3,
	dev_rst_i3c1,
	dev_rst_i3c2,
	dev_rst_uart7 = 0x11e,
	dev_rst_uart8,
	dev_rst_mdios = 0x125,
	dev_rst_fdcan = 0x128,
	dev_rst_ucpd1 = 0x132,
	dev_rst_tim1 = 0x140,
	dev_rst_tim8,
	dev_rst_usart1 = 0x144,
	dev_rst_usart6,
	dev_rst_uart9,
	dev_rst_usart10,
	dev_rst_spi1 = 0x14c,
	dev_rst_spi4,
	dev_rst_tim18 = 0x14f,
	dev_rst_tim15,
	dev_rst_tim16,
	dev_rst_tim17,
	dev_rst_tim9,
	dev_rst_spi5,
	dev_rst_sai1,
	dev_rst_sai2,
	dev_rst_hdp = 0x182,
	dev_rst_lpuart1,
	dev_rst_spi6 = 0x185,
	dev_rst_i2c4 = 0x187,
	dev_rst_lptim2 = 0x189,
	dev_rst_lptim3,
	dev_rst_lptim4,
	dev_rst_lptim5,
	dev_rst_vrefbuf = 0x18f,
	dev_rst_rtc,
	dev_rst_r2gret = 0x196,
	dev_rst_r2gnpu,
	dev_rst_serf = 0x19f,
	dev_rst_syscfg,
	dev_rst_dts = 0x1a2,
	dev_rst_busperfm = 0x1a4,
	dev_rst_ltdc = 0x1c1,
	dev_rst_dcmipp,
	dev_rst_gfxtim = 0x1c4,
	dev_rst_venc,
	dev_rst_csi,
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


/* Put device in our out of reset. status == 0 - out of reset, 1 - in reset */
extern int _stm32_rccDevReset(unsigned int dev, u32 status);


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


extern ssize_t _stm32_rngRead(u8 *val, size_t len);

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
