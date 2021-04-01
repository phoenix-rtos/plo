/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-loader
 *
 * usb client
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

#include "usbphy.h"
#include "client.h"


struct {
	usb_dc_t dc;
	usb_common_data_t data;
} imx_common;


int usbclient_send(int endpt, const void *data, unsigned int len)
{
	dtd_t *res;

	if (len > USB_BUFFER_SIZE)
		return -1;

	if (!imx_common.data.endpts[endpt].caps[USB_ENDPT_DIR_IN].init)
		return -1;

	hal_memcpy(imx_common.data.endpts[endpt].buf[USB_ENDPT_DIR_IN].buffer, data, len);
	res = ctrl_execTransfer(endpt, (u32)imx_common.data.endpts[endpt].buf[USB_ENDPT_DIR_IN].buffer, len, USB_ENDPT_DIR_IN);

	if (DTD_ERROR(res))
		return -1;

	return len - DTD_SIZE(res);
}


int usbclient_rcvEndp0(void *data, unsigned int len)
{
	int res = -1;

	while (imx_common.dc.op != DC_OP_RCV_ENDP0)
		;

	res = imx_common.data.endpts[0].buf[USB_ENDPT_DIR_OUT].len;
	hal_memcpy(data, (const char *)imx_common.data.endpts[0].buf[USB_ENDPT_DIR_OUT].buffer, res); /* copy data to buffer */
	imx_common.dc.op = DC_OP_NONE;

	ctrl_execTransfer(0, (u32)imx_common.data.endpts[0].buf[USB_ENDPT_DIR_IN].buffer, 0, USB_ENDPT_DIR_IN); /* ACK */

	return res;
}


int usbclient_receive(int endpt, void *data, unsigned int len)
{
	dtd_t *dtd;
	int res = -1;

	if (len > USB_BUFFER_SIZE)
		return -1;

	if (!imx_common.data.endpts[endpt].caps[USB_ENDPT_DIR_OUT].init)
		return -1;

	if (endpt) {
		dtd = ctrl_execTransfer(endpt, (u32)imx_common.data.endpts[endpt].buf[USB_ENDPT_DIR_OUT].buffer, USB_BUFFER_SIZE, USB_ENDPT_DIR_OUT);

		if (!DTD_ERROR(dtd)) {
			res = USB_BUFFER_SIZE - DTD_SIZE(dtd);
			if (res > len)
				res = len;

			hal_memcpy(data, (const char *)imx_common.data.endpts[endpt].buf[USB_ENDPT_DIR_OUT].buffer, res);
		}
		else {
			res = -1;
		}
	}
	else {
		res = usbclient_rcvEndp0(data, len);
	}

	return res;
}



int usbclient_intr(u16 irq, void *buff)
{
	ctrl_hfIrq();

	/* Low frequency interrupts, handle for OUT control endpoint */
	if (imx_common.dc.setupstat & 0x1)
		desc_classSetup(&imx_common.dc.setup);

	ctrl_lfIrq();

	return 0;
}


int usbclient_init(usb_desc_list_t *desList)
{
	int i;
	int res = 0;

	imx_common.dc.dev_addr = 0;
	imx_common.dc.base = (void *)phy_getBase();

	phy_init();

	if ((imx_common.data.setupMem = (char *)usbclient_allocBuff(USB_BUFFER_SIZE)) == NULL)
		return -1;

	for (i = 1; i < ENDPOINTS_NUMBER; ++i) {
		imx_common.data.endpts[i].caps[USB_ENDPT_DIR_OUT].init = 0;
		imx_common.data.endpts[i].caps[USB_ENDPT_DIR_IN].init = 0;
	}

	if (desc_init(desList, &imx_common.data, &imx_common.dc) < 0) {
		usbclient_buffReset();
		return -1;
	}

	if (ctrl_init(&imx_common.data, &imx_common.dc) < 0) {
		usbclient_buffReset();
		return -1;
	}

	hal_irqinst(phy_getIrq(), usbclient_intr, (void *)NULL);

	while (imx_common.dc.op != DC_OP_EXIT) {
		if (imx_common.dc.op == DC_OP_INIT) {
			for (i = 1; i < ENDPOINTS_NUMBER; ++i) {
				if ((res = ctrl_endptInit(i, &imx_common.data.endpts[i])) != ERR_NONE) {
					usbclient_destroy();
					return res;
				}
			}
			return res;
		}
	}

	return ERR_NONE;
}


int usbclient_destroy(void)
{
	ctrl_reset();
	usbclient_buffReset();

	phy_reset();

	return 0;
}
