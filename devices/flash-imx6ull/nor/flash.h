/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ULL NOR flash device driver
 *
 * Copyright 2021-2023 Phoenix Systems
 * Author: Gerard Swiderski, Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _FLASH_H_
#define _FLASH_H_


#define FLASH_CMD_READ_ID    0x90 /* Read manufacturer/device ID */
#define FLASH_CMD_RDID       0x9f /* Read JEDEC ID */
#define FLASH_CMD_RES        0xab /* Release from deep power - down*/
#define FLASH_CMD_RDSR1      0x05 /* Read Status Register - 1 */
#define FLASH_CMD_RDSR2      0x35 /* Read Status Register - 2 */
#define FLASH_CMD_RDSR3      0x15 /* Read Status Register - 3 */
#define FLASH_CMD_EQM        0x35 /* Enable quad input/output mode */
#define FLASH_CMD_RQM        0xF5 /* Reset quad input/output mode */
#define FLASH_CMD_WRR1       0x01 /* Write Status Register - 1 */
#define FLASH_CMD_WRR2       0x31 /* Write Status Register - 2 */
#define FLASH_CMD_WRR3       0x11 /* Write Status Register - 3 */
#define FLASH_CMD_WRDI       0x04 /* Write disable */
#define FLASH_CMD_WREN       0x06 /* Write enable */
#define FLASH_CMD_READ       0x03 /* Read data */
#define FLASH_CMD_4READ      0x13 /* 4 byte read data */
#define FLASH_CMD_FAST_READ  0x0b /* Fast read data */
#define FLASH_CMD_4FAST_READ 0x0c /* 4 byte fast read data */
#define FLASH_CMD_DDRFR      0x0d /* DTR fast read data */
#define FLASH_CMD_4DDRFR     0x0e /* 4 byte DTR fast read data */
#define FLASH_CMD_DOR        0x3b /* Dual output fast read data */
#define FLASH_CMD_4DOR       0x3c /* 4 byte dual output read data */
#define FLASH_CMD_QOR        0x6b /* Quad output read data */
#define FLASH_CMD_4QOR       0x6c /* 4 byte quad output read data */
#define FLASH_CMD_DIOR       0xbb /* Dual input/output fast read data */
#define FLASH_CMD_4DIOR      0xbc /* 4 byte dual input/output fast read data */
#define FLASH_CMD_DDRDIOR    0xbd /* DTR dual input/output fast read data */
#define FLASH_CMD_4DDRDIOR   0xbe /* 4 byte DTR dual input/output fast read data */
#define FLASH_CMD_QIOR       0xeb /* Quad input/output fast read data */
#define FLASH_CMD_4QIOR      0xec /* 4 byte Quad input/output fast read data */
#define FLASH_CMD_DDRQIOR    0xed /* DTR quad input/output fast read data */
#define FLASH_CMD_4DDRQIOR   0xee /* 4 byte DTR quad input/output fast read data */
#define FLASH_CMD_PP         0x02 /* Page program */
#define FLASH_CMD_4PP        0x12 /* 4 byte page program */
#define FLASH_CMD_QPP        0x32 /* Quad input fast program */
#define FLASH_CMD_4QPP       0x34 /* 4 byte quad input fast program */
#define FLASH_CMD_4QEPP      0x3e /* 4 byte extended quad input fast program */
#define FLASH_CMD_PGSP       0x85 /* Read volatile configuration */
#define FLASH_CMD_P4E        0x20 /* 4KB sector erase */
#define FLASH_CMD_4P4E       0x21 /* 4 byte 4 KB sector erase */
#define FLASH_CMD_BE         0x60 /* Chip erase */
#define FLASH_CMD_CE         0xc7 /* Chip erase fast */
#define FLASH_CMD_SE         0xd8 /* 64KB sector erase*/
#define FLASH_CMD_4SE        0xdc /* 4 byte 64KB sector erase */
#define FLASH_CMD_ERSP       0x75 /* Program/erase suspend */
#define FLASH_CMD_ERRS       0x7a /* Program/erase resume */


#endif /* _FLASH_H_ */
