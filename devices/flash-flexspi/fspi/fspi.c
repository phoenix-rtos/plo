/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * FlexSPI Controller driver
 *
 * Copyright 2021-2022 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <hal/hal.h>
#include <lib/errno.h>
#include "../fspi.h"
#include "../lut.h"


/* Include platform dependent pin config */
#if defined(IMXRT117X)
#include "fspi_rt117x.h"
#elif defined(IMXRT106X)
#include "fspi_rt106x.h"
#elif defined(IMXRT105X)
#include "fspi_rt105x.h"
#else
#error "FlexSPI is not supported on this target"
#endif


/* Stop/Start FlexSPI module */
__attribute__((section(".noxip"))) static void flexspi_disable(flexspi_t *fspi, int disable)
{
	register u32 reg = *(fspi->base + mcr0);

	if (disable) {
		reg |= (1 << 1);
	}
	else {
		reg &= ~(1 << 1);
	}

	*(fspi->base + mcr0) = reg;
}


/* Module Software Reset (all registers except control registers) */
__attribute__((section(".noxip"))) static inline void flexspi_swreset(flexspi_t *fspi)
{
	*(fspi->base + mcr0) |= 1;
	while ((*(fspi->base + mcr0) & 1) != 0)
		;
}


__attribute__((section(".noxip"))) void flexspi_lutUpdate(flexspi_t *fspi, u32 index, const u32 *lutTable, size_t count)
{
	u32 lutCopy[64];
	unsigned int i;
	volatile u32 *lutPtr = fspi->base + lut64 + index;

	for (i = 0; i < count; ++i) {
		lutCopy[i] = *lutTable++;
	}

	/* Wait for bus and lut sequencer to be idle */
	while (((*(fspi->base + sts0) & 2) != 0 && (*(fspi->base + sts0) & 1) != 0) == 0)
		;

	/* Unlock LUT */
	*(fspi->base + lutkey) = 0x5af05af0;
	*(fspi->base + lutcr) = 2;

	/* Update LUT */
	for (i = 0; i < count; ++i) {
		*lutPtr++ = lutCopy[i];
	}

	/* Lock LUT */
	*(fspi->base + lutkey) = 0x5af05af0;
	*(fspi->base + lutcr) = 1;
}


__attribute__((section(".noxip"))) int flexspi_init(flexspi_t *fspi, int instance, u8 slPortMask)
{
	int res;
	unsigned int i;
	u32 reg, lut[4], *base = flexspi_getBase(instance);

	if (base == NULL || (slPortMask & ~0xf) != 0 || (slPortMask & 0xf) == 0) {
		return -EINVAL;
	}

	fspi->base = base;
	fspi->instance = instance;
	fspi->ahbAddr = flexspi_ahbAddr(instance);
	fspi->slPortMask = slPortMask;

	res = flexspi_pinConfig(fspi);
	if (res < 0) {
		return res;
	}

	hal_interruptsDisable();

	hal_disableDCache();
	hal_disableICache();

	hal_invalDCacheAll();
	hal_cleanDCache();

	flexspi_clockConfig(fspi);

	/* Release FlexSPI from reset and power SRAM */
	flexspi_disable(fspi, 0);
	flexspi_swreset(fspi);

	/* Make sure controller is configured in module stop mode */
	flexspi_disable(fspi, 1);

	/* Set AHB and IP timeouts to default value do not enable MCR0[MDIS] module yet */
	reg = *(fspi->base + mcr0) & ((1 << 15) | (3 << 2));
	*(fspi->base + mcr0) = reg | (0xffu << 24) | (0xff << 16) | (1 << 4);

	/* Disable "same device" mode and configure all slave devices independently */
	reg = *(fspi->base + mcr2);
	*(fspi->base + mcr2) = reg & ~(1 << 15);

	/* Enable AHB Prefetch, Bufferable, Read Prefetch & Read Address option */
	*(fspi->base + ahbcr) |= (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);

	/* Disable all AHB buffers */
	for (i = 0; i < AHBRXBUF_CNT; ++i) {
		*(fspi->base + ahbrxbuf0cr0 + i) = 0;
	}

#if 1
	/* Set default all AHB buffers (many small buffers) with prefetch enabled */
	for (i = 0; i < AHBRXBUF_CNT; ++i) {
		*(fspi->base + ahbrxbuf0cr0 + i) = (1u << 31) | ((i & 7) << 16) | (1 << 6);
	}
#else
	/* Enable single 4k buffer with prefetch enabled (one big buffer) */
	*(fspi->base + ahbrxbuf0cr0) = (1u << 31) | (4096 / 8);
#endif

	for (i = 0; i < 4; ++i) {
		/* Reset flash size, set default 4MB for XIP */
		reg = *(fspi->base + flsha1cr0 + i) & (0xff << 23);

		if (fspi->slPortMask & (1 << i)) {
			reg |= 1 << 16;
		}

		*(fspi->base + flsha1cr0 + i) = reg;

		/* Set TCSH and TCSS for SDR; CS interval to 1 clock cycle; disable CAS  */
		*(fspi->base + flsha1cr1 + i) = (0x3 << 5) | 0x3;

		/* AHB Read - Set lut seq_id=0, seq_num=0 for all CS. */
		*(fspi->base + flsha1cr2 + i) &= (1 << 4) | (1 << 12);
	}

	/* Setup SLV slave clock delay line, enable OVRDEN, stop DLL calibration */
	reg = *(fspi->base + dllacr) & 0xffff8084;
	*(fspi->base + dllacr) = reg | (1 << 8);

	reg = *(fspi->base + dllbcr) & 0xffff8084;
	*(fspi->base + dllbcr) = reg | (1 << 8);

	/* Reset IP bus watermark levels and disable DMA */
	reg = *(fspi->base + iprxfcr) & ~((0x1f << 2) | (1 << 1));
	*(fspi->base + iprxfcr) = reg | (1 << 0);

	reg = *(fspi->base + iptxfcr) & ~((0x1f << 2) | (1 << 1));
	*(fspi->base + iptxfcr) = reg | (1 << 0);

	/* Disable status interrupts */
	reg = *(fspi->base + inten) & ~0xff7f;
	*(fspi->base + inten) = reg;

	/* Enable FlexSPI before updating LUT */
	flexspi_disable(fspi, 0);

	/* Default fast (up to 133MHz) read (single pad) used by AHB and XIP */
	lut[0] = LUT_SEQ(lutCmd_SDR, lutPad1, 0x0b, lutCmdRADDR_SDR, lutPad1, 0x18);
	lut[1] = LUT_SEQ(lutCmdDUMMY_SDR, lutPad1, 0x08, lutCmdREAD_SDR, lutPad1, 0x04);
	lut[2] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0);
	lut[3] = 0;

	/* Configure initial LUT sequences as needed (for AHB read and IP) */
	flexspi_lutUpdate(fspi, 0, lut, sizeof(lut) / sizeof(lut[0]));

	/* Reset all registers except control registers */
	flexspi_swreset(fspi);

	hal_enableICache();
	hal_enableDCache();

	hal_interruptsEnable();

	return EOK;
}


void flexspi_setFlashSize(flexspi_t *fspi, const size_t *flashSizes, size_t count)
{
	unsigned int i;

	if (count > 4) {
		count = 4;
	}

	for (i = 0; i < count; ++i) {
		/* Clear AHB access split control and reset chip size */
		u32 reg = (*(fspi->base + flsha1cr0 + i) & (0xff << 23));

		/* Assign flash size to ports that are initialized */
		if (fspi->slPortMask & (1 << i)) {
			fspi->slFlashSz[i] = ((flashSizes[i] >> 10) & ((1 << 22) - 1));
			reg |= fspi->slFlashSz[i];
			fspi->slFlashSz[i] <<= 10;
		}

		*(fspi->base + flsha1cr0 + i) = reg;
	}
}


__attribute__((section(".noxip"))) static addr_t flexspi_getAddressByPort(flexspi_t *fspi, u8 port, addr_t addr)
{
	unsigned int i;

	/* FlexSPI use the port (chip select) based on an offset of each memory size */
	for (i = 0; i < port; ++i) {
		if (fspi->slPortMask & (1 << i)) {
			addr += fspi->slFlashSz[i];
		}
	}

	return addr;
}


int flexspi_deinit(flexspi_t *fspi)
{
	/* Leaving initialized */

	return EOK;
}


__attribute__((section(".noxip"))) static int flexspi_checkFlags(flexspi_t *fspi)
{
	u32 flags = *(fspi->base + intr) & ((1 << 1) | (1 << 3) | (1 << 11));

	if (flags != 0) {
		/* Clear flags */
		*(fspi->base + intr) |= flags;

		/* Reset FIFOs */
		*(fspi->base + iptxfcr) |= 1;
		*(fspi->base + iprxfcr) |= 1;

		/* Command grant or sequence execution timeout */
		if (flags & ((1 << 11) | (1 << 1))) {
			return -ETIME;
		}

		return -EIO;
	}

	return EOK;
}


__attribute__((section(".noxip"))) static ssize_t flexspi_opRead(flexspi_t *fspi, time_t start, struct xferOp *xfer)
{
	int res;
	size_t size = xfer->data.read.sz;
	u8 *buf = xfer->data.read.ptr;
	u8 *end = xfer->data.read.ptr + size;
	u8 *rfdr = (u8 *)(fspi->base + rfdr32);
	u32 i, water = 1u + ((*(fspi->base + iprxfcr) & 0x7cu) >> 2);

	while (size != 0u) {
		if (size >= 8u * water) {
			/* Wait for rx FIFO available */
			while ((*(fspi->base + intr) & (1u << 5)) == 0) {
				res = flexspi_checkFlags(fspi);
				if (res != EOK) {
					return res;
				}
				else if (xfer->timeout > 0 && (hal_timerGet() - start) >= xfer->timeout) {
					return -ETIME;
				}
			}
		}
		else {
			while (size > (*(fspi->base + iprxfsts) & 0xffu) * 8u) {
				res = flexspi_checkFlags(fspi);
				if (res != EOK) {
					return res;
				}
				else if (xfer->timeout > 0 && (hal_timerGet() - start) >= xfer->timeout) {
					return -ETIME;
				}
			}
		}

		if (size >= 8u * water) {
			for (i = 0; i < 8u * water && buf < end; ++i) {
				*buf++ = rfdr[i];
			}
			size -= 8 * water;
		}
		else {
			for (i = 0; i < 4u * ((size + 3u) / 4u) && buf < end; ++i) {
				*buf++ = rfdr[i];
			}
			size = 0;
		}

		/* Move FIFO pointer to watermark level */
		*(fspi->base + intr) |= 1u << 5;
	}

	return buf - (u8 *)xfer->data.read.ptr;
}


__attribute__((section(".noxip"))) static ssize_t flexspi_opWrite(flexspi_t *fspi, time_t start, struct xferOp *xfer)
{
	int res;
	size_t ofs, len = xfer->data.write.sz, size = xfer->data.write.sz & ~7;
	const u8 *buf = xfer->data.write.ptr;

	/* note: FlexSPI FIFO watermark level is 64bit aligned */
	for (ofs = 0; ofs < size; ofs += 8, buf += 8, len -= 8) {
		/* Wait for tx FIFO available */
		while ((*(fspi->base + intr) & (1 << 6)) == 0) {
			res = flexspi_checkFlags(fspi);
			if (res != EOK) {
				return res;
			}
			else if (xfer->timeout > 0 && (hal_timerGet() - start) >= xfer->timeout) {
				return -ETIME;
			}
		}

		(fspi->base + tfdr32)[0] = ((u32 *)buf)[0];
		(fspi->base + tfdr32)[1] = ((u32 *)buf)[1];

		/* Move tx FIFO pointer to watermark level */
		*(fspi->base + intr) |= 1 << 6;
	}

	if (ofs < xfer->data.write.sz) {
		/* Wait for tx FIFO available */
		while ((*(fspi->base + intr) & (1 << 6)) == 0) {
			res = flexspi_checkFlags(fspi);
			if (res != EOK) {
				return res;
			}
			else if (xfer->timeout > 0 && (hal_timerGet() - start) >= xfer->timeout) {
				return -ETIME;
			}
		}

		for (ofs = tfdr32; len > 0; ++ofs) {
			u32 tmp;
			size = len < sizeof(tmp) ? len : sizeof(tmp);
			hal_memcpy(&tmp, buf, size);
			*(fspi->base + ofs) = tmp;
			len -= size;
			buf += size;
		}
		/* Move FIFO pointer to watermark level */
		*(fspi->base + intr) |= 1 << 6;
	}

	/* Reset tx FIFO */
	*(fspi->base + iptxfcr) |= 1;

	return buf - (u8 *)xfer->data.write.ptr;
}


__attribute__((section(".noxip"))) ssize_t flexspi_xferExec(flexspi_t *fspi, struct xferOp *xfer)
{
	u32 dataSize;
	time_t start = hal_timerGet();

	if (xfer->op == xfer_opRead) {
		/* For >64k read out the data directly from the AHB buffer (data may be cached) */
		if (xfer->data.read.sz > 0xffff) {
			hal_memcpy(xfer->data.read.ptr, fspi->ahbAddr + xfer->addr, xfer->data.read.sz);
			return EOK;
		}

		dataSize = xfer->data.read.sz & 0xffff;
	}
	else if (xfer->op == xfer_opWrite) {
		/* IP write is limited to IPDATSZ mask */
		if (xfer->data.read.sz > 0xffff) {
			return -EPERM;
		}

		dataSize = xfer->data.write.sz & 0xffff;
	}
	else {
		dataSize = 0;
	}

	/* Wait for either AHB & IP bus ready or sequence controller to be idle */
	while (((*(fspi->base + sts0) & 3) ^ 3) != 0) {
		if (xfer->timeout > 0 && (hal_timerGet() - start) >= xfer->timeout) {
			return -ETIME;
		}
	}

	/* Clear the instruction pointer */
	*(fspi->base + flsha1cr2 + xfer->port) |= 1 << 31;

	/* Clear any triggered AHB & IP errors and grant timeouts */
	*(fspi->base + intr) |= (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1);

	/* Set device's start address of transfer */
	*(fspi->base + ipcr0) = flexspi_getAddressByPort(fspi, xfer->port, xfer->addr);

	/* Reset tx/rx FIFOs, no DMA */
	*(fspi->base + iptxfcr) = (*(fspi->base + iptxfcr) & ~3) | 1;
	*(fspi->base + iprxfcr) = (*(fspi->base + iprxfcr) & ~3) | 1;

	/* Configure sequence index[number] and set xfer data size using "individual" mode */
	*(fspi->base + ipcr1) = dataSize | ((xfer->seqIdx & 0xf) << 16) | ((xfer->seqNum & 0x7) << 24);

	/* Trigger an IP transfer now */
	*(fspi->base + ipcmd) |= 1;

	switch (xfer->op) {
		case xfer_opWrite:
			return flexspi_opWrite(fspi, start, xfer);

		case xfer_opRead:
			return flexspi_opRead(fspi, start, xfer);

		case xfer_opCommand:
			/* fall-through */
		default:
			break;
	}

	/* Wait for IP command complete */
	while ((*(fspi->base + intr) & 1) == 0) {
		if (xfer->timeout > 0 && (hal_timerGet() - start) >= xfer->timeout) {
			return -ETIME;
		}
	}

	/* Acknowledge */
	*(fspi->base + intr) |= 1;

	return flexspi_checkFlags(fspi);
}
