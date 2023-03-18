/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR716 Flash driver
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _SPIMCTRL_H_
#define _SPIMCTRL_H_


#include <hal/hal.h>


#define SPIMCTRL_NUM 2

#define FLASH0_AHB_ADDR 0x02000000
#define FLASH1_AHB_ADDR 0x04000000

/* clang-format off */
enum { spimctrl_instance0 = 0, spimctrl_instance1 };
/* clang-format on */

typedef struct _spimctrl_t {
	vu32 *base;
	addr_t ahbStartAddr;
	u8 instance;
	u8 ear;
} spimctrl_t;


struct xferOp {
	/* clang-format off */
	enum { xfer_opRead = 0, xfer_opWrite } type;
	/* clang-format on */
	const u8 *cmd;
	size_t cmdLen;
	union {
		const u8 *txData;
		u8 *rxData;
	};
	size_t dataLen;
};


static inline addr_t spimctrl_ahbAddr(int instance)
{
	switch (instance) {
		case spimctrl_instance0: return FLASH0_AHB_ADDR;
		case spimctrl_instance1: return FLASH1_AHB_ADDR;
		default: return 0;
	}
}


static inline void *spimctrl_getBase(int instance)
{
	switch (instance) {
		case spimctrl_instance0: return SPIMCTRL0_BASE;
		case spimctrl_instance1: return SPIMCTRL1_BASE;
		default: return NULL;
	}
}


/* Execute a transfer through spimctrl */
extern int spimctrl_xfer(spimctrl_t *spimctrl, struct xferOp *op);


/* Reset spimctrl core */
extern void spimctrl_reset(spimctrl_t *spimctrl);


/* Initialize spimctrl instance */
extern int spimctrl_init(spimctrl_t *spimctrl, int instance);


#endif
