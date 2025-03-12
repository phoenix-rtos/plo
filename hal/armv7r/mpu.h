/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * MPU region allocation routines
 *
 * Copyright 2021 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _MPU_HAL_H_
#define _MPU_HAL_H_

#include <hal/hal.h>

typedef struct {
	u32 rbar;
	u32 rasr;
} mpu_region_t;


typedef struct {
	u32 type;
	u32 regCnt;
	u32 regMax;
	u32 mapCnt;
	mpu_region_t region[16] __attribute__((aligned(8)));
	u32 mapId[16];
} mpu_common_t;


/* Get const pointer to read only mpu_common structure */
extern const mpu_common_t *const mpu_getCommon(void);


/* Reset structures and detect MPU type */
extern void mpu_init(void);


/* Allocate MPU sub-regions and link with a map */
extern int mpu_regionAlloc(addr_t addr, addr_t end, u32 attr, u32 mapId, unsigned int enable);


/* Get MPU regions setup into hal_syspage_t structure */
extern void mpu_getHalData(hal_syspage_t *hal);


#endif
