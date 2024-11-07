/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * RTT pipes: communication through debug probe (driver)
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/lib.h>


#define RTT_TXCHANNELS 2
#define RTT_RXCHANNELS 2

/*
 * Note: RTT_TAG needs to be backward written string. This tag is used by
 * the RTT remote side e.g. openocd to find descriptor location during memory
 * scan, so as not to mislead the descriptor scan procedure, this tag needs
 * to be simply hidden before it is copied into RAM together with initialized
 * descriptors, that's why it is written backwards.
 * - Default RTT_TAG_REVERSED="EPIP TTR", which corresponds to "RTT PIPE"
 */

#ifndef RTT_TAG_BACKWARD
#define RTT_TAG_BACKWARD "EPIP TTR"
#endif


struct rtt_pipe {
	const char *name;
	unsigned char *ptr;
	unsigned int sz;
	volatile unsigned int wr;
	volatile unsigned int rd;
	unsigned int flags;
};


struct rtt_desc {
	char tag[16];
	unsigned int txChannels;
	unsigned int rxChannels;
	struct rtt_pipe txChannel[RTT_TXCHANNELS];
	struct rtt_pipe rxChannel[RTT_RXCHANNELS];
};


static struct {
	/* NOTE: each buffer must be aligned to cache line */
	unsigned char consoleTx[RTT_BUFSZ_CONSOLE_TX] __attribute__((aligned(32)));
	unsigned char consoleRx[RTT_BUFSZ_CONSOLE_RX] __attribute__((aligned(32)));
	unsigned char phoenixdTx[RTT_BUFSZ_PHOENIXD_TX] __attribute__((aligned(32)));
	unsigned char phoenixdRx[RTT_BUFSZ_PHOENIXD_RX] __attribute__((aligned(32)));
} rttBuffers __attribute__((section(".rttmem")));


static const char rtt_tagBackward[] = RTT_TAG_BACKWARD;


static const struct {
	struct {
		unsigned char *buf;
		unsigned int bufsz;
		const char *name;
	} tx[RTT_TXCHANNELS], rx[RTT_RXCHANNELS];
} channelDefs = {
	.tx = {
		{ rttBuffers.consoleTx, sizeof(rttBuffers.consoleTx), "Console TX" },
		{ rttBuffers.phoenixdTx, sizeof(rttBuffers.phoenixdTx), "phoenixd TX" },
	},
	.rx = {
		{ rttBuffers.consoleRx, sizeof(rttBuffers.consoleRx), "Console RX" },
		{ rttBuffers.phoenixdRx, sizeof(rttBuffers.phoenixdRx), "phoenixd RX" },
	},
};


static volatile struct rtt_desc *rtt = NULL;


int rtt_check(int chan)
{
	if ((rtt == NULL) || (chan < 0) || (chan >= RTT_TXCHANNELS)) {
		return -ENODEV;
	}

	return 0;
}


ssize_t rtt_read(int chan, void *buf, size_t count)
{
	if (rtt_check(chan) < 0) {
		return -ENODEV;
	}

	hal_cpuDataMemoryBarrier();

	unsigned char *srcBuf = (unsigned char *)rtt->rxChannel[chan].ptr;
	unsigned char *dstBuf = (unsigned char *)buf;
	unsigned int rd = rtt->rxChannel[chan].rd;
	unsigned int wr = rtt->rxChannel[chan].wr;
	unsigned int sz = rtt->rxChannel[chan].sz;
	size_t todo = count;

	while ((todo != 0) && (rd != wr)) {
		*dstBuf++ = srcBuf[rd];
		rd = (rd + 1) % sz;
		todo--;
	}

	hal_cpuDataMemoryBarrier();

	rtt->rxChannel[chan].rd = rd;

	return count - todo;
}


ssize_t rtt_write(int chan, const void *buf, size_t count)
{
	if (rtt_check(chan) < 0) {
		return -ENODEV;
	}

	hal_cpuDataMemoryBarrier();

	const unsigned char *srcBuf = (const unsigned char *)buf;
	unsigned char *dstBuf = (unsigned char *)rtt->txChannel[chan].ptr;
	unsigned int sz = rtt->txChannel[chan].sz;
	unsigned int rd = (rtt->txChannel[chan].rd + sz - 1) % sz;
	unsigned int wr = rtt->txChannel[chan].wr;
	size_t todo = count;

	while ((todo != 0) && (rd != wr)) {
		dstBuf[wr] = *srcBuf++;
		wr = (wr + 1) % sz;
		todo--;
	}

	hal_cpuDataMemoryBarrier();

	rtt->txChannel[chan].wr = wr;

	return count - todo;
}


static ssize_t rtt_writeBlocking(int chan, const void *buf, size_t count)
{
	const unsigned char *ptr = buf;
	size_t todo = count;
	time_t start = hal_timerGet();

	while (todo > 0) {
		if ((hal_timerGet() - start) >= 100) {
			rtt->txChannel[chan].wr = rtt->txChannel[chan].rd;
			return -ETIME;
		}

		ssize_t len = rtt_write(chan, ptr, todo);
		if (len < 0) {
			return len;
		}
		todo -= len;
		ptr += len;
	}

	return 0;
}


void rtt_init(void *addr)
{
	size_t n, m;

	if (rtt != NULL) {
		return;
	}

	rtt = (volatile struct rtt_desc *)addr;
	hal_memset((void *)rtt, 0, sizeof(*rtt));

	rtt->txChannels = RTT_TXCHANNELS;
	rtt->rxChannels = RTT_RXCHANNELS;

	for (n = 0; n < (sizeof(channelDefs.tx) / sizeof(channelDefs.tx[0])); n++) {
		rtt->txChannel[n].name = channelDefs.tx[n].name;
		rtt->txChannel[n].ptr = channelDefs.tx[n].buf;
		rtt->txChannel[n].sz = channelDefs.tx[n].bufsz;
		rtt->txChannel[n].wr = 0;
		rtt->txChannel[n].rd = 0;
	}

	for (n = 0; n < (sizeof(channelDefs.rx) / sizeof(channelDefs.rx[0])); n++) {
		rtt->rxChannel[n].name = channelDefs.rx[n].name;
		rtt->rxChannel[n].ptr = channelDefs.rx[n].buf;
		rtt->rxChannel[n].sz = channelDefs.rx[n].bufsz;
		rtt->rxChannel[n].wr = 0;
		rtt->rxChannel[n].rd = 0;
	}

	n = 0;
	m = sizeof(rtt_tagBackward) - 1;
	while (n < sizeof(rtt->tag) && m > 0) {
		rtt->tag[n++] = rtt_tagBackward[--m];
	}

	hal_consoleSetHooks(rtt_write);
	lib_consoleSetHooks(rtt_read, rtt_writeBlocking);
}


void rtt_done(void)
{
	if (rtt != NULL) {
		lib_consoleSetHooks(NULL, NULL);
		hal_memset((void *)rtt, 0, sizeof(*rtt));
		rtt = NULL;
	}
}
