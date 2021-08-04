/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Circular buffer - code partially derived from phoenix-rtos-kernel/lib/cbuffer.*
 *
 * Copyright 2019, 2021 Phoenix Systems
 * Author: Jan Sikorski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "cbuffer.h"


size_t lib_cbufSize(const cbuffer_t *buf)
{
	if (buf->tail == buf->head)
		return buf->full ? buf->capacity : 0;

	return (buf->tail - buf->head + buf->capacity) % buf->capacity;
}


int lib_cbufEmpty(const cbuffer_t *buf)
{
	return buf->head == buf->tail && !buf->full;
}


size_t lib_cbufWrite(cbuffer_t *buf, const void *data, size_t sz)
{
	int bytes = 0;

	if (!sz || buf->full)
		return 0;

	if (buf->head > buf->tail) {
		hal_memcpy(buf->data + buf->tail, data, bytes = min(sz, buf->head - buf->tail));
	}
	else {
		hal_memcpy(buf->data + buf->tail, data, bytes = min(sz, buf->capacity - buf->tail));

		if (bytes < sz && buf->head) {
			data += bytes;
			sz -= bytes;
			hal_memcpy(buf->data, data, min(sz, buf->head));
			bytes += min(sz, buf->head);
		}
	}

	buf->tail = (buf->tail + bytes) % buf->capacity;
	buf->full = buf->tail == buf->head;

	return bytes;
}


size_t lib_cbufRead(cbuffer_t *buf, void *data, size_t sz)
{
	int bytes = 0;

	if (!sz || (buf->head == buf->tail && !buf->full))
		return 0;

	if (buf->tail > buf->head) {
		hal_memcpy(data, buf->data + buf->head, bytes = min(sz, buf->tail - buf->head));
	}
	else {
		hal_memcpy(data, buf->data + buf->head, bytes = min(sz, buf->capacity - buf->head));

		if (bytes < sz) {
			data += bytes;
			sz -= bytes;
			hal_memcpy(data, buf->data, min(sz, buf->tail));
			bytes += min(sz, buf->tail);
		}
	}

	buf->head = (buf->head + bytes) % buf->capacity;
	buf->full = 0;

	return bytes;
}


void lib_cbufInit(cbuffer_t *buf, void *data, size_t capacity)
{
	hal_memset(buf, 0, sizeof(cbuffer_t));
	buf->capacity = capacity;
	buf->data = data;
}
