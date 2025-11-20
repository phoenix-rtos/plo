/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * STM32 Memory Cipher Engine driver
 *
 * Copyright 2025 Phoenix Systems
 * Author: Krzysztof Radzewicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "mce.h"


static mce_common_t common = {};

const static mce_setup_t setup = {
	.base = { MCE1_BASE, MCE2_BASE, MCE3_BASE, MCE4_BASE },
	.region = {
		{ MCE1_REG_START, MCE1_REG_END },
		{ MCE2_REG_START, MCE2_REG_END },
		{ MCE3_REG_START, MCE3_REG_END },
		{ MCE4_REG_START, MCE4_REG_END },
	},
	.dev = { dev_mce1, dev_mce2, dev_mce3, dev_mce4 }
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


/* Configure MCE keysize and master/fast mastery key sources.
 * Key bytes in little endian.
 * Keys must have correct length, according to cipher.
 * For restrictions see: RM0486 ch. 51 */
static int mce_configurePerInternal(mce_t per, u32 cipher, u8 *mk, u8 *fmk)
{
	u32 keysize, t;
	/* Select Cipher before writing key */
	if (common.cipher[per] == 0) {
		t = *(setup.base[per] + mce_cr) & ~MCE_CR_CIPHERSEL_MASK;
		t |= (cipher << MCE_CR_CIPHERSEL_OFF);
		*(setup.base[per] + mce_cr) = t;
		hal_cpuDataMemoryBarrier();
		common.cipher[per] = cipher;
	}

	/* For now only write keys if no valid key present */
	keysize = (cipher == MCE_CIPHER_AES256) ? (32) : (16);
	if ((mk != NULL) && ((*(setup.base[per] + mce_sr) & MCE_SR_MKVALID) == 0)) {
		mce_storeKey(per, mce_mkeyr1, mk, keysize);
		mce_waitBusy(per, mce_sr, MCE_SR_MKVALID);
	}
	if ((fmk != NULL) && ((*(setup.base[per] + mce_sr) & MCE_SR_FMKVALID) == 0)) {
		mce_storeKey(per, mce_fmkeyr1, fmk, keysize);
		mce_waitBusy(per, mce_sr, MCE_SR_FMKVALID);
	}

	return EOK;
}


/* Configure and enable MCE cipher context z=1,2.
 * Cannot reconfigure context already in use.
 * If stream mode used, must provide 8 byte nonce value (little endian). Set to NULL otherwise. */
static int mce_configureCipherContextInternal(mce_t per, u8 ctxid, u16 version, u32 mode, u8 *nonce, u8 *ctxkey)
{
	volatile u32 *cccfgr;
	u32 cckeyr0;
	u32 ccnr0;
	u32 t;
	if (ctxid != 1 && ctxid != 2) {
		return -EINVAL;
	}
	if (mode == MCE_MODE_STREAM && nonce == NULL) {
		return -EINVAL;
	}
	else if (mode != MCE_MODE_STREAM && nonce != NULL) {
		return -EINVAL;
	}

	/* For now configure context only if it's not yet enabled */
	if (((*(setup.base[per] + MCE_CCCFGR(ctxid)) & MCE_CCCFGR_CCEN) != 0)) {
		return EOK;
	}

	cccfgr = setup.base[per] + MCE_CCCFGR(ctxid);
	t = *cccfgr & ~((0xffff << MCE_CCCFGR_VERSION_OFF) | (MCE_MODE_MASK << MCE_CCCFGR_MODE_OFF));
	t |= (mode << MCE_CCCFGR_MODE_OFF);
	if (mode == MCE_MODE_STREAM) {
		ccnr0 = (ctxid == 1) ? (mce_cc1nr0) : (mce_cc2nr0);
		mce_storeKey(per, ccnr0, nonce, 8);
		t |= (version << MCE_CCCFGR_VERSION_OFF);
	}
	*cccfgr = t;
	hal_cpuDataMemoryBarrier();

	if (ctxkey != NULL) {
		cckeyr0 = (ctxid == 1) ? (mce_cc1keyr0) : (mce_cc2keyr0);
		mce_storeKey(per, cckeyr0, ctxkey, 16);
		/* TODO: consider computing the expected checksum and compare it */
		mce_waitBusy(per, MCE_CCCFGR(ctxid), MCE_CCCFGR_CRC_MASK); /* Wait until key check sum computed */
		/* NOTE: This could be a bug waiting to happen. I think we can't just wait for the checksum to not be zero? But there is no other way to know the key is usable now. */
		hal_cpuDataMemoryBarrier();
	}

	*cccfgr |= MCE_CCCFGR_CCEN;
	mce_waitBusy(per, MCE_CCCFGR(ctxid), MCE_CCCFGR_CCEN);

	return EOK;
}


/* Configure and enable a memory region to be encrypted/decrypted, by MCE.
 * Cannot reconfigure a region already in use.
 * Relevant keys/cipher context needs to be configured first.
 * Set ctxid to 0 if using master-key of fast-master-key. */
static int mce_configureRegionInternal(mce_t per, mce_reg_t reg, u32 mode, u32 ctxid, addr_t first, addr_t last)
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
		return -EINVAL;
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
	if (last < first) {
		return -EINVAL;
	}
	if (first < setup.region[per].base || last > setup.region[per].last) {
		return -EINVAL;
	}
	if ((first % MCE_REG_BOUNDARY != 0) || ((last + 1) % MCE_REG_BOUNDARY != 0)) {
		return -EINVAL;
	}

	t = *(setup.base[per] + MCE_REGCR(reg)) & ~(MCE_REGCR_CTXID_MASK | MCE_REGCR_ENC_MASK);
	t |= (mode << MCE_REGCR_ENC_OFF) | (ctxid << MCE_REGCR_CTXID_OFF);
	*(setup.base[per] + MCE_REGCR(reg)) = t;
	hal_cpuDataMemoryBarrier();

	*(setup.base[per] + MCE_SADDR(reg)) = first;
	*(setup.base[per] + MCE_EADDR(reg)) = last;
	hal_cpuDataMemoryBarrier();


	*(setup.base[per] + MCE_REGCR(reg)) |= MCE_REGCR_BREN;

	return EOK;
};


/* Lock some part of MCE configuration. Recommended to use global lock after mce regions are configured. */
static int mce_configureLockInternal(mce_t per, mce_lock_t lock)
{
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


/* start - refers to device memory
 * end - same as above, exclusive */
static int mce_validateParamsInternal(mce_t per, mce_reg_t reg, addr_t start, addr_t end, u32 cipher, u32 mode, u8 *key)
{
	if ((per < 0) || (per >= mce_count)) {
		return -EINVAL;
	}
	if (reg < 0 || reg >= mce_regcount) {
		return -EINVAL;
	}
	if (cipher != MCE_CIPHER_AES128 && cipher != MCE_CIPHER_AES256 && cipher != MCE_CIPHER_NOEKEON) {
		return -EINVAL;
	}
	if (mode != MCE_MODE_STREAM && mode != MCE_MODE_NBLOCK && mode != MCE_MODE_FBLOCK) {
		return -EINVAL;
	}

	/* Check cipher/mode validity */
	if (mode == MCE_MODE_STREAM) {
		if (cipher == MCE_CIPHER_AES256) {
			return -EINVAL;
		}
	}
	if ((common.cipher[per] != 0) && (common.cipher[per] != cipher)) {
		return -EINVAL;
	}


	/* Check size granularity. We assume that controller made sure the region is correct otherwise */
	if (((start % MCE_REG_BOUNDARY) != 0) || ((end % MCE_REG_BOUNDARY) != 0)) {
		return -EINVAL;
	}

	return EOK;
}


/* MCE public interface functions */
/* Note: For now we only allow a single region per xspi memory, so we don't have to worry about reusing previous keys. */
int mce_configureRegion(mce_t per, mce_reg_t reg, addr_t start, addr_t end, u32 cipher, u32 mode, u8 *key)
{
	int ret;
	u8 *mk = NULL, *fmk = NULL;
	u32 ctxid = 0;
	u8 nonce[8];
	u16 version;
	/* Check basic parameters */
	ret = mce_validateParamsInternal(per, reg, start, end, cipher, mode, key);
	if (ret < 0) {
		return ret;
	}

	/* The RM says that MCE clocks are automatically managed by the device, but that only applies to resetting clocks.
	 * We still need to turn them on. */
	if (common.initialized[per] == 0) {
		_stm32_rccSetDevClock(setup.dev[per], 1);
		common.initialized[per] = 1;
	}

	if (cipher == MCE_CIPHER_AES256) {
		if (mode == MCE_MODE_NBLOCK) {
			mk = key;
		}
		else if (mode == MCE_MODE_FBLOCK) {
			fmk = key;
		}
	}
	ret = mce_configurePerInternal(per, cipher, mk, fmk);
	if (ret < 0) {
		return ret;
	}

	if ((cipher == MCE_CIPHER_AES128) || (cipher == MCE_CIPHER_NOEKEON)) {
		if (common.contextCount[per] < 2) {
			ctxid = ++common.contextCount[per];
		}
		else {
			ctxid = common.contextCount[per];
		}
		if (mode == MCE_MODE_STREAM) {
			ret = _stm32_rngRead((u8 *)&version, 2);
			if (ret < 0) {
				return ret;
			}
			ret = _stm32_rngRead(nonce, 8);
			if (ret < 0) {
				return ret;
			}
			/* Note: Consider writing zeros to nonce and version as they will remain in memory after function returns. */
			ret = mce_configureCipherContextInternal(per, ctxid, version, mode, nonce, key);
		}
		else {
			/* Configure context without version and nonce */
			ret = mce_configureCipherContextInternal(per, ctxid, 0, mode, NULL, key);
		}

		if (ret < 0) {
			return ret;
		}
	}

	ret = mce_configureRegionInternal(per, reg, mode, ctxid, setup.region[per].base + start, setup.region[per].base + end - 1);
	if (ret < 0) {
		return ret;
	}

	/* No further reconfiguration required. Lock registers */
	mce_configureLockInternal(per, mce_globallock);

	return EOK;
}
