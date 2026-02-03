/*
 * Phoenix-RTOS
 *
 * Operating system kernel
 *
 * STM32U5 basic peripherals control functions
 *
 * Copyright 2017, 2020, 2021, 2025 Phoenix Systems
 * Copyright 2026 Apator Metrix
 * Author: Aleksander Kaminski, Pawel Pisarczyk, Hubert Buczynski, Jacek Maksymowicz, Mateusz Karcz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_STM32U5_H_
#define _HAL_STM32U5_H_

#include "../types.h"
#include "stm32u5_regs.h"

#define _STM32_ID(base, reg, pos) ((((reg) - (base)) << 5) | (pos))
#define _STM32_DEV_ID(reg, pos) _STM32_ID(rcc_ahb1enr, reg, pos)
#define _STM32_RST_ID(reg, pos) _STM32_ID(rcc_ahb1rstr, reg, pos)

/* Device clocks */
enum {
	dev_gpdma1 = _STM32_DEV_ID(rcc_ahb1enr, 0),
	dev_cordic = _STM32_DEV_ID(rcc_ahb1enr, 1),
	dev_fmac = _STM32_DEV_ID(rcc_ahb1enr, 2),
	dev_mdf1 = _STM32_DEV_ID(rcc_ahb1enr, 3),
	dev_flash = _STM32_DEV_ID(rcc_ahb1enr, 8),
	dev_crc = _STM32_DEV_ID(rcc_ahb1enr, 12),
	dev_tsc = _STM32_DEV_ID(rcc_ahb1enr, 16),
	dev_ramcfg = _STM32_DEV_ID(rcc_ahb1enr, 17),
	dev_dma2d = _STM32_DEV_ID(rcc_ahb1enr, 18),
	dev_gtzc1 = _STM32_DEV_ID(rcc_ahb1enr, 24),
	dev_bkpsram = _STM32_DEV_ID(rcc_ahb1enr, 28),
	dev_dcache1 = _STM32_DEV_ID(rcc_ahb1enr, 30),
	dev_sram1 = _STM32_DEV_ID(rcc_ahb1enr, 31),
	dev_gpioa = _STM32_DEV_ID(rcc_ahb2lenr, 0),
	dev_gpiob = _STM32_DEV_ID(rcc_ahb2lenr, 1),
	dev_gpioc = _STM32_DEV_ID(rcc_ahb2lenr, 2),
	dev_gpiod = _STM32_DEV_ID(rcc_ahb2lenr, 3),
	dev_gpioe = _STM32_DEV_ID(rcc_ahb2lenr, 4),
	dev_gpiof = _STM32_DEV_ID(rcc_ahb2lenr, 5),
	dev_gpiog = _STM32_DEV_ID(rcc_ahb2lenr, 6),
	dev_gpioh = _STM32_DEV_ID(rcc_ahb2lenr, 7),
	dev_gpioi = _STM32_DEV_ID(rcc_ahb2lenr, 8),
	dev_adc12 = _STM32_DEV_ID(rcc_ahb2lenr, 10),
	dev_dcmipssi = _STM32_DEV_ID(rcc_ahb2lenr, 12),
	dev_otg = _STM32_DEV_ID(rcc_ahb2lenr, 14),
	dev_aes = _STM32_DEV_ID(rcc_ahb2lenr, 16),
	dev_hash = _STM32_DEV_ID(rcc_ahb2lenr, 17),
	dev_rng = _STM32_DEV_ID(rcc_ahb2lenr, 18),
	dev_pka = _STM32_DEV_ID(rcc_ahb2lenr, 19),
	dev_saes = _STM32_DEV_ID(rcc_ahb2lenr, 20),
	dev_octospim = _STM32_DEV_ID(rcc_ahb2lenr, 21),
	dev_otfdec1 = _STM32_DEV_ID(rcc_ahb2lenr, 23),
	dev_otfdec2 = _STM32_DEV_ID(rcc_ahb2lenr, 24),
	dev_sdmmc1 = _STM32_DEV_ID(rcc_ahb2lenr, 27),
	dev_sdmmc2 = _STM32_DEV_ID(rcc_ahb2lenr, 28),
	dev_sram2 = _STM32_DEV_ID(rcc_ahb2lenr, 30),
	dev_sram3 = _STM32_DEV_ID(rcc_ahb2lenr, 31),
	dev_fsmc = _STM32_DEV_ID(rcc_ahb2henr, 0),
	dev_octospi1 = _STM32_DEV_ID(rcc_ahb2henr, 4),
	dev_octospi2 = _STM32_DEV_ID(rcc_ahb2henr, 8),
	dev_lpgpio1 = _STM32_DEV_ID(rcc_ahb3enr, 0),
	dev_pwr = _STM32_DEV_ID(rcc_ahb3enr, 2),
	dev_adc4 = _STM32_DEV_ID(rcc_ahb3enr, 5),
	dev_dac1 = _STM32_DEV_ID(rcc_ahb3enr, 6),
	dev_lpdma1 = _STM32_DEV_ID(rcc_ahb3enr, 9),
	dev_adf1 = _STM32_DEV_ID(rcc_ahb3enr, 10),
	dev_gtzc2 = _STM32_DEV_ID(rcc_ahb3enr, 12),
	dev_sram4 = _STM32_DEV_ID(rcc_ahb3enr, 31),
	dev_tim2 = _STM32_DEV_ID(rcc_apb1lenr, 0),
	dev_tim3 = _STM32_DEV_ID(rcc_apb1lenr, 1),
	dev_tim4 = _STM32_DEV_ID(rcc_apb1lenr, 2),
	dev_tim5 = _STM32_DEV_ID(rcc_apb1lenr, 3),
	dev_tim6 = _STM32_DEV_ID(rcc_apb1lenr, 4),
	dev_tim7 = _STM32_DEV_ID(rcc_apb1lenr, 5),
	dev_wwdg = _STM32_DEV_ID(rcc_apb1lenr, 11),
	dev_spi2 = _STM32_DEV_ID(rcc_apb1lenr, 14),
	dev_usart2 = _STM32_DEV_ID(rcc_apb1lenr, 17),
	dev_usart3 = _STM32_DEV_ID(rcc_apb1lenr, 18),
	dev_uart4 = _STM32_DEV_ID(rcc_apb1lenr, 19),
	dev_uart5 = _STM32_DEV_ID(rcc_apb1lenr, 20),
	dev_i2c1 = _STM32_DEV_ID(rcc_apb1lenr, 21),
	dev_i2c2 = _STM32_DEV_ID(rcc_apb1lenr, 22),
	dev_crs = _STM32_DEV_ID(rcc_apb1lenr, 24),
	dev_i2c4 = _STM32_DEV_ID(rcc_apb1henr, 1),
	dev_lptim2 = _STM32_DEV_ID(rcc_apb1henr, 5),
	dev_fdcan1 = _STM32_DEV_ID(rcc_apb1henr, 9),
	dev_ucpd1 = _STM32_DEV_ID(rcc_apb1henr, 23),
	dev_tim1 = _STM32_DEV_ID(rcc_apb2enr, 11),
	dev_spi1 = _STM32_DEV_ID(rcc_apb2enr, 12),
	dev_tim8 = _STM32_DEV_ID(rcc_apb2enr, 13),
	dev_usart1 = _STM32_DEV_ID(rcc_apb2enr, 14),
	dev_tim15 = _STM32_DEV_ID(rcc_apb2enr, 16),
	dev_tim16 = _STM32_DEV_ID(rcc_apb2enr, 17),
	dev_tim17 = _STM32_DEV_ID(rcc_apb2enr, 18),
	dev_sai1 = _STM32_DEV_ID(rcc_apb2enr, 21),
	dev_sai2 = _STM32_DEV_ID(rcc_apb2enr, 22),
	dev_syscfg = _STM32_DEV_ID(rcc_apb3enr, 1),
	dev_spi3 = _STM32_DEV_ID(rcc_apb3enr, 5),
	dev_lpuart1 = _STM32_DEV_ID(rcc_apb3enr, 6),
	dev_i2c3 = _STM32_DEV_ID(rcc_apb3enr, 7),
	dev_lptim1 = _STM32_DEV_ID(rcc_apb3enr, 11),
	dev_lptim3 = _STM32_DEV_ID(rcc_apb3enr, 12),
	dev_lptim4 = _STM32_DEV_ID(rcc_apb3enr, 13),
	dev_opamp = _STM32_DEV_ID(rcc_apb3enr, 14),
	dev_comp = _STM32_DEV_ID(rcc_apb3enr, 15),
	dev_vref = _STM32_DEV_ID(rcc_apb3enr, 20),
	dev_rtcapb = _STM32_DEV_ID(rcc_apb3enr, 21),
};


/* Device resets */
enum {
	dev_rst_gpdma1rst = _STM32_RST_ID(rcc_ahb1rstr, 0),
	dev_rst_cordicrst = _STM32_RST_ID(rcc_ahb1rstr, 1),
	dev_rst_fmacrst = _STM32_RST_ID(rcc_ahb1rstr, 2),
	dev_rst_mdf1rst = _STM32_RST_ID(rcc_ahb1rstr, 3),
	dev_rst_crcrst = _STM32_RST_ID(rcc_ahb1rstr, 12),
	dev_rst_tscrst = _STM32_RST_ID(rcc_ahb1rstr, 16),
	dev_rst_ramcfgrst = _STM32_RST_ID(rcc_ahb1rstr, 17),
	dev_rst_dma2drst = _STM32_RST_ID(rcc_ahb1rstr, 18),
	dev_rst_gpioarst = _STM32_RST_ID(rcc_ahb2lrstr, 0),
	dev_rst_gpiobrst = _STM32_RST_ID(rcc_ahb2lrstr, 1),
	dev_rst_gpiocrst = _STM32_RST_ID(rcc_ahb2lrstr, 2),
	dev_rst_gpiodrst = _STM32_RST_ID(rcc_ahb2lrstr, 3),
	dev_rst_gpioerst = _STM32_RST_ID(rcc_ahb2lrstr, 4),
	dev_rst_gpiofrst = _STM32_RST_ID(rcc_ahb2lrstr, 5),
	dev_rst_gpiogrst = _STM32_RST_ID(rcc_ahb2lrstr, 6),
	dev_rst_gpiohrst = _STM32_RST_ID(rcc_ahb2lrstr, 7),
	dev_rst_gpioirst = _STM32_RST_ID(rcc_ahb2lrstr, 8),
	dev_rst_adc12rst = _STM32_RST_ID(rcc_ahb2lrstr, 10),
	dev_rst_dcmipssirst = _STM32_RST_ID(rcc_ahb2lrstr, 12),
	dev_rst_otgrst = _STM32_RST_ID(rcc_ahb2lrstr, 14),
	dev_rst_aesrst = _STM32_RST_ID(rcc_ahb2lrstr, 16),
	dev_rst_hashrst = _STM32_RST_ID(rcc_ahb2lrstr, 17),
	dev_rst_rngrst = _STM32_RST_ID(rcc_ahb2lrstr, 18),
	dev_rst_pkarst = _STM32_RST_ID(rcc_ahb2lrstr, 19),
	dev_rst_saesrst = _STM32_RST_ID(rcc_ahb2lrstr, 20),
	dev_rst_octospimrst = _STM32_RST_ID(rcc_ahb2lrstr, 21),
	dev_rst_otfdec1rst = _STM32_RST_ID(rcc_ahb2lrstr, 23),
	dev_rst_otfdec2rst = _STM32_RST_ID(rcc_ahb2lrstr, 24),
	dev_rst_sdmmc1rst = _STM32_RST_ID(rcc_ahb2lrstr, 27),
	dev_rst_sdmmc2rst = _STM32_RST_ID(rcc_ahb2lrstr, 28),
	dev_rst_fsmcrst = _STM32_RST_ID(rcc_ahb2hrstr, 0),
	dev_rst_octospi1rst = _STM32_RST_ID(rcc_ahb2hrstr, 4),
	dev_rst_octospi2rst = _STM32_RST_ID(rcc_ahb2hrstr, 8),
	dev_rst_lpgpio1rst = _STM32_RST_ID(rcc_ahb3rstr, 0),
	dev_rst_adc4rst = _STM32_RST_ID(rcc_ahb3rstr, 5),
	dev_rst_dac1rst = _STM32_RST_ID(rcc_ahb3rstr, 6),
	dev_rst_lpdma1rst = _STM32_RST_ID(rcc_ahb3rstr, 9),
	dev_rst_adf1rst = _STM32_RST_ID(rcc_ahb3rstr, 10),
	dev_rst_tim2rst = _STM32_RST_ID(rcc_apb1lrstr, 0),
	dev_rst_tim3rst = _STM32_RST_ID(rcc_apb1lrstr, 1),
	dev_rst_tim4rst = _STM32_RST_ID(rcc_apb1lrstr, 2),
	dev_rst_tim5rst = _STM32_RST_ID(rcc_apb1lrstr, 3),
	dev_rst_tim6rst = _STM32_RST_ID(rcc_apb1lrstr, 4),
	dev_rst_tim7rst = _STM32_RST_ID(rcc_apb1lrstr, 5),
	dev_rst_spi2rst = _STM32_RST_ID(rcc_apb1lrstr, 14),
	dev_rst_usart2rst = _STM32_RST_ID(rcc_apb1lrstr, 17),
	dev_rst_usart3rst = _STM32_RST_ID(rcc_apb1lrstr, 18),
	dev_rst_uart4rst = _STM32_RST_ID(rcc_apb1lrstr, 19),
	dev_rst_uart5rst = _STM32_RST_ID(rcc_apb1lrstr, 20),
	dev_rst_i2c1rst = _STM32_RST_ID(rcc_apb1lrstr, 21),
	dev_rst_i2c2rst = _STM32_RST_ID(rcc_apb1lrstr, 22),
	dev_rst_crsrst = _STM32_RST_ID(rcc_apb1lrstr, 24),
	dev_rst_i2c4rst = _STM32_RST_ID(rcc_apb1hrstr, 1),
	dev_rst_lptim2rst = _STM32_RST_ID(rcc_apb1hrstr, 5),
	dev_rst_fdcan1rst = _STM32_RST_ID(rcc_apb1hrstr, 9),
	dev_rst_ucpd1rst = _STM32_RST_ID(rcc_apb1hrstr, 23),
	dev_rst_tim1rst = _STM32_RST_ID(rcc_apb2rstr, 11),
	dev_rst_spi1rst = _STM32_RST_ID(rcc_apb2rstr, 12),
	dev_rst_tim8rst = _STM32_RST_ID(rcc_apb2rstr, 13),
	dev_rst_usart1rst = _STM32_RST_ID(rcc_apb2rstr, 14),
	dev_rst_tim15rst = _STM32_RST_ID(rcc_apb2rstr, 16),
	dev_rst_tim16rst = _STM32_RST_ID(rcc_apb2rstr, 17),
	dev_rst_tim17rst = _STM32_RST_ID(rcc_apb2rstr, 18),
	dev_rst_sai1rst = _STM32_RST_ID(rcc_apb2rstr, 21),
	dev_rst_sai2rst = _STM32_RST_ID(rcc_apb2rstr, 22),
	dev_rst_syscfgrst = _STM32_RST_ID(rcc_apb3rstr, 1),
	dev_rst_spi3rst = _STM32_RST_ID(rcc_apb3rstr, 5),
	dev_rst_lpuart1rst = _STM32_RST_ID(rcc_apb3rstr, 6),
	dev_rst_i2c3rst = _STM32_RST_ID(rcc_apb3rstr, 7),
	dev_rst_lptim1rst = _STM32_RST_ID(rcc_apb3rstr, 11),
	dev_rst_lptim3rst = _STM32_RST_ID(rcc_apb3rstr, 12),
	dev_rst_lptim4rst = _STM32_RST_ID(rcc_apb3rstr, 13),
	dev_rst_opamprst = _STM32_RST_ID(rcc_apb3rstr, 14),
	dev_rst_comprst = _STM32_RST_ID(rcc_apb3rstr, 15),
	dev_rst_vrefrst = _STM32_RST_ID(rcc_apb3rstr, 20),
};


enum ipclks {
	ipclk_usart1sel = 0,
	ipclk_usart2sel,
	ipclk_usart3sel,
	ipclk_uart4sel,
	ipclk_uart5sel,
	ipclk_i2c1sel,
	ipclk_i2c2sel,
	ipclk_i2c4sel,
	ipclk_spi2sel,
	ipclk_lptim2sel,
	ipclk_spi1sel,
	ipclk_systicksel,
	ipclk_fdcansel,
	ipclk_iclksel,
	ipclk_timicsel,
	ipclk_mdf1sel,
	ipclk_sai1sel,
	ipclk_sai2sel,
	ipclk_saessel,
	ipclk_rngsel,
	ipclk_sdmmcsel,
	ipclk_octospisel,
	ipclk_lpuart1sel,
	ipclk_spi3sel,
	ipclk_i2c3sel,
	ipclk_lptim34sel,
	ipclk_lptim1sel,
	ipclk_adcdacsel,
	ipclk_dac1sel,
	ipclk_adf1sel,
	ipclks_count,
};


enum pwr_supplies {
	pwr_supply_vddio2 = 0,
	pwr_supply_vusb,
	pwr_supply_vdda,
	pwr_supplies_count,
};


enum pwr_supply_ops {
	pwr_supply_op_monitor_enable = 0,
	pwr_supply_op_valid,
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


extern int _stm32_rccSetCPUClock(u32 hz);


/* Sets independent peripheral clock configuration */
extern int _stm32_rccSetIPClk(unsigned int ipclk, u8 setting);


/* Get frequency of CPU clock in Hz */
extern u32 _stm32_rccGetCPUClock(void);


extern void _stm32_rccClearResetFlags(void);


extern int _stm32_gpioConfig(unsigned int d, u8 pin, u8 mode, u8 af, u8 otype, u8 ospeed, u8 pupd);


/* Perform an operation (pwr_supply_op_*) on a selected power supply (pwr_supply_*) */
extern int _stm32_pwrSupplyOperation(unsigned int supply, unsigned int operation, int status);


extern u8 _stm32_pwrGetCPUVolt(void);


extern void _stm32_pwrSetCPUVolt(u8 range);


extern int _stm32_systickInit(u32 interval);


extern void _stm32_systickDone(void);


extern void _stm32_wdgReload(void);


extern void _stm32_init(void);

#endif
