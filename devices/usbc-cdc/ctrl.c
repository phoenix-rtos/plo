/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * USB device controller driver
 *
 * Copyright 2019-2023 Phoenix Systems
 * Author: Kamil Amanowicz, Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "client.h"
#include "usbphy.h"

#include <hal/hal.h>
#include <lib/errno.h>


#define USBCTRL_TIMEOUT 250


static struct {
	volatile u32 qtdOffs;

	usb_dc_t *dc;
	usb_common_data_t *data;
} ctrl_common;


/* device cotroller register offsets */
enum {
	/* identification regs */
	id = 0x0, hwgeneral, hwhost, hwdevice, hwtxbuf, hwrxbuf,

	/* operational regs */
	gptimer0ld  = 0x20, gptimer0ctrl, gptimer1ld, gptimer1ctrl, sbuscfg,

	/* capability regs */
	caplength = 0x40, hciversion = 0x40, hcsparams, hccparams,
	dciversion = 0x48, dccparams,

	/* operational regs cont. */
	usbcmd = 0x50, usbsts, usbintr, frindex,
	periodiclistbase = 0x55, deviceaddr = 0x55, asynclistaddr = 0x56,
	endpointlistaddr = 0x56, burstsize = 0x58, txfilltunning, endptnak = 0x5E,
	endptnaken, configflag, portsc1, otgsc = 0x69, usbmode, endptsetupstat,
	endptprime, endptflush, endptstat, endptcomplete, endptctrl0, endptctrl1,
	endptctrl2, endptctrl3, endptctrl4, endptctrl5, endptctrl6, endptctrl7
};


static int ctrl_allocBuff(int endpt, int dir)
{
	/* Allocate buffer for the first time (initially is NULL) */
	if (ctrl_common.data->endpts[endpt].buf[dir].buffer == NULL) {
		ctrl_common.data->endpts[endpt].buf[dir].buffer = usbclient_allocBuff(USB_BUFFER_SIZE);
		if (ctrl_common.data->endpts[endpt].buf[dir].buffer == NULL) {
			return -ENOMEM;
		}
	}

	ctrl_common.data->endpts[endpt].buf[dir].len = 0;

	return EOK;
}


static void *ctrl_allocQtdMem(void)
{
	if (ctrl_common.qtdOffs == 0) {
		/* Allocate buffer for the first time (initially dtdMem is NULL) */
		if (ctrl_common.dc->dtdMem == NULL) {
			ctrl_common.dc->dtdMem = usbclient_allocBuff(USB_BUFFER_SIZE);
			if (ctrl_common.dc->dtdMem == NULL) {
				return NULL;
			}
		}

		hal_memset(ctrl_common.dc->dtdMem, 0, USB_BUFFER_SIZE);
	}

	return (void *)(ctrl_common.dc->dtdMem + (addr_t)64 * (ctrl_common.qtdOffs++));
}


static int ctrl_dtdInit(int endpt, int inQH, int outQH)
{
	dtd_t *dtd;
	int qh = endpt * 2;

	/* initialize dtd for selected endpoints */
	if (outQH != 0) {
		dtd = ctrl_allocQtdMem();
		if (dtd == NULL) {
			return -ENOMEM;
		}

		ctrl_common.dc->endptqh[qh].base = (((u32)dtd) & ~(USB_BUFFER_SIZE - 1));
		ctrl_common.dc->endptqh[qh].size = 64;
		ctrl_common.dc->endptqh[qh].head = dtd;
		ctrl_common.dc->endptqh[qh].tail = dtd;
	}

	if (inQH != 0) {
		dtd = ctrl_allocQtdMem();
		if (dtd == NULL) {
			return -ENOMEM;
		}

		qh++;
		ctrl_common.dc->endptqh[qh].base = (((u32)dtd) & ~(USB_BUFFER_SIZE - 1)) + (64 * sizeof(dtd_t));
		ctrl_common.dc->endptqh[qh].size = 64;
		ctrl_common.dc->endptqh[qh].head = dtd + 64;
		ctrl_common.dc->endptqh[qh].tail = dtd + 64;
	}

	return EOK;
}


static u32 ctrl_initEndptQh(int endpt, int dir, endpt_data_t *endpt_init)
{
	u32 setup, caps;
	int qh = endpt * 2 + dir;

	if (ctrl_allocBuff(endpt, dir) < 0) {
		return -ENOMEM;
	}

	caps = (endpt_init->caps[dir].max_pkt_len & 0x7ff) << 16;
	caps |= (endpt_init->caps[dir].ios & 1) << 15;
	caps |= (endpt_init->caps[dir].zlt & 1) << 29;
	caps |= (endpt_init->caps[dir].mult & 3) << 30;
	ctrl_common.dc->endptqh[qh].caps = caps;
	ctrl_common.dc->endptqh[qh].dtd_next = 1;

	setup = (endpt_init->ctrl[dir].data_toggle & 1) << 6;
	setup |= (endpt_init->ctrl[dir].type & 3) << 2;
	setup |= (endpt_init->ctrl[dir].stall & 1);

	return dir != 0 ? setup << 16 : setup;
}


void ctrl_initQtd(void)
{
	/*
	 * Reset qtd offset for non control endpoints every time device is re/connected to
	 * host, at any time when desc_setup(REQ_SET_ADDRESS) is initiated
	 */

	ctrl_common.qtdOffs = 0;
}


int ctrl_endptInit(int endpt, endpt_data_t *endpt_init)
{
	u8 i;
	int res = EOK;
	u32 setup = 0;

	if (endpt <= 0) {
		return -ENXIO;
	}

	res = ctrl_dtdInit(endpt, endpt_init->caps[USB_ENDPT_DIR_IN].init, endpt_init->caps[USB_ENDPT_DIR_OUT].init);
	if (res != EOK) {
		return res;
	}

	/* Setup RX & TX endpoints */
	for (i = 0; i < ENDPOINTS_DIR_NB; ++i) {
		if (endpt_init->caps[i].init != 0) {
			setup |= ctrl_initEndptQh(endpt, i, endpt_init);
		}
	}

	*(ctrl_common.dc->base + endptctrl0 + endpt) = setup;

	/* Enable endpoints */
	for (i = 0; i < ENDPOINTS_DIR_NB; ++i) {
		if (endpt_init->caps[i].init != 0) {
			*(ctrl_common.dc->base + endptctrl0 + endpt) |= 1U << (7 + i * 16);
		}
	}

	return res;
}


int ctrl_endpt0Init(void)
{
	u32 qh_addr, caps;

	if (ctrl_allocBuff(0, USB_ENDPT_DIR_IN) != EOK) {
		return -ENOMEM;
	}

	if (ctrl_allocBuff(0, USB_ENDPT_DIR_OUT) != EOK) {
		return -ENOMEM;
	}

	ctrl_common.data->endpts[0].caps[USB_ENDPT_DIR_IN].init = 1;
	ctrl_common.data->endpts[0].caps[USB_ENDPT_DIR_OUT].init = 1;

	/* map queue head list for the first time (initially endptqh is NULL) */
	if (ctrl_common.dc->endptqh == NULL) {
		ctrl_common.dc->endptqh = usbclient_allocBuff(USB_BUFFER_SIZE);

		if (ctrl_common.dc->endptqh == NULL) {
			return -ENOMEM;
		}
	}

	hal_memset((void *)ctrl_common.dc->endptqh, 0, USB_BUFFER_SIZE);

	qh_addr = ((u32)((void *)ctrl_common.dc->endptqh)) & ~(USB_BUFFER_SIZE - 1);

	/* ctrl packet_len=64, zlt=1, ios=1 */
	caps = (64 << 16) | (1 << 29) | (1 << 15);

	ctrl_common.dc->endptqh[0].size = 64;
	ctrl_common.dc->endptqh[0].caps = caps;
	ctrl_common.dc->endptqh[0].dtd_next = 1;
	ctrl_common.dc->endptqh[0].base = qh_addr + (32 * sizeof(dqh_t));
	ctrl_common.dc->endptqh[0].head = (dtd_t *)(ctrl_common.dc->endptqh + 32);
	ctrl_common.dc->endptqh[0].tail = (dtd_t *)(ctrl_common.dc->endptqh + 32);

	ctrl_common.dc->endptqh[1].size = 16;
	ctrl_common.dc->endptqh[1].caps = caps;
	ctrl_common.dc->endptqh[1].dtd_next = 1;
	ctrl_common.dc->endptqh[1].base = qh_addr + (48 * sizeof(dqh_t));
	ctrl_common.dc->endptqh[1].head = (dtd_t *)(ctrl_common.dc->endptqh + 48);
	ctrl_common.dc->endptqh[1].tail = (dtd_t *)(ctrl_common.dc->endptqh + 48);

	*(ctrl_common.dc->base + endpointlistaddr) = qh_addr;
	*(ctrl_common.dc->base + endptprime) |= 1;
	*(ctrl_common.dc->base + endptprime) |= 1 << 16;


	return EOK;
}


static dtd_t *ctrl_getDtd(int endpt, int dir)
{
	int qh = endpt * 2 + dir;
	dtd_t *ret = ctrl_common.dc->endptqh[qh].tail;
	u32 qhmask = ((ctrl_common.dc->endptqh[qh].size) * sizeof(dtd_t)) - 1;
	ctrl_common.dc->endptqh[qh].tail = (dtd_t *)((u32)(ctrl_common.dc->endptqh[qh].tail + 1) & ~qhmask);

	return ret;
}


static int ctrl_buildDtd(dtd_t *dtd, u32 paddr, u32 size)
{
	int i = 0;
	int tempSize = size;

	if (size > USB_BUFFER_SIZE) {
		return -ENOMEM;
	}

	dtd->dtd_next = 1;
	dtd->dtd_token = size << 16;
	dtd->dtd_token |= 1 << 7;

	/* TODO: allow to use additional dtd within the same dQH */
	while (tempSize > 0) {
		dtd->buff_ptr[i++] = paddr;
		tempSize -= 0x1000;
		paddr += 0x1000;
	}

	return EOK;
}


dtd_t *ctrl_execTransfer(int endpt, u32 paddr, u32 sz, int dir)
{
	time_t endts;
	int shift;
	u32 offs;
	volatile dtd_t *dtd;

	int qh = (endpt << 1) + dir;

	if (ctrl_common.dc->connected != 0) {
		dtd = ctrl_getDtd(endpt, dir);

		ctrl_buildDtd((dtd_t *)dtd, paddr, sz);

		shift = endpt + ((qh & 1) ? 16 : 0);
		offs = (u32)dtd & (((ctrl_common.dc->endptqh[qh].size) * sizeof(dtd_t)) - 1);

		ctrl_common.dc->endptqh[qh].dtd_next = (ctrl_common.dc->endptqh[qh].base + offs) & ~1;
		ctrl_common.dc->endptqh[qh].dtd_token &= ~(1 << 6);
		ctrl_common.dc->endptqh[qh].dtd_token &= ~(1 << 7);

		while ((*(ctrl_common.dc->base + endptprime) & (1U << shift)) != 0) {
		}

		*(ctrl_common.dc->base + endptprime) |= 1U << shift;

		/* prime the endpoint and wait for it to prime */
		while ((*(ctrl_common.dc->base + endptprime) & (1U << shift)) != 0) {
		}

		*(ctrl_common.dc->base + endptprime) |= 1U << shift;

		endts = hal_timerGet() + USBCTRL_TIMEOUT;
		while ((*(ctrl_common.dc->base + endptprime) & (1U << shift)) == 0 &&
			(*(ctrl_common.dc->base + endptstat) & (1U << shift)) != 0 &&
			(*(ctrl_common.dc->base + portsc1) & 1) != 0) {
			if (hal_timerGet() > endts) {
				return NULL;
			}
		}

		/* wait to finish transaction while device is attached to the host */
		while (DTD_ACTIVE(dtd) != 0 &&
			DTD_ERROR(dtd) == 0 &&
			(*(ctrl_common.dc->base + portsc1) & 1) != 0) {
			if (hal_timerGet() > endts) {
				return NULL;
			}
		}

		/* check if device is attached */
		if ((*(ctrl_common.dc->base + portsc1) & 1) != 0) {
			return (dtd_t *)dtd;
		}
	}

	return NULL;
}


void ctrl_hfIrq(void)
{
	int endpt = 0;

	ctrl_common.dc->setupstat = *(ctrl_common.dc->base + endptsetupstat);
	if ((ctrl_common.dc->setupstat & 1) != 0) {
		/* trip winre set */
		while (((ctrl_common.dc->setupstat >> endpt) & 1) == 0) {
			if (++endpt > 15) {
				/* Clear USB interrupt */
				*(ctrl_common.dc->base + usbsts) |= 1;
				return;
			}
		}

		/* Trip wire: ensure that the setup data payload is extracted from a QH by the DCD without being corrupted */
		do {
			*(ctrl_common.dc->base + usbcmd) |= 1 << 13;
			hal_memcpy(&ctrl_common.dc->setup, (void *)ctrl_common.dc->endptqh[endpt].setup_buff, sizeof(usb_setup_packet_t));
		} while ((*(ctrl_common.dc->base + usbcmd) & 1 << 13) == 0);

		/* Acknowledge setup transfer */
		*(ctrl_common.dc->base + endptsetupstat) |= 1U << endpt;

		/* Trip wire semaphore clear */
		*(ctrl_common.dc->base + usbcmd) &= ~(1 << 13);

		/* Clear USB interrupt */
		*(ctrl_common.dc->base + usbsts) |= 1;

		ctrl_common.dc->endptqh[0].head = ctrl_common.dc->endptqh[0].tail;
		ctrl_common.dc->endptqh[1].head = ctrl_common.dc->endptqh[1].tail;

		while ((*(ctrl_common.dc->base + endptsetupstat) & 1) != 0) {
		}

		desc_setup(&ctrl_common.dc->setup);
	}
	else {
		/* Clear USB interrupt */
		*(ctrl_common.dc->base + usbsts) |= 1;
	}
}


void ctrl_lfIrq(void)
{
	u8 prevState = ctrl_common.dc->connected;

	ctrl_common.dc->connected = (*(ctrl_common.dc->base + portsc1) & 1) && (*(ctrl_common.dc->base + otgsc) & 1 << 9);

	/* Reset received */
	if ((*(ctrl_common.dc->base + usbsts) & 1 << 6) != 0) {

		*(ctrl_common.dc->base + endptsetupstat) = *(ctrl_common.dc->base + endptsetupstat);
		*(ctrl_common.dc->base + endptcomplete) = *(ctrl_common.dc->base + endptcomplete);

		while ((*(ctrl_common.dc->base + endptprime)) != 0) {
		}

		*(ctrl_common.dc->base + endptflush) = 0xffffffff;

		while ((*(ctrl_common.dc->base + portsc1) & 1 << 8) != 0) {
		}

		*(ctrl_common.dc->base + usbsts) |= 1 << 6;
		ctrl_common.dc->status = DC_DEFAULT;
	}

	/* Connect state has changed */
	if (ctrl_common.dc->connected != prevState) {
		while ((*(ctrl_common.dc->base + endptprime)) != 0) {
		}

		*(ctrl_common.dc->base + endptflush) = 0xffffffff;
	}
}


static int ctrl_devInit(void)
{
	time_t endts = hal_timerGet() + USBCTRL_TIMEOUT;

	*(ctrl_common.dc->base + endptflush) = 0xffffffff;
	/* Run/Stop bit */
	*(ctrl_common.dc->base + usbcmd) &= ~1;
	/* Controller resets its internal pipelines, timers etc. */
	*(ctrl_common.dc->base + usbcmd) |= 1 << 1;
	ctrl_common.dc->status = DC_POWERED;

	/* Run/Stop register is set to 0 when the reset process is complete. */
	while ((*(ctrl_common.dc->base + usbcmd) & (1 << 1)) != 0) {
		if (hal_timerGet() > endts) {
			return -ETIME;
		}
	}

	/* set usb mode to device */
	*(ctrl_common.dc->base + usbmode) |= 2;
	/* trip wire mode (setup lockout mode disabled) */
	*(ctrl_common.dc->base + usbmode) |= 1 << 3;

	*(ctrl_common.dc->base + usbintr) |= 0x57;
	*(ctrl_common.dc->base + usbintr) |= 3 << 18;
	*(ctrl_common.dc->base + usbintr) |= 1 << 6;

	*(ctrl_common.dc->base + usbsts) |= 1;

	ctrl_common.dc->status = DC_ATTACHED;
	*(ctrl_common.dc->base + usbcmd) |= 1;

	return EOK;
}


int ctrl_init(usb_common_data_t *usb_data_in, usb_dc_t *dc_in)
{
	int res;

	ctrl_common.dc = dc_in;
	ctrl_common.data = usb_data_in;

	ctrl_initQtd();

	res = ctrl_devInit();
	if (res != EOK) {
		ctrl_reset();
		return res;
	}

	return ctrl_endpt0Init();
}


void ctrl_setAddress(u32 addr)
{
	*(ctrl_common.dc->base + deviceaddr) = addr;
}


void ctrl_reset(void)
{
	/* stop controller */
	*(ctrl_common.dc->base + endptflush) = 0xffffffff;
	*(ctrl_common.dc->base + usbintr) = 0;

	/* reset controller */
	*(ctrl_common.dc->base + usbcmd) &= ~1;
	*(ctrl_common.dc->base + usbcmd) |= 2;
}
