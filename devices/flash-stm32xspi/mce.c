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


#define MCE1_BASE ((void *)0x5802b800)
#define MCE2_BASE ((void *)0x5802bc00)
#define MCE3_BASE ((void *)0x5802c000)
#define MCE4_BASE ((void *)0x5802e000)

#define MCE_MODE_MASK 3UL

#define MCE_CR_CIPHERSEL_MASK (3UL << 4)
#define MCE_CR_CIPHERSEL_OFF  4UL
#define MCE_CR_MKLOCK         (1UL << 1)
#define MCE_CR_GLOCK          (1UL << 0)

#define MCE_SR_MKVALID  (1UL << 0)
#define MCE_SR_FMKVALID (1UL << 2)
#define MCE_SR_ENCDIS   (1UL << 4)

#define MCE_IASR_IAEF (1UL << 1)

#define MCE_REGCR_BREN       (1UL << 0)
#define MCE_REGCR_CTXID_OFF  9UL
#define MCE_REGCR_CTXID_MASK (3UL << 9)
#define MCE_REGCR_ENC_OFF    14UL
#define MCE_REGCR_ENC_MASK   (3UL << 14)

#define MCE_REGCR(reg_id) (mce_regcr1 + 4 * (reg_id))
#define MCE_SADDR(reg_id) (mce_saddr1 + 4 * (reg_id))
#define MCE_EADDR(reg_id) (mce_eaddr1 + 4 * (reg_id))

#define MCE_CCCFGR_VERSION_OFF 16UL
#define MCE_CCCFGR_CRC_MASK    (0xffUL << 8)
#define MCE_CCCFGR_MODE_OFF    4UL
#define MCE_CCCFGR_KEYLOCK     (1UL << 2)
#define MCE_CCCFGR_CCLOCK      (1UL << 1)
#define MCE_CCCFGR_CCEN        (1UL << 0)
#define MCE_CCCFGR(ctxid)      (((ctxid) == 1) ? (mce_cc1cfgr) : (mce_cc2cfgr))


#define MCE1_REG_START 0x90000000
#define MCE1_REG_END   0x9fffffff
#define MCE2_REG_START 0x70000000
#define MCE2_REG_END   0x7fffffff
#define MCE3_REG_START 0x80000000
#define MCE3_REG_END   0x8fffffff
#define MCE4_REG_START 0x60000000
#define MCE4_REG_END   0x6fffffff

#define MCE_REG_BOUNDARY 4 * 1024

/* MCE peripherals */
typedef enum {
	mce1,
	mce2,
	mce3,
	mce4,
	mce_count,
} mce_t;

enum {
	mce_cr,
	mce_sr,
	mce_iasr,
	mce_iacr,
	mce_iaier,
	mce_iaddr = 0x9,
	mce_regcr1 = 0x10,
	mce_saddr1,
	mce_eaddr1,
	mce_regcr2 = 0x14,
	mce_saddr2,
	mce_eaddr2,
	mce_regcr3 = 0x18,
	mce_saddr3,
	mce_eaddr3,
	mce_regcr4 = 0x1c,
	mce_saddr4,
	mce_eaddr4,
	mce_mkeyr1 = 0x80,
	mce_mkeyr2,
	mce_mkeyr3,
	mce_mkeyr4,
	mce_mkeyr5,
	mce_mkeyr6,
	mce_mkeyr7,
	mce_mkeyr8,
	mce_fmkeyr1 = 0x88,
	mce_fmkeyr2,
	mce_fmkeyr3,
	mce_fmkeyr4,
	mce_fmkeyr5,
	mce_fmkeyr6,
	mce_fmkeyr7,
	mce_fmkeyr8,
	mce_cc1cfgr = 0x90,
	mce_cc1nr0,
	mce_cc1nr1,
	mce_cc1keyr0,
	mce_cc1keyr1,
	mce_cc1keyr2,
	mce_cc1keyr3,
	mce_cc2cfgr,
	mce_cc2nr0,
	mce_cc2nr1,
	mce_cc2keyr0,
	mce_cc2keyr1,
	mce_cc2keyr2,
	mce_cc2keyr3,
};


/* MCE lock types */
#define MCE_GLOBALLOCK  (1 << 0)
#define MCE_MASTERLOCK  (1 << 1)
#define MCE_CTX1LOCK    (1 << 2)
#define MCE_CTX2LOCK    (1 << 3)
#define MCE_CTX1KEYLOCK (1 << 4)
#define MCE_CTX2KEYLOCK (1 << 5)

static struct mce_params {
	u8 initialized;
	u8 cipher;
	u8 contextCount;
} mce_params[mce_count] = {
	[mce1] = { .initialized = 0 },
	[mce2] = { .initialized = 0 },
	[mce3] = { .initialized = 0 },
	[mce4] = { .initialized = 0 },
};


static const struct mce_setup {
	volatile u32 *base;
	struct {
		u32 base;
		u32 last;
	} region;
	unsigned int dev;
} mce_setup[mce_count] = {
	[mce1] = { .base = MCE1_BASE, .region = { MCE1_REG_START, MCE1_REG_END }, .dev = dev_mce1 },
	[mce2] = { .base = MCE2_BASE, .region = { MCE2_REG_START, MCE2_REG_END }, .dev = dev_mce2 },
	[mce3] = { .base = MCE3_BASE, .region = { MCE3_REG_START, MCE3_REG_END }, .dev = dev_mce3 },
	[mce4] = { .base = MCE4_BASE, .region = { MCE4_REG_START, MCE4_REG_END }, .dev = dev_mce4 },
};


/* reg - lowest key register
 * keysize - in bytes (16/32)  */
static void mce_storeKey(mce_t per, u32 reg, const u8 *key, u32 keysize)
{
	size_t i;
	/* Assume correct arguments */
	for (i = 0; i < keysize; i += 4) {
		*(mce_setup[per].base + (reg + (i / 4))) =
				((u32)key[i + 0] << 0) |
				((u32)key[i + 1] << 8) |
				((u32)key[i + 2] << 16) |
				((u32)key[i + 3] << 24);
	}
}


static void mce_waitBusy(mce_t per, u32 reg, u32 flag)
{
	while ((*(mce_setup[per].base + reg) & flag) == 0) {
		;
	}
}


/* Configure MCE keysize and master/fast mastery key sources.
 * Key bytes in little endian.
 * Keys must have correct length, according to cipher.
 * For restrictions see: RM0486 ch. 51 */
static int mce_configurePerInternal(mce_t per, u32 cipher, const u8 *mk, const u8 *fmk)
{
	const struct mce_setup *s = &mce_setup[per];
	u32 keysize, t;
	/* Select Cipher before writing key */
	if (mce_params[per].cipher == 0) {
		t = s->base[mce_cr] & ~MCE_CR_CIPHERSEL_MASK;
		t |= (cipher << MCE_CR_CIPHERSEL_OFF);
		s->base[mce_cr] = t;
		hal_cpuDataMemoryBarrier();
		mce_params[per].cipher = cipher;
	}

	/* For now only write keys if no valid key present */
	keysize = (cipher == MCE_CIPHER_AES256) ? (32) : (16);
	if ((mk != NULL) && ((s->base[mce_sr] & MCE_SR_MKVALID) == 0)) {
		mce_storeKey(per, mce_mkeyr1, mk, keysize);
		mce_waitBusy(per, mce_sr, MCE_SR_MKVALID);
	}
	if ((fmk != NULL) && ((s->base[mce_sr] & MCE_SR_FMKVALID) == 0)) {
		mce_storeKey(per, mce_fmkeyr1, fmk, keysize);
		mce_waitBusy(per, mce_sr, MCE_SR_FMKVALID);
	}

	return EOK;
}


/* Configure and enable MCE cipher context z=1,2.
 * Cannot reconfigure context already in use.
 * If stream mode used, must provide 8 byte nonce value (little endian). Set to NULL otherwise. */
static int mce_configureCipherContextInternal(mce_t per, u8 ctxid, u16 version, u32 mode, const u8 *nonce, const u8 *ctxkey)
{
	const struct mce_setup *s = &mce_setup[per];
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
	if (((s->base[MCE_CCCFGR(ctxid)] & MCE_CCCFGR_CCEN) != 0)) {
		return EOK;
	}

	cccfgr = &s->base[MCE_CCCFGR(ctxid)];
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
	const struct mce_setup *s = &mce_setup[per];
	volatile u32 *cccfgr;
	u32 t;
	if (ctxid != 0 && ctxid != 1 && ctxid != 2) {
		return -EINVAL;
	}
	if (mode == MCE_MODE_STREAM) {
		if (ctxid == 0) {
			return -EINVAL;
		}
	}
	if (ctxid != 0) {
		cccfgr = &s->base[MCE_CCCFGR(ctxid)];
		if (((*cccfgr >> MCE_CCCFGR_MODE_OFF) & MCE_MODE_MASK) != mode) {
			return -EINVAL;
		}
	}
	if (last < first) {
		return -EINVAL;
	}
	if (first < s->region.base || last > s->region.last) {
		return -EINVAL;
	}
	if ((first % MCE_REG_BOUNDARY != 0) || ((last + 1) % MCE_REG_BOUNDARY != 0)) {
		return -EINVAL;
	}

	t = s->base[MCE_REGCR(reg)] & ~(MCE_REGCR_CTXID_MASK | MCE_REGCR_ENC_MASK);
	t |= (mode << MCE_REGCR_ENC_OFF) | (ctxid << MCE_REGCR_CTXID_OFF);
	s->base[MCE_REGCR(reg)] = t;
	hal_cpuDataMemoryBarrier();

	s->base[MCE_SADDR(reg)] = first;
	s->base[MCE_EADDR(reg)] = last;
	hal_cpuDataMemoryBarrier();


	s->base[MCE_REGCR(reg)] |= MCE_REGCR_BREN;

	return EOK;
};


/* Lock some part of MCE configuration. Recommended to use global lock after mce regions are configured. */
static int mce_configureLockInternal(mce_t per, u32 lock)
{
	const struct mce_setup *s = &mce_setup[per];
	if ((lock & MCE_CTX1KEYLOCK) != 0) {
		s->base[mce_cc1cfgr] |= MCE_CCCFGR_KEYLOCK;
	}
	if ((lock & MCE_CTX2KEYLOCK) != 0) {
		s->base[mce_cc2cfgr] |= MCE_CCCFGR_KEYLOCK;
	}
	if ((lock & MCE_CTX1LOCK) != 0) {
		s->base[mce_cc1cfgr] |= MCE_CCCFGR_CCLOCK;
	}
	if ((lock & MCE_CTX2LOCK) != 0) {
		s->base[mce_cc2cfgr] |= MCE_CCCFGR_CCLOCK;
	}
	if ((lock & MCE_MASTERLOCK) != 0) {
		s->base[mce_cr] |= MCE_CR_MKLOCK;
	}
	if ((lock & MCE_GLOBALLOCK) != 0) {
		s->base[mce_cr] |= MCE_CR_GLOCK;
	}
	return EOK;
}


/* start - refers to device memory
 * end - same as above, exclusive */
static int mce_validateParamsInternal(mce_t per, mce_reg_t reg, addr_t start, addr_t end, u32 cipher, u32 mode, const u8 *key)
{
	struct mce_params *p;
	if ((per < 0) || (per >= mce_count)) {
		return -EINVAL;
	}
	if ((reg < 0) || (reg >= mce_regcount)) {
		return -EINVAL;
	}
	if ((cipher != MCE_CIPHER_AES128) && (cipher != MCE_CIPHER_AES256) && (cipher != MCE_CIPHER_NOEKEON)) {
		return -EINVAL;
	}
	if ((mode != MCE_MODE_STREAM) && (mode != MCE_MODE_NBLOCK) && (mode != MCE_MODE_FBLOCK)) {
		return -EINVAL;
	}

	p = &mce_params[per];
	/* Check cipher/mode validity */
	if (mode == MCE_MODE_STREAM) {
		if (cipher == MCE_CIPHER_AES256) {
			return -EINVAL;
		}
	}

	/* MCE already configured with a different cipher */
	if ((p->cipher != 0) && (p->cipher != cipher)) {
		return -EINVAL;
	}


	/* Check size granularity. We assume that controller made sure the region is correct otherwise */
	if (((start % MCE_REG_BOUNDARY) != 0) || ((end % MCE_REG_BOUNDARY) != 0)) {
		return -EINVAL;
	}

	return EOK;
}


static int mce_getMceForDevice(int dev)
{
	switch (dev) {
		case dev_mce1: return mce1;
		case dev_mce2: return mce2;
		case dev_mce3: return mce3;
		case dev_mce4: return mce4;
		default: return mce_count;
	}
}


/* MCE public interface functions */
/* Note: For now we only allow a single region per xspi memory, so we don't have to worry about reusing previous keys. */
int mce_configureRegion(int dev, mce_reg_t reg, const dev_memcrypt_args_t *args)
{
	int ret;
	const u8 *mk = NULL, *fmk = NULL;
	u32 ctxid = 0;
	u32 cipher;
	mce_t per;

	switch (args->algo) {
		case DEV_MEMCRYPT_ALGO_AES128: cipher = MCE_CIPHER_AES128; break;
		case DEV_MEMCRYPT_ALGO_NOEKEON: cipher = MCE_CIPHER_NOEKEON; break;
		case DEV_MEMCRYPT_ALGO_AES256: cipher = MCE_CIPHER_AES256; break;
		default: return -EINVAL;
	}

	/* Check basic parameters */
	per = mce_getMceForDevice(dev);
	ret = mce_validateParamsInternal(per, reg, args->start, args->end, cipher, args->mode, args->key);
	if (ret < 0) {
		return ret;
	}

	struct mce_params *p = &mce_params[per];
	const struct mce_setup *s = &mce_setup[per];

	/* The RM says that MCE clocks are automatically managed by the device, but that only applies to resetting clocks.
	 * We still need to turn them on. */
	if (p->initialized == 0) {
		_stm32_rccSetDevClock(s->dev, 1);
		p->initialized = 1;
	}

	if (cipher == MCE_CIPHER_AES256) {
		if (args->mode == MCE_MODE_NBLOCK) {
			mk = args->key;
		}
		else if (args->mode == MCE_MODE_FBLOCK) {
			fmk = args->key;
		}
	}
	ret = mce_configurePerInternal(per, cipher, mk, fmk);
	if (ret < 0) {
		return ret;
	}

	if ((cipher == MCE_CIPHER_AES128) || (cipher == MCE_CIPHER_NOEKEON)) {
		if (p->contextCount < 2) {
			ctxid = ++p->contextCount;
		}
		else {
			ctxid = p->contextCount;
		}
		if (args->mode == MCE_MODE_STREAM) {
			if (args->ivSize < 8) {
				return -EINVAL;
			}

			/* TODO: for now we always set version to 0 - do we have any use case where
			 * changing the version would be desirable? */
			ret = mce_configureCipherContextInternal(per, ctxid, 0, args->mode, args->iv, args->key);
		}
		else {
			if (args->ivSize != 0) {
				/* In this mode IV is unused. Treat this as an invalid argument to signal to the user
				 * that they should not expect this value to do anything. */
				return -EINVAL;
			}

			/* Configure context without version and nonce */
			ret = mce_configureCipherContextInternal(per, ctxid, 0, args->mode, NULL, args->key);
		}

		if (ret < 0) {
			return ret;
		}
	}

	ret = mce_configureRegionInternal(
			per,
			reg,
			args->mode,
			ctxid,
			s->region.base + args->start,
			s->region.base + args->end - 1);
	if (ret < 0) {
		return ret;
	}

	/* No further reconfiguration required. Lock registers */
	mce_configureLockInternal(per, MCE_GLOBALLOCK);

	return EOK;
}


int mce_getGranularity(int dev, mce_reg_t reg)
{
	mce_t per = mce_getMceForDevice(dev);
	if ((per >= mce_count) || (reg < 0) || (reg >= mce_regcount)) {
		return -EINVAL;
	}

	u32 regcr = *(mce_setup[per].base + MCE_REGCR(reg));

	if ((regcr & MCE_REGCR_BREN) == 0) {
		/* Encryption disabled */
		return 0;
	}

	switch ((regcr >> MCE_REGCR_ENC_OFF) & MCE_MODE_MASK) {
		case MCE_MODE_STREAM: return 0;
		case MCE_MODE_NBLOCK: return 16;
		case MCE_MODE_FBLOCK: return 16;
		default: return 0;
	}
}
