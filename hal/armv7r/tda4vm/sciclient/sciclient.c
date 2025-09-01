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
 */

#include "sciclient.h"
#include "tisci_protocol.h"

#include <phoenix/errno.h>

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

static void Sciclient_prepareHeader(const Sciclient_ReqPrm_t *pReqPrm);
static u32 getThreadAddress(u32 threadID);
static void HW_REG32_WR(u32 regAddr, u32 regPayload);
static int Sciclient_send(const Sciclient_ReqPrm_t *pReqPrm, Sciclient_RespPrm_t  *pRespPrm, const u8 *pSecHdr);


/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

Sciclient_ServiceHandle_t gSciclientHandle = {0};
static u32 gSciclient_maxMsgSizeBytes;
static struct tisci_sec_header gSciclient_secHeader = {0,0};

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

static void Sciclient_prepareHeader(const Sciclient_ReqPrm_t *pReqPrm)
{
	struct tisci_header *th = (struct tisci_header *)pReqPrm->pReqPayload;
	th->type = pReqPrm->messageType;
	th->flags = pReqPrm->flags;
	th->host = TISCI_HOST_ID_MCU_0_R5_2;
	th->seq = gSciclientHandle.currSeqId;

	gSciclientHandle.currSeqId = (gSciclientHandle.currSeqId + 1) %
								 SCICLIENT_MAX_QUEUE_SIZE;
}

static u32 getThreadAddress(u32 threadID)
{
	u32 baseAddr = 0x2A480004;
	return (u32)(baseAddr + (threadID * 0x1000));
}

static inline void HW_REG32_WR(u32 regAddr, u32 regPayload)
{
	*(u32 *)regAddr = regPayload;
}

static int Sciclient_send(const Sciclient_ReqPrm_t *pReqPrm, Sciclient_RespPrm_t  *pRespPrm, const u8 *pSecHdr)
{
	u8 i;
	u16 numWords;
	int status = EOK;
	volatile u32 threadAddr;
	u32 payload32;
	const u8 *msg = NULL;

	threadAddr = getThreadAddress(TISCI_SEC_PROXY_MCU_0_R5_2_WRITE_HIGH_PRIORITY_THREAD_ID);

	Sciclient_prepareHeader(pReqPrm);

	if (pSecHdr != NULL)
	{
		/* Write security header first. Word aligned so operating on words */
		numWords = sizeof(gSciclient_secHeader)/sizeof(u32);
		msg = pSecHdr;
		for (i = 0; i < numWords; i++) {
			hal_memcpy((void *)&payload32, (const void *)msg, sizeof(u32));
			HW_REG32_WR(threadAddr, payload32);
			threadAddr += sizeof(u32);
			msg += sizeof(u32);
		}
	}

	/* Write TISCI header and message payload */
	numWords = (pReqPrm->reqPayloadSize + 3U) / sizeof(u32);
	msg = pReqPrm->pReqPayload;

	for (i = 0; i < numWords; i++) {
		hal_memcpy((void *)&payload32, (const void *)msg, sizeof(u32));
		HW_REG32_WR(threadAddr, payload32);
		threadAddr += sizeof(u32);
		msg += sizeof(u32);
	}

	/* Write to the last register of TX thread to trigger msg send */
	if ((sizeof(gSciclient_secHeader) + pReqPrm->reqPayloadSize) <= gSciclient_maxMsgSizeBytes) {
		threadAddr = getThreadAddress(TISCI_SEC_PROXY_MCU_0_R5_2_WRITE_HIGH_PRIORITY_THREAD_ID) + 
					 gSciclient_maxMsgSizeBytes;
		payload32=0;
		HW_REG32_WR(threadAddr - sizeof(u32), payload32);			 
	}

	/**  Since only reboot is serviced by TISCI call in PLO, no response is expected.*/
	status = -EAGAIN;
	return status;

}

int Sciclient_sys_reset()
{
	int status = EOK;

	struct tisci_msg_sys_reset_req msgReq;
	msgReq.domain = DOMGRP_00;

	const Sciclient_ReqPrm_t pReq = 
	{
		TISCI_MSG_SYS_RESET,
		TISCI_MSG_FLAG_AOP,
		(u8 *)&msgReq,
		sizeof(msgReq),
		SCICLIENT_SERVICE_WAIT_FOREVER,
		0
	};

	struct tisci_msg_sys_reset_resp msgResp;
	Sciclient_RespPrm_t pResp = 
	{
		0,
		(u8 *)&msgResp,
		sizeof(msgResp)
	};

	status = Sciclient_send(&pReq, &pResp, NULL);

	/* Not expecting response. If reached, return error. */
	return status;
}

void Sciclient_init(void)
{
	/** Initialize client config */
	if( gSciclientHandle.initCount == 0) {

		gSciclientHandle.opModeFlag = SCICLIENT_SERVICE_OPERATION_MODE_POLLED;
		gSciclient_maxMsgSizeBytes = SCICLIENT_MSG_MAX_SIZE	- SCICLIENT_MSG_RSVD;

		/**  Initialize currSeqId. Make sure currSeqId is never 0 */
    	gSciclientHandle.currSeqId = 1;

		/**  configuring secure proxy for DM communication */
		gSciclientHandle.txThread = TISCI_SEC_PROXY_MCU_0_R5_2_WRITE_HIGH_PRIORITY_THREAD_ID;
		gSciclientHandle.rxThread = TISCI_SEC_PROXY_MCU_0_R5_2_READ_RESPONSE_THREAD_ID;

		gSciclientHandle.initCount++;
	}
}

void Sciclient_deinit(void)
{
	/** Since we don't acquire any HW resources, so far nothing to do */
}