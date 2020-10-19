/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * i.MXRT1064 Serial driver
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczyński
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "peripherals.h"
#include "../serial.h"
#include "../errors.h"
#include "../timer.h"
#include "../low.h"



int serial_rxEmpty(unsigned int pn)
{
	//TODO
	return 0;
}



int serial_read(unsigned int pn, u8 *buff, u16 len, u16 timeout)
{
	//TODO
	return 0;
}


int serial_write(unsigned int pn, const u8 *buff, u16 len)
{
	//TODO
	return 0;
}


int serial_safewrite(unsigned int pn, const u8 *buff, u16 len)
{
	return 0;
}


void serial_init(u32 baud, u32 *st)
{
	//TODO
	return;
}


void serial_done(void)
{
	//TODO
}
