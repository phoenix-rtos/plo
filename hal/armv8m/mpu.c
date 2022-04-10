/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * MPU API - not implemented
 *
 * Copyright 2021, 2022 Phoenix Systems
 * Author: Gerard Swiderski, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <lib/errno.h>
#include "mpu.h"


static mpu_common_t mpu_common;


const mpu_common_t *const mpu_getCommon(void)
{
	/* TODO: return initialized structure, when enabling MPU on armv8m */
	return &mpu_common;
}


void mpu_init(void)
{
	/* TODO: add implementation, when enabling MPU on armv8m */
}


int mpu_regionAlloc(addr_t addr, addr_t end, u32 attr, u32 mapId, unsigned int enable)
{
	/* TODO: add implementation, when enabling MPU on armv8m */
	return 0;
}


void mpu_getHalData(hal_syspage_t *hal)
{
	/* TODO: add implementation, when enabling MPU on armv8m */
}
