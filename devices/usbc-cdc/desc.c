/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Descriptors handler
 *
 * Copyright 2019-2023 Phoenix Systems
 * Author: Kamil Amanowicz, Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "client.h"

#include <hal/hal.h>
#include <lib/errno.h>


static struct {
	volatile u32 dev;
	volatile u32 cfg;

	volatile u32 str_0;
	volatile u32 str_man;
	volatile u32 str_prod;

	volatile u32 hid_reports;

	volatile usb_dc_t *dc;
	volatile usb_common_data_t *data;
} desc_common;


static void desc_endptInit(usb_endpoint_desc_t *endpt)
{
	int endNb = endpt->bEndpointAddress & 7;
	int dir = (endpt->bEndpointAddress & (1 << 7)) ? 1 : 0;

	desc_common.data->endpts[endNb].caps[dir].mult = 0;
	desc_common.data->endpts[endNb].caps[dir].zlt = 1;
	desc_common.data->endpts[endNb].caps[dir].max_pkt_len = endpt->wMaxPacketSize;
	desc_common.data->endpts[endNb].caps[dir].ios = 1;
	desc_common.data->endpts[endNb].caps[dir].init = 1;

	desc_common.data->endpts[endNb].ctrl[dir].type = endpt->bmAttributes & 3;
	desc_common.data->endpts[endNb].ctrl[dir].data_toggle = 1;
	desc_common.data->endpts[endNb].ctrl[dir].data_inhibit = 0;
	desc_common.data->endpts[endNb].ctrl[dir].stall = 0;
}


static void desc_strInit(usb_desc_list_t *desList, int *localOffset, int strOrder)
{
	switch (strOrder) {
		case 0:
			desc_common.str_0 = (u32)(desc_common.data->setupMem + *localOffset);
			hal_memcpy((void *)desc_common.str_0, desList->descriptor, sizeof(usb_string_desc_t));
			*localOffset += desList->descriptor->bFunctionLength;
			break;

		case 1:
			desc_common.str_man = (u32)(desc_common.data->setupMem + *localOffset);
			hal_memcpy((void *)desc_common.str_man, desList->descriptor, desList->descriptor->bFunctionLength);
			*localOffset += desList->descriptor->bFunctionLength;
			break;

		case 2:
			desc_common.str_prod = (u32)(desc_common.data->setupMem + *localOffset);
			hal_memcpy((void *)desc_common.str_prod, desList->descriptor, desList->descriptor->bFunctionLength);
			*localOffset += desList->descriptor->bFunctionLength;
			break;

		default:
			break;
	}
}


int desc_init(usb_desc_list_t *desList, usb_common_data_t *usb_data_in, usb_dc_t *dc_in)
{
	int i, localOffset = 0;
	u32 string_desc_count = 0;

	desc_common.dc = dc_in;
	desc_common.data = usb_data_in;

	for (i = 0; i < USB_BUFFER_SIZE; ++i)
		desc_common.data->setupMem[i] = 0;

	/* Extract mandatory descriptors to mapped memory */
	for (; desList != NULL; desList = desList->next) {

		if (localOffset > USB_BUFFER_SIZE) {
			return -1;
		}

		switch (desList->descriptor->bDescriptorType) {
			case USB_DESC_DEVICE:
				desc_common.dev = (u32)(desc_common.data->setupMem + localOffset);
				hal_memcpy((void *)desc_common.dev, desList->descriptor, sizeof(usb_device_desc_t));
				localOffset += desList->descriptor->bFunctionLength;
				break;

			case USB_DESC_CONFIG:
				desc_common.cfg = (u32)(desc_common.data->setupMem + localOffset);
				hal_memcpy((void *)desc_common.cfg, desList->descriptor, sizeof(usb_configuration_desc_t));
				localOffset += desList->descriptor->bFunctionLength;
				break;

			case USB_DESC_INTERFACE:
				hal_memcpy(desc_common.data->setupMem + localOffset, desList->descriptor, desList->descriptor->bFunctionLength);
				localOffset += desList->descriptor->bFunctionLength;
				break;

			case USB_DESC_ENDPOINT:
				hal_memcpy(desc_common.data->setupMem + localOffset, desList->descriptor, desList->descriptor->bFunctionLength);
				localOffset += desList->descriptor->bFunctionLength;
				desc_endptInit((usb_endpoint_desc_t *)desList->descriptor);
				break;

			case USB_DESC_TYPE_HID:
				hal_memcpy(desc_common.data->setupMem + localOffset, desList->descriptor, desList->descriptor->bFunctionLength);
				localOffset += desList->descriptor->bFunctionLength;
				break;

			case USB_DESC_TYPE_CDC_CS_INTERFACE:
				hal_memcpy(desc_common.data->setupMem + localOffset, desList->descriptor, desList->descriptor->bFunctionLength);
				localOffset += desList->descriptor->bFunctionLength;
				break;

			case USB_DESC_TYPE_HID_REPORT:
				desc_common.hid_reports = (u32)(desc_common.data->setupMem + localOffset);
				hal_memcpy((void *)desc_common.hid_reports, &desList->descriptor->bDescriptorSubtype, desList->descriptor->bFunctionLength - 2);
				localOffset += desList->descriptor->bFunctionLength - 2;
				break;

			case USB_DESC_STRING:
				desc_strInit(desList, &localOffset, string_desc_count++);
				break;

			case USB_DESC_TYPE_DEV_QUAL:
			case USB_DESC_TYPE_OTH_SPD_CFG:
			case USB_DESC_TYPE_INTF_PWR:
				/* Not implemented yet */
				break;

			default:
				break;
		}
	}

	return EOK;
}


static void desc_ReqSetAddress(const usb_setup_packet_t *setup)
{
	if (setup->wValue != 0) {
		desc_common.dc->status = DC_ADDRESS;

		desc_common.dc->dev_addr = setup->wValue << 25;
		desc_common.dc->dev_addr |= 1 << 24;

		ctrl_setAddress(desc_common.dc->dev_addr);

		desc_common.dc->op = DC_OP_INIT;
		ctrl_execTransfer(0, (u32)desc_common.data->endpts[0].buf[USB_ENDPT_DIR_IN].buffer, 0, USB_ENDPT_DIR_IN);
	}
	else if (desc_common.dc->status != DC_CONFIGURED) {
		desc_common.dc->status = DC_DEFAULT;
	}
	else {
	}
}


static void desc_ReqSetConfig(void)
{
	if (desc_common.dc->status == DC_ADDRESS) {
		desc_common.dc->status = DC_CONFIGURED;
		ctrl_execTransfer(0, (u32)desc_common.data->endpts[0].buf[USB_ENDPT_DIR_IN].buffer, 0, USB_ENDPT_DIR_IN);
	}
}


static int desc_ReqGetConfig(const usb_setup_packet_t *setup)
{
	if (setup->wValue != 0 || setup->wIndex != 0 || setup->wLength != 1) {
		return EOK;
	}

	if (desc_common.dc->status != DC_CONFIGURED) {
		desc_common.data->endpts[0].buf[USB_ENDPT_DIR_OUT].buffer[0] = 0;
	}
	else {
		desc_common.data->endpts[0].buf[USB_ENDPT_DIR_OUT].buffer[1] = 1;
	}

	ctrl_execTransfer(0, (u32)desc_common.data->endpts[0].buf[USB_ENDPT_DIR_OUT].buffer, setup->wLength, USB_ENDPT_DIR_OUT);

	return EOK;
}


static void desc_ReqGetDescriptor(const usb_setup_packet_t *setup)
{
	usb_string_desc_t *strdesc;

	if ((setup->wValue >> 8) == USB_DESC_DEVICE) {
		ctrl_execTransfer(0, desc_common.dev, sizeof(usb_device_desc_t), USB_ENDPT_DIR_IN);
	}
	else if ((setup->wValue >> 8) == USB_DESC_CONFIG) {
		ctrl_execTransfer(0, desc_common.cfg, setup->wLength, USB_ENDPT_DIR_IN);
	}
	else if ((setup->wValue >> 8) == USB_DESC_STRING) {
		if ((setup->wValue & 0xff) == 0) {
			strdesc = (usb_string_desc_t *)desc_common.str_0;
			ctrl_execTransfer(0, desc_common.str_0, MIN(strdesc->bLength, setup->wLength), USB_ENDPT_DIR_IN);
		}
		else if ((setup->wValue & 0xff) == 1) {
			strdesc = (usb_string_desc_t *)desc_common.str_man;
			ctrl_execTransfer(0, desc_common.str_man, MIN(strdesc->bLength, setup->wLength), USB_ENDPT_DIR_IN);
		}
		else if ((setup->wValue & 0xff) == 2) {
			strdesc = (usb_string_desc_t *)desc_common.str_prod;
			ctrl_execTransfer(0, desc_common.str_prod, MIN(strdesc->bLength, setup->wLength), USB_ENDPT_DIR_IN);
		}
		else if ((setup->wValue & 0xff) == 4) {
			ctrl_execTransfer(0, (u32)desc_common.data->endpts[0].buf[USB_ENDPT_DIR_IN].buffer, 0, USB_ENDPT_DIR_IN);
			ctrl_execTransfer(0, (u32)desc_common.data->endpts[0].buf[USB_ENDPT_DIR_OUT].buffer, 71, USB_ENDPT_DIR_OUT);
			return;
		}
		else {
		}
	}
	else if ((setup->wValue >> 8) == USB_DESC_TYPE_HID_REPORT) {
		ctrl_execTransfer(0, desc_common.hid_reports, 76, USB_ENDPT_DIR_IN);
	}
	else {
	}

	ctrl_execTransfer(0, (u32)desc_common.data->endpts[0].buf[USB_ENDPT_DIR_OUT].buffer, 64, USB_ENDPT_DIR_OUT);
}


static void desc_defaultSetup(const usb_setup_packet_t *setup)
{
	if (*(u32 *)setup == 0xdeadc0de) {
		desc_common.dc->op = DC_OP_EXIT;
	}
	else {
		ctrl_execTransfer(0, (u32)desc_common.data->endpts[0].buf[USB_ENDPT_DIR_OUT].buffer, setup->wLength, USB_ENDPT_DIR_OUT);
		ctrl_execTransfer(0, (u32)desc_common.data->endpts[0].buf[USB_ENDPT_DIR_IN].buffer, 0, USB_ENDPT_DIR_IN);
		desc_common.data->endpts[0].buf[USB_ENDPT_DIR_OUT].buffer[0] = 0;

		ctrl_execTransfer(1, (u32)desc_common.data->endpts[0].buf[USB_ENDPT_DIR_OUT].buffer, 64 + 64, USB_ENDPT_DIR_OUT);
		desc_common.dc->op = DC_OP_RCV_ENDP0;
	}
}


int desc_setup(const usb_setup_packet_t *setup)
{
	int res = EOK;

	if (EXTRACT_REQ_TYPE(setup->bmRequestType) != REQUEST_TYPE_STANDARD) {
		return EOK;
	}

	switch (setup->bRequest) {
		case REQ_SET_ADDRESS:
			desc_ReqSetAddress(setup);
			break;

		case REQ_SET_CONFIGURATION:
			desc_ReqSetConfig();
			break;

		case REQ_GET_DESCRIPTOR:
			desc_ReqGetDescriptor(setup);
			break;

		case REQ_CLEAR_FEATURE:
		case REQ_GET_STATUS:
		case REQ_GET_INTERFACE:
		case REQ_SET_INTERFACE:
		case REQ_SET_FEATURE:
		case REQ_SET_DESCRIPTOR:
		case REQ_SYNCH_FRAME:
			break;

		case REQ_GET_CONFIGURATION:
			desc_ReqGetConfig(setup);
			break;

		default:
			desc_defaultSetup(setup);
			break;
	}

	return res;
}


static void desc_ClassReqSetReport(const usb_setup_packet_t *setup)
{
	/* read data to buffer with URB struct*/
	dtd_t *dtd = ctrl_execTransfer(0, (u32)desc_common.data->endpts[0].buf[USB_ENDPT_DIR_OUT].buffer, 64 + setup->wLength, USB_ENDPT_DIR_OUT);

	if (dtd == NULL || DTD_ERROR(dtd) != 0) {
		/* mark that data is ready */
		desc_common.dc->op = DC_OP_RCV_ENDP0;
		desc_common.data->endpts[0].buf[USB_ENDPT_DIR_OUT].len = 64 + setup->wLength - DTD_SIZE(dtd);
	}
	else {
		/* mark that data is incomplete */
		desc_common.dc->op = DC_OP_RCV_ERR;
		desc_common.data->endpts[0].buf[USB_ENDPT_DIR_OUT].len = -1;
	}
}


int desc_classSetup(const usb_setup_packet_t *setup)
{
	int res = EOK;
	dtd_t *dtd;

	if (EXTRACT_REQ_TYPE(setup->bmRequestType) != REQUEST_TYPE_CLASS) {
		return EOK;
	}

	switch (setup->bRequest) {
		case CLASS_REQ_SET_IDLE:
			ctrl_execTransfer(0, (u32)desc_common.data->endpts[0].buf[USB_ENDPT_DIR_IN].buffer, 0, USB_ENDPT_DIR_IN);
			break;

		case CLASS_REQ_SET_REPORT:
			desc_ClassReqSetReport(setup);
			break;

		case CLASS_REQ_GET_IDLE:
		case CLASS_REQ_GET_PROTOCOL:
		case CLASS_REQ_GET_REPORT:
		case CLASS_REQ_SET_PROTOCOL:
			break;

		case CLASS_REQ_SET_LINE_CODING:
			/* read data to buffer with URB struct */
			dtd = ctrl_execTransfer(0, (u32)desc_common.data->endpts[0].buf[USB_ENDPT_DIR_OUT].buffer, 64 + setup->wLength, USB_ENDPT_DIR_OUT);
			if (dtd == NULL || DTD_ERROR(dtd) != 0) {
				/* mark that data is incomplete */
				desc_common.dc->op = DC_OP_RCV_ERR;
				desc_common.data->endpts[0].buf[USB_ENDPT_DIR_OUT].len = -1;
				return -1;
			}
			/* ACK */
			ctrl_execTransfer(0, (u32)desc_common.data->endpts[0].buf[USB_ENDPT_DIR_IN].buffer, 0, USB_ENDPT_DIR_IN);
			break;

		case CLASS_REQ_SET_CONTROL_LINE_STATE:
			/* ACK */
			ctrl_execTransfer(0, (u32)desc_common.data->endpts[0].buf[USB_ENDPT_DIR_IN].buffer, 0, USB_ENDPT_DIR_IN);
			break;

		default:
			break;
	}

	return res;
}
