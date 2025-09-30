/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Device Interface
 *
 * Copyright 2021-2024 Phoenix Systems
 * Author: Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "devs.h"

#include <lib/errno.h>

#define SIZE_MAJOR 10
#define SIZE_MINOR 16

struct {
	const dev_t *devs[SIZE_MAJOR][SIZE_MINOR];
} devs_common;


void devs_register(unsigned int major, unsigned int nb, const dev_t *dev)
{
	unsigned int minor;
	unsigned int i = 0;

	if ((major >= SIZE_MAJOR) || (nb >= SIZE_MINOR)) {
		return;
	}

	for (minor = 0; (minor < SIZE_MINOR) && (i < nb); ++minor) {
		if (devs_common.devs[major][minor] == NULL) {
			devs_common.devs[major][minor] = dev;
			++i;
		}
	}
}


void devs_init(void)
{
	const dev_t *dev;
	unsigned int major;
	unsigned int minor;

	for (major = 0; major < SIZE_MAJOR; ++major) {
		for (minor = 0; minor < SIZE_MINOR; ++minor) {
			dev = devs_common.devs[major][minor];
			if ((dev != NULL) && (dev->init != NULL)) {
				/* TODO: check in dtb the availability of a device in the current platform */
				/* TODO: check initialization */
				dev->init(minor);
			}
		}
	}
}


const dev_t *devs_iterNext(unsigned int *ctx, unsigned int *major, unsigned int *minor)
{
	unsigned int currMajor, currMinor;

	if ((ctx == NULL) || (major == NULL) || (minor == NULL)) {
		return NULL;
	}

	currMajor = ((*ctx >> 16) & 0xffff);
	currMinor = (*ctx & 0xffff);
	while (currMajor < SIZE_MAJOR) {
		while (currMinor < SIZE_MINOR) {
			const dev_t *dev = devs_common.devs[currMajor][currMinor];

			*major = currMajor;
			*minor = currMinor++;
			*ctx = (*ctx & ~0xffff) + currMinor;

			return dev;
		}
		currMajor++;
		currMinor = 0;
		*ctx = (*ctx & 0xffff) | (currMajor << 16);
	}

	/* Iterator has finished enumerating */
	return DEVS_ITER_STOP;
}


static const dev_t *devs_get(unsigned int major, unsigned int minor)
{
	return ((major < SIZE_MAJOR) && (minor < SIZE_MINOR)) ?
		devs_common.devs[major][minor] :
		NULL;
}


static const dev_ops_t *devs_ops(unsigned int major, unsigned int minor)
{
	const dev_t *dev = devs_get(major, minor);

	return (dev != NULL) ?
		dev->ops :
		NULL;
}


int devs_check(unsigned int major, unsigned int minor)
{
	const dev_t *dev = devs_get(major, minor);

	return ((dev != NULL) && (dev->name != NULL) && (dev->init != NULL) && (dev->done != NULL)) ?
		EOK :
		-ENODEV;
}


ssize_t devs_read(unsigned int major, unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	const dev_ops_t *ops = devs_ops(major, minor);

	return ((ops != NULL) && (ops->read != NULL)) ?
		ops->read(minor, offs, buff, len, timeout) :
		-ENOSYS;
}


ssize_t devs_write(unsigned int major, unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	const dev_ops_t *ops = devs_ops(major, minor);

	return ((ops != NULL) && (ops->write != NULL)) ?
		ops->write(minor, offs, buff, len) :
		-ENOSYS;
}


ssize_t devs_erase(unsigned int major, unsigned int minor, addr_t offs, size_t len, unsigned int flags)
{
	const dev_ops_t *ops = devs_ops(major, minor);

	return ((ops != NULL) && (ops->erase != NULL)) ?
		ops->erase(minor, offs, len, flags) :
		-ENOSYS;
}


int devs_sync(unsigned int major, unsigned int minor)
{
	const dev_ops_t *ops = devs_ops(major, minor);

	return ((ops != NULL) && (ops->sync != NULL)) ?
		ops->sync(minor) :
		-ENOSYS;
}


int devs_map(unsigned int major, unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	const dev_ops_t *ops = devs_ops(major, minor);

	return ((ops != NULL) && (ops->map != NULL)) ?
		ops->map(minor, addr, sz, mode, memaddr, memsz, memmode, a) :
		-ENOSYS;
}


int devs_control(unsigned int major, unsigned int minor, int cmd, void *args)
{
	const dev_ops_t *ops = devs_ops(major, minor);

	return ((ops != NULL) && (ops->control != NULL)) ?
		ops->control(minor, cmd, args) :
		-ENOSYS;
}


void devs_done(void)
{
	const dev_t *dev;
	unsigned int major, minor;

	for (major = 0; major < SIZE_MAJOR; ++major) {
		for (minor = 0; minor < SIZE_MINOR; ++minor) {
			dev = devs_common.devs[major][minor];
			if ((dev != NULL) && (dev->done != NULL)) {
				dev->done(minor);
			}
		}
	}
}
