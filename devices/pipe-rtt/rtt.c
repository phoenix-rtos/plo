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
	unsigned char bytes[1024] __attribute__((aligned(32)));
} rttBufferPool[RTT_TXCHANNELS + RTT_RXCHANNELS];


static const char rtt_tagBackward[] = RTT_TAG_BACKWARD;
static const char *const rtt_txName[RTT_TXCHANNELS] = { "Console TX", "phoenixd TX" };
static const char *const rtt_rxName[RTT_RXCHANNELS] = { "Console RX", "phoenixd RX" };
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

	rtt->rxChannel[chan].rd = rd;

	hal_cpuDataMemoryBarrier();

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

	rtt->txChannel[chan].wr = wr;

	hal_cpuDataMemoryBarrier();

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
	unsigned int n, m;

	if (rtt != NULL) {
		return;
	}

	rtt = (volatile struct rtt_desc *)addr;
	hal_memset((void *)rtt, 0, sizeof(*rtt));

	rtt->txChannels = RTT_TXCHANNELS;
	rtt->rxChannels = RTT_RXCHANNELS;

	m = 0;
	for (n = 0; n < rtt->txChannels; n++) {
		rtt->txChannel[n].name = rtt_txName[n];
		rtt->txChannel[n].ptr = rttBufferPool[m].bytes;
		rtt->txChannel[n].sz = sizeof(rttBufferPool[m].bytes);
		m++;
	}

	for (n = 0; n < rtt->rxChannels; n++) {
		rtt->rxChannel[n].name = rtt_rxName[n];
		rtt->rxChannel[n].ptr = rttBufferPool[m].bytes;
		rtt->rxChannel[n].sz = sizeof(rttBufferPool[m].bytes);
		m++;
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
