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

#include "pka.h"
#include <lib/lib.h>


/* https://www.secg.org/SEC2-Ver-1.0.pdf
 * We store curve parameres in little-endian.
 * Instead of storing the immediate value of the 'a' coefficient, we need abs(a) mod p.
 * PKA accepts two formats of 'a', as stated in stm32n6 reference manual (ch. 52.5.1)
 */
/* clang-format off */
static const struct {
	pka_ecc256_t primev256;

} params = {
	.primev256 = {
		.size = PKA_ECC256_BYTES,
		.n = {
			0x51, 0x25, 0x63, 0xfc, 0xc2, 0xca, 0xb9, 0xf3,
			0x84, 0x9e, 0x17, 0xa7, 0xad, 0xfa, 0xe6, 0xbc,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff
		},
		.p = {
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff
		}, 
		.Gx = {
			0x96, 0xc2, 0x98, 0xd8, 0x45, 0x39, 0xa1, 0xf4,
			0xa0, 0x33, 0xeb, 0x2d, 0x81, 0x7d, 0x03, 0x77,
			0xf2, 0x40, 0xa4, 0x63, 0xe5, 0xe6, 0xbc, 0xf8,
			0x47, 0x42, 0x2c, 0xe1, 0xf2, 0xd1, 0x17, 0x6b
		},
		.Gy = {
			0xf5, 0x51, 0xbf, 0x37, 0x68, 0x40, 0xb6, 0xcb,
			0xce, 0x5e, 0x31, 0x6b, 0x57, 0x33, 0xce, 0x2b,
			0x16, 0x9e, 0x0f, 0x7c, 0x4a, 0xeb, 0xe7, 0x8e,
			0x9b, 0x7f, 0x1a, 0xfe, 0xe2, 0x42, 0xe3, 0x4f
		},
		.absa = {
 			0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		},
		.b = {
			0x4b, 0x60, 0xd2, 0x27, 0x3e, 0x3c, 0xce, 0x3b,
			0xf6, 0xb0, 0x53, 0xcc, 0xb0, 0x06, 0x1d, 0x65,
			0xbc, 0x86, 0x98, 0x76, 0x55, 0xbd, 0xeb, 0xb3,
			0xe7, 0x93, 0x3a, 0xaa, 0xd8, 0x35, 0xc6, 0x5a
		},
		.asign = {
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}
	}
};
/* clang-format on */


static struct {
	volatile u32 *base;
	u8 *ram; /* Needs to be non cacheable */
	u8 buffer[PKA_MAX_PARAM_BYTES];
} common = {
	.buffer = {}
};


static void pka_writeRamParam(size_t offset, const void *data, size_t bytes)
{
	hal_memcpy(common.ram + offset, data, bytes);
	/* Insert 8 null bytes termination */
	hal_memset(common.ram + offset + bytes, 0, 8);
}


static u32 pka_process(u32 mode)
{
	u32 sr;
	u32 cr = *(common.base + pka_cr);
	cr = (cr & ~PKA_CR_MODE_MASK) | (mode << PKA_CR_MODE_OFF) | PKA_CR_START;
	*(common.base + pka_cr) = cr;
	hal_cpuDataMemoryBarrier();

	while ((*(common.base + pka_sr) & PKA_SR_PROCENDF) != PKA_SR_PROCENDF) {
		/* Wait */
	}

	/* If a tamper event was detected, ramerr flag is set */
	sr = *(common.base + pka_sr) & (PKA_SR_OPERRF | PKA_SR_ADDRERRF | PKA_SR_RAMERRF);

	/* Clear all flags */
	*(common.base + pka_clrfr) |= PKA_CLRFR_PROCENDFC | PKA_CLRFR_RAMERRFC | PKA_CLRFR_ADDRERRFC | PKA_CLRFR_OPERRFC;

	return sr;
}


/* Attempt to generate random value in range [1, n-1] */
static int pka_randRange(u8 *out, const u8 *n, size_t len)
{
	int ret, attempt = 0;
	size_t off;
	int kLess;

	while (attempt < 1000) {
		ret = _stm32_rngRead(out, len);
		if (ret < 0) {
			return ret;
		}

		off = len;
		kLess = 0;
		while (off > 0) {
			if (out[off - 1] > n[off - 1]) {
				kLess = 0;
				break;
			}
			else if (out[off - 1] < n[off - 1]) {
				kLess = 1;
				break;
			}
			off--;
		}
		if (kLess == 1) {
			return EOK;
		}
		attempt++;
	}

	return -EAGAIN;
}


static int pka_montgomeryParameter(const u8 *n, size_t nbits, u8 *out)
{
	u32 bytes = nbits / 8;
	u32 status;

	hal_memcpy(common.ram + 0x408, &nbits, sizeof(u64));
	pka_writeRamParam(0x1088, n, bytes);
	hal_cpuDataMemoryBarrier();

	status = pka_process(PKA_MODE_MONTGOMERY);
	if (status != 0) {
		return -EINVAL;
	}

	hal_cpuDataMemoryBarrier();

	hal_memcpy(out, common.ram + 0x620, bytes);

	return 0;
}


static int pka_pointOnCurveCheck(const u8 *x, const u8 *y, size_t keybits)
{
	u64 bitlen = keybits;
	u32 bytes = keybits / 8;
	u32 status = 0;
	int ret;
	const pka_ecc256_t *curve = &params.primev256;

	if (keybits != (curve->size * 8)) {
		return -EINVAL;
	}

	/* For this operation we need montgomery parameter R^2 mod n */
	ret = pka_montgomeryParameter(curve->p, keybits, common.buffer);
	if (ret != 0) {
		return ret;
	}

	hal_cpuDataMemoryBarrier();

	hal_memcpy(common.ram + 0x408, &bitlen, sizeof(u64));
	hal_memcpy(common.ram + 0x410, curve->asign, sizeof(u64));

	pka_writeRamParam(0x418, curve->absa, bytes);
	pka_writeRamParam(0x520, curve->b, bytes);
	pka_writeRamParam(0x470, curve->p, bytes);
	pka_writeRamParam(0x578, x, bytes);
	pka_writeRamParam(0x5d0, y, bytes);
	pka_writeRamParam(0x4c8, common.buffer, bytes);

	hal_cpuDataMemoryBarrier();

	status = pka_process(PKA_MODE_EC_CURVE_CHECK);
	if (status != 0) {
		return -EINVAL;
	}
	hal_cpuDataMemoryBarrier();

	hal_memcpy(&status, common.ram + 0x680, sizeof(u32));
	hal_cpuDataMemoryBarrier();

	if (status == PKA_EC_CURVE_CHECK_SUCCESS) {
		return EOK;
	}
	else if (status == PKA_EC_CURVE_CHECK_FAIL) {
		return -1;
	}
	else {
		return -EINVAL;
	}
}


static int pka_eccMultiply(const u8 *x, const u8 *y, const u8 *k, size_t keybits, u8 *outx, u8 *outy)
{
	const pka_ecc256_t *curve = &params.primev256;
	u64 bitlen = keybits;
	u32 bytes = keybits / 8;
	u32 status = 0;
	int ret;

	if (keybits != (curve->size * 8)) {
		return -EINVAL;
	}

	/* If (x, y) is not on the curve, a tamper event is triggered in PKA */
	ret = pka_pointOnCurveCheck(x, y, keybits);
	if (ret < 0) {
		return ret;
	}

	hal_memcpy(common.ram + 0x400, &bitlen, sizeof(u64));
	hal_memcpy(common.ram + 0x408, &bitlen, sizeof(u64));
	hal_memcpy(common.ram + 0x410, curve->asign, sizeof(u64));

	pka_writeRamParam(0x418, curve->absa, bytes);
	pka_writeRamParam(0x520, curve->b, bytes);
	pka_writeRamParam(0x1088, curve->p, bytes);
	pka_writeRamParam(0x12a0, k, bytes);
	pka_writeRamParam(0x578, x, bytes);
	pka_writeRamParam(0x470, y, bytes);
	pka_writeRamParam(0xf88, curve->n, bytes);

	hal_cpuDataMemoryBarrier();

	status = pka_process(PKA_MODE_EC_MULTIPLY);
	if (status != 0) {
		return -EINVAL;
	}

	hal_memcpy(&status, common.ram + 0x680, sizeof(u32));
	if (status == PKA_EC_MULTIPLY_SUCCESS) {
		hal_memcpy(outx, common.ram + 0x578, bytes);
		hal_memcpy(outy, common.ram + 0x5d0, bytes);
		return EOK;
	}
	else {
		return -EINVAL;
	}

	return EOK;
}


/* Public pka interface */
/* Currently the p-256 nist curve is used. Alternatively the user could provide the curve parameters. */


int pka_eccPubKey(const u8 *privateKey, size_t keybits, u8 *Qx, u8 *Qy)
{
	const pka_ecc256_t *curve = &params.primev256;

	if (keybits != (curve->size * 8)) {
		return -EINVAL;
	}

	return pka_eccMultiply(curve->Gx, curve->Gy, privateKey, keybits, Qx, Qy);
}


int pka_ecdsaSign(const u8 *hash, const u8 *privateKey, size_t keybits, u8 *r, u8 *s)
{
	const pka_ecc256_t *curve = &params.primev256;
	u32 bytes = keybits / 8;
	u32 status = 0;
	u32 attempt = 0;
	int ret;

	if (keybits != (curve->size * 8)) {
		return -ENOSYS;
	}

	while (attempt < 1000) {
		/* Generate random k parameter in range [1, n-1] */
		ret = pka_randRange(common.buffer, curve->n, bytes);
		if (ret == -EAGAIN) {
			attempt++;
			continue;
		}
		else if (ret < 0) {
			return ret;
		}

		hal_memcpy(common.ram + 0x400, &keybits, sizeof(u64));
		hal_memcpy(common.ram + 0x408, &keybits, sizeof(u64));
		hal_memcpy(common.ram + 0x410, curve->asign, sizeof(u64));

		pka_writeRamParam(0x418, curve->absa, bytes);
		pka_writeRamParam(0x520, curve->b, bytes);
		pka_writeRamParam(0x1088, curve->p, bytes);
		pka_writeRamParam(0x12a0, common.buffer, bytes);
		pka_writeRamParam(0x578, curve->Gx, bytes);
		pka_writeRamParam(0x470, curve->Gy, bytes);
		pka_writeRamParam(0xfe8, hash, bytes);
		pka_writeRamParam(0xf28, privateKey, bytes);
		pka_writeRamParam(0xf88, curve->n, bytes);
		hal_cpuDataMemoryBarrier();

		status = pka_process(PKA_MODE_ECDSA_SIGN);
		if (status != 0) {
			return -EINVAL;
		}
		hal_cpuDataMemoryBarrier();

		hal_memcpy(&status, common.ram + 0xfe0, sizeof(u32));
		if (status == PKA_ECDSA_SIGN_SUCCESS) {
			hal_memcpy(r, common.ram + 0x730, bytes);
			hal_memcpy(s, common.ram + 0x788, bytes);
			hal_memset(common.buffer, 0, bytes);
			return EOK;
		}
		else if (status == PKA_ECDSA_SIGN_FAIL) {
			return -EINVAL;
		}
		else {
			attempt++;
		}
	}

	return -EAGAIN;
}


int pka_ecdsaVerify(const u8 *r, const u8 *s, const u8 *hash, const u8 *Qx, const u8 *Qy, size_t keybits)
{
	int ret;
	const pka_ecc256_t *curve = &params.primev256;
	u32 bytes = keybits / 8;
	u32 status = 0;

	if (keybits != (curve->size * 8)) {
		return -EINVAL;
	}

	/* Check that (Qx, Qy) is on the curve */
	ret = pka_pointOnCurveCheck(Qx, Qy, keybits);
	if (ret < 0) {
		return -EINVAL;
	}

	hal_memcpy(common.ram + 0x408, &keybits, sizeof(u64));
	hal_memcpy(common.ram + 0x4c8, &keybits, sizeof(u64));
	hal_memcpy(common.ram + 0x468, curve->asign, sizeof(u64));

	pka_writeRamParam(0x470, curve->absa, bytes);
	pka_writeRamParam(0x4d0, curve->p, bytes);
	pka_writeRamParam(0x678, curve->Gx, bytes);
	pka_writeRamParam(0x6d0, curve->Gy, bytes);
	pka_writeRamParam(0x12f8, Qx, bytes);
	pka_writeRamParam(0x1350, Qy, bytes);
	pka_writeRamParam(0x10e0, r, bytes);
	pka_writeRamParam(0xc68, s, bytes);
	pka_writeRamParam(0x13a8, hash, bytes);
	pka_writeRamParam(0x1088, curve->n, bytes);

	hal_cpuDataMemoryBarrier();

	status = pka_process(PKA_MODE_ECDSA_VERIFY);
	if (status != 0) {
		return -EINVAL;
	}

	hal_memcpy(&status, common.ram + 0x5d0, sizeof(u32));
	if (status == PKA_ECDSA_VERIFY_SUCCESS) {
		return EOK;
	}
	else {
		return -EINVAL;
	}
}


void pka_init(void)
{
	common.base = PKA_BASE;
	common.ram = (u8 *)(common.base);

	_stm32_rccSetDevClock(dev_pka, 1);

	/* Wait the end of PKA RAM erase */
	while ((*(common.base + pka_cr) & PKA_CR_EN) != PKA_CR_EN) {
		*(common.base + pka_cr) |= PKA_CR_EN;
	}
	while ((*(common.base + pka_sr) & PKA_SR_INITOK) != PKA_SR_INITOK) {
		/* Wait */
	}
	/* Clear any pending flags */
	*(common.base + pka_clrfr) |= PKA_CLRFR_PROCENDFC | PKA_CLRFR_RAMERRFC | PKA_CLRFR_ADDRERRFC | PKA_CLRFR_OPERRFC;
}
