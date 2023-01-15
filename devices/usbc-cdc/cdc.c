/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * CDC - USB Communication Device Class
 *
 * Copyright 2019, 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cdc.h"
#include "usbclient.h"

#include <devices/devs.h>
#include <hal/hal.h>
#include <lib/lib.h>


#define SIZE_USB_ENDPTS (2 * (PHFS_ACM_PORTS_NB) + 1) /* Add control endpoint */


struct {
	usb_desc_list_t *descList;

	usb_desc_list_t dev;
	usb_desc_list_t conf;

	struct {
		usb_desc_list_t iad;

		usb_desc_list_t comIface;
		usb_desc_list_t header;
		usb_desc_list_t call;
		usb_desc_list_t acm;
		usb_desc_list_t unio;
		usb_desc_list_t comEp;

		usb_desc_list_t dataIface;
		usb_desc_list_t dataEpOUT;
		usb_desc_list_t dataEpIN;
	} ports[PHFS_ACM_PORTS_NB];

	usb_desc_list_t str0;
	usb_desc_list_t strman;
	usb_desc_list_t strprod;

	u32 init;
} cdc_common;


/* Endpoints available in a CDC driver */
enum { endpt_irq_acm0 = 0x01, endpt_bulk_acm0, endpt_irq_acm1, endpt_bulk_acm1 };


/* Device descriptor */
const usb_device_desc_t dDev = {
	.bLength = sizeof(usb_device_desc_t),
	.bDescriptorType = USB_DESC_DEVICE,
	.bcdUSB = 0x0002,
	.bDeviceClass = 0x0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x16f9,
	.idProduct = 0x0003,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 0,
	.bNumConfigurations = 1
};


/* Configuration descriptor */
const usb_configuration_desc_t dConfig = {
	.bLength = 9, .bDescriptorType = USB_DESC_CONFIG,
	.wTotalLength = sizeof(usb_configuration_desc_t) + (sizeof(usb_interface_desc_t) + sizeof(usb_desc_cdc_header_t) + sizeof(usb_desc_cdc_call_t)
				+ sizeof(usb_desc_cdc_acm_t) + sizeof(usb_desc_cdc_union_t) + sizeof(usb_endpoint_desc_t) + sizeof(usb_interface_desc_t)
				+ sizeof(usb_endpoint_desc_t) + sizeof(usb_endpoint_desc_t)) * PHFS_ACM_PORTS_NB,
	.bNumInterfaces = 2 * PHFS_ACM_PORTS_NB,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0xc0,
	.bMaxPower = 5
};


const usb_interface_association_desc_t dIad[] = {
	{
		.bLength = 0x08,
		.bDescriptorType = 0x0b,
		.bFirstInterface = 0,
		.bInterfaceCount = 2,
		.bFunctionClass = 2,
		.bFunctionSubClass = 2,
		.bFunctionProtocol = 1,
		.iFunction = 0,
	},
#if PHFS_ACM_PORTS_NB == 2
	{
		.bLength = 0x08,
		.bDescriptorType = 0x0b,
		.bFirstInterface = 0x2,
		.bInterfaceCount = 2,
		.bFunctionClass = 2,
		.bFunctionSubClass = 2,
		.bFunctionProtocol = 1,
		.iFunction = 0,
	}
#endif
};

/* Communication Interface Descriptor */
const usb_interface_desc_t dComIntf[] = {
	{
		.bLength = 9,
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 1,
		.bInterfaceClass = 0x02,
		.bInterfaceSubClass = 0x02,
		.bInterfaceProtocol = 0x00,
		.iInterface = 4
	},
#if PHFS_ACM_PORTS_NB == 2
	{
		.bLength = 9,
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 2,
		.bAlternateSetting = 0,
		.bNumEndpoints = 1,
		.bInterfaceClass = 0x02,
		.bInterfaceSubClass = 0x02,
		.bInterfaceProtocol = 0x00,
		.iInterface = 4
	}
#endif
};


const usb_desc_cdc_header_t dHeader[] = {
	{
		.bLength = 5,
		.bType = USB_DESC_TYPE_CDC_CS_INTERFACE,
		.bSubType = 0,
		.bcdCDC = 0x0110
	},
#if PHFS_ACM_PORTS_NB == 2
	{
		.bLength = 5,
		.bType = USB_DESC_TYPE_CDC_CS_INTERFACE,
		.bSubType = 0,
		.bcdCDC = 0x0110
	}
#endif
};


const usb_desc_cdc_call_t dCall[] = {
	{
		.bLength = 5,
		.bType = USB_DESC_TYPE_CDC_CS_INTERFACE,
		.bSubType = 0x01,
		.bmCapabilities = 0x01,
		.bDataInterface = 0x1
	},
#if PHFS_ACM_PORTS_NB == 2
	{
		.bLength = 5,
		.bType = USB_DESC_TYPE_CDC_CS_INTERFACE,
		.bSubType = 0x01,
		.bmCapabilities = 0x01,
		.bDataInterface = 0x1
	}
#endif
};


const usb_desc_cdc_acm_t dAcm[] = {
	{
		.bLength = 4,
		.bType = USB_DESC_TYPE_CDC_CS_INTERFACE,
		.bSubType = 0x02,
		.bmCapabilities = 0x03
	},
#if PHFS_ACM_PORTS_NB == 2
	{
		.bLength = 4,
		.bType = USB_DESC_TYPE_CDC_CS_INTERFACE,
		.bSubType = 0x02,
		.bmCapabilities = 0x03
	}
#endif
};


const usb_desc_cdc_union_t dUnion[] = {
	{
		.bLength = 5,
		.bType = USB_DESC_TYPE_CDC_CS_INTERFACE,
		.bSubType = 0x06,
		.bControlInterface = 0x0,
		.bSubordinateInterface = 0x1
	},
#if PHFS_ACM_PORTS_NB == 2
	{
		.bLength = 5,
		.bType = USB_DESC_TYPE_CDC_CS_INTERFACE,
		.bSubType = 0x06,
		.bControlInterface = 0x2,
		.bSubordinateInterface = 0x3
	}
#endif
};


/* Communication Interrupt Endpoint IN */
const usb_endpoint_desc_t dComEp[] = {
	{
		.bLength = 7,
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x80 | endpt_irq_acm0, /* direction IN */
		.bmAttributes = 0x03,
		.wMaxPacketSize = 0x20,
		.bInterval = 0x08
	},
#if PHFS_ACM_PORTS_NB == 2
	{
		.bLength = 7,
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x80 | endpt_irq_acm1, /* direction IN */
		.bmAttributes = 0x03,
		.wMaxPacketSize = 0x20,
		.bInterval = 0x08
	}
#endif
};


/* CDC Data Interface Descriptor */
const usb_interface_desc_t dDataIntf[] = {
	{
		.bLength = 9,
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 1,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = 0x0a,
		.bInterfaceSubClass = 0x00,
		.bInterfaceProtocol = 0x00,
		.iInterface = 0
	},
#if PHFS_ACM_PORTS_NB == 2
	{
		.bLength = 9,
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 3,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = 0x0a,
		.bInterfaceSubClass = 0x00,
		.bInterfaceProtocol = 0x00,
		.iInterface = 0
	}
#endif
};


/* Data Bulk Endpoint OUT */
const usb_endpoint_desc_t dEpOUT[] = {
	{
		.bLength = 7,
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = endpt_bulk_acm0, /* direction OUT */
		.bmAttributes = 0x02,
		.wMaxPacketSize = 0x0200,
		.bInterval = 0
	},
#if PHFS_ACM_PORTS_NB == 2
	{
		.bLength = 7,
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = endpt_bulk_acm1, /* direction OUT */
		.bmAttributes = 0x02,
		.wMaxPacketSize = 0x0200,
		.bInterval = 0
	}
#endif
};


/* Data Bulk Endpoint IN */
const usb_endpoint_desc_t dEpIN[] = {
	{
		.bLength = 7,
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x80 | endpt_bulk_acm0, /* direction IN */
		.bmAttributes = 0x02,
		.wMaxPacketSize = 0x0200,
		.bInterval = 1
	},
#if PHFS_ACM_PORTS_NB == 2
	{
		.bLength = 7,
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x80 | endpt_bulk_acm1, /* direction IN */
		.bmAttributes = 0x02,
		.wMaxPacketSize = 0x0200,
		.bInterval = 1
	}
#endif
};


/* String Data */
const usb_string_desc_t dStrman = {
	.bLength = 2 * 15 + 2,
	.bDescriptorType = USB_DESC_STRING,
	.wData = { 'P', 0, 'h', 0, 'o', 0, 'e', 0, 'n', 0, 'i', 0, 'x', 0, ' ', 0, 'S', 0, 'y', 0, 's', 0, 't', 0, 'e', 0, 'm', 0, 's', 0 }
};


const usb_string_desc_t dStr0 = {
	.bLength = 4,
	.bDescriptorType = USB_DESC_STRING,
	.wData = { 0x09, 0x04 } /* English */
};


const usb_string_desc_t dStrprod = {
	.bLength = 2 * 11 + 2,
	.bDescriptorType = USB_DESC_STRING,
	.wData = { 'p', 0, 'l', 0, 'o', 0, ' ', 0, 'C', 0, 'D', 0, 'C', 0, ' ', 0, 'A', 0, 'C', 0, 'M', 0 }
};


static ssize_t cdc_recv(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	(void)offs;

	if (minor > SIZE_USB_ENDPTS) {
		return -EINVAL;
	}

	return usbclient_receive(minor, buff, len);
}


static ssize_t cdc_send(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	(void)offs;

	if (minor > SIZE_USB_ENDPTS) {
		return -EINVAL;
	}

	return usbclient_send(minor, buff, len);
}


static int cdc_initUsbClient(void)
{
	int i;
	int res;

	if (cdc_common.init != 0) {
		/* Already initialized */
		return EOK;
	}

	cdc_common.descList = NULL;
	cdc_common.dev.descriptor = (usb_functional_desc_t *)&dDev;
	LIST_ADD(&cdc_common.descList, &cdc_common.dev);

	cdc_common.conf.descriptor = (usb_functional_desc_t *)&dConfig;
	LIST_ADD(&cdc_common.descList, &cdc_common.conf);

	for (i = 0; i < PHFS_ACM_PORTS_NB; ++i) {
		cdc_common.ports[i].iad.descriptor = (usb_functional_desc_t *)&dIad[i];
		LIST_ADD(&cdc_common.descList, &cdc_common.ports[i].iad);

		cdc_common.ports[i].comIface.descriptor = (usb_functional_desc_t *)&dComIntf[i];
		LIST_ADD(&cdc_common.descList, &cdc_common.ports[i].comIface);

		cdc_common.ports[i].header.descriptor = (usb_functional_desc_t *)&dHeader[i];
		LIST_ADD(&cdc_common.descList, &cdc_common.ports[i].header);

		cdc_common.ports[i].call.descriptor = (usb_functional_desc_t *)&dCall[i];
		LIST_ADD(&cdc_common.descList, &cdc_common.ports[i].call);

		cdc_common.ports[i].acm.descriptor = (usb_functional_desc_t *)&dAcm[i];
		LIST_ADD(&cdc_common.descList, &cdc_common.ports[i].acm);

		cdc_common.ports[i].unio.descriptor = (usb_functional_desc_t *)&dUnion[i];
		LIST_ADD(&cdc_common.descList, &cdc_common.ports[i].unio);

		cdc_common.ports[i].comEp.descriptor = (usb_functional_desc_t *)&dComEp[i];
		LIST_ADD(&cdc_common.descList, &cdc_common.ports[i].comEp);

		cdc_common.ports[i].dataIface.descriptor = (usb_functional_desc_t *)&dDataIntf[i];
		LIST_ADD(&cdc_common.descList, &cdc_common.ports[i].dataIface);

		cdc_common.ports[i].dataEpOUT.descriptor = (usb_functional_desc_t *)&dEpOUT[i];
		LIST_ADD(&cdc_common.descList, &cdc_common.ports[i].dataEpOUT);

		cdc_common.ports[i].dataEpIN.descriptor = (usb_functional_desc_t *)&dEpIN[i];
		LIST_ADD(&cdc_common.descList, &cdc_common.ports[i].dataEpIN);
	}

	cdc_common.str0.descriptor = (usb_functional_desc_t *)&dStr0;
	LIST_ADD(&cdc_common.descList, &cdc_common.str0);

	cdc_common.strman.descriptor = (usb_functional_desc_t *)&dStrman;
	LIST_ADD(&cdc_common.descList, &cdc_common.strman);

	cdc_common.strprod.descriptor = (usb_functional_desc_t *)&dStrprod;
	LIST_ADD(&cdc_common.descList, &cdc_common.strprod);

	cdc_common.strprod.next = NULL;

	res = usbclient_init(cdc_common.descList);
	if (res != EOK) {
		return res;
	}

	cdc_common.init = 1;

	return EOK;
}


static int cdc_done(unsigned int minor)
{
	if (PHFS_ACM_PORTS_NB < 1 || PHFS_ACM_PORTS_NB > 2 || minor != endpt_bulk_acm0) {
		return -EINVAL;
	}

	return usbclient_destroy();
}


static int cdc_sync(unsigned int minor)
{
	if (minor > SIZE_USB_ENDPTS) {
		return -EINVAL;
	}

	/* TBD */

	return EOK;
}


static int cdc_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	if (minor > PHFS_ACM_PORTS_NB * 2) {
		return -EINVAL;
	}

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode) {
		return -EINVAL;
	}

	/* Cdc is not mappable to any region */
	return dev_isNotMappable;
}


static int cdc_init(unsigned int minor)
{
	int res;

	if (PHFS_ACM_PORTS_NB < 1 || PHFS_ACM_PORTS_NB > 2 || minor != endpt_bulk_acm0) {
		return -EINVAL;
	}

	res = cdc_initUsbClient();
	if (res < 0) {
		return res;
	}

	lib_printf("\ndev/usb: Initializing usb-cdc(%d.%d)", DEV_USB, minor);

	return res;
}


__attribute__((constructor)) static void cdc_reg(void)
{
	static const dev_handler_t h = {
		.init = cdc_init,
		.done = cdc_done,
		.read = cdc_recv,
		.write = cdc_send,
		.erase = NULL,
		.sync = cdc_sync,
		.map = cdc_map,
	};

	devs_register(DEV_USB, SIZE_USB_ENDPTS, &h);
}
