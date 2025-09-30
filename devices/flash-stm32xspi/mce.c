/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * STM32 MCE driver
 *
 * Copyright 2025 Phoenix Systems
 * Author: Krzysztof Radzewicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "mce.h"


// static mce_common_t common = {

// };

const static mce_setup_t setup = {
	.base = { MCE1_BASE, MCE2_BASE, MCE3_BASE, MCE4_BASE },
	.perRegions = {
		{ MCE1_REG_START, MCE1_REG_END },
		{ MCE2_REG_START, MCE2_REG_END },
		{ MCE3_REG_START, MCE3_REG_END },
		{ MCE4_REG_START, MCE4_REG_END },
	},
};


/* reg - lowest key register
 * keysize - in bytes (16/32)  */
static int mce_storeKey(mce_t per, u32 reg, u8 *key, u32 keysize)
{
	u32 *key32 = (u32 *)key;
	/* Assume correct arguments */
	for (u32 i = 0; i * 4 < keysize; i++) {
		*(setup.base[per] + (reg + i)) = *(key32++);
	}
	return EOK;
}


static void mce_waitBusy(mce_t per, u32 reg, u32 flag)
{
	while ((*(setup.base[per] + reg) & flag) == 0) {
		;
	}
}


int mce_configureKeys(mce_t per, u32 cipher, u8 *mk, u8 *fmk, u8 *ctxk1, u8 *ctxk2)
{
	u32 keysize, t;
	if (per < 0 || per >= mce_count) {
		return -EINVAL;
	}
	if (cipher != MCE_CIPHER_AES128 && cipher != MCE_CIPHER_AES256 && cipher != MCE_CIPHER_NOEKEON) {
		return -EINVAL;
	}
	if (cipher == MCE_CIPHER_AES256 && (ctxk1 != NULL || ctxk2 != NULL)) {
		return -EPERM;
	}
	/* Select Cipher before writing key */
	t = *(setup.base[per] + mce_cr) & ~MCE_CR_CIPHERSEL_MASK;
	t |= (cipher << MCE_CR_CIPHERSEL_OFF);
	*(setup.base[per] + mce_cr) = t;
	hal_cpuDataMemoryBarrier();

	keysize = (cipher == MCE_CIPHER_AES256) ? (32) : (16);
	if (mk != NULL) {
		mce_storeKey(per, mce_mkeyr1, mk, keysize);
		mce_waitBusy(per, mce_sr, MCE_SR_MKVALID);
	}
	if (fmk != NULL) {
		mce_storeKey(per, mce_fmkeyr1, mk, keysize);
		mce_waitBusy(per, mce_sr, MCE_SR_FMKVALID);
	}
	if (ctxk1 != NULL) {
		mce_storeKey(per, mce_cc1keyr0, ctxk1, keysize);
		mce_waitBusy(per, mce_cc1cfgr, MCE_CCCFGR_CRC_MASK); /* Wait until key check sum computed */
	}
	if (ctxk2 != NULL) {
		mce_storeKey(per, mce_cc2keyr0, ctxk1, keysize);
		mce_waitBusy(per, mce_cc2cfgr, MCE_CCCFGR_CRC_MASK); /* Wait until key check sum computed */
	}

	return EOK;
}

int mce_configureCipherContext(mce_t per, u8 ctxid, u16 version, u32 mode, u8 *nonce)
{
	volatile u32 *cccfgr;
	u32 ccnr0;
	u32 t;
	if (per < 0 || per >= mce_count) {
		return -EINVAL;
	}
	if (ctxid != 1 && ctxid != 2) {
		return -EINVAL;
	}
	if (mode != MCE_MODE_STREAM && mode != MCE_MODE_NBLOCK && mode != MCE_MODE_FBLOCK) {
		return -EINVAL;
	}
	if (mode == MCE_MODE_STREAM && nonce == NULL) {
		return -EINVAL;
	}
	else if (mode != MCE_MODE_STREAM && nonce != NULL) {
		return -EINVAL;
	}

	cccfgr = setup.base[per] + MCE_CCCFGR(ctxid);
	t = *cccfgr & ~((0xffff << MCE_CCCFGR_VERSION_OFF) | (MCE_MODE_MASK << MCE_CCCFGR_MODE_OFF));
	t |= (mode << MCE_CCCFGR_MODE_OFF) | (MCE_CCCFGR_CCEN);
	if (mode == MCE_MODE_STREAM) {
		ccnr0 = (ctxid == 1) ? (mce_cc1nr0) : (mce_cc2nr0);
		mce_storeKey(per, ccnr0, nonce, 8);
		t |= (version << MCE_CCCFGR_VERSION_OFF);
	}
	*cccfgr = t;
	hal_cpuDataMemoryBarrier();

	mce_waitBusy(per, MCE_CCCFGR(ctxid), MCE_CCCFGR_CCEN);
	return EOK;
}

int mce_configureRegion(mce_t per, mce_reg_t reg, u32 mode, u32 ctxid, addr_t start, addr_t end)
{
	volatile u32 *cccfgr;
	u32 t;
	if (per < 0 || per >= mce_count) {
		return -EINVAL;
	}
	if (reg < 0 || reg >= mce_regcount) {
		return -EINVAL;
	}
	if (mode != MCE_MODE_STREAM && mode != MCE_MODE_NBLOCK && mode != MCE_MODE_FBLOCK) {
		return ENOSYS;
	}
	if (ctxid != 0 && ctxid != 1 && ctxid != 2) {
		return -EINVAL;
	}
	if (mode == MCE_MODE_STREAM) {
		if (ctxid == 0) {
			return -EINVAL;
		}
	}
	if (ctxid != 0) {
		cccfgr = setup.base[per] + MCE_CCCFGR(ctxid);
		if (((*cccfgr >> MCE_CCCFGR_MODE_OFF) & MCE_MODE_MASK) != mode) {
			return -EINVAL;
		}
	}
	if (end < start) {
		return -EINVAL;
	}
	if (start < setup.perRegions[per].start || end > setup.perRegions[per].end) {
		return -EINVAL;
	}
	if ((start % MCE_REG_BOUNDARY != 0) || ((end + 1) % MCE_REG_BOUNDARY != 0)) {
		return -EINVAL;
	}

	*(setup.base[per] + MCE_SADDR(reg)) = start;
	*(setup.base[per] + MCE_EADDR(reg)) = end;
	hal_cpuDataMemoryBarrier();

	t = *(setup.base[per] + MCE_REGCR(reg)) & ~(MCE_REGCR_CTXID_MASK | MCE_REGCR_ENC_MASK);
	t |= (mode << MCE_REGCR_ENC_OFF) | (ctxid << MCE_REGCR_CTXID_OFF);
	*(setup.base[per] + MCE_REGCR(reg)) = t;
	hal_cpuDataMemoryBarrier();


	*(setup.base[per] + MCE_REGCR(reg)) |= MCE_REGCR_BREN;

	return EOK;
};


int mce_configureLock(mce_t per, mce_lock_t lock)
{
	if (per < 0 || per >= mce_count) {
		return -EINVAL;
	}
	if ((lock & mce_ctx1keylock) != 0) {
		*(setup.base[per] + mce_cc1cfgr) |= MCE_CCCFGR_KEYLOCK;
	}
	if ((lock & mce_ctx2keylock) != 0) {
		*(setup.base[per] + mce_cc2cfgr) |= MCE_CCCFGR_KEYLOCK;
	}
	if ((lock & mce_ctx1lock) != 0) {
		*(setup.base[per] + mce_cc1cfgr) |= MCE_CCCFGR_CCLOCK;
	}
	if ((lock & mce_ctx2lock) != 0) {
		*(setup.base[per] + mce_cc2cfgr) |= MCE_CCCFGR_CCLOCK;
	}
	if ((lock & mce_masterlock) != 0) {
		*(setup.base[per] + mce_cr) |= MCE_CR_MKLOCK;
	}
	if ((lock & mce_globallock) != 0) {
		*(setup.base[per] + mce_cr) |= MCE_CR_GLOCK;
	}
	return EOK;
}


void mce_init(void)
{
/* The RM says that MCE clocks are automatically manged by the device, but that only applies to reseting clocks.
 * We still need to turn them on. */
#if USE_MCE1
	_stm32_rccSetDevClock(dev_mce1, 1);
#endif
#if USE_MCE2
	_stm32_rccSetDevClock(dev_mce2, 1);
#endif
#if USE_MCE3
	_stm32_rccSetDevClock(dev_mce3, 1);
#endif
#if USE_MCE4
	_stm32_rccSetDevClock(dev_mce4, 1);
#endif
	return;
}

void mce_test(mce_t per, mce_reg_t reg, u32 mode, u8 *key, addr_t start, addr_t end, u32 cipher)
{
	// u32 t;
	// /* Select Cipher before writing key */
	// t = *(setup.base[per] + mce_cr) & ~MCE_CR_CIPHERSEL_MASK;
	// t |= (cipher << MCE_CR_CIPHERSEL_OFF);
	// *(setup.base[per] + mce_cr) = t;
	// hal_cpuDataMemoryBarrier();

	// *(setup.base[per] + MCE_SADDR(reg)) = start;
	// *(setup.base[per] + MCE_EADDR(reg)) = end;
	// hal_cpuDataMemoryBarrier();

	// t = *(setup.base[per] + MCE_REGCR(reg)) & ~(MCE_REGCR_CTXID_MASK | MCE_REGCR_ENC_MASK);
	// t |= (mode << MCE_REGCR_ENC_OFF) | (ctxid << MCE_REGCR_CTXID_OFF);
	// *(setup.base[per] + MCE_REGCR(reg)) = t;
	// hal_cpuDataMemoryBarrier();


	*(setup.base[per] + MCE_REGCR(reg)) |= MCE_REGCR_BREN;
}
