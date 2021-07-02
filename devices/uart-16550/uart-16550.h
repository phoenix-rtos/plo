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
	RBR = 0, /* Receiver Buffer Register */
	THR = 0, /* Transmitter Holding Register */
	DLL = 0, /* Divisor Latch LSB */
	IER = 1, /* Interrupt Enable Register */
	DLM = 1, /* Divisor Latch MSB */
	IIR = 2, /* Interrupt Identification Register */
	FCR = 2, /* FIFO Control Register */
	LCR = 3, /* Line Control Register */
	MCR = 4, /* Modem Control Register */
	LSR = 5, /* Line Status Register */
	MSR = 6, /* Modem Status Register */
	SPR = 7, /* Scratch Pad Register */
};


/* Baudrates (index to bauds table) */
enum {
	BPS_50,     BPS_75,     BPS_110,    BPS_135,    BPS_150,
	BPS_300,    BPS_600,    BPS_1200,   BPS_1800,   BPS_2000,
	BPS_2400,   BPS_3600,   BPS_4800,   BPS_7200,   BPS_9600,
	BPS_19200,  BPS_28800,  BPS_31250,  BPS_38400,  BPS_57600,
	BPS_115200, BPS_230400, BPS_460800, BPS_921600, BPS_1500000
};


typedef struct {
	unsigned int speed;
	unsigned int divisor;
	unsigned char mode;
} baud_t;


/* Baudrates table */
extern const baud_t bauds[];


#endif
