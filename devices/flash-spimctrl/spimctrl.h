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
#include "nor/nor.h"


typedef struct _spimctrl_t {
	vu32 *base;
	addr_t maddr; /* Address where flash is mapped in AHB */
	struct nor_device dev;
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


/* Execute a transfer through spimctrl */
int spimctrl_xfer(spimctrl_t *spimctrl, struct xferOp *op);


/* Reset spimctrl core */
void spimctrl_reset(spimctrl_t *spimctrl);


/* Initialize spimctrl instance */
void spimctrl_init(spimctrl_t *spimctrl, unsigned int minor);


#endif
