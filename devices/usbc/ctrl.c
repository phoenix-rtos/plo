/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-loader
 *
 * usb device controller driver
 *
 * Copyright 2019-2021 Phoenix Systems
 * Author: Kamil Amanowicz, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "../hal.h"
#include "../errors.h"

#include "client.h"
#include "usbphy.h"


struct {
	u32 qtdOffs;

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
	ctrl_common.data->endpts[endpt].buf[dir].buffer = usbclient_allocBuff(USB_BUFFER_SIZE);

	if (ctrl_common.data->endpts[endpt].buf[dir].buffer == NULL)
		return ERR_MEM;

	ctrl_common.data->endpts[endpt].buf[dir].len = 0;

	return ERR_NONE;
}


static void *ctrl_allocQtdMem(void)
{
	int i;

	if (!ctrl_common.qtdOffs) {
		ctrl_common.dc->dtdMem = usbclient_allocBuff(USB_BUFFER_SIZE);

		if (ctrl_common.dc->dtdMem == NULL)
			return NULL;

		for (i = 0; i < USB_BUFFER_SIZE; ++i)
			((char *)ctrl_common.dc->dtdMem)[i] = 0;
	}

	return  (void *)(ctrl_common.dc->dtdMem + 0x40 * (ctrl_common.qtdOffs++));
}


static int ctrl_dtdInit(int endpt, int inQH, int outQh)
{
	dtd_t *dtd;
	int qh = endpt * 2;

	if (!endpt)
		return -1;

	/* initialize dtd for selected endpoints */
	if (outQh) {
		dtd = ctrl_allocQtdMem();
		if (dtd == NULL)
			return ERR_MEM;

		ctrl_common.dc->endptqh[qh].base = (((u32)dtd) & ~0xfff);
		ctrl_common.dc->endptqh[qh].size = 0x40;
		ctrl_common.dc->endptqh[qh].head = dtd;
		ctrl_common.dc->endptqh[qh].tail = dtd;
	}

	if (inQH) {
		dtd = ctrl_allocQtdMem();
		if (dtd == NULL)
			return ERR_MEM;

		ctrl_common.dc->endptqh[++qh].base = (((u32)dtd) & ~0xfff) + (64 * sizeof(dtd_t));
		ctrl_common.dc->endptqh[qh].size = 0x40;
		ctrl_common.dc->endptqh[qh].head = dtd + 64;
		ctrl_common.dc->endptqh[qh].tail = dtd + 64;
	}

	return ERR_NONE;
}


static int ctrl_initEndptQh(int endpt, int dir, endpt_data_t *endpt_init)
{
	u32 setup = 0, tmp;
	int qh = endpt * 2 + dir;

	ctrl_allocBuff(endpt, dir);

	tmp = endpt_init->caps[dir].max_pkt_len << 16;
	tmp |= endpt_init->caps[dir].ios << 15;
	tmp |= endpt_init->caps[dir].zlt << 29;
	tmp |= endpt_init->caps[dir].mult << 30;
	ctrl_common.dc->endptqh[qh].caps = tmp;
	ctrl_common.dc->endptqh[qh].dtd_next = 1;

	setup |= endpt_init->ctrl[dir].data_toggle << (6 + dir * 16);
	setup |= endpt_init->ctrl[dir].type << (2 + dir * 16);

	return setup;
}


int ctrl_endptInit(int endpt, endpt_data_t *endpt_init)
{
	s8 i;
	int res = ERR_NONE;
	u32 setup = 0;

	if (endpt == 0)
		return -1;

	if ((res = ctrl_dtdInit(endpt, endpt_init->caps[USB_ENDPT_DIR_IN].init, endpt_init->caps[USB_ENDPT_DIR_OUT].init)) != ERR_NONE)
		return res;

	for (i = 0; i < ENDPOINTS_DIR_NB; ++i) {
		if (endpt_init->caps[i].init)
			setup |= ctrl_initEndptQh(endpt, i, endpt_init);
	}

	*(ctrl_common.dc->base + endptctrl0 + endpt) = setup;

	for (i = 0; i < ENDPOINTS_DIR_NB; ++i) {
		if (endpt_init->caps[i].init)
			*(ctrl_common.dc->base + endptctrl0 + endpt) |= 1 << (7 + i * 16);
	}

	return res;
}


int ctrl_endpt0Init(void)
{
	int i;
	u32 qh_addr, tmp;

	if (ctrl_allocBuff(0, USB_ENDPT_DIR_IN) < 0)
		return ERR_MEM;

	if (ctrl_allocBuff(0, USB_ENDPT_DIR_OUT) < 0)
		return ERR_MEM;

	ctrl_common.data->endpts[0].caps[USB_ENDPT_DIR_IN].init = 1;
	ctrl_common.data->endpts[0].caps[USB_ENDPT_DIR_OUT].init = 1;

	/* map queue head list */
	ctrl_common.dc->endptqh = usbclient_allocBuff(USB_BUFFER_SIZE);

	if (ctrl_common.dc->endptqh == NULL)
		return ERR_MEM;

	for (i = 0; i < USB_BUFFER_SIZE; ++i)
		((char *)ctrl_common.dc->endptqh)[i] = 0;

	qh_addr = ((u32)((void *)ctrl_common.dc->endptqh)) & ~0xfff;

	tmp =  0x40 << 16 /* max 64 bytes */ | 0x1 << 29 | 0x1 << 15 /* ios */;
	ctrl_common.dc->endptqh[0].caps = tmp;
	ctrl_common.dc->endptqh[0].dtd_next = 0x1; /* invalid */
	ctrl_common.dc->endptqh[1].caps = tmp;
	ctrl_common.dc->endptqh[1].dtd_next = 1;

	ctrl_common.dc->endptqh[0].base = qh_addr + (32 * sizeof(dqh_t));
	ctrl_common.dc->endptqh[0].size = 0x40;
	ctrl_common.dc->endptqh[0].head = (dtd_t *)(ctrl_common.dc->endptqh + 32);
	ctrl_common.dc->endptqh[0].tail = (dtd_t *)(ctrl_common.dc->endptqh + 32);

	ctrl_common.dc->endptqh[1].base = qh_addr + (48 * sizeof(dqh_t));
	ctrl_common.dc->endptqh[1].size = 0x10;
	ctrl_common.dc->endptqh[1].head = (dtd_t *)(ctrl_common.dc->endptqh + 48);
	ctrl_common.dc->endptqh[1].tail = (dtd_t *)(ctrl_common.dc->endptqh + 48);

	*(ctrl_common.dc->base + endpointlistaddr) = qh_addr;
	*(ctrl_common.dc->base + endptprime) |= 1;
	*(ctrl_common.dc->base + endptprime) |= 1 << 16;


	return ERR_NONE;
}


static dtd_t *ctrl_getDtd(int endpt, int dir)
{
	int qh = endpt * 2 + dir;
	u32 base_addr;
	dtd_t *ret;

	base_addr = ((u32)ctrl_common.dc->endptqh[qh].head & ~((ctrl_common.dc->endptqh[qh].size * sizeof(dtd_t)) - 1));

	ret = ctrl_common.dc->endptqh[qh].tail++;
	ctrl_common.dc->endptqh[qh].tail = (dtd_t *)(base_addr | ((u32)ctrl_common.dc->endptqh[qh].tail & (((ctrl_common.dc->endptqh[qh].size) * sizeof(dtd_t)) - 1)));

	return ret;
}


static int ctrl_buildDtd(dtd_t *dtd, u32 paddr, u32 size)
{
	int i = 0;
	int tempSize = size;

	if (size > USB_BUFFER_SIZE)
		return -1;

	dtd->dtd_next = 1;
	dtd->dtd_token = size << 16;
	dtd->dtd_token |= 1 << 7;

	/* TODO: allow to use additional dtd within the same dQH */
	while (tempSize > 0) {
		dtd->buff_ptr[i++] = paddr;
		tempSize -= 0x1000;
		paddr += 0x1000;
	}

	return ERR_NONE;
}


dtd_t *ctrl_execTransfer(int endpt, u32 paddr, u32 sz, int dir)
{
	int shift;
	u32 offs;
	volatile dtd_t *dtd;

	int qh = (endpt << 1) + dir;

	dtd = ctrl_getDtd(endpt, dir);

	ctrl_buildDtd((dtd_t *)dtd, paddr, sz);

	shift = endpt + ((qh & 1) ? 16 : 0);
	offs = (u32)dtd & (((ctrl_common.dc->endptqh[qh].size) * sizeof(dtd_t)) - 1);

	ctrl_common.dc->endptqh[qh].dtd_next = (ctrl_common.dc->endptqh[qh].base + offs) & ~1;
	ctrl_common.dc->endptqh[qh].dtd_token &= ~(1 << 6);
	ctrl_common.dc->endptqh[qh].dtd_token &= ~(1 << 7);

	while ((*(ctrl_common.dc->base + endptprime) & (1 << shift)))
		;

	*(ctrl_common.dc->base + endptprime) |= 1 << shift;

	/* prime the endpoint and wait for it to prime */
	while ((*(ctrl_common.dc->base + endptprime) & (1 << shift)))
		;
	*(ctrl_common.dc->base + endptprime) |= 1 << shift;
	while (!(*(ctrl_common.dc->base + endptprime) & (1 << shift)) && (*(ctrl_common.dc->base + endptstat) & (1 << shift)))
		;

	/* wait to finish transaction */
	while (DTD_ACTIVE(dtd) && !DTD_ERROR(dtd))
		;

	return (dtd_t *)dtd;
}


int ctrl_hfIrq(void)
{
	int endpt = 0;

	if ((ctrl_common.dc->setupstat = *(ctrl_common.dc->base + endptsetupstat)) & 0x1) {
		/* trip winre set */
		while (!((ctrl_common.dc->setupstat >> endpt) & 1)) {
			if (++endpt > 15)
				return -1;
		}

		do {
			*(ctrl_common.dc->base + usbcmd) |= 1 << 13;
			hal_memcpy(&ctrl_common.dc->setup, (void *)ctrl_common.dc->endptqh[endpt].setup_buff, sizeof(usb_setup_packet_t));
		} while (!(*(ctrl_common.dc->base + usbcmd) & 1 << 13));

		*(ctrl_common.dc->base + endptsetupstat) |= 1 << endpt;
		*(ctrl_common.dc->base + usbcmd) &= ~(1 << 13);
		*(ctrl_common.dc->base + usbsts) |= 1;

		ctrl_common.dc->endptqh[0].head = ctrl_common.dc->endptqh[0].tail;
		ctrl_common.dc->endptqh[1].head = ctrl_common.dc->endptqh[1].tail;

		while (*(ctrl_common.dc->base + endptsetupstat) & 1)
			;

		desc_setup(&ctrl_common.dc->setup);
	}

	return 1;
}


int ctrl_lfIrq(void)
{
	if ((*(ctrl_common.dc->base + usbsts) & 1 << 6)) {

		*(ctrl_common.dc->base + endptsetupstat) = *(ctrl_common.dc->base + endptsetupstat);
		*(ctrl_common.dc->base + endptcomplete) = *(ctrl_common.dc->base + endptcomplete);

		while (*(ctrl_common.dc->base + endptprime))
			;

		*(ctrl_common.dc->base + endptflush) = 0xffffffff;

		while (*(ctrl_common.dc->base + portsc1) & 1 << 8)
			;

		*(ctrl_common.dc->base + usbsts) |= 1 << 6;
		ctrl_common.dc->status = DC_DEFAULT;
	}

	return 1;
}


static void ctrl_devInit(void)
{
	*(ctrl_common.dc->base + endptflush) = 0xffffffff;
	/* Run/Stop bit */
	*(ctrl_common.dc->base + usbcmd) &= ~1;
	/* Controller resets its internal pipelines, timers etc. */
	*(ctrl_common.dc->base + usbcmd) |= 1 << 1;
	ctrl_common.dc->status = DC_POWERED;

	/* Run/Stop register is set to 0 when the reset process is complete. */
	while (*(ctrl_common.dc->base + usbcmd) & (1 << 1))
		;

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
}


int ctrl_init(usb_common_data_t *usb_data_in, usb_dc_t *dc_in)
{
	ctrl_common.dc = dc_in;
	ctrl_common.qtdOffs = 0;
	ctrl_common.data = usb_data_in;

	ctrl_devInit();

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
