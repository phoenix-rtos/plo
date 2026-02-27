/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Peripheral register definitions for STM32U3 platform
 * Based on stm32u3c5xx.h by STMicroelectronics
 *
 * Copyright 2026 Apator Metrix
 * Authors: Mateusz Karcz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _STM32U3_REGS_H_
#define _STM32U3_REGS_H_

enum rcc_regs {
	rcc_cr = 0x0,
	rcc_icscr1 = 0x2,
	rcc_icscr2,
	rcc_icscr3,
	rcc_crrcr,
	rcc_cfgr1 = 0x7,
	rcc_cfgr2,
	rcc_cfgr3,
	rcc_cfgr4,
	rcc_cier = 0x14,
	rcc_cifr,
	rcc_cicr,
	rcc_ahb1rstr1 = 0x18,
	rcc_ahb2rstr1,
	rcc_ahb2rstr2,
	rcc_apb1rstr1 = 0x1d,
	rcc_apb1rstr2,
	rcc_apb2rstr,
	rcc_apb3rstr,
	rcc_ahb1enr1 = 0x22,
	rcc_ahb2enr1,
	rcc_ahb2enr2,
	rcc_ahb1enr2,
	rcc_apb1enr1 = 0x27,
	rcc_apb1enr2,
	rcc_apb2enr,
	rcc_apb3enr,
	rcc_ahb1slpenr1 = 0x2c,
	rcc_ahb2slpenr1,
	rcc_ahb2slpenr2,
	rcc_ahb1slpenr2,
	rcc_apb1slpenr1 = 0x31,
	rcc_apb1slpenr2,
	rcc_apb2slpenr,
	rcc_apb3slpenr,
	rcc_ahb1stpenr1 = 0x36,
	rcc_ahb2stpenr1,
	rcc_apb1stpenr1 = 0x3b,
	rcc_apb1stpenr2,
	rcc_apb2stpenr,
	rcc_apb3stpenr,
	rcc_ccipr1 = 0x40,
	rcc_ccipr2,
	rcc_ccipr3,
	rcc_bdcr = 0x44,
	rcc_csr,
	rcc_seccfgr = 0x4c,
	rcc_privcfgr,
};


enum gpio_regs {
	gpio_moder = 0x0,
	gpio_otyper,
	gpio_ospeedr,
	gpio_pupdr,
	gpio_idr,
	gpio_odr,
	gpio_bsrr,
	gpio_lckr,
	gpio_afrl,
	gpio_afrh,
	gpio_brr,
	gpio_hslvr,
	gpio_seccfgr,
};


enum pwr_regs {
	pwr_cr1 = 0x0,
	pwr_cr2,
	pwr_cr3,
	pwr_vosr,
	pwr_svmcr,
	pwr_wucr1,
	pwr_wucr2,
	pwr_wucr3,
	pwr_bdcr = 0x9,
	pwr_dbpr,
	pwr_seccfgr = 0xc,
	pwr_privcfgr,
	pwr_sr,
	pwr_svmsr,
	pwr_wusr = 0x11,
	pwr_wuscr,
	pwr_apcr,
	pwr_pucra,
	pwr_pdcra,
	pwr_pucrb,
	pwr_pdcrb,
	pwr_pucrc,
	pwr_pdcrc,
	pwr_pucrd,
	pwr_pdcrd,
	pwr_pucre,
	pwr_pdcre,
	pwr_pucrg = 0x20,
	pwr_pdcrg,
	pwr_pucrh,
	pwr_pdcrh,
	pwr_i3cpucr1 = 0x2c,
	pwr_i3cpucr2,
};


enum iwdg_regs {
	iwdg_kr = 0x0,
	iwdg_pr,
	iwdg_rlr,
	iwdg_sr,
	iwdg_winr,
	iwdg_ewcr,
};

enum syscfg_regs {
	syscfg_seccfgr = 0x0,
	syscfg_cfgr1,
	syscfg_fpuimr,
	syscfg_cnslckr,
	syscfg_cslckr,
	syscfg_cfgr2,
	syscfg_cccsr = 0x7,
	syscfg_ccvr,
	syscfg_cccr,
	syscfg_rsscmdr = 0xb,
};

enum ramcfg_regs {
	ramcfg_sram1cr = 0x0,
	ramcfg_sram1ier,
	ramcfg_sram1isr,
	ramcfg_sram1pear = 0x4,
	ramcfg_sram1icr,
	ramcfg_sram1wpr1,
	ramcfg_sram1wpr2,
	ramcfg_sram1parkeyr = 0x9,
	ramcfg_sram1erkeyr,
	ramcfg_sram2cr = 0x10,
	ramcfg_sram2ier,
	ramcfg_sram2isr,
	ramcfg_sram2pear = 0x14,
	ramcfg_sram2icr,
	ramcfg_sram2wpr1,
	ramcfg_sram2wpr2,
	ramcfg_sram2parkeyr = 0x19,
	ramcfg_sram2erkeyr,
	ramcfg_sram3cr = 0x20,
	ramcfg_sram3ier,
	ramcfg_sram3isr,
	ramcfg_sram3pear = 0x24,
	ramcfg_sram3icr,
	ramcfg_sram3wpr1,
	ramcfg_sram3wpr2,
	ramcfg_sram3parkeyr = 0x29,
	ramcfg_sram3erkeyr,
};

#endif /* _STM32U3_REGS_H_ */
