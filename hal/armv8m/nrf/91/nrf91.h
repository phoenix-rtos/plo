/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * nRF9160 basic peripherals control functions
 *
 * Copyright 2022 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_NRF91_H_
#define _HAL_NRF91_H_

#include "../types.h"


/* clang-format off */
enum { gpio_input = 0, gpio_output };
enum { gpio_low = 0, gpio_high };
enum { gpio_nopull = 0, gpio_pulldown, gpio_pullup = 3};


/* nRF9160 peripheral id's - same as irq numbers */
enum { spu = 3, regulators, clock = 5, power = 5, ctrlapperi, spi0 = 8, twi0 = 8, uarte0 = 8,
	spi1 = 9, twi1 = 9, uarte1 = 9, spi2 = 10, twi2 = 10, uarte2 = 10, spi3 = 11, twi3 = 11, uarte3 = 11,
	gpiote0 = 13, saadc, timer0, timer1, timer2, rtc0 = 20, rtc1, ddpic = 23, wdt,
	egu0 = 27, egu1, egu2, egu3, egu4, egu5, pwm0, pwm1, pwm2, pwm3, pdm = 38,
	i2s = 40, ipc = 42, fpu = 44, gpiote1 = 49, kmu = 57, nvmc = 57, vmc, cc_host_rgf = 64,
	cryptocell = 64, gpio = 66 };
/* clang-format on */


extern int _nrf91_gpioConfig(u8 pin, u8 dir, u8 pull);


extern int _nrf91_gpioSet(u8 pin, u8 val);


extern void _nrf91_timerClearEvent(void);


extern void _nrf91_timerDone(void);


extern void _nrf91_init(void);

#endif
