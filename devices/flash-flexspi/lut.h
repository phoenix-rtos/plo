/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX RT FlexSPI lookup table defs
 *
 * Copyright 2021 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _FLEXSPI_LUT_H_
#define _FLEXSPI_LUT_H_

#define LUT_ENTRIES 16
#define LUT_SEQSZ   4

#define LUT_OPCODE(cmd) ((cmd & 0x3f) << 10)
#define LUT_PAD(pad)    ((pad & 0x3) << 8)
#define LUT_OPERAND(op) (op & 0xff)

#define LUT_SEQ(cmd0, pad0, op0, cmd1, pad1, op1) ( \
	((LUT_OPCODE(cmd1) | LUT_PAD(pad1) | LUT_OPERAND(op1)) << 16) | \
	((LUT_OPCODE(cmd0) | LUT_PAD(pad0) | LUT_OPERAND(op0))))

/* TODO: Use command enum (IDX/NUM sequence mapping) as per device implementation */
#define LUT_SEQIDX(seq) (seq)
#define LUT_SEQNUM(seq) (0)


enum {
	/* Set of Single Data Rate LUT commands */
	lutCmd_SDR = 0x01,           /* Transmit Command code to Flash, using SDR mode. */
	lutCmdRADDR_SDR = 0x02,      /* Transmit Row Address to Flash, using SDR mode. */
	lutCmdCADDR_SDR = 0x03,      /* Transmit Column Address to Flash, using SDR mode. */
	lutCmdMODE1_SDR = 0x04,      /* Transmit 1bit bits to Flash, using SDR mode. */
	lutCmdMODE2_SDR = 0x05,      /* Transmit 2bit bits to Flash, using SDR mode. */
	lutCmdMODE4_SDR = 0x06,      /* Transmit 4bit bits to Flash, using SDR mode. */
	lutCmdMODE8_SDR = 0x07,      /* Transmit 8bit bits to Flash, using SDR mode. */
	lutCmdWRITE_SDR = 0x08,      /* Transmit Programming Data to Flash, using SDR mode. */
	lutCmdREAD_SDR = 0x09,       /* Receive Read Data from Flash, using SDR mode. */
	lutCmdLEARN_SDR = 0x0a,      /* Receive Read Data or Preamble bit from Flash, SDR mode. */
	lutCmdDATSZ_SDR = 0x0b,      /* Transmit Read/Program Data size (byte) to Flash, SDR mode. */
	lutCmdDUMMY_SDR = 0x0c,      /* Leave data lines undriven */
	lutCmdDUMMY_RWDS_SDR = 0x0d, /* Leave data lines undriven, dummy cycles decided by RWDS. */

	/* Set of Double Data Rate commands */
	lutCmd_DDR = 0x21,           /* Transmit Command code to Flash, using DDR mode. */
	lutCmdRADDR_DDR = 0x22,      /* Transmit Row Address to Flash, using DDR mode. */
	lutCmdCADDR_DDR = 0x23,      /* Transmit Column Address to Flash, using DDR mode. */
	lutCmdMODE1_DDR = 0x24,      /* Transmit 1bit bits to Flash, using DDR mode. */
	lutCmdMODE2_DDR = 0x25,      /* Transmit 2bit bits to Flash, using DDR mode. */
	lutCmdMODE4_DDR = 0x26,      /* Transmit 4bit bits to Flash, using DDR mode. */
	lutCmdMODE8_DDR = 0x27,      /* Transmit 8bit bits to Flash, using DDR mode. */
	lutCmdWRITE_DDR = 0x28,      /* Transmit Programming Data to Flash, using DDR mode. */
	lutCmdREAD_DDR = 0x29,       /* Receive Read Data from Flash, using DDR mode. */
	lutCmdLEARN_DDR = 0x2a,      /* Receive Read Data or Preamble bit from Flash, DDR mode. */
	lutCmdDATSZ_DDR = 0x2b,      /* Transmit Read/Program Data size (byte) to Flash, DDR mode. */
	lutCmdDUMMY_DDR = 0x2c,      /* Leave data lines undriven */
	lutCmdDUMMY_RWDS_DDR = 0x2d, /* Leave data lines undriven, dummy cycles decided by RWDS. */

	/* Set of sequence flow control commands */
	lutCmdSTOP = 0x00,       /* Stop execution, deassert CS. */
	lutCmdJUMP_ON_CS = 0x1f, /* STOP; deassert CS; save operand for next sequence (use with enhanced XIP) */
};


/* Number of pads used to transmit/receive, use to form LUT instruction */
enum {
	lutPad1 = 0x00, /* single SPI: transmit/receive using DATA0/DATA1 wires */
	lutPad2 = 0x01, /* dual SPI: transmit/receive using DATA[1:0] wires */
	lutPad4 = 0x02, /* quad SPI: transmit/receive using DATA[3:0] wires */
	lutPad8 = 0x03, /* octo SPI: transmit/receive using DATA[7:0] wires */
};


#endif /* _FLEXSPI_LUT_H_ */
