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
 * %LICENSE%
 */

#include "pka.h"
#include <lib/lib.h>


/* https://www.secg.org/SEC2-Ver-1.0.pdf
 * Curve parameres are to be stored in little-endian.
 * Instead of storing the immediate value of the 'a' coefficient, we need abs(a) mod p.
 * PKA accepts two formats, as stated in RM (52.5.1)
 */
/* clang-format off */
static const struct {
	pka_ecc384_t secp384r1;

} params = {
	.secp384r1 = {
		.n = {
			0x73, 0x29, 0xC5, 0xCC, 0x6A, 0x19, 0xEC, 0xEC,
			0x7A, 0xA7, 0xB0, 0x48, 0xB2, 0x0D, 0x1A, 0x58,
			0xDF, 0x2D, 0x37, 0xF4, 0x81, 0x4D, 0x63, 0xC7,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		},
		.p = {
			0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		}, 
		.Gx = {
			0xB7, 0x0A, 0x76, 0x72, 0x38, 0x5E, 0x54, 0x3A,
			0x6C, 0x29, 0x55, 0xBF, 0x5D, 0xF2, 0x02, 0x55,
			0x38, 0x2A, 0x54, 0x82, 0xE0, 0x41, 0xF7, 0x59,
			0x98, 0x9B, 0xA7, 0x8B, 0x62, 0x3B, 0x1D, 0x6E,
			0x74, 0xAD, 0x20, 0xF3, 0x1E, 0xC7, 0xB1, 0x8E,
			0x37, 0x05, 0x8B, 0xBE, 0x22, 0xCA, 0x87, 0xAA
		},
		.Gy = {
			0x5F, 0x0E, 0xEA, 0x90, 0x7C, 0x1D, 0x43, 0x7A,
			0x9D, 0x81, 0x7E, 0x1D, 0xCE, 0xB1, 0x60, 0x0A,
			0xC0, 0xB8, 0xF0, 0xB5, 0x13, 0x31, 0xDA, 0xE9,
			0x7C, 0x14, 0x9A, 0x28, 0xBD, 0x1D, 0xF4, 0xF8,
			0x29, 0xDC, 0x92, 0x92, 0xBF, 0x98, 0x9E, 0x5D,
			0x6F, 0x2C, 0x26, 0x96, 0x4A, 0xDE, 0x17, 0x36
		},
		.absa = {
 			0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		},
		.b = {
			0xEF, 0x2A, 0xEC, 0xD3, 0xED, 0xC8, 0x85, 0x2A,
			0x9D, 0xD1, 0x2E, 0x8A, 0x8D, 0x39, 0x56, 0xC6,
			0x5A, 0x87, 0x13, 0x50, 0x8F, 0x08, 0x14, 0x03,
			0x12, 0x41, 0x81, 0xFE, 0x6E, 0x9C, 0x1D, 0x18,
			0x19, 0x2D, 0xF8, 0xE3, 0x6B, 0x05, 0x8E, 0x98,
			0xE4, 0xE7, 0x3E, 0xE2, 0xA7, 0x2F, 0x31, 0xB3
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


static __attribute__((unused)) void print_buf(const u8 *buf, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		lib_printf("%02x ", buf[i]);
		if (i % 16 == 15) {
			lib_printf("\n");
		}
	}
}


static void pka_writeRamParam(size_t offset, const void *data, size_t bytes)
{
	hal_memcpy(common.ram + offset, data, bytes);
	/* Insert 8 null bytes termination */
	hal_memset(common.ram + offset + bytes, 0, 8);
}


static u32 pka_process(u32 mode)
{
	u32 cr = *(common.base + pka_cr);
	cr = (cr & ~PKA_CR_MODE_MASK) | (mode << PKA_CR_MODE_OFF) | PKA_CR_START;
	*(common.base + pka_cr) = cr;
	hal_cpuDataMemoryBarrier();

	while ((*(common.base + pka_sr) & PKA_SR_PROCENDF) != PKA_SR_PROCENDF) {
		/* Wait */
	}

	/* TODO: Check errors and restart if ramerr was found. That would mean someone tried reading ram
	 * during processing */

	/* Clear all flags */
	*(common.base + pka_clrfr) |= PKA_CLRFR_PROCENDFC | PKA_CLRFR_RAMERRFC | PKA_CLRFR_ADDRERRFC | PKA_CLRFR_OPERRFC;

	return 0;
}


/* Attempt to generate random value in range [1, n-1] */
static int pka_randRange(u8 *out, const u8 *n, size_t len)
{
	int ret, attempt = 0;
	size_t off;
	int kLesser;

	while (attempt < 1000) {
		ret = _stm32_rngRead(out, len);
		if (ret < 0) {
			return ret;
		}

		off = len;
		kLesser = 0;
		while (off > 0) {
			if (out[off - 1] > n[off - 1]) {
				kLesser = 0;
				break;
			}
			else if (out[off - 1] < n[off - 1]) {
				kLesser = 1;
				break;
			}
			off--;
		}
		if (kLesser == 1) {
			return EOK;
		}
		attempt++;
	}

	return -EAGAIN;
}


/* For testing only */
/* oplen - operand length in bytes */
// void pka_modAdd(const void *a, const void *b, const void *n, size_t oplen, void *out)
// {
// 	u64 opbits = oplen * 8;
// 	hal_memcpy(common.ram + 0x408, &opbits, 8);

// 	hal_memcpy(common.ram + 0xa50, a, oplen);
// 	hal_memset(common.ram + 0xa50 + oplen, 0, 8);

// 	hal_memcpy(common.ram + 0xc68, b, oplen);
// 	hal_memset(common.ram + 0xc68 + oplen, 0, 8);

// 	hal_memcpy(common.ram + 0x1088, n, oplen);
// 	hal_memset(common.ram + 0x1088 + oplen, 0, 8);

// 	hal_cpuDataMemoryBarrier();

// 	pka_process(PKA_MODE_MOD_ADD);

// 	hal_memcpy(out, common.ram + 0xe78, oplen);

// 	hal_cpuDataMemoryBarrier();
// 	*(common.base + pka_clrfr) |= PKA_CLRFR_PROCENDFC;
// }


static void pka_montgomeryParameter(const u8 *n, size_t nbits, u8 *out)
{
	/* TODO: N value cannot have leading zero bytes (need to check if leading 0
	   bits are ok still). Filter those out before continuing */
	u64 bitlen = nbits;
	u32 bytes = nbits / 8;

	hal_memcpy(common.ram + 0x408, &bitlen, sizeof(u64));
	pka_writeRamParam(0x1088, n, bytes);
	hal_cpuDataMemoryBarrier();

	pka_process(PKA_MODE_MONTGOMERY);

	hal_cpuDataMemoryBarrier();

	hal_memcpy(out, common.ram + 0x620, bytes);
}


// int pka_modExpFast(const u8 *base, const u8 *exp, const u8 *n, size_t size, u8 *out)
// {
// 	u64 bits = size * 8;
// 	pka_montgomeryParameterInternal(n, size * 8, common.buffer);

// 	hal_memcpy(common.ram + 0x400, &bits, 8);
// 	hal_memcpy(common.ram + 0x408, &bits, 8);
// 	pka_writeRamParam(0xc68, base, size);
// 	pka_writeRamParam(0xe78, exp, size);
// 	pka_writeRamParam(0x1088, n, size);
// 	pka_writeRamParam(0x620, common.buffer, size);

// 	hal_cpuDataMemoryBarrier();
// 	pka_process(PKA_MODE_MOD_EXP);

// 	hal_cpuDataMemoryBarrier();
// 	hal_memcpy(out, common.ram + 0x838, size);

// 	return EOK;
// }


static int pka_pointOnCurveCheckInternal(const u8 *x, const u8 *y, size_t keybits)
{
	u64 bitlen = keybits;
	u32 bytes = keybits / 8;
	u32 status = 0;
	const pka_ecc384_t *curve = &params.secp384r1;

	if (keybits != 384) {
		return -EINVAL;
	}

	/* For this operation we need montgomery parameter R^2 mod n */
	pka_montgomeryParameter(curve->p, keybits, common.buffer);

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

	pka_process(PKA_MODE_EC_CURVE_CHECK);
	hal_cpuDataMemoryBarrier();

	hal_memcpy(&status, common.ram + 0x680, sizeof(u32));
	hal_cpuDataMemoryBarrier();

	if (status == PKA_EC_CURVE_CHECK_SUCCESS) {
		return EOK;
	}
	else if (status == PKA_EC_CURVE_CHECK_FAIL) {
		return -69;
	}
	else {
		return -EINVAL;
	}
}


static int pka_eccMultiplicationInternal(const u8 *x, const u8 *y, const u8 *k, size_t keybits, u8 *outx, u8 *outy)
{
	const pka_ecc384_t *curve = &params.secp384r1;
	u64 bitlen = keybits;
	u32 bytes = keybits / 8;
	u32 status = 0;
	int ret;

	if (keybits != 384) {
		return -EINVAL;
	}

	/* If (x, y) is not on the curve, a tamper event is triggered in PKA */
	ret = pka_pointOnCurveCheckInternal(x, y, keybits);
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

	pka_process(PKA_MODE_EC_MULTIPLY);

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
/* For now secp384r1 is hardcoded everywhere. Could let the user choose a curve */


int pka_eccPubKey(const u8 *privateKey, size_t keybits, u8 *Qx, u8 *Qy)
{
	const pka_ecc384_t *curve = &params.secp384r1;

	if (keybits != 384) {
		return -EINVAL;
	}

	return pka_eccMultiplicationInternal(curve->Gx, curve->Gy, privateKey, keybits, Qx, Qy);
}


int pka_ecdsaSign(const u8 *hash, const u8 *privateKey, size_t keybits, u8 *r, u8 *s)
{
	const pka_ecc384_t *curve = &params.secp384r1;
	u64 bitlen = PKA_ECC384_BYTES * 8;
	u32 bytes = PKA_ECC384_BYTES;
	u32 status = 0;
	u32 attempt = 0;
	int ret;

	if (keybits != 384) {
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

		hal_memcpy(common.ram + 0x400, &bitlen, sizeof(u64));
		hal_memcpy(common.ram + 0x408, &bitlen, sizeof(u64));
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

		pka_process(PKA_MODE_ECDSA_SIGN);
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


/* keybits - length of private key */
int pka_ecdsaVerify(const u8 *r, const u8 *s, const u8 *hash, const u8 *Qx, const u8 *Qy, size_t keybits)
{
	int ret;
	const pka_ecc384_t *curve = &params.secp384r1;
	u64 bitlen = keybits;
	u32 bytes = keybits / 8;
	u32 status = 0;

	if (keybits != 384) {
		return -EINVAL;
	}

	/* Check that (Qx, Qy) is on the curve */
	ret = pka_pointOnCurveCheckInternal(Qx, Qy, keybits);
	if (ret < 0) {
		return -EINVAL;
	}

	hal_memcpy(common.ram + 0x408, &bitlen, sizeof(u64));
	hal_memcpy(common.ram + 0x4c8, &bitlen, sizeof(u64));
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

	pka_process(PKA_MODE_ECDSA_VERIFY);

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
	// *(common.base + pka_cr) |= PKA_CR_ADDRERRIE | PKA_CR_OPERRIE | PKA_CR_RAMERRIE;
	while ((*(common.base + pka_sr) & PKA_SR_INITOK) != PKA_SR_INITOK) {
		/* Wait */
	}
	/* Clear any pending flags */
	*(common.base + pka_clrfr) |= PKA_CLRFR_PROCENDFC | PKA_CLRFR_RAMERRFC | PKA_CLRFR_ADDRERRFC | PKA_CLRFR_OPERRFC;
}
