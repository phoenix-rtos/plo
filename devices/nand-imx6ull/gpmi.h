/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ULL GPMI
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _GPMI_H_
#define _GPMI_H_

#include "dma.h"


extern int gpmi_w4ready(dma_desc_t *desc, unsigned int cs);


extern int gpmi_cmdaddr(dma_desc_t *desc, unsigned int cs, const void *buff, u16 addrsz);


extern int gpmi_readcmp(dma_desc_t *desc, unsigned int cs, u16 mask, u16 val);


extern int gpmi_disableBCH(dma_desc_t *desc, unsigned int cs);


extern int gpmi_read(dma_desc_t *desc, unsigned int cs, void *buff, u16 size);


extern int gpmi_ecread(dma_desc_t *desc, unsigned int cs, void *buff, void *aux, u16 size);


extern int gpmi_write(dma_desc_t *desc, unsigned int cs, const void *buff, u16 size);


extern int gpmi_ecwrite(dma_desc_t *desc, unsigned int cs, const void *buff, const void *aux, u16 size);


extern void gpmi_done(void);


extern void gpmi_init(void);


#endif
