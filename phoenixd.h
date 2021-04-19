/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * phoenixd communication
 *
 * Copyright 2012 Phoenix Systems
 * Copyright 2005 Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _PHOENIXD_H_
#define _PHOENIXD_H_


#include "phfs.h"
#include "config.h"


extern int phoenixd_open(const char *file, unsigned int flags, const phfs_clbk_t *cblk);


extern ssize_t phoenixd_read(unsigned int fd, addr_t offs, u8 *buff, unsigned int len, const phfs_clbk_t *cblk);


extern ssize_t phoenixd_write(unsigned int fd, addr_t offs, const u8 *buff, unsigned int len, const phfs_clbk_t *cblk);


extern int phoenixd_close(unsigned int fd, const phfs_clbk_t *cblk);


#endif
