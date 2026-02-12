/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * RNG driver for STM32 devices
 *
 * Copyright 2026 Phoenix Systems
 * Author: Jacek Maksymowicz, Krzysztof Radzewicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/lib.h>
#include <devices/devs.h>

#define N_MINORS  1
#define N_RETRIES 1000

#define RNG_CR_RNGEN (1UL << 2)

#if defined(__CPU_STM32N6)
#define RNG_BASE ((void *)0x54020000)
#else
#error "Platform not supported"
#endif

static volatile u32 *rng_base = RNG_BASE;


enum rng_regs {
	rng_cr = 0,
	rng_sr,
	rng_dr,
	rng_nscr,
	rng_htcr,
};


static int rng_isValidMinor(unsigned int minor)
{
	return minor < N_MINORS;
}


static int _stm32_rngReadDR(u32 *val)
{
	int ret = -EAGAIN;
	u32 sr = rng_base[rng_sr];

	if ((sr & 1u) != 0) {
		*val = rng_base[rng_dr];

		/* Zero check is recommended in Reference Manual due to rare race condition */
		ret = (*val != 0) ? 0 : -EIO;
	}

	/* Has error been detected? */
	if ((sr & (3u << 5)) != 0) {
		/* Mark data as faulty, but check cause of interrupts */
		if ((sr & (3u << 1)) == 0) {
			/* Situation has been recovered from, try again */
			ret = -EAGAIN;
		}
		else {
			/* Request IP reinit only for SE */
			ret = (sr & (1 << 6)) ? -EIO : -EAGAIN;
		}

		/* Clear flags */
		rng_base[rng_sr] &= ~(sr & (3u << 5));
		hal_cpuDataMemoryBarrier();
	}

	return ret;
}


/* Device interface */
static ssize_t rng_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	int err;
	size_t pos = 0, chunk, retry;
	u32 t;

	(void)timeout;
	(void)offs;

	if (rng_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	rng_base[rng_cr] |= RNG_CR_RNGEN;

	while (pos < len) {
		retry = 0;
		err = _stm32_rngReadDR(&t);
		if (err < 0) {
			if (err != -EAGAIN) {
				/* Reinitialize IP */
				rng_base[rng_cr] &= ~RNG_CR_RNGEN;
				rng_base[rng_cr] |= RNG_CR_RNGEN;
			}

			++retry;
			if (retry < N_RETRIES) {
				/* Retry */
				continue;
			}

			rng_base[rng_cr] &= ~RNG_CR_RNGEN;
			return -EIO;
		}

		chunk = min(sizeof(t), len - pos);
		hal_memcpy(buff + pos, &t, chunk);
		pos += chunk;
	}

	rng_base[rng_cr] &= ~RNG_CR_RNGEN;
	return (ssize_t)len;
}


static int rng_init(unsigned int minor)
{
	u32 v;
	if (rng_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	/* Enable clock */
	_stm32_rccSetDevClock(dev_rng, 1);

	/* Disable RNG */
	rng_base[rng_cr] &= ~RNG_CR_RNGEN;
	hal_cpuDataMemoryBarrier();

	v = rng_base[rng_cr];
	v |= (1u << 30); /* Conditioning soft reset */
	v &= ~(1u << 7); /* Enable auto reset if seed error detected */
	v &= ~(1u << 5); /* Enable clock error detection */
	/* AN4230 5.1.3 NIST compliant RNG configuration */
	v &= ~0x03FFFF00;
	v |= 0x00F00D00;
	rng_base[rng_cr] = v; /* Apply new configuration, hold in reset */
	/* AN4230 5.1.3 NIST compliant RNG configuration */
	rng_base[rng_htcr] = 0xAAC7;
	rng_base[rng_nscr] = 0x0003FFFF; /* Activate all noise sources */
	rng_base[rng_cr] &= ~(1u << 30); /* Release from reset */
	while ((rng_base[rng_cr] & (1u << 30)) != 0) {
		/* Wait for peripheral to become ready (2 AHB cycles + 2 RNG clock cycles) */
	}

	/* Disable interrupt */
	rng_base[rng_cr] &= ~(1u << 3);
	hal_cpuDataMemoryBarrier();

	return EOK;
}


static int rng_done(unsigned int minor)
{
	if (rng_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	_stm32_rccSetDevClock(dev_rng, 0);
	return EOK;
}


__attribute__((constructor)) static void rng_stm32_reg(void)
{
	static const dev_ops_t opsRng = {
		.read = rng_read,
		.write = NULL,
		.erase = NULL,
		.control = NULL,
		.sync = NULL,
		.map = NULL,
	};

	static const dev_t devRng = {
		.name = "rng-stm32",
		.init = rng_init,
		.done = rng_done,
		.ops = &opsRng,
	};

	devs_register(DEV_RNG, 1, &devRng);
}
