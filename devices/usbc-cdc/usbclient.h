/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * USB client controller driver
 *
 * Copyright 2019-2021 Phoenix Systems
 * Author: Kamil Amanowicz, Bartosz Ciesla, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _USBCLIENT_H_
#define _USBCLIENT_H_

#include "usb.h"


/* Library configuration structure */
typedef struct _usb_desc_list_t {
	struct _usb_desc_list_t *next, *prev;
	usb_functional_desc_t *descriptor;
} usb_desc_list_t;


/* Initialize library with given configuration */
extern int usbclient_init(usb_desc_list_t *desList);


/* Cleanup data */
extern int usbclient_destroy(void);


/* Send data on given endpoint - blocking */
extern ssize_t usbclient_send(int endpt, const void *data, size_t len);


/* Receive data from given endpoint - blocking */
extern ssize_t usbclient_receive(int endpt, void *data, size_t len);


#endif /* _USBCLIENT_H_ */
