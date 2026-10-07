/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Messaging routines
 *
 * Copyright 2012 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _MSG_H_
#define _MSG_H_

#include <hal/hal.h>

/* Special characters */
#define MSG_MARK    0x7e
#define MSG_ESC     0x7d
#define MSG_ESCMARK 0x5e
#define MSG_ESCESC  0x5d


/* Transmission parameters */
#define MSGRECV_TIMEOUT 500 /* milliseconds */
#define MSGRECV_MAXRETR 3


#define MSG_HDRSZ  (2u * sizeof(u32))
#define MSG_MAXLEN 512u


typedef struct _msg_t {
	u32 csum;
	u32 type;
	u8 data[MSG_MAXLEN];
} msg_t;


/* Message types */
#define MSG_ERR 0


#define msg_settype(m, t) ((m)->type = ((m)->type & ~0xffff) | ((t)&0xffff))
#define msg_gettype(m)    ((m)->type & 0xffff)

#define msg_setlen(m, l) ((m)->type = ((m)->type & 0xffff) | ((u32)(l) << 16))
#define msg_getlen(m)    ((m)->type >> 16)

#define msg_setcsum(m, c) ((m)->csum = ((m)->csum & ~0xffff) | ((c)&0xffff))
#define msg_getcsum(m)    ((m)->csum & 0xffff)

#define msg_setseq(m, s) ((m)->csum = ((m)->csum & 0xffff) | ((u32)(s) << 16))
#define msg_getseq(m)    ((m)->csum >> 16)


extern int msg_send(unsigned int major, unsigned int minor, msg_t *smsg, msg_t *rmsg);


static inline void msg_serialize32(u8 *to, u32 from)
{
	to[0] = from & 0xff;
	to[1] = (from >> 8) & 0xff;
	to[2] = (from >> 16) & 0xff;
	to[3] = (from >> 24) & 0xff;
}


static inline u32 msg_deserialize32(u8 *from)
{
	return (from[0] | (from[1] << 8) | (from[2] << 16) | (from[3] << 24));
}


#endif
