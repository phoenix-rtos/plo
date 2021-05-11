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


/* Message types */
#define MSG_OPEN    1
#define MSG_READ    2
#define MSG_WRITE   3
#define MSG_COPY    4
#define MSG_FSTAT   6


typedef struct {
	u32 st_dev;
	u32 st_ino;
	u16 st_mode;
	u16 st_nlink;
	u16 st_uid;
	u16 st_gid;
	u32 st_rdev;
	u32 st_size;

	u32 st_atime_;
	u32 st_mtime_;
	u32 st_ctime_;
	s32 st_blksize;
	s32 st_blocks;
} phoenixd_stat_t;


typedef struct {
	u32 handle;
	u32 pos;
	u32 len;
	u8  data[MSG_MAXLEN - 3 * sizeof(u32)];
} msg_phoenixd_t;


int phoenixd_open(const char *file, unsigned int major, unsigned int minor, unsigned int flags)
{
	u16 l;
	unsigned int fd;
	msg_t smsg, rmsg;

	l = hal_strlen(file) + 1;

	*(u32 *)smsg.data = flags;
	hal_memcpy(&smsg.data[sizeof(u32)], file, l);
	l += sizeof(u32);

	msg_settype(&smsg, MSG_OPEN);
	msg_setlen(&smsg, l);

	if (msg_send(major, minor, &smsg, &rmsg) < 0)
		return ERR_PHFS_IO;
	else if (msg_gettype(&rmsg) != MSG_OPEN)
		return ERR_PHFS_PROTO;
	else if (msg_getlen(&rmsg) != sizeof(u32))
		return ERR_PHFS_PROTO;
	else if (!(fd = *(u32 *)rmsg.data))
		return ERR_PHFS_FILE;

	return fd;
}


ssize_t phoenixd_read(unsigned int fd, unsigned int major, unsigned int minor, addr_t offs, u8 *buff, unsigned int len)
{
	msg_t smsg, rmsg;
	msg_phoenixd_t *io;
	u16 hdrsz;
	u32 l;

	io = (msg_phoenixd_t *)smsg.data;
	hdrsz = (u16)((u32)io->data - (u32)io);

	if (len > MSG_MAXLEN - hdrsz)
		return ERR_ARG;

	io->handle = fd;
	io->pos = offs;
	io->len = len;

	msg_settype(&smsg, MSG_READ);
	msg_setlen(&smsg, hdrsz);

	if (msg_send(major, minor, &smsg, &rmsg) < 0)
		return ERR_PHFS_IO;

	if (msg_gettype(&rmsg) != MSG_READ)
		return ERR_PHFS_PROTO;

	io = (msg_phoenixd_t *)rmsg.data;

	if ((long)io->len < 0)
		return ERR_PHFS_FILE;

	/* TODO: check io->pos */
	// *pos = io->pos;
	l = min(io->len, msg_getlen(&rmsg) - hdrsz);
	hal_memcpy(buff, io->data, l);

	return l;
}


ssize_t phoenixd_write(unsigned int fd, unsigned int major, unsigned int minor, addr_t offs, const u8 *buff, unsigned int len)
{
	/* TODO */
	return ERR_NONE;
}


int phoenixd_stat(unsigned int fd, unsigned int major, unsigned int minor, phfs_stat_t *stat)
{
	msg_t smsg, rmsg;
	msg_phoenixd_t *io;
	phoenixd_stat_t *pstat;
	u16 hdrsz;

	io = (msg_phoenixd_t *)smsg.data;
	hdrsz = (u16)((u32)io->data - (u32)io);

	io->handle = fd;
	io->pos = 0;
	io->len = 0;

	smsg.type = 0;
	msg_settype(&smsg, MSG_FSTAT);
	msg_setlen(&smsg, hdrsz);

	if (msg_send(major, minor, &smsg, &rmsg) < 0)
		return ERR_PHFS_IO;

	if (msg_gettype(&rmsg) != MSG_FSTAT)
		return ERR_PHFS_PROTO;

	io = (msg_phoenixd_t *)rmsg.data;
	if (io->len != sizeof(phoenixd_stat_t))
		return ERR_PHFS_FILE;

	pstat = (phoenixd_stat_t *)io->data;
	stat->size = pstat->st_size;

	return ERR_NONE;
}


int phoenixd_close(unsigned int fd, unsigned int major, unsigned int minor)
{
	/* TODO */
	return ERR_NONE;
}
