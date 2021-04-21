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

#include "phoenixd.h"
#include "msg.h"
#include "hal.h"
#include "errors.h"
#include "plostd.h"


/* Message types */
#define MSG_OPEN    1
#define MSG_READ    2
#define MSG_WRITE   3
#define MSG_COPY    4


typedef struct _msg_phoenixd_t {
	u32 handle;
	u32 pos;
	u32 len;
	u8  data[MSG_MAXLEN - 3 * sizeof(u32)];
} msg_phoenixd_t;


int phoenixd_open(const char *file, unsigned int flags, const phfs_clbk_t *cblk)
{
	u16 l;
	unsigned int fd;
	msg_t smsg, rmsg;

	if (cblk == NULL)
		return ERR_PHFS_IO;

	l = plostd_strlen(file) + 1;

	*(u32 *)smsg.data = flags;
	hal_memcpy(&smsg.data[sizeof(u32)], file, l);
	l += sizeof(u32);

	msg_settype(&smsg, MSG_OPEN);
	msg_setlen(&smsg, l);

	smsg.write = cblk->write;
	rmsg.read = cblk->read;

	if (msg_send(cblk->dn, &smsg, &rmsg) < 0)
		return ERR_PHFS_IO;
	else if (msg_gettype(&rmsg) != MSG_OPEN)
		return ERR_PHFS_PROTO;
	else if (msg_getlen(&rmsg) != sizeof(u32))
		return ERR_PHFS_PROTO;
	else if (!(fd = *(u32 *)rmsg.data))
		return ERR_PHFS_FILE;

	return fd;
}


ssize_t phoenixd_read(unsigned int fd, addr_t offs, u8 *buff, unsigned int len, const phfs_clbk_t *cblk)
{
	msg_t smsg, rmsg;
	msg_phoenixd_t *io;
	u16 hdrsz;
	u32 l;

	/* Read from serial */
	io = (msg_phoenixd_t *)smsg.data;
	hdrsz = (u16)((u32)io->data - (u32)io);

	if ((len > MSG_MAXLEN - hdrsz) || cblk == NULL)
		return ERR_ARG;

	io->handle = fd;
	io->pos = offs;
	io->len = len;

	msg_settype(&smsg, MSG_READ);
	msg_setlen(&smsg, hdrsz);

	smsg.write = cblk->write;
	rmsg.read = cblk->read;

	if (msg_send(cblk->dn, &smsg, &rmsg) < 0)
		return ERR_PHFS_IO;

	if (msg_gettype(&rmsg) != MSG_READ) {
		return ERR_PHFS_PROTO;
	}
	io = (msg_phoenixd_t *)rmsg.data;

	if ((long)io->len < 0) {
		return ERR_PHFS_FILE;
	}

	/* TODO: check io->pos */
	// *pos = io->pos;
	l = min(io->len, msg_getlen(&rmsg) - hdrsz);
	hal_memcpy(buff, io->data, l);

	return l;
}


ssize_t phoenixd_write(unsigned int fd, addr_t offs, const u8 *buff, unsigned int len, const phfs_clbk_t *cblk)
{
	/* TODO */
	return ERR_NONE;
}


int phoenixd_close(unsigned int fd, const phfs_clbk_t *cblk)
{
	/* TODO */
	return ERR_NONE;
}
