/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-loader
 *
 * cdc - USB Communication Device Class
 *
 * Copyright 2019, 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _CDC_CLIENT_H_
#define _CDC_CLIENT_H_

#include <cdc.h>


/* Endpoints available in a CDC driver */
enum { endpt_irq_acm0 = 0x01, endpt_bulk_acm0, endpt_irq_acm1, endpt_bulk_acm1 };


/* Function initialize a USB controller and setup descriptors. */
int cdc_init(void);


/* Function receives data on an given endpoint */
int cdc_recv(int endpt, char *data, unsigned int len);


/* Function sends data on an given endpoint. */
int cdc_send(int endpt, const char *data, unsigned int len);


/* Functions resets a USB controller and a physicall layer */
void cdc_destroy(void);


#endif
