/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * PCI bus access
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _PCI_H_
#define _PCI_H_


#include <types.h>


typedef struct {
	unsigned int bus;
	unsigned int device;
	unsigned int function;
} hal_pciAddr_t;


#define HAL_PCI_NUM_BUS     256u
#define HAL_PCI_NUM_DEVICES 32u
#define HAL_PCI_NUM_FUNCS   8u


#define HAL_PCI_REG_PCI_ID      0x00u
#define HAL_PCI_REG_VENDOR      0x00u
#define HAL_PCI_REG_DEVICE      0x02u
#define HAL_PCI_REG_COMMAND     0x04u
#define HAL_PCI_REG_STATUS      0x06u
#define HAL_PCI_REG_REVISION    0x08u
#define HAL_PCI_REG_CLASS       0x08u
#define HAL_PCI_REG_CACHELINE   0x0cu
#define HAL_PCI_REG_LAT_TIMER   0x0du
#define HAL_PCI_REG_HEADER_TYPE 0x0eu
#define HAL_PCI_REG_BIST        0x0fu
#define HAL_PCI_REG_ADDRESSES   0x10u


#define HAL_PCI_COMMAND_IO_ENABLED   (1uL << 0u)
#define HAL_PCI_COMMAND_MEM_ENABLED  (1uL << 1u)
#define HAL_PCI_COMMAND_BUS_MASTER   (1uL << 2u)
#define HAL_PCI_COMMAND_PARITY_ERROR (1uL << 6u)
#define HAL_PCI_COMMAND_SERR_ENABLE  (1uL << 8u)


/* Function checks presence of PCI bus */
extern int hal_pciDetect(void);


/* Function makes PCI address */
extern u32 hal_pciAddrBDF(hal_pciAddr_t *bdf);


/* Function reads 32bit word of PCI configuration */
extern u32 hal_pciRead32(hal_pciAddr_t *bdf, u8 offs);


/* Function reads 16bit half word of PCI configuration */
extern u16 hal_pciRead16(hal_pciAddr_t *bdf, u8 offs);


/* Function reads 8bit byte of PCI configuration */
extern u8 hal_pciRead8(hal_pciAddr_t *bdf, u8 offs);


/* Function writes 32bit word of PCI configuration */
void hal_pciWrite32(hal_pciAddr_t *bdf, u8 offs, u32 value);


/* Function writes 16bit half word of PCI configuration */
void hal_pciWrite16(hal_pciAddr_t *bdf, u8 offs, u16 value);


/* Function writes 8bit byte of PCI configuration */
void hal_pciWrite8(hal_pciAddr_t *bdf, u8 offs, u8 value);


/* Commands PCI device to generate accesses: bus mastering and mem/io address spaces */
void hal_pciBusMaster(hal_pciAddr_t *bdf, int enableBM, int enableIO, int enableMem);


/* Helper function to simplify pci device detection */
void hal_pciIterate(int (*cb)(hal_pciAddr_t bdf, u32 id, void *data), void *cbData);


#endif /* end of _PCI_H_ */
