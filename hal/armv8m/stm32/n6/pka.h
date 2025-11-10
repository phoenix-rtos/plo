/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32N6 Public Key Accelerator driver
 *
 * Copyright 2025 Phoenix Systems
 * Author: Krzysztof Radzewicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PKA_H__
#define _PKA_H__

#include <hal/armv8m/cpu.h>
#include <hal/hal.h>
#include <hal/string.h>
#include <lib/errno.h>
#include "stm32n6.h"

#define PKA_BASE ((void *)0x54022000)

#define PKA_RAM_SIZE 5336 /* In bytes */


#define PKA_MODE_MONTGOMERY     0x01
#define PKA_MODE_MOD_EXP        0x02
#define PKA_MODE_MOD_ADD        0x0e
#define PKA_MODE_EC_MULTIPLY    0x20
#define PKA_MODE_ECDSA_SIGN     0x24
#define PKA_MODE_ECDSA_VERIFY   0x26
#define PKA_MODE_EC_CURVE_CHECK 0x28

#define PKA_ECDSA_SIGN_SUCCESS 0xd60d
#define PKA_ECDSA_SIGN_FAIL    0xcbc9
#define PKA_ECDSA_SIGN_RZERO   0xa3b7
#define PKA_ECDSA_SIGN_SZERO   0xf946

#define PKA_EC_CURVE_CHECK_SUCCESS 0xd60d
#define PKA_EC_CURVE_CHECK_FAIL    0xa3b7
#define PKA_EC_CURVE_CHECK_EINVAL  0xf946

#define PKA_EC_MULTIPLY_SUCCESS 0xd60d
#define PKA_EC_MULTIPLY_FAIL    0xcbc9

#define PKA_ECDSA_VERIFY_SUCCESS 0xd60d
#define PKA_ECDSA_VERIFY_FAIL    0xa3b7


#define PKA_CR_EN        1u
#define PKA_CR_START     (1u << 1)
#define PKA_CR_MODE_MASK (0x3f << 8)
#define PKA_CR_MODE_OFF  8u
#define PKA_CR_PROCENDIE (1u << 17)
#define PKA_CR_RAMERRIE  (1u << 19)
#define PKA_CR_ADDRERRIE (1u << 20)
#define PKA_CR_OPERRIE   (1u << 21)

#define PKA_SR_INITOK   1u
#define PKA_SR_LMF      (1u << 1)
#define PKA_SR_BUSY     (1u << 16)
#define PKA_SR_PROCENDF (1u << 17)
#define PKA_SR_RAMERRF  (1u << 19)
#define PKA_SR_ADDRERRF (1u << 20)
#define PKA_SR_OPERRF   (1u << 21)

#define PKA_CLRFR_PROCENDFC (1u << 17)
#define PKA_CLRFR_RAMERRFC  (1u << 19)
#define PKA_CLRFR_ADDRERRFC (1u << 20)
#define PKA_CLRFR_OPERRFC   (1u << 21)

#define PKA_ECC384_BYTES    48
#define PKA_ECC256_BYTES    32
#define PKA_MAX_PARAM_BYTES 64


typedef struct {
	u8 n[PKA_ECC384_BYTES];
	u8 p[PKA_ECC384_BYTES];
	u8 Gx[PKA_ECC384_BYTES];
	u8 Gy[PKA_ECC384_BYTES];
	u8 absa[PKA_ECC384_BYTES];
	u8 b[PKA_ECC384_BYTES];
	u8 asign[8]; /* 0 - positive, 1 - negative */
} pka_ecc384_t;


typedef struct {
	u32 size;
	u8 n[PKA_ECC256_BYTES];
	u8 p[PKA_ECC256_BYTES];
	u8 Gx[PKA_ECC256_BYTES];
	u8 Gy[PKA_ECC256_BYTES];
	u8 absa[PKA_ECC256_BYTES];
	u8 b[PKA_ECC256_BYTES];
	u8 asign[8]; /* 0 - positive, 1 - negative */
} pka_ecc256_t;


void pka_init(void);


/* Generate a public key that matches the provided private key */
int pka_eccPubKey(const u8 *privateKey, size_t keybits, u8 *Qx, u8 *Qy);


int pka_ecdsaSign(const u8 *hash, const u8 *privkey, size_t keybits, u8 *r, u8 *s);


int pka_ecdsaVerify(const u8 *r, const u8 *s, const u8 *hash, const u8 *Qx, const u8 *Qy, size_t keybits);


#endif /* _PKA_H__ */
