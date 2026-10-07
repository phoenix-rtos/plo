/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * zynq-7000 basic peripherals control functions
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _ZYNQ_H_
#define _ZYNQ_H_


#include "types.h"


enum { clk_disable = 0, clk_enable };


/* AMBA peripherals */
enum {
	amba_dma_clk = 0, amba_usb0_clk = 2, amba_usb1_clk, amba_gem0_clk = 6, amba_gem1_clk, amba_sdi0_clk = 10, amba_sdi1_clk,
	amba_spi0_clk = 14, amba_spi1_clk, amba_can0_clk, amba_can1_clk, amba_i2c0_clk, amba_i2c1_clk, amba_uart0_clk,
	amba_uart1_clk, amba_gpio_clk, amba_lqspi_clk, amba_smc_clk
};


/* Devices' clocks controllers */
enum {
	ctrl_usb0_clk = 0, ctrl_usb1_clk, ctrl_gem0_rclk, ctrl_gem1_rclk, ctrl_gem0_clk, ctrl_gem1_clk, ctrl_smc_clk,
	ctrl_lqspi_clk, ctrl_sdio_clk, ctrl_uart_clk, ctrl_spi_clk, ctrl_can_clk, ctrl_can_mioclk,
};


enum {
	mio_pin_00 = 0, mio_pin_01, mio_pin_02, mio_pin_03, mio_pin_04, mio_pin_05, mio_pin_06, mio_pin_07, mio_pin_08,
	mio_pin_09, mio_pin_10, mio_pin_11, mio_pin_12, mio_pin_13, mio_pin_14, mio_pin_15, mio_pin_16, mio_pin_17,
	mio_pin_18, mio_pin_19, mio_pin_20, mio_pin_21, mio_pin_22, mio_pin_23, mio_pin_24, mio_pin_25, mio_pin_26,
	mio_pin_27, mio_pin_28, mio_pin_29, mio_pin_30, mio_pin_31, mio_pin_32, mio_pin_33, mio_pin_34, mio_pin_35,
	mio_pin_36, mio_pin_37, mio_pin_38, mio_pin_39, mio_pin_40, mio_pin_41, mio_pin_42, mio_pin_43, mio_pin_44,
	mio_pin_45, mio_pin_46, mio_pin_47, mio_pin_48, mio_pin_49, mio_pin_50, mio_pin_51, mio_pin_52, mio_pin_53,
};


typedef struct {
	int dev;
	union {
		struct {
			char divisor1;
			char divisor0;
			char srcsel;
			char clkact1;
			char clkact0;
		} pll;

		struct {
			char ref1;
			char mux1;
			char ref0;
			char mux0;
		} mio;
	};
} ctl_clock_t;


typedef struct {
	int pin;
	char disableRcvr;
	char pullup;
	char ioType;
	char speed;
	char l3;
	char l2;
	char l1;
	char l0;
	char triEnable;
} ctl_mio_t;


/* Function disables or enables device in AMBA clock controler  */
extern int _zynq_setAmbaClk(u32 dev, u32 state);


/* Function returns state of device in AMBA clock controler     */
extern int _zynq_getAmbaClk(u32 dev, u32 *state);


/* Function sets device's clock configuration                   */
extern int _zynq_setCtlClock(const ctl_clock_t *clk);


/* Function returns device's clock configuration                */
extern int _zynq_getCtlClock(ctl_clock_t *clk);


/* Function sets MIO's configuration                            */
extern int _zynq_setMIO(const ctl_mio_t *mio);


/* Function returns MIO's configuration                         */
extern int _zynq_getMIO(ctl_mio_t *mio);


/* Function sets WP and CD pins for the SDIO controller */
extern int _zynq_setSDWpCd(char dev, unsigned char wpPin, unsigned char cdPin);


/* Function returns WP and CD pins that were set for the SDIO controller */
extern int _zynq_getSDWpCd(char dev, unsigned char *wpPin, unsigned char *cdPin);


/* Function loads bitstream data from specific memory address   */
extern int _zynq_loadPL(u32 srcAddr, u32 srcLen);


/* Processing System software reset control signal. */
extern void _zynq_softRst(void);


/* Function initializes plls, clocks, ddr and basic peripherals */
extern void _zynq_init(void);


#endif
