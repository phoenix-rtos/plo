/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * MPU API
 *
 * Copyright 2021, 2022 Phoenix Systems
 * Author: Gerard Swiderski, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _MPU_HAL_H_
#define _MPU_HAL_H_

#include <hal/hal.h>

#define MPU_MAX_REGIONS 16


typedef struct {
	u32 rbar;
	u32 rlar;
} mpu_region_t;


#ifdef MPUTEST_ORGIMPL
#error "To run ORGIMPL checkout plo to master!"
#endif


typedef struct {
	u32 regCnt;
	mpu_region_t region[16] __attribute__((aligned(8)));
	u32 mapId[16];
} mpu_part_t;


typedef struct {
	u32 type;
	u32 regMax;
	mpu_part_t curPart;
} mpu_common_t;


/* Get const pointer to read only mpu_common structure */
extern const mpu_common_t *const mpu_getCommon(void);


/* Reset structures and detect MPU type */
extern void mpu_init(void);


/* Get MPU regions setup into hal_syspage_t structure */
extern void mpu_getHalData(hal_syspage_t *hal);


extern void mpu_getProgHal(hal_syspage_prog_t *progHal, const char *imaps, size_t imapSz, const char *dmaps, size_t dmapSz);


#endif
