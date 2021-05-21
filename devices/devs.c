/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-loader
 *
 * Device Interface
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "devs.h"

#include <lib/errno.h>

#define SIZE_MAJOR   4
#define SIZE_MINOR   16


struct {
	const dev_handler_t *devs[SIZE_MAJOR][SIZE_MINOR];
} devs_common;


void devs_register(unsigned int major, unsigned int nb, const dev_handler_t *h)
{
	unsigned int minor = 0, i = 0;

	if (major >= SIZE_MAJOR || nb >= SIZE_MINOR)
		return;

	while (minor < SIZE_MINOR && i < nb) {
		if (devs_common.devs[major][minor] == NULL) {
			devs_common.devs[major][minor] = h;
			++i;
		}
		++minor;
	}
}


void devs_init(void)
{
	const dev_handler_t *h;
	unsigned int major, minor;

	for (major = 0; major < SIZE_MAJOR; ++major) {
		for (minor = 0; minor < SIZE_MINOR; ++minor) {
			h = devs_common.devs[major][minor];
			if (h != NULL && h->init != NULL) {
				/* TODO: check in dtb the availability of a device in the current platform */
				/* TODO: check initialization */
				h->init(minor);
			}
		}
	}
}


int devs_check(unsigned int major, unsigned int minor)
{
	const dev_handler_t *h;

	if (major >= SIZE_MAJOR || minor >= SIZE_MINOR)
		return -EINVAL;

	h = devs_common.devs[major][minor];
	if (h == NULL || h->init == NULL || h->done == NULL ||
	    h->sync == NULL || h->read == NULL || h->write == NULL)
		return -EINVAL;

	return EOK;
}


ssize_t devs_read(unsigned int major, unsigned int minor, addr_t offs, u8 *buff, unsigned int len, unsigned int timeout)
{
	const dev_handler_t *h;

	if (major >= SIZE_MAJOR || minor >= SIZE_MINOR)
		return -EINVAL;

	h = devs_common.devs[major][minor];
	if (h == NULL || h->read == NULL)
		return -EINVAL;

	return h->read(minor, offs, buff, len, timeout);
}


ssize_t devs_write(unsigned int major, unsigned int minor, addr_t offs, const u8 *buff, unsigned int len)
{
	const dev_handler_t *h;

	if (major >= SIZE_MAJOR || minor >= SIZE_MINOR)
		return -EINVAL;

	h = devs_common.devs[major][minor];
	if (h == NULL || h->write == NULL)
		return -EINVAL;

	return h->write(minor, offs, buff, len);
}


int devs_sync(unsigned int major, unsigned int minor)
{
	const dev_handler_t *h;

	if (major >= SIZE_MAJOR || minor >= SIZE_MINOR)
		return -EINVAL;

	h = devs_common.devs[major][minor];
	if (h == NULL || h->sync == NULL)
		return -EINVAL;

	return h->sync(minor);
}


int devs_map(unsigned int major, unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	const dev_handler_t *h;

	if (major >= SIZE_MAJOR || minor >= SIZE_MINOR)
		return -EINVAL;

	h = devs_common.devs[major][minor];
	if (h == NULL || h->map == NULL)
		return -EINVAL;

	return h->map(minor, addr, sz, mode, memaddr, memsz, memmode, a);
}


void devs_done(void)
{
	const dev_handler_t *h;
	unsigned int major, minor;

	for (major = 0; major < SIZE_MAJOR; ++major) {
		for (minor = 0; minor < SIZE_MINOR; ++minor) {
			h = devs_common.devs[major][minor];
			if (h != NULL && h->done != NULL)
				h->done(minor);
		}
	}
}
