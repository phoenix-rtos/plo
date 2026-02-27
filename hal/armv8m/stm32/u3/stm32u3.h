/*
 * Phoenix-RTOS
 *
 * Operating system kernel
 *
 * STM32U3 basic peripherals control functions
 *
 * Copyright 2017, 2020, 2021, 2025 Phoenix Systems
 * Copyright 2026 Apator Metrix
 * Author: Aleksander Kaminski, Pawel Pisarczyk, Hubert Buczynski, Jacek Maksymowicz, Mateusz Karcz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_STM32U3_H_
#define _HAL_STM32U3_H_

#include "../types.h"
#include "stm32u3_regs.h"

#define _STM32_ID(base, reg, pos) ((((reg) - (base)) << 5) | (pos))
#define _STM32_DEV_ID(reg, pos)   _STM32_ID(rcc_ahb1enr1, reg, pos)
#define _STM32_RST_ID(reg, pos)   _STM32_ID(rcc_ahb1rstr1, reg, pos)

/* Device clocks */
enum {
	dev_gpdma1 = _STM32_DEV_ID(rcc_ahb1enr1, 0),
	dev_adf1 = _STM32_DEV_ID(rcc_ahb1enr1, 3),
	dev_hsp1 = _STM32_DEV_ID(rcc_ahb1enr1, 4),
	dev_flash = _STM32_DEV_ID(rcc_ahb1enr1, 8),
	dev_crc = _STM32_DEV_ID(rcc_ahb1enr1, 12),
	dev_tsc = _STM32_DEV_ID(rcc_ahb1enr1, 16),
	dev_ramcfg = _STM32_DEV_ID(rcc_ahb1enr1, 17),
	dev_gtzc1 = _STM32_DEV_ID(rcc_ahb1enr1, 24),
	dev_sram4 = _STM32_DEV_ID(rcc_ahb1enr1, 30),
	dev_sram1 = _STM32_DEV_ID(rcc_ahb1enr1, 31),
	dev_gpioa = _STM32_DEV_ID(rcc_ahb2enr1, 0),
	dev_gpiob = _STM32_DEV_ID(rcc_ahb2enr1, 1),
	dev_gpioc = _STM32_DEV_ID(rcc_ahb2enr1, 2),
	dev_gpiod = _STM32_DEV_ID(rcc_ahb2enr1, 3),
	dev_gpioe = _STM32_DEV_ID(rcc_ahb2enr1, 4),
	dev_gpiof = _STM32_DEV_ID(rcc_ahb2enr1, 5),
	dev_gpiog = _STM32_DEV_ID(rcc_ahb2enr1, 6),
	dev_gpioh = _STM32_DEV_ID(rcc_ahb2enr1, 7),
	dev_adc12 = _STM32_DEV_ID(rcc_ahb2enr1, 10),
	dev_dac1 = _STM32_DEV_ID(rcc_ahb2enr1, 11),
	dev_aes = _STM32_DEV_ID(rcc_ahb2enr1, 16),
	dev_hash = _STM32_DEV_ID(rcc_ahb2enr1, 17),
	dev_rng = _STM32_DEV_ID(rcc_ahb2enr1, 18),
	dev_pka = _STM32_DEV_ID(rcc_ahb2enr1, 19),
	dev_saes = _STM32_DEV_ID(rcc_ahb2enr1, 20),
	dev_ccb = _STM32_DEV_ID(rcc_ahb2enr1, 21),
	dev_sdmmc1 = _STM32_DEV_ID(rcc_ahb2enr1, 27),
	dev_sram2 = _STM32_DEV_ID(rcc_ahb2enr1, 30),
	dev_sram3 = _STM32_DEV_ID(rcc_ahb2enr1, 31),
	dev_octospi1 = _STM32_DEV_ID(rcc_ahb2enr2, 4),
	dev_pwr = _STM32_DEV_ID(rcc_ahb1enr2, 2),
	dev_tim2 = _STM32_DEV_ID(rcc_apb1enr1, 0),
	dev_tim3 = _STM32_DEV_ID(rcc_apb1enr1, 1),
	dev_tim4 = _STM32_DEV_ID(rcc_apb1enr1, 2),
	dev_tim6 = _STM32_DEV_ID(rcc_apb1enr1, 4),
	dev_tim7 = _STM32_DEV_ID(rcc_apb1enr1, 5),
	dev_spi3 = _STM32_DEV_ID(rcc_apb1enr1, 8),
	dev_spi4 = _STM32_DEV_ID(rcc_apb1enr1, 9),
	dev_wwdg = _STM32_DEV_ID(rcc_apb1enr1, 11),
	dev_spi2 = _STM32_DEV_ID(rcc_apb1enr1, 14),
	dev_usart2 = _STM32_DEV_ID(rcc_apb1enr1, 17),
	dev_usart3 = _STM32_DEV_ID(rcc_apb1enr1, 18),
	dev_uart4 = _STM32_DEV_ID(rcc_apb1enr1, 19),
	dev_uart5 = _STM32_DEV_ID(rcc_apb1enr1, 20),
	dev_i2c1 = _STM32_DEV_ID(rcc_apb1enr1, 21),
	dev_i2c2 = _STM32_DEV_ID(rcc_apb1enr1, 22),
	dev_i3c1 = _STM32_DEV_ID(rcc_apb1enr1, 23),
	dev_crs = _STM32_DEV_ID(rcc_apb1enr1, 24),
	dev_opamp = _STM32_DEV_ID(rcc_apb1enr1, 28),
	dev_vref = _STM32_DEV_ID(rcc_apb1enr1, 29),
	dev_rtcapb = _STM32_DEV_ID(rcc_apb1enr1, 30),
	dev_i2c4 = _STM32_DEV_ID(rcc_apb1enr2, 1),
	dev_lptim2 = _STM32_DEV_ID(rcc_apb1enr2, 5),
	dev_fdcan = _STM32_DEV_ID(rcc_apb1enr2, 9),
	dev_tim1 = _STM32_DEV_ID(rcc_apb2enr, 11),
	dev_spi1 = _STM32_DEV_ID(rcc_apb2enr, 12),
	dev_tim8 = _STM32_DEV_ID(rcc_apb2enr, 13),
	dev_usart1 = _STM32_DEV_ID(rcc_apb2enr, 14),
	dev_tim12 = _STM32_DEV_ID(rcc_apb2enr, 15),
	dev_tim15 = _STM32_DEV_ID(rcc_apb2enr, 16),
	dev_tim16 = _STM32_DEV_ID(rcc_apb2enr, 17),
	dev_tim17 = _STM32_DEV_ID(rcc_apb2enr, 18),
	dev_sai1 = _STM32_DEV_ID(rcc_apb2enr, 21),
	dev_usb1 = _STM32_DEV_ID(rcc_apb2enr, 24),
	dev_i3c2 = _STM32_DEV_ID(rcc_apb2enr, 27),
	dev_syscfg = _STM32_DEV_ID(rcc_apb3enr, 1),
	dev_lpuart1 = _STM32_DEV_ID(rcc_apb3enr, 6),
	dev_i2c3 = _STM32_DEV_ID(rcc_apb3enr, 7),
	dev_lptim1 = _STM32_DEV_ID(rcc_apb3enr, 11),
	dev_lptim3 = _STM32_DEV_ID(rcc_apb3enr, 12),
	dev_lptim4 = _STM32_DEV_ID(rcc_apb3enr, 13),
	dev_comp = _STM32_DEV_ID(rcc_apb3enr, 15),
};


/* Device resets */
enum {
	dev_rst_gpdma1 = _STM32_RST_ID(rcc_ahb1rstr1, 0),
	dev_rst_adf1 = _STM32_RST_ID(rcc_ahb1rstr1, 3),
	dev_rst_hsp1 = _STM32_RST_ID(rcc_ahb1rstr1, 4),
	dev_rst_crc = _STM32_RST_ID(rcc_ahb1rstr1, 12),
	dev_rst_tsc = _STM32_RST_ID(rcc_ahb1rstr1, 16),
	dev_rst_ramcfg = _STM32_RST_ID(rcc_ahb1rstr1, 17),
	dev_rst_gpioa = _STM32_RST_ID(rcc_ahb2rstr1, 0),
	dev_rst_gpiob = _STM32_RST_ID(rcc_ahb2rstr1, 1),
	dev_rst_gpioc = _STM32_RST_ID(rcc_ahb2rstr1, 2),
	dev_rst_gpiod = _STM32_RST_ID(rcc_ahb2rstr1, 3),
	dev_rst_gpioe = _STM32_RST_ID(rcc_ahb2rstr1, 4),
	dev_rst_gpiof = _STM32_RST_ID(rcc_ahb2rstr1, 5),
	dev_rst_gpiog = _STM32_RST_ID(rcc_ahb2rstr1, 6),
	dev_rst_gpioh = _STM32_RST_ID(rcc_ahb2rstr1, 7),
	dev_rst_adc12 = _STM32_RST_ID(rcc_ahb2rstr1, 10),
	dev_rst_dac1 = _STM32_RST_ID(rcc_ahb2rstr1, 11),
	dev_rst_aes = _STM32_RST_ID(rcc_ahb2rstr1, 16),
	dev_rst_hash = _STM32_RST_ID(rcc_ahb2rstr1, 17),
	dev_rst_rng = _STM32_RST_ID(rcc_ahb2rstr1, 18),
	dev_rst_pka = _STM32_RST_ID(rcc_ahb2rstr1, 19),
	dev_rst_saes = _STM32_RST_ID(rcc_ahb2rstr1, 20),
	dev_rst_ccb = _STM32_RST_ID(rcc_ahb2rstr1, 21),
	dev_rst_sdmmc1 = _STM32_RST_ID(rcc_ahb2rstr1, 27),
	dev_rst_octospi1 = _STM32_RST_ID(rcc_ahb2rstr2, 4),
	dev_rst_tim2 = _STM32_RST_ID(rcc_apb1rstr1, 0),
	dev_rst_tim3 = _STM32_RST_ID(rcc_apb1rstr1, 1),
	dev_rst_tim4 = _STM32_RST_ID(rcc_apb1rstr1, 2),
	dev_rst_tim6 = _STM32_RST_ID(rcc_apb1rstr1, 4),
	dev_rst_tim7 = _STM32_RST_ID(rcc_apb1rstr1, 5),
	dev_rst_spi3 = _STM32_RST_ID(rcc_apb1rstr1, 8),
	dev_rst_spi4 = _STM32_RST_ID(rcc_apb1rstr1, 9),
	dev_rst_spi2 = _STM32_RST_ID(rcc_apb1rstr1, 14),
	dev_rst_usart2 = _STM32_RST_ID(rcc_apb1rstr1, 17),
	dev_rst_usart3 = _STM32_RST_ID(rcc_apb1rstr1, 18),
	dev_rst_uart4 = _STM32_RST_ID(rcc_apb1rstr1, 19),
	dev_rst_uart5 = _STM32_RST_ID(rcc_apb1rstr1, 20),
	dev_rst_i2c1 = _STM32_RST_ID(rcc_apb1rstr1, 21),
	dev_rst_i2c2 = _STM32_RST_ID(rcc_apb1rstr1, 22),
	dev_rst_i3c1 = _STM32_RST_ID(rcc_apb1rstr1, 23),
	dev_rst_crs = _STM32_RST_ID(rcc_apb1rstr1, 24),
	dev_rst_opamp = _STM32_RST_ID(rcc_apb1rstr1, 28),
	dev_rst_vref = _STM32_RST_ID(rcc_apb1rstr1, 29),
	dev_rst_i2c4 = _STM32_RST_ID(rcc_apb1rstr2, 1),
	dev_rst_lptim2 = _STM32_RST_ID(rcc_apb1rstr2, 5),
	dev_rst_fdcan = _STM32_RST_ID(rcc_apb1rstr2, 9),
	dev_rst_tim1 = _STM32_RST_ID(rcc_apb2rstr, 11),
	dev_rst_spi1 = _STM32_RST_ID(rcc_apb2rstr, 12),
	dev_rst_tim8 = _STM32_RST_ID(rcc_apb2rstr, 13),
	dev_rst_usart1 = _STM32_RST_ID(rcc_apb2rstr, 14),
	dev_rst_tim12 = _STM32_RST_ID(rcc_apb2rstr, 15),
	dev_rst_tim15 = _STM32_RST_ID(rcc_apb2rstr, 16),
	dev_rst_tim16 = _STM32_RST_ID(rcc_apb2rstr, 17),
	dev_rst_tim17 = _STM32_RST_ID(rcc_apb2rstr, 18),
	dev_rst_sai1 = _STM32_RST_ID(rcc_apb2rstr, 21),
	dev_rst_usb1 = _STM32_RST_ID(rcc_apb2rstr, 24),
	dev_rst_i3c2 = _STM32_RST_ID(rcc_apb2rstr, 27),
	dev_rst_syscfg = _STM32_RST_ID(rcc_apb3rstr, 1),
	dev_rst_lpuart1 = _STM32_RST_ID(rcc_apb3rstr, 6),
	dev_rst_i2c3 = _STM32_RST_ID(rcc_apb3rstr, 7),
	dev_rst_lptim1 = _STM32_RST_ID(rcc_apb3rstr, 11),
	dev_rst_lptim3 = _STM32_RST_ID(rcc_apb3rstr, 12),
	dev_rst_lptim4 = _STM32_RST_ID(rcc_apb3rstr, 13),
	dev_rst_comp = _STM32_RST_ID(rcc_apb3rstr, 15),
};


enum ipclks {
	ipclk_usart1sel = 0,
	ipclk_usart3sel,
	ipclk_uart4sel,
	ipclk_uart5sel,
	ipclk_i3c1sel,
	ipclk_i2c1sel,
	ipclk_i2c2sel,
	ipclk_i3c2sel,
	ipclk_spi2sel,
	ipclk_lptim2sel,
	ipclk_spi1sel,
	ipclk_systicksel,
	ipclk_fdcansel,
	ipclk_iclksel,
	ipclk_usb1sel,
	ipclk_timicsel,
	ipclk_adf1sel,
	ipclk_spi3sel,
	ipclk_sai1sel,
	ipclk_spi4sel,
	ipclk_i2c4sel,
	ipclk_rngsel,
	ipclk_adcdacsel,
	ipclk_dac1shsel,
	ipclk_octospisel,
	ipclk_usart2sel,
	ipclk_lpuart1sel,
	ipclk_i2c3sel,
	ipclk_lptim34sel,
	ipclk_lptim1sel,
	ipclks_count
};


enum pwr_supplies {
	pwr_supply_vddio2 = 0,
	pwr_supply_vusb,
	pwr_supply_vdda,
	pwr_supplies_count,
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


/* Sets independent peripheral clock configuration */
extern int _stm32_rccSetIPClk(unsigned int ipclk, u8 setting);


/* Get frequency of CPU clock in Hz */
extern u32 _stm32_rccGetCPUClock(void);


extern void _stm32_rccClearResetFlags(void);


/* Get frequency of the PCLKx clock in Hz */
extern u32 _stm32_rccGetPclkClock(void);


extern int _stm32_gpioConfig(unsigned int d, u8 pin, u8 mode, u8 af, u8 otype, u8 ospeed, u8 pupd);


extern int _stm32_systickInit(u32 interval);


extern void _stm32_systickDone(void);


extern void _stm32_wdgReload(void);


extern void _stm32_init(void);

#endif /* _HAL_STM32U3_H_ */
