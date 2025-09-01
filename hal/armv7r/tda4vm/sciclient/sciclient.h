/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * sciclient module for PLO
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
 

#ifndef SCICLIENT_H
#define SCICLIENT_H

#include "../../types.h"

#include <lib/lib.h>

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define SciApp_print(fmt, ...)		lib_printf("sciclient: "fmt"\n", ##__VA_ARGS__)

#define SCICLIENT_MSG_TX_REQ	(1U)
#define SCICLIENT_MSG_RX_REQ	(0U)

#define SCICLIENT_MSG_MAX_SIZE	(64U)
#define SCICLIENT_MSG_RSVD		(4U)

#define SCICLIENT_SERVICE_OPERATION_MODE_POLLED           (0U)
#define SCICLIENT_SERVICE_OPERATION_MODE_INTERRUPT        (1U)

#define SCICLIENT_SERVICE_WAIT_FOREVER                    (0xFFFFFFFFU)
#define SCICLIENT_SERVICE_NO_WAIT                         (0x0U)

/* R5_0(Non Secure): Cortex R5 context 0 on MCU island */
#define SCICLIENT_CONTEXT_R5_NONSEC (0U)
/* R5_1(Secure): Cortex R5 context 1 on MCU island(Boot) */
#define SCICLIENT_CONTEXT_R5_SEC (1U)

/* Secure Proxy configurations for MCU_0_R5_2 host
 */

/** Thread ID macro for MCU_0_R5_2 response */
#define TISCI_SEC_PROXY_MCU_0_R5_2_READ_RESPONSE_THREAD_ID (11U)
/** Num messages macro for MCU_0_R5_2 response */
#define TISCI_SEC_PROXY_MCU_0_R5_2_READ_RESPONSE_NUM_MESSAGES (2U)

/** Thread ID macro for MCU_0_R5_2 high_priority */
#define TISCI_SEC_PROXY_MCU_0_R5_2_WRITE_HIGH_PRIORITY_THREAD_ID (12U)
/** Num messages macro for MCU_0_R5_2 high_priority */
#define TISCI_SEC_PROXY_MCU_0_R5_2_WRITE_HIGH_PRIORITY_NUM_MESSAGES (1U)


/** SoC defined domgrp 00 (MCU) */
#define DOMGRP_00               ((0x01U) << 0U)
/** SoC defined domgrp 01 (MAIN)*/
#define DOMGRP_01               ((0x01U) << 1U)

/** TISCI hosts numbers
 */

/** MCU_0_R5_0(Non Secure): Cortex R5 context 0 on MCU island */
#define TISCI_HOST_ID_MCU_0_R5_0 (3U)
/** MCU_0_R5_1(Secure): Cortex R5 context 1 on MCU island(Boot) */
#define TISCI_HOST_ID_MCU_0_R5_1 (4U)
/** MCU_0_R5_2(Non Secure): Cortex R5 context 2 on MCU island */
#define TISCI_HOST_ID_MCU_0_R5_2 (5U)
/** MCU_0_R5_3(Secure): Cortex R5 context 3 on MCU island */
#define TISCI_HOST_ID_MCU_0_R5_3 (6U)

#define SCICLIENT_MAX_QUEUE_SIZE            (7U)

typedef u32 tisci_msg_type;
typedef u8  tx_flag;

/* ========================================================================== */
/*                          Structure definitions                             */
/* ========================================================================== */

/** Input parameters for #Sciclient_service function.
 */
typedef struct
{
	/**< Type of message. */
    u16       messageType;
    
	/**< Flags for messages that are being transmitted. */
    u32       flags;
    
	/**< Pointer to the payload to be transmitted */
    const u8 *pReqPayload;
    
	/**< Size of the payload to be transmitted (in bytes)*/
    u32       reqPayloadSize;
    
	/**< Timeout(number of iterations) for receiving response
     *(Refer \ref Sciclient_ServiceOperationTimeout) */
    u32       timeout;

    /**< Indicates whether the request is being forwarded to another
     *        service provider. Only to be set internally by sciserver, if
     *        integrated into this build. Unused otherwise. */
    u8        forwardStatus;

} Sciclient_ReqPrm_t;

/** Output parameters for #Sciclient_service function.
 */
typedef struct
{
    /**< Flags of response to messages. */	
    u32 flags;

	/**<  Pointer to the received payload. The pointer is an input. The
     *        API will populate this with the firmware response upto the
     *        size mentioned in respPayloadSize. Please ensure respPayloadSize
     *        bytes are allocated.
     */
    u8 *pRespPayload;

	/**< Size of the response payload(in bytes) */
    u32 respPayloadSize;

} Sciclient_RespPrm_t;


/** Handle for #Sciclient_service function
 */
typedef struct
{
	/** tx and rx threads numbers for firmware communication */
	u32 	txThread, rxThread;

    /**< Sequence ID of the current request **/
    u32	currSeqId;

	/** Interrupt line number for response message  */
	u32	respIntr;

	/**< Operation mode for the Sciclient Service API. Refer to
    * \ref Sciclient_ServiceOperationMode for valid values. */
    u32	opModeFlag;

	/**< Count to keep track of the number of inits/de-inits done. Actual
     *   initialization done
     *   only when initCount=0, and de-init done only when initCount=1 */
    u8		initCount;

	/**< Variable to check whether Core context is secure/non-secure. This has
     * to be given by the user via configParams. Default value is 0.
     */
    u32	isSecureMode;
} Sciclient_ServiceHandle_t;


/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/*  This API is called once for registering interrupts and creating handlers to talk to firmware. */
void Sciclient_init(void);

/** De-initialization of sciclient. It only de-initializes the interrupts etc. which are initialized 
 *  in #Sciclient_init. It does not de-initialize the system firmware.
 */
void Sciclient_deinit(void);

/** TISCI_MSG_SYS_RESET request */
int Sciclient_sys_reset(void);


#endif	/**  SCICLIENT_H */