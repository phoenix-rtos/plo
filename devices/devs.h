/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Devices Interface
 *
 * Copyright 2021-2024 Phoenix Systems
 * Author: Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _DEVS_H_
#define _DEVS_H_

#include <hal/hal.h>

#define DEV_UART      0
#define DEV_USB       1
#define DEV_STORAGE   2
#define DEV_TTY       3
#define DEV_RAM       4
#define DEV_NAND_DATA 5
#define DEV_NAND_META 6
#define DEV_NAND_RAW  7
#define DEV_PIPE      8

#define DEVS_ITER_STOP ((const dev_t *)-1)

#define DEV_CONTROL_SETBAUD         1
#define DEV_CONTROL_GETBAUD         2
#define DEV_CONTROL_GETPROP_TOTALSZ 3
#define DEV_CONTROL_GETPROP_BLOCKSZ 4

/* clang-format off */
enum { dev_isMappable = 0, dev_isNotMappable };
/* clang-format on */


/* Device operations */
typedef struct {
	int (*sync)(unsigned int minor);
	int (*map)(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a);
	int (*control)(unsigned int minor, int cmd, void *args);
	ssize_t (*read)(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout);
	ssize_t (*write)(unsigned int minor, addr_t offs, const void *buff, size_t len);
	ssize_t (*erase)(unsigned int minor, addr_t offs, size_t len, unsigned int flags);
} dev_ops_t;


/* Device enclosure */
typedef struct _dev_t {
	const char *name;
	int (*init)(unsigned int minor);
	int (*done)(unsigned int minor);
	const dev_ops_t *ops;
} dev_t;


/* This function should be called only before devs_init(),
 * preferably from device drivers constructors */
extern void devs_register(unsigned int major, unsigned int nb, const dev_t *dev);


/* Initialize registered devices */
extern void devs_init(void);


/* Enumerate all devices */
const dev_t *devs_iterNext(unsigned int *ctx, unsigned int *major, unsigned int *minor);


/* Check whether device is available */
extern int devs_check(unsigned int major, unsigned int minor);


/* Make synchronization on device. Preferred usage after write function */
extern int devs_sync(unsigned int major, unsigned int minor);


/* Check whether device's region is mappable to map's region. If device is mappable, the device address in memory map is written to a */
extern int devs_map(unsigned int major, unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a);


/* Control and manipulate the underlying device functions, parameters */
extern int devs_control(unsigned int major, unsigned int minor, int cmd, void *args);


/* Read data from device */
extern ssize_t devs_read(unsigned int major, unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout);


/* Write data to device */
extern ssize_t devs_write(unsigned int major, unsigned int minor, addr_t offs, const void *buff, size_t len);


/* Erase data from storage device */
extern ssize_t devs_erase(unsigned int major, unsigned int minor, addr_t offs, size_t len, unsigned int flags);


/* Reset registered devices */
extern void devs_done(void);


#endif
