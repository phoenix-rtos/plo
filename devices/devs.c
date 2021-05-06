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
#include "../errors.h"

#define SIZE_MAJOR   4
#define SIZE_MINOR   16


struct {
	dev_handler_t *devs[SIZE_MAJOR][SIZE_MINOR];
} devs_common;


void devs_register(unsigned int major, unsigned int nb, dev_handler_t *h)
{
	unsigned int minor = 0, i = 0;

	if (major >= SIZE_MAJOR || nb >= SIZE_MINOR)
		return;

	while (minor < SIZE_MAJOR && i < nb) {
		if (devs_common.devs[major][minor] == NULL) {
			devs_common.devs[major][minor] = h;
			++i;
		}
		++minor;
	}
}


void devs_init(void)
{
	dev_handler_t *handler;
	unsigned int major, minor;

	for (major = 0; major < SIZE_MAJOR; ++major) {
		for (minor = 0; minor < SIZE_MINOR; ++minor) {
			handler = devs_common.devs[major][minor];
			if (handler != NULL && handler->init != NULL) {
				/* TODO: check in dtb the availability of a device in the current platform */
				/* TODO: check initialization */
				handler->init(minor);
			}
		}
	}
}


int devs_check(unsigned int major, unsigned int minor)
{
	dev_handler_t *handler;

	if (major >= SIZE_MAJOR || minor >= SIZE_MINOR)
		return ERR_ARG;

	handler = devs_common.devs[major][minor];
	if (handler == NULL || handler->init == NULL || handler->done == NULL ||
	    handler->sync == NULL || handler->read == NULL || handler->write == NULL)
		return ERR_ARG;

	return ERR_NONE;
}


ssize_t devs_read(unsigned int major, unsigned int minor, addr_t offs, u8 *buff, unsigned int len, unsigned int timeout)
{
	dev_handler_t *handler;

	if (major >= SIZE_MAJOR || minor >= SIZE_MINOR)
		return ERR_ARG;

	handler = devs_common.devs[major][minor];
	if (handler == NULL || handler->read == NULL)
		return ERR_ARG;

	return handler->read(minor, offs, buff, len, timeout);
}


ssize_t devs_write(unsigned int major, unsigned int minor, addr_t offs, const u8 *buff, unsigned int len)
{
	dev_handler_t *handler;

	if (major >= SIZE_MAJOR || minor >= SIZE_MINOR)
		return ERR_ARG;

	handler = devs_common.devs[major][minor];
	if (handler == NULL || handler->write == NULL)
		return ERR_ARG;

	return handler->write(minor, offs, buff, len);
}


int devs_sync(unsigned int major, unsigned int minor)
{
	dev_handler_t *handler;

	if (major >= SIZE_MAJOR || minor >= SIZE_MINOR)
		return ERR_ARG;

	handler = devs_common.devs[major][minor];
	if (handler == NULL || handler->sync == NULL)
		return ERR_ARG;

	return handler->sync(minor);
}


int devs_isMappable(unsigned int major, unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *devOffs)
{
	dev_handler_t *h;

	if (major >= SIZE_MAJOR || minor >= SIZE_MINOR)
		return ERR_ARG;

	h = devs_common.devs[major][minor];
	if (h == NULL || h->isMappable == NULL)
		return ERR_ARG;

	return h->isMappable(minor, addr, sz, mode, memaddr, memsz, memmode, devOffs);
}


void devs_done(void)
{
	dev_handler_t *handler;
	unsigned int major, minor;

	for (major = 0; major < SIZE_MAJOR; ++major) {
		for (minor = 0; minor < SIZE_MINOR; ++minor) {
			handler = devs_common.devs[major][minor];
			if (handler != NULL && handler->done != NULL)
				handler->done(minor);
		}
	}
}
