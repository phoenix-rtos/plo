/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Spike TTY driver
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/lib.h>
#include <devices/devs.h>


static inline void uart_txData(const void *buff, size_t len)
{
	size_t i = 0;
	const char *c = buff;
	long res;

	while (i < len) {
		res = sbi_putchar(c[i]);
		if (res != -1) {
			i++;
		}
	}
}

/* Device interface */

static ssize_t uart_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	long res;
	int i = 0;
	time_t start = hal_timerGet();

	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	while (i < len) {
		if (hal_timerGet() - start >= timeout) {
			return i;
		}

		res = sbi_getchar();
		if (res != -1) {
			((char *)buff)[i] = (char)res;
			i++;
		}
	}

	return len;
}


static ssize_t uart_write(unsigned int minor, const void *buff, size_t len)
{
	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	uart_txData(buff, len);

	return len;
}


static ssize_t uart_safeWrite(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	ssize_t res = 0;
	size_t wrote = 0;

	while (wrote < len) {
		res = uart_write(minor, buff + wrote, len - wrote);
		if (res < 0) {
			return -ENXIO;
		}
		wrote += res;
	}

	return len;
}


static int uart_sync(unsigned int minor)
{
	return EOK;
}


static int uart_done(unsigned int minor)
{
	return EOK;
}


static int uart_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	/* UART is not mappable to any region */
	int err = dev_isNotMappable;

	if (minor >= UART_MAX_CNT) {
		err = -EINVAL;
	}
	/* Device mode cannot be higher than map mode to copy data */
	else if ((mode & memmode) != mode) {
		err = -EINVAL;
	}
	return err;
}


static int uart_init(unsigned int minor)
{
	return EOK;
}


__attribute__((constructor)) static void uart_reg(void)
{
	static const dev_handler_t h = {
		.init = uart_init,
		.done = uart_done,
		.read = uart_read,
		.write = uart_safeWrite,
		.sync = uart_sync,
		.map = uart_map,
	};

	devs_register(DEV_UART, UART_MAX_CNT, &h);
}
