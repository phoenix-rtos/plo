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


#define MPU_BASE ((void *)0xe000ed90)

#define MPU_MAX_REGIONS 16


#ifdef MPUTEST_ORGIMPL
#error "To run ORGIMPL checkout plo to master!"
#endif


typedef struct {
	u32 type;
	u32 regMax;
} mpu_common_t;


/* Reset structures and detect MPU type */
extern void mpu_init(void);


/* Get MPU regions setup into hal_syspage_t structure */
extern void mpu_getHalData(hal_syspage_t *hal);


extern int mpu_getHalProgData(syspage_prog_t *prog, const char *imaps, size_t imapSz, const char *dmaps, size_t dmapSz);


#endif
