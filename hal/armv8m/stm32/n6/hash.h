/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32N6 Hash processor driver
 *
 * Copyright 2025 Phoenix Systems
 * Author: Krzysztof Radzewicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HASH_H__
#define _HASH_H__

#include <hal/hal.h>
#include <phfs/phfs.h>
#include "stm32n6_regs.h"
#include "stm32n6.h"

#define HASH_BASE ((void *)0x54020400)

#define HASH_FIFOSZ 16

#define HASH_CR_INIT       (1U << 2)
#define HASH_CR_DMAE       (1U << 3)
#define HASH_CR_MODE       (1U << 6)
#define HASH_CR_SWAP_BYTES (2U << 4)

#define HASH_ALGO_SHA1         (0x0UL << 17)
#define HASH_ALGO_SHA2_224     (0x2UL << 17)
#define HASH_ALGO_SHA2_256     (0x3UL << 17)
#define HASH_ALGO_SHA2_384     (0xcUL << 17)
#define HASH_ALGO_SHA2_512_224 (0xdUL << 17)
#define HASH_ALGO_SHA2_512_256 (0xeUL << 17)
#define HASH_ALGO_SHA2_512     (0xfUL << 17)

#define HASH_SR_NBWE_OFF 16U
#define HASH_SR_DINIS    1U
#define HASH_SR_DCIS     (1U << 1)

#define HASH_STR_NBLW_MASK 0x1fU
#define HASH_STR_DCAL      (1UL << 8)


void hash_init(void);


int hash_initDigest(u32 algo);


int hash_feedMessage(const u8 *data, u32 bytes);


int hash_getDigest(u8 *out, u32 outbytes);


/* To be called if feeding data fails, or if operation is to be cancelled */
void hash_cancelOperation(void);


int hash_digest(u32 algo, const u8 *mess, u32 mbytes, u8 *out, u32 outbytes);


int hash_base64_decode(const char *data, size_t datasize, u8 *out, size_t outsize);


#endif
