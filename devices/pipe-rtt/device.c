/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * RTT pipes: communication through debug probe (device)
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <devices/devs.h>
#include <hal/hal.h>
#include <lib/lib.h>

#include "rtt.h"

static ssize_t pipe_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	(void)offs;
	time_t start = hal_timerGet();

	while (1 == 1) {
		ssize_t ret = rtt_read(minor + 1, buff, len);

		if (ret != 0) {
			return ret;
		}
		if ((hal_timerGet() - start) >= timeout) {
			return -ETIME;
		}
	}

	return 0;
}


static ssize_t pipe_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	(void)offs;

	return rtt_write(minor + 1, buff, len);
}


static int pipe_done(unsigned int minor)
{
	return rtt_check(minor + 1);
}


static int pipe_sync(unsigned int minor)
{
	return rtt_check(minor + 1);
}


static int pipe_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	(void)addr;
	(void)sz;
	(void)memaddr;
	(void)memsz;
	(void)a;

	if (rtt_check(minor + 1) < 0) {
		return -ENODEV;
	}

	/* mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode) {
		return -EINVAL;
	}

	/* is not mappable to any region */
	return dev_isNotMappable;
}


static int pipe_init(unsigned int minor)
{
	int res = rtt_check(minor + 1);
	if (res == 0) {
		lib_printf("\ndev/pipe: Initialized rtt channel(%d.%d)", DEV_PIPE, minor);
	}

	return res;
}


__attribute__((constructor)) static void pipe_reg(void)
{
	static const dev_ops_t opsPipeRTT = {
		.read = pipe_read,
		.write = pipe_write,
		.erase = NULL,
		.sync = pipe_sync,
		.map = pipe_map,
	};

	static const dev_t devPipeRTT = {
		.name = "pipe-rtt",
		.init = pipe_init,
		.done = pipe_done,
		.ops = &opsPipeRTT,
	};

	devs_register(DEV_PIPE, 1, &devPipeRTT);
}
