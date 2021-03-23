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

#include "msg.h"
#include "phfs.h"


/* Message types */
#define MSG_OPEN    1
#define MSG_READ    2
#define MSG_WRITE   3
#define MSG_COPY    4


typedef struct {
	int (*read)(unsigned int, u8 *, u16, u16);
	int (*write)(unsigned int, const u8 *, u16);
} phfs_clbk_t;


extern handle_t phoenixd_open(u16 dn, const char *name, u32 flags, phfs_clbk_t *cblk);


extern s32 phoenixd_read(u16 dn, handle_t handle, addr_t *pos, u8 *buff, u32 len, phfs_clbk_t *cblk);


extern s32 phoenixd_write(u16 dn, handle_t handle, addr_t *pos, u8 *buff, u32 len, u8 sync, phfs_clbk_t *cblk);


extern s32 phoenixd_close(u16 dn, handle_t handle, phfs_clbk_t *cblk);


#endif
