/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * DTB parser
 *
 * Copyright 2018, 2024 Phoenix Systems
 * Author: Pawel Pisarczyk, Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_DTB_H_

#include <types.h>

#define ntoh16(x) (((x << 8) & 0xff00) | ((x >> 8) & 0xff))
#define ntoh32(x) ((ntoh16(x) << 16) | ntoh16(x >> 16))
#define ntoh64(x) ((ntoh32(x) << 32) | ntoh32(x >> 32))


extern void dtb_parse(void *dtb);


extern int dtb_getPLIC(void);


#endif
