/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * sciclient module
 *
 * Copyright 2025 Phoenix Systems
 * Author: Rafał Mikielis
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%

 *  Copyright (C) 2015-2025 Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#ifndef _TISCI_PROT_H_
#define _TISCI_PROT_H_

#include "../../types.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define TISCI_BIT(n)  (1UL << (n))

/** This flag is reserved and not to be used. */
#define TISCI_MSG_FLAG_RESERVED0    TISCI_BIT(0)

/** ACK on Processed: Send a response to a message after it has been processed
 * with TISCI_MSG_FLAG_ACK set if the processing succeeded, or a NAK otherwise.
 * This response contains the complete response to the message with the result
 * of the actual action that was requested.
 */
#define TISCI_MSG_FLAG_AOP    TISCI_BIT(1)

/** Indicate that this message is marked secure */
#define TISCI_MSG_FLAG_SEC    TISCI_BIT(2)

/** Response flag for a message that indicates success. If this flag is NOT
 * set then that is to be interpreted as a NAK.
 */
#define TISCI_MSG_FLAG_ACK    TISCI_BIT(1)

/* TISCI Message IDs */
#define TISCI_MSG_SYS_RESET                     (0x0005U)

typedef u8 domgrp_t;

/* ========================================================================== */
/*                          Structure definitions                             */
/* ========================================================================== */


 /** Header that prefixes all TISCI messages sent via secure transport.*/
struct tisci_sec_header {
	u16	integ_check;
	u16	rsvd;
};

/** Header that prefixes all TISCI messages.
 *
 * type: Type of message identified by a TISCI_MSG_* ID
 * host: Host of the message.
 * seq: Message identifier indicating a transfer sequence.
 * flags: TISCI_MSG_FLAG_* for the message
 */
struct tisci_header {
    u16   type;
    u8    host;
    u8    seq;
    u32   flags;
};

/** Request for TISCI_MSG_SYS_RESET.
 *
 * hdr: TISCI header to provide ACK/NAK flags to the host.
 * domain :Domain to be reset.
 */
struct tisci_msg_sys_reset_req {
	struct tisci_header	hdr;
	domgrp_t		domain;
} __attribute__((__packed__));

/** Empty response for TISCI_MSG_SYS_RESET.
 *
 * Although this message is essentially empty and contains only a header
 * a full data structure is created for consistency in implementation.
 *
 */
struct tisci_msg_sys_reset_resp {
	struct tisci_header hdr;
} __attribute__((__packed__));


#endif  /** _TISCI_PROT_H_  */