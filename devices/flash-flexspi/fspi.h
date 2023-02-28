/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * FlexSPI Controller driver
 *
 * Copyright 2021-2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _FLEXSPI_H_
#define _FLEXSPI_H_

#define FLEXSPI_PORTS 4

static inline int isXIP(void *addr)
{
	u32 pc;
	__asm__ volatile("mov %0, pc"
					 : "=r"(pc));
	return pc >= (u32)addr && pc < (u32)addr + 0x10000000;
}


/* Use to enable particular slave buses of FlexSPI controller (slPortMask) */
enum { flexspi_slBusA1 = 0x1,
	flexspi_slBusA2 = 0x2,
	flexspi_slBusB1 = 0x4,
	flexspi_slBusB2 = 0x8 };


/* Select particular slave bus during xferExec and xferOp */
enum { flexspi_portA1 = 0,
	flexspi_portA2,
	flexspi_portB1,
	flexspi_portB2 };

enum { flexspi_instance1 = 1, /* referred as FLEXSPI or FLEXSPI1 */
	flexspi_instance2 = 2     /* referred as FLEXSPI2 */
};


typedef struct _flexspi_t {
	volatile u32 *base;
	addr_t ahbAddr;
	u8 instance;
	u8 slPortMask;
	size_t slFlashSz[4];
} flexspi_t;


struct xferOp {
	enum { xfer_opCommand = 0,
		xfer_opRead,
		xfer_opWrite } op;
	time_t timeout;
	u32 addr;
	u8 port;
	u8 seqIdx;
	u8 seqNum;
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


/* Initialize single FlexSPI module with up to four slave devices */
extern int flexspi_init(flexspi_t *fspi, int instance, u8 slPortMask);


/* Safely deinit leaving XIP working */
extern int flexspi_deinit(flexspi_t *fspi);


/* Setup sizes for each slave device (particular bus must be enabled with slPortMask) */
extern void flexspi_setFlashSize(flexspi_t *fspi, const size_t *flashSizes, size_t count);


/* Update FlexSPI sequence lookup table (raw table) */
extern void flexspi_lutUpdate(flexspi_t *fspi, u32 index, const u32 *lutTable, size_t count);


/* Update FlexSPI sequence lookup table (table of pointers) */
extern void flexspi_lutUpdateEntries(flexspi_t *fspi, u32 index, const u32 *lutTable[], size_t elems, size_t count);


/* Execute a transfer using lookup table of FlexSPI sequences */
extern ssize_t flexspi_xferExec(flexspi_t *fspi, struct xferOp *xfer);


#endif /* _FLEXSPI_H_ */
