/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ull qspi lookup table defs
 *
 * Copyright 2021, 2023 Phoenix Systems
 * Author: Gerard Swiderski, Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _QSPI_LUT_H_
#define _QSPI_LUT_H_


#define LUT_OPCODE(cmd) ((cmd & 0x3fu) << 10)
#define LUT_PAD(pad)    ((pad & 0x3u) << 8)
#define LUT_OPERAND(op) (op & 0xffu)

#define LUT_SEQ(cmd0, pad0, op0, cmd1, pad1, op1) ( \
	((LUT_OPCODE(cmd1) | LUT_PAD(pad1) | LUT_OPERAND(op1)) << 16) | \
	((LUT_OPCODE(cmd0) | LUT_PAD(pad0) | LUT_OPERAND(op0))))

/* TODO: Use command enum (IDX/NUM sequence mapping) as per device implementation */
#define LUT_SEQIDX(seq) (seq)


enum {
	/* Set of Single Data Rate LUT commands */
	lutCmdADDR_SDR = 2,  /* Transmit Address to Flash, using SDR mode. */
	lutCmdMODE1_SDR = 4, /* Transmit 1bit bits to Flash, using SDR mode. */
	lutCmdMODE2_SDR = 5, /* Transmit 2bit bits to Flash, using SDR mode. */
	lutCmdMODE4_SDR = 6, /* Transmit 4bit bits to Flash, using SDR mode. */
	lutCmdREAD_SDR = 7,  /* Receive Read Data from Flash, using SDR mode. */
	lutCmdWRITE_SDR = 8, /* Transmit Programming Data to Flash, using SDR mode. */

	/* Set of Double Data Rate commands */
	lutCmdADDR_DDR = 10,  /* Transmit Address to Flash, using DDR mode. */
	lutCmdMODE1_DDR = 11, /* Transmit 1bit bits to Flash, using DDR mode. */
	lutCmdMODE2_DDR = 12, /* Transmit 2bit bits to Flash, using DDR mode. */
	lutCmdMODE4_DDR = 13, /* Transmit 4bit bits to Flash, using DDR mode. */
	lutCmdREAD_DDR = 14,  /* Receive Read Data from Flash, using DDR mode. */
	lutCmdWRITE_DDR = 15, /* Transmit Programming Data to Flash, using DDR mode. */

	/* Set of sequence flow control commands */
	lutCmdSTOP = 0,       /* Stop execution, deassert CS.*/
	lutCmd = 1,           /* Transmit Command code to Flash.*/
	lutCmdDUMMY = 3,      /* Leave data lines undriven */
	lutCmdJUMP_ON_CS = 9, /* STOP; deassert CS; save operand for next sequence (use with enhanced XIP) */
	lutCmdLEARN = 19,     /* Receive Read Data or Preamble bit from Flash(not implemented on this chip). */
};


/* Number of pads used to transmit/receive, use to form LUT instruction */
enum {
	lutPad1 = 0x00, /* single SPI: transmit/receive using DATA0/DATA1 wires */
	lutPad2 = 0x01, /* dual SPI: transmit/receive using DATA[1:0] wires */
	lutPad4 = 0x02, /* quad SPI: transmit/receive using DATA[3:0] wires */
};


#endif /* _QSPI_LUT_H_ */
