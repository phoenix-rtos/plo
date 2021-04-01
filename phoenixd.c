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


#include "errors.h"
#include "hal.h"
#include "plostd.h"
#include "serial.h"
#include "msg.h"
#include "phoenixd.h"


typedef struct _msg_phoenixd_t {
	u32 handle;
	u32 pos;
	u32 len;
	u8  data[MSG_MAXLEN - 3 * sizeof(u32)];
} msg_phoenixd_t;


handle_t phoenixd_open(u16 dn, const char *name, u32 flags, phfs_clbk_t *cblk)
{
	u16 l;
	msg_t smsg, rmsg;
	handle_t handle;

	if (cblk == NULL) {
		handle.h = ERR_PHFS_IO;
		return handle;
	}

	handle.offs = 0;
	l = plostd_strlen(name) + 1;

	*(u32 *)smsg.data = flags;
	hal_memcpy(&smsg.data[sizeof(u32)], name, l);
	l += sizeof(u32);

	msg_settype(&smsg, MSG_OPEN);
	msg_setlen(&smsg, l);

	smsg.write = cblk->write;
	rmsg.read = cblk->read;

	if (msg_send(dn, &smsg, &rmsg) < 0)
		handle.h = ERR_PHFS_IO;
	else if (msg_gettype(&rmsg) != MSG_OPEN)
		handle.h = ERR_PHFS_PROTO;
	else if (msg_getlen(&rmsg) != sizeof(u32))
		handle.h = ERR_PHFS_PROTO;
	else if (!(handle.h = *(u32 *)rmsg.data))
		handle.h = ERR_PHFS_FILE;

	return handle;
}


s32 phoenixd_read(u16 dn, handle_t handle, addr_t *pos, u8 *buff, u32 len, phfs_clbk_t *cblk)
{
	msg_t smsg, rmsg;
	msg_phoenixd_t *io;
	u16 hdrsz;
	u32 l;

	/* Read from serial */
	io = (msg_phoenixd_t *)smsg.data;
	hdrsz = (u16)((u32)io->data - (u32)io);

	if ((handle.h <= 0) || (len > MSG_MAXLEN - hdrsz) || cblk == NULL)
		return ERR_ARG;

	io->handle = handle.h;
	io->pos = *pos;
	io->len = len;

	msg_settype(&smsg, MSG_READ);
	msg_setlen(&smsg, hdrsz);

	smsg.write = cblk->write;
	rmsg.read = cblk->read;

	if (msg_send(dn, &smsg, &rmsg) < 0)
		return ERR_PHFS_IO;

	if (msg_gettype(&rmsg) != MSG_READ) {
		return ERR_PHFS_PROTO;
	}
	io = (msg_phoenixd_t *)rmsg.data;

	if ((long)io->len < 0) {
		return ERR_PHFS_FILE;
	}

	*pos = io->pos;
	l = min(io->len, msg_getlen(&rmsg) - hdrsz);
	hal_memcpy(buff, io->data, l);

	return l;
}


s32 phoenixd_write(u16 dn, handle_t handle, addr_t *pos, u8 *buff, u32 len, u8 sync, phfs_clbk_t *cblk)
{
	/* TODO */
	return ERR_NONE;
}


s32 phoenixd_close(u16 dn, handle_t handle, phfs_clbk_t *cblk)
{
	/* TODO */
	return ERR_NONE;
}
