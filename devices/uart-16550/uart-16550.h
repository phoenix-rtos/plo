/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * UART 16550
 *
 * Copyright 2012-2015, 2020, 2021 Phoenix Systems
 * Copyright 2001, 2005, 2008 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Pawel Kolodziej, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _DEV_UART16550_H_
#define _DEV_UART16550_H_


/* Receiver and transmitter buffer size */
#define SIZE_RX 0x800
#define SIZE_TX 0x800


/* UART 16550 registers */
enum {
	rbr = 0, /* Receiver Buffer Register */
	thr = 0, /* Transmitter Holding Register */
	dll = 0, /* Divisor Latch LSB */
	ier = 1, /* Interrupt Enable Register */
	dlm = 1, /* Divisor Latch MSB */
	iir = 2, /* Interrupt Identification Register */
	fcr = 2, /* FIFO Control Register */
	lcr = 3, /* Line Control Register */
	mcr = 4, /* Modem Control Register */
	lsr = 5, /* Line Status Register */
	msr = 6, /* Modem Status Register */
	spr = 7, /* Scratch Pad Register */
};


/* Baudrates (index to bauds table) */
enum {
	bps_50,
	bps_75,
	bps_110,
	bps_135,
	bps_150,
	bps_300,
	bps_600,
	bps_1200,
	bps_1800,
	bps_2000,
	bps_2400,
	bps_3600,
	bps_4800,
	bps_7200,
	bps_9600,
	bps_19200,
	bps_28800,
	bps_31250,
	bps_38400,
	bps_57600,
	bps_115200,
	bps_230400,
	bps_460800,
	bps_921600,
	bps_1500000
};


typedef struct {
	unsigned int speed;
	unsigned int divisor;
	unsigned char mode;
} baud_t;


/* Baudrates table */
extern const baud_t bauds[];


#endif
