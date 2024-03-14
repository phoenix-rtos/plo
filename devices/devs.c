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

#define SIZE_MAJOR 9
#define SIZE_MINOR 16


struct {
	const dev_t *devs[SIZE_MAJOR][SIZE_MINOR];
} devs_common;


static int common_devcheck(const dev_t *dev)
{
	return (dev == NULL) || (dev->name == NULL) || (dev->ops == NULL) ? -EINVAL : 0;
}


void devs_register(unsigned int major, unsigned int nb, const dev_t *dev)
{
	unsigned int minor = 0, i = 0;

	if ((major >= SIZE_MAJOR) || (nb >= SIZE_MINOR)) {
		return;
	}

	if (common_devcheck(dev) == 0) {
		while (minor < SIZE_MINOR && i < nb) {
			if (devs_common.devs[major][minor] == NULL) {
				devs_common.devs[major][minor] = dev;
				++i;
			}
			++minor;
		}
	}
}


void devs_init(void)
{
	const dev_t *dev;
	unsigned int major, minor;

	for (major = 0; major < SIZE_MAJOR; ++major) {
		for (minor = 0; minor < SIZE_MINOR; ++minor) {
			dev = devs_common.devs[major][minor];
			if ((common_devcheck(dev) == 0) && (dev->ops->init != NULL)) {
				/* TODO: check in dtb the availability of a device in the current platform */
				/* TODO: check initialization */
				dev->ops->init(minor);
			}
		}
	}
}


int devs_check(unsigned int major, unsigned int minor)
{
	const dev_t *dev;
	const dev_ops_t *ops;

	if ((major >= SIZE_MAJOR) || (minor >= SIZE_MINOR)) {
		return -EINVAL;
	}

	dev = devs_common.devs[major][minor];
	if (common_devcheck(dev) < 0) {
		return -EINVAL;
	}

	ops = dev->ops;
	if ((ops->init == NULL) || (ops->done == NULL) ||
		/* FIXME: is sync, read, write check necessary ? (respective wrapper already checks) */
		(ops->sync == NULL) || (ops->read == NULL) || (ops->write == NULL)) {
		return -EINVAL;
	}

	return EOK;
}


ssize_t devs_read(unsigned int major, unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	const dev_t *dev;

	if ((major >= SIZE_MAJOR) || (minor >= SIZE_MINOR)) {
		return -EINVAL;
	}

	dev = devs_common.devs[major][minor];
	if ((common_devcheck(dev) < 0) || (dev->ops->read == NULL)) {
		return -EINVAL;
	}

	return dev->ops->read(minor, offs, buff, len, timeout);
}


ssize_t devs_write(unsigned int major, unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	const dev_t *dev;

	if ((major >= SIZE_MAJOR) || (minor >= SIZE_MINOR)) {
		return -EINVAL;
	}

	dev = devs_common.devs[major][minor];
	if ((common_devcheck(dev) < 0) || (dev->ops->write == NULL)) {
		return -EINVAL;
	}

	return dev->ops->write(minor, offs, buff, len);
}


ssize_t devs_erase(unsigned int major, unsigned int minor, addr_t offs, size_t len, unsigned int flags)
{
	const dev_t *dev;

	if ((major >= SIZE_MAJOR) || (minor >= SIZE_MINOR)) {
		return -EINVAL;
	}

	dev = devs_common.devs[major][minor];
	if (common_devcheck(dev) < 0) {
		return -ENODEV;
	}

	if (dev->ops->erase == NULL) {
		return -ENOSYS;
	}

	return dev->ops->erase(minor, offs, len, flags);
}


int devs_sync(unsigned int major, unsigned int minor)
{
	const dev_t *dev;

	if ((major >= SIZE_MAJOR) || (minor >= SIZE_MINOR)) {
		return -EINVAL;
	}

	dev = devs_common.devs[major][minor];
	if ((common_devcheck(dev) < 0) || (dev->ops->sync == NULL)) {
		return -EINVAL;
	}

	return dev->ops->sync(minor);
}


int devs_map(unsigned int major, unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	const dev_t *dev;

	if ((major >= SIZE_MAJOR) || (minor >= SIZE_MINOR)) {
		return -EINVAL;
	}

	dev = devs_common.devs[major][minor];
	if ((common_devcheck(dev) < 0) || (dev->ops->map == NULL)) {
		return -EINVAL;
	}

	return dev->ops->map(minor, addr, sz, mode, memaddr, memsz, memmode, a);
}


void devs_done(void)
{
	const dev_t *dev;
	unsigned int major, minor;

	for (major = 0; major < SIZE_MAJOR; ++major) {
		for (minor = 0; minor < SIZE_MINOR; ++minor) {
			dev = devs_common.devs[major][minor];
			if ((common_devcheck(dev) == 0) && (dev->ops->done != NULL)) {
				dev->ops->done(minor);
			}
		}
	}
}
