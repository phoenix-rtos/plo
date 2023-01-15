/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * USB basic structures
 *
 * Copyright 2018-2020 Phoenix Systems
 * Author: Jan Sikorski, Kamil Amanowicz, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_

#include <hal/hal.h>

#define REQUEST_DIR_HOST2DEV (0 << 7)
#define REQUEST_DIR_DEV2HOST (1 << 7)
#define REQUEST_DIR_MASK     (1 << 7)

#define REQUEST_TYPE_STANDARD (0 << 5)
#define REQUEST_TYPE_CLASS    (1 << 5)
#define REQUEST_TYPE_VENDOR   (2 << 5)

#define REQUEST_RECIPIENT_DEVICE    0
#define REQUEST_RECIPIENT_INTERFACE 1
#define REQUEST_RECIPIENT_ENDPOINT  2
#define REQUEST_RECIPIENT_OTHER     3

#define EXTRACT_REQ_TYPE(req_type) ((3 << 5) & (req_type))

/* request types */
#define REQ_GET_STATUS        0
#define REQ_CLEAR_FEATURE     1
#define REQ_SET_FEATURE       3
#define REQ_SET_ADDRESS       5
#define REQ_GET_DESCRIPTOR    6
#define REQ_SET_DESCRIPTOR    7
#define REQ_GET_CONFIGURATION 8
#define REQ_SET_CONFIGURATION 9
#define REQ_GET_INTERFACE     10
#define REQ_SET_INTERFACE     11
#define REQ_SYNCH_FRAME       12


/* class request codes */
#define CLASS_REQ_GET_REPORT             1
#define CLASS_REQ_GET_IDLE               2
#define CLASS_REQ_GET_PROTOCOL           3
#define CLASS_REQ_SET_REPORT             9
#define CLASS_REQ_SET_IDLE               10
#define CLASS_REQ_SET_PROTOCOL           11
#define CLASS_REQ_SET_LINE_CODING        0x20
#define CLASS_REQ_SET_CONTROL_LINE_STATE 0x22


/* descriptor types */
#define USB_DESC_DEVICE                1
#define USB_DESC_CONFIG                2
#define USB_DESC_STRING                3
#define USB_DESC_INTERFACE             4
#define USB_DESC_ENDPOINT              5
#define USB_DESC_TYPE_DEV_QUAL         6
#define USB_DESC_TYPE_OTH_SPD_CFG      7
#define USB_DESC_TYPE_INTF_PWR         8
#define USB_DESC_INTERFACE_ASSOCIATION 11
#define USB_DESC_TYPE_HID              0x21
#define USB_DESC_TYPE_HID_REPORT       0x22
#define USB_DESC_TYPE_CDC_CS_INTERFACE 0x24


/* endpoint types */
#define USB_ENDPT_TYPE_CONTROL 0
#define USB_ENDPT_TYPE_ISO     1
#define USB_ENDPT_TYPE_BULK    2
#define USB_ENDPT_TYPE_INTR    3


/* endpoint direction */
#define USB_ENDPT_DIR_OUT 0
#define USB_ENDPT_DIR_IN  1


/* endpoint feature */
#define USB_ENDPOINT_HALT 0

/* class specific desctriptors */
#define USB_DESC_CS_INTERFACE 0x24
#define USB_DESC_CS_ENDPOINT  0x25

#define USB_TIMEOUT 5000000

enum { pid_out = 0xe1,
	pid_in = 0x69,
	pid_setup = 0x2d };


enum { out_token = 0,
	in_token,
	setup_token };


typedef struct usb_setup_packet {
	u8 bmRequestType;
	u8 bRequest;
	u16 wValue;
	u16 wIndex;
	u16 wLength;
} __attribute__((packed)) usb_setup_packet_t;


struct usb_desc_header {
	u8 bLength;         /* size of descriptor */
	u8 bDescriptorType; /* descriptor type */
};


typedef struct usb_device_desc {
	u8 bLength;            /* size of descriptor */
	u8 bDescriptorType;    /* descriptor type */
	u16 bcdUSB;            /* usb specification in BCD */
	u8 bDeviceClass;       /* device class code (USB-IF)*/
	u8 bDeviceSubClass;    /* device subclass code (USB-IF)*/
	u8 bDeviceProtocol;    /* protocol code  (USB-IF)*/
	u8 bMaxPacketSize0;    /* max packet size for endpoint0 */
	u16 idVendor;          /* vendor id (USB-IF) */
	u16 idProduct;         /* product id */
	u16 bcdDevice;         /* device release number in BCD */
	u8 iManufacturer;      /* manufacturer string index */
	u8 iProduct;           /* product string index */
	u8 iSerialNumber;      /* serial number string index */
	u8 bNumConfigurations; /* number of possible configurations */
} __attribute__((packed)) usb_device_desc_t;


typedef struct usb_configuration_desc {
	u8 bLength;             /* size of descriptor */
	u8 bDescriptorType;     /* descriptor type */
	u16 wTotalLength;       /* total bytes returned for this configuration */
	u8 bNumInterfaces;      /* number of interfaces supported */
	u8 bConfigurationValue; /* value to use for SET_CONFIGURATION request */
	u8 iConfiguration;      /* configuration string index */
	u8 bmAttributes;        /* attributes bitmap */
	u8 bMaxPower;           /* maximum power consumption */
} __attribute__((packed)) usb_configuration_desc_t;


typedef struct usb_interface_desc {
	u8 bLength;            /* size of descriptor */
	u8 bDescriptorType;    /* descriptor type */
	u8 bInterfaceNumber;   /* number of this interface */
	u8 bAlternateSetting;  /* value for the alternate setting */
	u8 bNumEndpoints;      /* number of endpoints */
	u8 bInterfaceClass;    /* interface class code */
	u8 bInterfaceSubClass; /* interface subclass code */
	u8 bInterfaceProtocol; /* interface protocol code */
	u8 iInterface;         /* interface string index */
} __attribute__((packed)) usb_interface_desc_t;


typedef struct usb_interface_association_desc {
	u8 bLength;
	u8 bDescriptorType;
	u8 bFirstInterface;
	u8 bInterfaceCount;
	u8 bFunctionClass;
	u8 bFunctionSubClass;
	u8 bFunctionProtocol;
	u8 iFunction;
} __attribute__((packed)) usb_interface_association_desc_t;


typedef struct usb_string_desc {
	u8 bLength;
	u8 bDescriptorType;
	u8 wData[256];
} __attribute__((packed)) usb_string_desc_t;


typedef struct usb_endpoint_desc {
	u8 bLength;          /* size of descriptor */
	u8 bDescriptorType;  /* descriptor type */
	u8 bEndpointAddress; /* endpoint address */
	u8 bmAttributes;     /* attributes bitmap */
	u16 wMaxPacketSize;  /* maximum packet size */
	u8 bInterval;        /* polling interval for data transfers */
} __attribute__((packed)) usb_endpoint_desc_t;


/* generic descriptor
 * used when there is no defined descriptor (e.g. HID descriptor or Report descriptor) */
typedef struct usb_functional_desc {
	u8 bFunctionLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
} __attribute__((packed)) usb_functional_desc_t;


#endif
