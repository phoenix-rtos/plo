/*
 * Phoenix-RTOS
 *
 * Operating system loader
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

#include <hal/hal.h>
#include <lib/lib.h>


/* Message types */
#define MSG_OPEN  1
#define MSG_READ  2
#define MSG_WRITE 3
#define MSG_COPY  4
#define MSG_FSTAT 6


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


#define PHOENIXD_HDRSZ (2u * sizeof(u32) + sizeof(s32))


typedef struct {
	u32 handle;
	u32 pos;
	s32 len;
	u8 data[MSG_MAXLEN - PHOENIXD_HDRSZ];
} msg_phoenixd_t;


static void phoenixd_serializeMsgPhd(u8 *buff, u32 handle, u32 pos, u32 len)
{
	msg_serialize32(buff, handle);
	msg_serialize32(buff + sizeof(u32), pos);
	msg_serialize32(buff + (2u * sizeof(u32)), len);
}


static void phoenixd_serializeDataMsgPhd(u8 *buff, u32 handle, u32 pos, u32 len, const u8 *data)
{
	msg_serialize32(buff, handle);
	msg_serialize32(buff + sizeof(u32), pos);
	msg_serialize32(buff + (2u * sizeof(u32)), len);
	hal_memcpy(buff + (3u * sizeof(u32)), data, len);
}


static void phoenixd_deserializeMsgPhd(msg_phoenixd_t *msg, u8 *buff)
{
	msg->handle = msg_deserialize32(buff);
	msg->pos = msg_deserialize32(buff + sizeof(u32));
	msg->len = msg_deserialize32(buff + (2u * sizeof(u32)));
}


int phoenixd_open(const char *file, unsigned int major, unsigned int minor, unsigned int flags)
{
	size_t l;
	unsigned int fd;
	msg_t smsg, rmsg;

	l = hal_strlen(file) + 1;

	msg_serialize32(smsg.data, flags);

	hal_memcpy(&smsg.data[sizeof(u32)], file, l);
	l += sizeof(u32);

	msg_settype(&smsg, MSG_OPEN);
	msg_setlen(&smsg, l);

	if (msg_send(major, minor, &smsg, &rmsg) < 0) {
		return -EIO;
	}
	else if (msg_gettype(&rmsg) != MSG_OPEN) {
		return -EIO;
	}
	else if (msg_getlen(&rmsg) != sizeof(u32)) {
		return -EIO;
	}
	else {
		fd = msg_deserialize32(rmsg.data);
		if (fd == 0u) {
			return -EIO;
		}
	}

	return fd;
}


ssize_t phoenixd_read(unsigned int fd, unsigned int major, unsigned int minor, addr_t offs, void *buff, size_t len)
{
	msg_t smsg, rmsg;
	msg_phoenixd_t *io;
	u32 l;

	io = (msg_phoenixd_t *)smsg.data;

	if (len > (MSG_MAXLEN - PHOENIXD_HDRSZ)) {
		return -EINVAL;
	}

	phoenixd_serializeMsgPhd(smsg.data, fd, offs, len);

	msg_settype(&smsg, MSG_READ);
	msg_setlen(&smsg, PHOENIXD_HDRSZ);

	if (msg_send(major, minor, &smsg, &rmsg) < 0) {
		return -EIO;
	}

	if (msg_gettype(&rmsg) != MSG_READ) {
		return -EIO;
	}

	io = (msg_phoenixd_t *)rmsg.data;

	phoenixd_deserializeMsgPhd(io, rmsg.data);

	if (io->len < 0) {
		return -EIO;
	}

	/* TODO: check io->pos */
	// *pos = io->pos;
	l = min(io->len, msg_getlen(&rmsg) - PHOENIXD_HDRSZ);
	hal_memcpy(buff, io->data, l);

	return l;
}


ssize_t phoenixd_write(unsigned int fd, unsigned int major, unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	msg_t smsg, rmsg;
	msg_phoenixd_t *io;

	io = (msg_phoenixd_t *)smsg.data;

	if (len > (MSG_MAXLEN - PHOENIXD_HDRSZ)) {
		return -EINVAL;
	}

	phoenixd_serializeDataMsgPhd(smsg.data, fd, offs, len, buff);

	msg_settype(&smsg, MSG_WRITE);
	msg_setlen(&smsg, PHOENIXD_HDRSZ + len);

	if (msg_send(major, minor, &smsg, &rmsg) < 0) {
		return -EIO;
	}

	if (msg_gettype(&rmsg) != MSG_WRITE) {
		return -EIO;
	}

	io = (msg_phoenixd_t *)rmsg.data;

	phoenixd_deserializeMsgPhd(io, rmsg.data);

	return (io->len < 0) ? -EIO : io->len;
}


int phoenixd_stat(unsigned int fd, unsigned int major, unsigned int minor, phfs_stat_t *stat)
{
	msg_t smsg, rmsg;
	msg_phoenixd_t *io;
	phoenixd_stat_t *pstat;
	int offs;

	io = (msg_phoenixd_t *)smsg.data;

	phoenixd_serializeMsgPhd(smsg.data, fd, 0, 0);

	smsg.type = 0;
	msg_settype(&smsg, MSG_FSTAT);
	msg_setlen(&smsg, PHOENIXD_HDRSZ);

	if (msg_send(major, minor, &smsg, &rmsg) < 0) {
		return -EIO;
	}

	if (msg_gettype(&rmsg) != MSG_FSTAT) {
		return -EIO;
	}

	io = (msg_phoenixd_t *)rmsg.data;

	phoenixd_deserializeMsgPhd(io, rmsg.data);

	if (io->len != sizeof(phoenixd_stat_t)) {
		return -EIO;
	}

	pstat = (phoenixd_stat_t *)io->data;
	offs = ((addr_t)&pstat->st_size) - (addr_t)pstat;

	stat->size = msg_deserialize32(io->data + offs);

	return EOK;
}


int phoenixd_close(unsigned int fd, unsigned int major, unsigned int minor)
{
	/* TODO */
	return EOK;
}
