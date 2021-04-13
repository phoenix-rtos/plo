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

#define SIZE_DEV             4
#define SIZE_DEV_INSTANCES   16


struct {
	dev_handler_t devs[SIZE_DEV][SIZE_DEV_INSTANCES];
	int (*init[SIZE_DEV])(unsigned int dn, dev_handler_t *h);
} devs_common;


void devs_regDriver(unsigned int dev, int (*init)(unsigned int dn, dev_handler_t *h))
{
	if (dev >= SIZE_DEV)
		return;

	devs_common.init[dev] = init;
}


void devs_init(void)
{
	unsigned int i, j;
	dev_handler_t *handler;

	for (i = 0; i < SIZE_DEV; ++i) {
		if (devs_common.init[i] != NULL) {
			for (j = 0; j < SIZE_DEV_INSTANCES; ++j) {
				/* TODO: check in dtb the availability of a device in the current platform */
				handler = &devs_common.devs[i][j];
				/* TODO: check initialization */
				devs_common.init[i](j, handler);
			}
		}
	}
}


int devs_getHandler(unsigned int dev, unsigned int dn, dev_handler_t **h)
{
	dev_handler_t *temph;

	if (dev >= SIZE_DEV || dn >= SIZE_DEV_INSTANCES)
		return ERR_ARG;

	temph = &devs_common.devs[dev][dn];
	if (temph->deinit == NULL ||
		temph->sync == NULL ||
		temph->read == NULL ||
		temph->write == NULL)
		return ERR_ARG;

	*h = temph;

	return ERR_NONE;
}


ssize_t devs_read(unsigned int dev, unsigned int dn, addr_t offs, u8 *buff, unsigned int len)
{
	dev_handler_t *handler;

	if (dev >= SIZE_DEV || dn >= SIZE_DEV_INSTANCES)
		return ERR_ARG;

	handler = &devs_common.devs[dev][dn];
	if (handler->read == NULL)
		return ERR_ARG;

	return handler->read(dn, offs, buff, len);
}


ssize_t devs_write(unsigned int dev, unsigned int dn, addr_t offs, const u8 *buff, unsigned int len)
{
	dev_handler_t *handler;

	if (dev >= SIZE_DEV || dn >= SIZE_DEV_INSTANCES)
		return ERR_ARG;

	handler = &devs_common.devs[dev][dn];
	if (handler->write == NULL)
		return ERR_ARG;

	return handler->write(dn, offs, buff, len);
}


int devs_deinit(unsigned int dev, unsigned int dn)
{
	dev_handler_t *handler;

	if (dev >= SIZE_DEV || dn >= SIZE_DEV_INSTANCES)
		return ERR_ARG;

	handler = &devs_common.devs[dev][dn];
	if (handler->deinit == NULL)
		return ERR_ARG;

	return handler->deinit(dn);
}


int devs_sync(unsigned int dev, unsigned int dn)
{
	dev_handler_t *handler;

	if (dev >= SIZE_DEV || dn >= SIZE_DEV_INSTANCES)
		return ERR_ARG;

	handler = &devs_common.devs[dev][dn];
	if (handler->sync == NULL)
		return ERR_ARG;

	return handler->sync(dn);
}



void devs_deinitAll(void)
{
	unsigned int i, j;
	dev_handler_t *handler;

	for (i = 0; i < SIZE_DEV; ++i) {
		for (j = 0; j < SIZE_DEV_INSTANCES; ++j) {
			handler = &devs_common.devs[i][j];
			if (handler->deinit != NULL)
				handler->deinit(i);
		}
	}
}
