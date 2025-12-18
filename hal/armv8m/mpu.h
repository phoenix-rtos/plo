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


/* Reset structures and detect MPU type */
extern void mpu_init(void);


extern void mpu_kernelEntryPoint(addr_t addr);


/* Get MPU info into hal_syspage_t structure */
extern void mpu_getHalData(hal_syspage_t *hal);


/* Get MPU regions setup into syspage_part_t structure */
extern int mpu_getHalPartData(syspage_part_t *part, const char *imaps, size_t imapSz, const char *dmaps, size_t dmapSz);


#endif
