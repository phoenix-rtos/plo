/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * USB client
 *
 * Copyright 2019-2023 Phoenix Systems
 * Author: Kamil Amanowicz, Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "usbphy.h"
#include "client.h"

#include <hal/hal.h>
#include <lib/errno.h>


static struct {
	usb_dc_t dc;
	usb_common_data_t data;
} usbclient_common;


ssize_t usbclient_send(int endpt, const void *data, size_t len)
{
	dtd_t *res;

	if (len > USB_BUFFER_SIZE) {
		return -ENOMEM;
	}

	if (usbclient_common.data.endpts[endpt].caps[USB_ENDPT_DIR_IN].init == 0) {
		return -ENXIO;
	}

	if (usbclient_common.dc.connected == 0) {
		return -ECONNREFUSED;
	}

	hal_memcpy(usbclient_common.data.endpts[endpt].buf[USB_ENDPT_DIR_IN].buffer, data, len);

	res = ctrl_execTransfer(endpt, (u32)usbclient_common.data.endpts[endpt].buf[USB_ENDPT_DIR_IN].buffer, len, USB_ENDPT_DIR_IN);
	if (res == NULL || DTD_ERROR(res) != 0) {
		return -EIO;
	}

	return len - DTD_SIZE(res);
}


ssize_t usbclient_rcvEndp0(void *data, size_t len)
{
	ssize_t res = -EIO;

	(void)len; /* FIXME: unused */

	while ((usbclient_common.dc.op != DC_OP_RCV_ENDP0) && (usbclient_common.dc.op != DC_OP_RCV_ERR)) {
	}

	if (usbclient_common.dc.op != DC_OP_RCV_ERR) {
		res = usbclient_common.data.endpts[0].buf[USB_ENDPT_DIR_OUT].len;
		hal_memcpy(data, usbclient_common.data.endpts[0].buf[USB_ENDPT_DIR_OUT].buffer, res); /* copy data to buffer */
		usbclient_common.dc.op = DC_OP_NONE;

		ctrl_execTransfer(0, (u32)usbclient_common.data.endpts[0].buf[USB_ENDPT_DIR_IN].buffer, 0, USB_ENDPT_DIR_IN); /* ACK */
	}

	return res;
}


ssize_t usbclient_receive(int endpt, void *data, size_t len)
{
	dtd_t *dtd;
	ssize_t res;

	if (len > USB_BUFFER_SIZE) {
		return -ENOMEM;
	}

	if (usbclient_common.data.endpts[endpt].caps[USB_ENDPT_DIR_OUT].init == 0) {
		return -ENXIO;
	}

	if (usbclient_common.dc.connected == 0) {
		return -ECONNREFUSED;
	}

	if (endpt == 0) {
		return usbclient_rcvEndp0(data, len);
	}

	dtd = ctrl_execTransfer(endpt, (u32)usbclient_common.data.endpts[endpt].buf[USB_ENDPT_DIR_OUT].buffer, USB_BUFFER_SIZE, USB_ENDPT_DIR_OUT);

	if (dtd == NULL || DTD_ERROR(dtd) != 0) {
		return -EIO;
	}

	res = USB_BUFFER_SIZE - DTD_SIZE(dtd);
	if (res > len) {
		res = len;
	}

	hal_memcpy(data, usbclient_common.data.endpts[endpt].buf[USB_ENDPT_DIR_OUT].buffer, res);

	return res;
}


static int usbclient_intr(unsigned int irq, void *arg)
{
	unsigned int i;

	(void)irq;
	(void)arg;

	ctrl_hfIrq();

	/* Low frequency interrupts, handle for OUT control endpoint */
	if ((usbclient_common.dc.setupstat & 1) != 0) {
		desc_classSetup(&usbclient_common.dc.setup);
	}

	ctrl_lfIrq();

	/* Initialize endpoints */
	if (usbclient_common.dc.op == DC_OP_INIT) {
		usbclient_common.dc.endptFailed = 0;

		ctrl_initQtd();

		for (i = 1; i < ENDPOINTS_NUMBER; ++i) {
			if (ctrl_endptInit(i, &usbclient_common.data.endpts[i]) < 0) {
				usbclient_common.dc.endptFailed = i;
				break;
			}
		}

		usbclient_common.dc.op = DC_OP_NONE;
	}

	return 0;
}


static void usbclient_cleanData(void)
{
	unsigned int i;
	usbclient_buffReset();

	usbclient_common.data.setupMem = NULL;
	usbclient_common.dc.dtdMem = NULL;
	usbclient_common.dc.endptqh = NULL;

	for (i = 0; i < ENDPOINTS_NUMBER; ++i) {
		if (usbclient_common.data.endpts[i].caps[USB_ENDPT_DIR_IN].init != 0) {
			usbclient_common.data.endpts[i].buf[USB_ENDPT_DIR_IN].buffer = NULL;
		}

		if (usbclient_common.data.endpts[i].caps[USB_ENDPT_DIR_OUT].init != 0) {
			usbclient_common.data.endpts[i].buf[USB_ENDPT_DIR_OUT].buffer = NULL;
		}
	}
}


int usbclient_init(usb_desc_list_t *desList)
{
	int res;
	unsigned int i;

	usbclient_common.dc.endptFailed = 0;

	usbclient_common.dc.connected = 0;
	usbclient_common.dc.dev_addr = 0;
	usbclient_common.dc.base = (void *)phy_getBase();

	phy_init();

	do {
		usbclient_common.data.setupMem = (char *)usbclient_allocBuff(USB_BUFFER_SIZE);
		if (usbclient_common.data.setupMem == NULL) {
			res = -ENOMEM;
			break;
		}

		for (i = 1; i < ENDPOINTS_NUMBER; ++i) {
			usbclient_common.data.endpts[i].caps[USB_ENDPT_DIR_OUT].init = 0;
			usbclient_common.data.endpts[i].caps[USB_ENDPT_DIR_IN].init = 0;
		}

		res = desc_init(desList, &usbclient_common.data, &usbclient_common.dc);
		if (res != EOK) {
			break;
		}

		res = ctrl_init(&usbclient_common.data, &usbclient_common.dc);
		if (res != EOK) {
			break;
		}

		hal_interruptsSet(phy_getIrq(), usbclient_intr, (void *)NULL);
		return EOK;

	} while (0);

	usbclient_cleanData();
	return res;
}


int usbclient_destroy(void)
{
	ctrl_reset();
	hal_interruptsSet(phy_getIrq(), NULL, NULL);
	usbclient_cleanData();

	phy_reset();

	return 0;
}
