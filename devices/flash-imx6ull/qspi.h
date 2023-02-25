/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * imx6ull QSPI Controller driver
 *
 * Copyright 2021-2023 Phoenix Systems
 * Author: Gerard Swiderski, Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _QSPI_H_
#define _QSPI_H_

#define QSPI_PORTS 4


/* Use to enable particular slave buses of QSPI controller (slPortMask) */
enum { qspi_slBusA1 = 0x1,
	qspi_slBusA2 = 0x2,
	qspi_slBusB1 = 0x4,
	qspi_slBusB2 = 0x8 };


typedef struct {
	addr_t ahbAddr;
	u8 slPortMask;
	size_t slFlashSz[4];
} qspi_t;


struct xferOp {
	enum { xfer_opCommand = 0,
		xfer_opRead,
		xfer_opWrite } op;
	time_t timeout;
	u32 addr;
	u8 port;
	u8 seqIdx;
	union {
		struct {
			void *ptr;
			size_t sz;
		} read;
		struct {
			const void *ptr;
			size_t sz;
		} write;
		u8 command;
	} data;
};


/* Initialize single QSPI module with up to four slave devices */
extern int qspi_init(qspi_t *qspi, u8 slPortMask);


/* Safely deinit leaving XIP working */
extern int qspi_deinit(qspi_t *qspi);


/* Setup sizes for each slave device */
extern void qspi_setFlashSize(qspi_t *qspi, const size_t flashSizes[4]);


/* Update QSPI sequence lookup table */
extern void qspi_lutUpdate(qspi_t *qspi, u32 index, const u32 *lutTable, size_t count);


/* Execute a transfer using lookup table of QSPI sequences */
extern ssize_t qspi_xferExec(qspi_t *qspi, struct xferOp *xfer);


#endif /* _QSPI_H_ */
