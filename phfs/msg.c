/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
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

#include "../errors.h"
#include "../hal.h"
#include "../plostd.h"
#include "uart.h"
#include "msg.h"
#include "devs.h"


#define MSGREAD_DESYN   0
#define MSGREAD_FRAME   1


static u32 msg_csum(msg_t *msg)
{
	u32 k;
	u16 csum;

	csum = 0;
	for (k = sizeof(msg->csum); k < MSG_HDRSZ + msg_getlen(msg); k++)
		csum += *((u8 *)msg + k);

	csum += msg_getseq(msg);
	return csum;
}


static int msg_write(unsigned int major, unsigned int minor, msg_t *msg)
{
	u8 *p = (u8 *)msg;
	u8 cs[2];
	u16 k;
	int res;

	/* Frame start */
	cs[0] = MSG_MARK;
	if ((res = devs_write(major, minor, 0, cs, 1)) < 0)
		return res;

	for (k = 0; k < MSG_HDRSZ + msg_getlen(msg); k++) {
		if ((p[k] == MSG_MARK) || (p[k] == MSG_ESC)) {
			cs[0] = MSG_ESC;
			cs[1] = p[k] == MSG_MARK ? MSG_ESCMARK : MSG_ESCESC;

			if ((res = devs_write(major, minor, 0, cs, 2)) < 0)
				return res;
		}
		else {
			if ((res = devs_write(major, minor, 0, &p[k], 1)) < 0)
				return res;
		}
	}
	return k;
}


static int msg_read(unsigned int major, unsigned int minor, msg_t *msg, u16 timeout, int *state)
{
	u8 c;
	u8 buff[MSG_HDRSZ + MSG_MAXLEN];
	int i, escfl = 0, res = 0;
	unsigned int l = 0;

	for (;;) {
		if ((res = devs_read(major, minor, 0, buff, sizeof(buff), MSGRECV_TIMEOUT)) < 0)
			break;

		for (i = 0; i < res; ++i) {
			c = buff[i];

			if (*state == MSGREAD_FRAME) {
				/* Return error if frame is to long */
				if (l == MSG_HDRSZ + MSG_MAXLEN) {
					*state = MSGREAD_DESYN;
					return ERR_MSG_IO;
				}

				/* Return error if terminator discovered */
				if (c == MSG_MARK)
					return ERR_MSG_IO;

				if (!escfl && (c == MSG_ESC)) {
					escfl = 1;
					continue;
				}
				if (escfl) {
					if (c == MSG_ESCMARK)
						c = MSG_MARK;
					if (c == MSG_ESCESC)
						c = MSG_ESC;
					escfl = 0;
				}
				*((u8 *)msg + l++) = c;

				/* Frame received */
				if ((l >= MSG_HDRSZ) && (l == msg_getlen(msg) + MSG_HDRSZ)) {
					*state = MSGREAD_DESYN;

					/* Verify received message */
					if (msg_getcsum(msg) != msg_csum(msg))
						return ERR_MSG_IO;

					return l;
				}
			}
			else {
				/* Synchronize */
				if (c == MSG_MARK)
					*state = MSGREAD_FRAME;
			}
		}
	}

	*state = MSGREAD_DESYN;

	return ERR_MSG_IO;
}


int msg_send(unsigned int major, unsigned int minor, msg_t *smsg, msg_t *rmsg)
{
	unsigned int retr;
	int state = MSGREAD_DESYN;

	msg_setcsum(smsg, msg_csum(smsg));
	for (retr = 0; retr < MSGRECV_MAXRETR; retr++) {
		if (msg_write(major, minor, smsg) < 0)
			continue;

		if ((msg_read(major, minor, rmsg, MSGRECV_TIMEOUT, &state)) > 0) {
			return ERR_NONE;
		}
	}

	return ERR_MSG_IO;
}
