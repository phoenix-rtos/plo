/*
 * Phoenix-RTOS
 *
 * Operating system kernel
 *
 * STM32H5 basic peripherals control functions
 *
 * Copyright 2017, 2020, 2021, 2025, 2026 Phoenix Systems
 * Author: Aleksander Kaminski, Pawel Pisarczyk, Hubert Buczynski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_STM32H5_H_
#define _HAL_STM32H5_H_

#include "../types.h"

/* Device clocks */
enum {
	/* AHB1 */
	dev_gpdma1 = 0 + 0,
	dev_gpdma2,
	dev_flitf = 0 + 8,
	dev_crc = 0 + 12,
	dev_cordic = 0 + 14,
	dev_fmac,
	dev_ramcfg = 0 + 17,
	dev_eth = 0 + 19,
	dev_ethtx,
	dev_ethrx,
	dev_tzsc1 = 0 + 24,
	dev_bkpram = 0 + 28,
	dev_dcache = 0 + 30,
	dev_sram1,

	/* AHB2 */
	dev_gpioa = 32 + 0,
	dev_gpiob,
	dev_gpioc,
	dev_gpiod,
	dev_gpioe,
	dev_gpiof,
	dev_gpiog,
	dev_gpioh,
	dev_gpioi,
	dev_adc = 32 + 10,
	dev_dac1,
	dev_dcmi_pssi,
	dev_aes = 32 + 16,
	dev_hash,
	dev_rng,
	dev_pka,
	dev_saes,
	dev_sram2 = 32 + 30,
	dev_sram3,

	/* AHB4 */
	dev_otfdec1 = 64 + 7,
	dev_sdmmc1 = 64 + 11,
	dev_sdmmc2,
	dev_fmc = 64 + 16,
	dev_octospi1 = 64 + 20,

	/* APB1 low */
	dev_tim2 = 96 + 0,
	dev_tim3,
	dev_tim4,
	dev_tim5,
	dev_tim6,
	dev_tim7,
	dev_tim12,
	dev_tim13,
	dev_tim14,
	dev_wwdg = 96 + 11,
	dev_spi2 = 96 + 14,
	dev_spi3,
	dev_usart2 = 96 + 17,
	dev_usart3,
	dev_uart4,
	dev_uart5,
	dev_i2c1,
	dev_i2c2,
	dev_i3c1,
	dev_crs,
	dev_usart6,
	dev_usart10,
	dev_usart11,
	dev_cec,
	dev_uart7 = 96 + 30,
	dev_uart8,

	/* APB1 high */
	dev_uart9 = 128 + 0,
	dev_uart12,
	dev_dts = 128 + 3,
	dev_lptim2 = 128 + 5,
	dev_fdcan = 128 + 9,
	dev_ucpd1 = 128 + 23,

	/* APB2 */
	dev_tim1 = 160 + 11,
	dev_spi1,
	dev_tim8,
	dev_usart1,
	dev_tim15 = 160 + 16,
	dev_tim16,
	dev_tim17,
	dev_spi4,
	dev_spi6,
	dev_sai1,
	dev_sai2,
	dev_usb = 160 + 24,

	/* APB3 */
	dev_sbs = 192 + 1,
	dev_spi5 = 192 + 5,
	dev_lpuart1,
	dev_i2c3,
	dev_i2c4,
	dev_i3c2,
	dev_lptim1 = 192 + 11,
	dev_lptim3,
	dev_lptim4,
	dev_lptim5,
	dev_lptim6,
	dev_vref = 192 + 20,
	dev_rtcapb
};


/* Device resets */
enum {
	/* AHB1 */
	dev_rst_gpdma1 = 0 + 0,
	dev_rst_gpdma2,
	dev_rst_crc = 0 + 12,
	dev_rst_cordic = 0 + 14,
	dev_rst_fmac,
	dev_rst_ramcfg = 0 + 17,
	dev_rst_eth = 0 + 19,

	/* AHB2 */
	dev_rst_gpioa = 32 + 0,
	dev_rst_gpiob,
	dev_rst_gpioc,
	dev_rst_gpiod,
	dev_rst_gpioe,
	dev_rst_gpiof,
	dev_rst_gpiog,
	dev_rst_gpioh,
	dev_rst_gpioi,
	dev_rst_adc = 32 + 10,
	dev_rst_dac1,
	dev_rst_dcmi_pssi,
	dev_rst_aes = 32 + 16,
	dev_rst_hash,
	dev_rst_rng,
	dev_rst_pka,
	dev_rst_saes,

	/* AHB4 */
	dev_rst_otfdec1 = 48 + 7,
	dev_rst_sdmmc1 = 48 + 11,
	dev_rst_sdmmc2,
	dev_rst_fmc = 48 + 16,
	dev_rst_octospi1 = 48 + 20,

	/* APB1 low */
	dev_rst_tim2 = 64 + 0,
	dev_rst_tim3,
	dev_rst_tim4,
	dev_rst_tim5,
	dev_rst_tim6,
	dev_rst_tim7,
	dev_rst_tim12,
	dev_rst_tim13,
	dev_rst_tim14,
	dev_rst_spi2 = 64 + 14,
	dev_rst_spi3,
	dev_rst_usart2 = 64 + 17,
	dev_rst_usart3,
	dev_rst_uart4,
	dev_rst_uart5,
	dev_rst_i2c1,
	dev_rst_i2c2,
	dev_rst_i3c1,
	dev_rst_crs,
	dev_rst_usart6,
	dev_rst_usart10,
	dev_rst_usart11,
	dev_rst_cec,
	dev_rst_uart7 = 64 + 30,
	dev_rst_uart8,

	/* APB1 high */
	dev_rst_uart9 = 96 + 0,
	dev_rst_uart12,
	dev_rst_dts = 96 + 3,
	dev_rst_lptim2 = 96 + 5,
	dev_rst_fdcan = 96 + 9,
	dev_rst_ucpd1 = 96 + 23,

	/* APB2 */
	dev_rst_tim1 = 128 + 11,
	dev_rst_spi1,
	dev_rst_tim8,
	dev_rst_usart1,
	dev_rst_tim15 = 128 + 16,
	dev_rst_tim16,
	dev_rst_tim17,
	dev_rst_spi4,
	dev_rst_spi6,
	dev_rst_sai1,
	dev_rst_sai2,
	dev_rst_usb = 128 + 24,

	/* APB3 */
	dev_rst_spi5 = 160 + 5,
	dev_rst_lpuart1,
	dev_rst_i2c3,
	dev_rst_i2c4,
	dev_rst_i3c2,
	dev_rst_lptim1 = 160 + 11,
	dev_rst_lptim3,
	dev_rst_lptim4,
	dev_rst_lptim5,
	dev_rst_lptim6,
	dev_rst_vref = 160 + 20,
};


enum ipclks {
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
	ipclk_timicsel,
	ipclk_usart11sel,
	ipclk_uart12sel,
	ipclk_lptim1sel,
	ipclk_lptim2sel,
	ipclk_lptim3sel,
	ipclk_lptim4sel,
	ipclk_lptim5sel,
	ipclk_lptim6sel,
	ipclk_spi1sel,
	ipclk_spi2sel,
	ipclk_spi3sel,
	ipclk_spi4sel,
	ipclk_spi5sel,
	ipclk_spi6sel,
	ipclk_lpuart1sel,
	ipclk_octospi1sel,
	ipclk_systicksel,
	ipclk_usbsel,
	ipclk_sdmmc1sel,
	ipclk_sdmmc2sel,
	ipclk_i2c1sel,
	ipclk_i2c2sel,
	ipclk_i2c3sel,
	ipclk_i2c4sel,
	ipclk_i3c1sel,
	ipclk_i3c2sel,
	ipclk_adcdacsel,
	ipclk_dacsel,
	ipclk_rngsel,
	ipclk_cecsel,
	ipclk_fdcansel,
	ipclk_sai1sel,
	ipclk_sai2sel,
	ipclk_ckpersel,
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


extern void _stm32_pwrSetCPUVolt(u8 range);


extern void _stm32_rtcUnlockRegs(void);


extern void _stm32_rtcLockRegs(void);


extern int _stm32_systickInit(u32 interval);


extern void _stm32_systickDone(void);


extern void _stm32_wdgReload(void);


/* Initialize only STM32 HAL functions for use without initializing the platform. */
extern void _stm32_initHalOnly(void);


extern void _stm32_init(void);

#endif
