/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * phoenixd communication
 *
 * Copyright 2012, 2021 Phoenix Systems
 * Copyright 2005 Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _PHOENIXD_H_
#define _PHOENIXD_H_


#include "phfs.h"
#include <hal/hal.h>


extern int phoenixd_open(const char *file, unsigned int major, unsigned int minor, unsigned int flags);


extern ssize_t phoenixd_read(unsigned int fd, unsigned int major, unsigned int minor, addr_t offs, void *buff, size_t len);


extern ssize_t phoenixd_write(unsigned int fd, unsigned int major, unsigned int minor, addr_t offs, const void *buff, size_t len);


extern int phoenixd_stat(unsigned int fd, unsigned int major, unsigned int minor, phfs_stat_t *stat);


extern int phoenixd_close(unsigned int fd, unsigned int major, unsigned int minor);


#endif
