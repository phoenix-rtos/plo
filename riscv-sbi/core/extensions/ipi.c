/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * IPI handling
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "csr.h"
#include "hart.h"
#include "list.h"
#include "sbi.h"
#include "spinlock.h"
#include "string.h"

#include "devices/clint.h"
#include "extensions/ipi.h"

#include "ld/noelv.ldt"


#define MAX_TASK_COUNT 16

#define SBI_IPI_RETRY 1


typedef struct _ipi_node_t {
	ipi_task_t task;
	struct _ipi_node_t *next;
	struct _ipi_node_t *prev;
} ipi_node_t;


struct {
	spinlock_t lock[MAX_HART_COUNT];
	u16 poolMask[MAX_HART_COUNT];
	ipi_node_t pool[MAX_HART_COUNT][MAX_TASK_COUNT];
	ipi_node_t *tasks[MAX_HART_COUNT];
} ipi_common;


static void sbi_ipiRawSend(u32 hartid)
{
	RISCV_FENCE(ow, ow);
	clint_sendIpi(hartid);
}


static void sbi_ipiRawClear(u32 hartid)
{
	clint_clearIpi(hartid);
	RISCV_FENCE(ow, ow);
}


void sbi_ipiHandler(void)
{
	spinlock_ctx_t ctx;
	u32 hartid = csr_read(CSR_MHARTID);
	ipi_node_t *node;

	sbi_ipiRawClear(hartid);

	spinlock_set(&ipi_common.lock[hartid], &ctx);

	node = ipi_common.tasks[hartid];

	while (node != NULL) {
		LIST_REMOVE(&ipi_common.tasks[hartid], node);
		if (node->task.handler != NULL) {
			node->task.handler(node->task.data);
		}
		ipi_common.poolMask[hartid] |= 1 << (node - ipi_common.pool[hartid]);
		node = ipi_common.tasks[hartid];
	}

	spinlock_clear(&ipi_common.lock[hartid], &ctx);
}


long sbi_ipiSend(u32 hartid, void (*handler)(void *), void *data)
{
	size_t idx;
	spinlock_ctx_t ctx;
	ipi_node_t *node;

	if (hartid == csr_read(CSR_MHARTID)) {
		if (handler != NULL) {
			handler(data);
		}
		return SBI_SUCCESS;
	}

	spinlock_set(&ipi_common.lock[hartid], &ctx);

	if (ipi_common.poolMask[hartid] == 0) {
		spinlock_clear(&ipi_common.lock[hartid], &ctx);
		return SBI_IPI_RETRY;
	}

	idx = sbi_getFirstBit(ipi_common.poolMask[hartid]);
	ipi_common.poolMask[hartid] &= ~(1 << idx);

	node = &ipi_common.pool[hartid][idx];
	node->task.handler = handler;
	node->task.data = data;

	if (ipi_common.tasks[hartid] == NULL) {
		LIST_ADD(&ipi_common.tasks[hartid], node);
	}
	else {
		LIST_ADD(&ipi_common.tasks[hartid]->prev, node);
	}

	sbi_ipiRawSend(hartid);

	spinlock_clear(&ipi_common.lock[hartid], &ctx);

	return SBI_SUCCESS;
}


long sbi_ipiSendMany(unsigned long hartMask, unsigned long hartMaskBase, void (*handler)(void *), void *data)
{
	size_t i;
	long ret;

	if (hartMaskBase > sbi_getHartCount()) {
		return SBI_ERR_INVALID_PARAM;
	}
	else {
		hartMask = (hartMask << hartMaskBase) & sbi_hartMask;
	}

	if (hartMask == 0) {
		return SBI_ERR_INVALID_PARAM;
	}

	for (i = 0; i < sbi_getHartCount(); i++) {
		if (hartMask & (1 << i)) {
			do {
				ret = sbi_ipiSend(i, handler, data);
				/* We might run out of IPI slots, retry */
			} while (ret == SBI_IPI_RETRY);

			if (ret < 0) {
				return ret;
			}
		}
	}

	return SBI_SUCCESS;
}


void sbi_ipiInit(void)
{
	size_t i;
	for (i = 0; i < sbi_getHartCount(); i++) {
		spinlock_create(&ipi_common.lock[i], "ipi");
	}

	sbi_memset(ipi_common.poolMask, 0xff, sizeof(ipi_common.poolMask));

	RISCV_FENCE(w, rw);
}
