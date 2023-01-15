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

#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "usbclient.h"


#define USB_BUFFER_SIZE 0x1000

#define ENDPOINTS_NUMBER 7
#define ENDPOINTS_DIR_NB 2

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define DTD_SIZE(dtd)   ((dtd->dtd_token >> 16) & 0x7fff)
#define DTD_ERROR(dtd)  (dtd->dtd_token & (0xd << 3))
#define DTD_ACTIVE(dtd) (dtd->dtd_token & (1 << 7))


/* device controller structures */

/* data transfer descriptor */
typedef struct _dtd_t {
	u32 dtd_next;
	u32 dtd_token;
	u32 buff_ptr[5];
	u8 padding[4];
} __attribute__((packed)) dtd_t;


/* endpoint queue head */
typedef struct _dqh_t {
	u32 caps;
	u32 dtd_current;

	/* overlay area for dtd */
	u32 dtd_next;
	u32 dtd_token;
	u32 buff_ptr[5];

	u32 reserved;

	/* setup packet buffer */
	u32 setup_buff[2];

	/* head and tail for dtd list */
	dtd_t *head;
	dtd_t *tail;
	u32 base;
	u32 size;
} __attribute__((packed)) dqh_t;


/* dcd structures */

/* dc states */
enum {
	DC_POWERED,
	DC_ATTACHED,
	DC_DEFAULT,
	DC_ADDRESS,
	DC_CONFIGURED
};


/* dc operation */
enum {
	DC_OP_NONE,
	DC_OP_RCV_ENDP0,
	DC_OP_RCV_ERR,
	DC_OP_EXIT,
	DC_OP_INIT
};


typedef struct _usb_dc_t {
	volatile u32 *base;
	void *dtdMem;
	volatile dqh_t *endptqh;
	u32 status;
	u32 dev_addr;
	u8 connected;

	volatile u8 op;
	usb_setup_packet_t setup;

	volatile int endptFailed;
	volatile u32 setupstat;
} usb_dc_t;


/* usb spec related stuff */
typedef struct _endpt_caps_t {
	u8 mult;
	u8 zlt;
	u16 max_pkt_len;
	u8 ios;
	u8 init;
} endpt_caps_t;


typedef struct _endpt_ctrl_t {
	u8 type;
	u8 data_toggle;
	u8 data_inhibit;
	u8 stall;
} endpt_ctrl_t;


typedef struct _usb_buffer_t {
	u8 *buffer;
	s16 len;
} usb_buffer_t;


typedef struct _endpt_data_t {
	endpt_caps_t caps[ENDPOINTS_DIR_NB];
	endpt_ctrl_t ctrl[ENDPOINTS_DIR_NB];

	usb_buffer_t buf[ENDPOINTS_DIR_NB];
} endpt_data_t;


typedef struct {
	char *setupMem;
	endpt_data_t endpts[ENDPOINTS_NUMBER];
} usb_common_data_t;


/* Descriptors Manager's functions */

extern int desc_init(usb_desc_list_t *desList, usb_common_data_t *usb_data_in, usb_dc_t *dc_in);


extern int desc_classSetup(const usb_setup_packet_t *setup);


extern int desc_setup(const usb_setup_packet_t *setup);


/* Controller's functions */

extern int ctrl_init(usb_common_data_t *usb_data_in, usb_dc_t *dc_in);


extern void ctrl_setAddress(u32 addr);


extern void ctrl_initQtd(void);


extern int ctrl_endptInit(int endpt, endpt_data_t *ctrl_endptInit);


extern void ctrl_lfIrq(void);


extern void ctrl_hfIrq(void);


extern dtd_t *ctrl_execTransfer(int endpt, u32 paddr, u32 sz, int dir);


extern void ctrl_reset(void);


#endif /* _CLIENT_H_ */
