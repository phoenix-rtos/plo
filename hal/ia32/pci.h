/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * PCI bus access
 *
 * Copyright 2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _PCI_H_
#define _PCI_H_


#include <types.h>


/* Function checks presence of PCI bus */
extern int hal_pciDetect(void);


/* Function makes PCI address */
extern u32 hal_pciAddrBDF(u8 bus, u8 dev, u8 func);


/* Function reads 32bit word of PCI configuration */
extern u32 hal_pciRead32(u8 bus, u8 dev, u8 func, u8 offs);


/* Function reads 16bit half word of PCI configuration */
extern u16 hal_pciRead16(u8 bus, u8 dev, u8 func, u8 offs);


/* Function reads 8bit byte of PCI configuration */
extern u8 hal_pciRead8(u8 bus, u8 dev, u8 func, u8 offs);


#endif /* end of _PCI_H_ */
