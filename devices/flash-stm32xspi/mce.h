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


#ifndef _MCE_H_
#define _MCE_H_

#include <hal/hal.h>
#include <lib/lib.h>

#include <board_config.h>
#include <hal/armv8m/stm32/n6/stm32n6.h>

/* Enable/disable encryption of each external memory in board config */
#ifndef USE_MCE1
#define USE_MCE1 0
#endif

#ifndef USE_MCE2
#define USE_MCE2 0
#endif

#ifndef USE_MCE3
#define USE_MCE3 0
#endif

#ifndef USE_MCE4
#define USE_MCE4 0
#endif

#define MCE1_BASE ((void *)0x5802b800)
#define MCE2_BASE ((void *)0x5802bc00)
#define MCE3_BASE ((void *)0x5802c000)
#define MCE4_BASE ((void *)0x5802e000)

#define MCE_MODE_STREAM 1UL
#define MCE_MODE_NBLOCK 2UL
#define MCE_MODE_FBLOCK 3UL
#define MCE_MODE_MASK   3UL

#define MCE_CIPHER_AES128  1UL
#define MCE_CIPHER_NOEKEON 2UL
#define MCE_CIPHER_AES256  3UL

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

/* MCE regions */
typedef enum {
	mce_r1,
	mce_r2,
	mce_r3,
	mce_r4,
	mce_regcount,
} mce_reg_t;

/* MCE lock types */
typedef enum {
	mce_globallock = (1 << 0),
	mce_masterlock = (1 << 1),
	mce_ctx1lock = (1 << 2),
	mce_ctx2lock = (1 << 3),
	mce_ctx1keylock = (1 << 4),
	mce_ctx2keylock = (1 << 5),
	mce_lockcount,
} mce_lock_t;

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

typedef struct mce_common_t {
	u8 enabledReg[mce_count];
	u32 flags;
} mce_common_t;

typedef struct mce_regAddr_t {
	u32 start, end;
} mce_regAddr_t;

typedef struct mce_setup_t {
	volatile u32 *base[mce_count];
	mce_regAddr_t perRegions[mce_count];
} mce_setup_t;


/* Configure all necessary keys that a given MCE peripheral can use.
 * Key bytes in little endian.
 * Once a key is in use, you cannot change it.
 * Use NULL if you want to skip providing a key.
 * Keys must have correct length, according to cipher.
 * For restrictions see: RM0486 ch. 51 */
int mce_configureKeys(mce_t per, u32 cipher, u8 *mk, u8 *fmk, u8 *ctxk1, u8 *ctxk2);

/* Configure and enable MCE cipher context z=1,2.
 * Cipher context key must already be written.
 * Cannot reconfigure context already in use.
 * If stream mode used, must provide 8 byte nonce value (little endian). Set to NULL otherwise. */
int mce_configureCipherContext(mce_t per, u8 ctxid, u16 version, u32 mode, u8 *nonce);

/* Configure and enable a memory region to be enrypted/decrypted, by MCE.
 * Cannot reconfigure a region already in use.
 * Relevant keys/cipher context needs to be configured first.
 * Set ctxid to 0 if using master-key of fast-master-key. */
int mce_configureRegion(mce_t per, mce_reg_t reg, u32 mode, u32 ctxid, addr_t start, addr_t end);


/* Lock some part of MCE configuration. Recomended to use global lock after mce regions are configured. */
int mce_configureLock(mce_t per, mce_lock_t lock);


void mce_init(void);


void xspi_hb_test(unsigned int minor);

#endif
