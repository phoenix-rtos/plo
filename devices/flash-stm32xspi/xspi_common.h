/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * STM32 XSPI driver
 *
 * Copyright 2025 Phoenix Systems
 * Author: Jacek Maksymowicz, Krzysztof Radzewicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _XSPI_COMMON_H_
#define _XSPI_COMMON_H_

#include <hal/hal.h>
#include <lib/lib.h>
#include <devices/devs.h>

#include <board_config.h>
#include <hal/armv8m/stm32/n6/stm32n6.h>
#include "mce.h"


/* It's not practical to automatically determine if a given XSPI bus uses HyperBus protocol
 * or regular commands, so use compile-time configuration for it. */

#ifndef XSPI1_IS_HYPERBUS
#define XSPI1_IS_HYPERBUS 0
#endif

#ifndef XSPI2_IS_HYPERBUS
#define XSPI2_IS_HYPERBUS 0
#endif

#ifndef XSPI3_IS_HYPERBUS
#define XSPI3_IS_HYPERBUS 0
#endif

#ifndef XSPI1
#define XSPI1 0
#endif

#ifndef XSPI2
#define XSPI2 0
#endif

#ifndef XSPI3
#define XSPI3 0
#endif

#define XSPI_FIFO_SIZE     64 /* Size of hardware FIFO */
#define XSPI_N_CONTROLLERS 2
#define XSPI_MCE_REGIONS   1 /* Hardware supports up to 4 */

#define XSPI_SR_BUSY (1UL << 5) /* Controller busy */
#define XSPI_SR_TOF  (1UL << 4) /* Timeout */
#define XSPI_SR_SMF  (1UL << 3) /* Status match in auto-polling mode */
#define XSPI_SR_FTF  (1UL << 2) /* FIFO threshold */
#define XSPI_SR_TCF  (1UL << 1) /* Transfer complete */
#define XSPI_SR_TEF  (1UL << 0) /* Transfer error */

#define XSPI_CR_MODE_IWRITE   (0UL << 28) /* Indirect write mode */
#define XSPI_CR_MODE_IREAD    (1UL << 28) /* Indirect read mode */
#define XSPI_CR_MODE_AUTOPOLL (2UL << 28) /* Auto-polling mode */
#define XSPI_CR_MODE_MEMORY   (3UL << 28) /* Memory-mapped mode */
#define XSPI_CR_MODE_MASK     (3UL << 28) /* Mask of mode bits */

#define XSPI_DCR1_MTYP_MICRON       (0UL << 24) /* In DDR 8-bit data mode: D0 comes before D1, DQS polarity inverted */
#define XSPI_DCR1_MTYP_MACRONIX     (1UL << 24) /* In DDR 8-bit data mode: D1 comes before D0 */
#define XSPI_DCR1_MTYP_STANDARD     (2UL << 24)
#define XSPI_DCR1_MTYP_MACRONIX_RAM (3UL << 24) /* In DDR 8-bit data mode: D1 comes before D0. Addresses are translated into row/column format required by manufacturer. */
#define XSPI_DCR1_MTYP_HBUS_MEM     (4UL << 24) /* HyperBus memory access mode */
#define XSPI_DCR1_MTYP_HBUS_REG     (5UL << 24) /* HyperBus register access mode */
#define XSPI_DCR1_MTYP_APMEMORY     (6UL << 24) /* In 16-bit data mode addresses are translated into format required by manufacturer */
#define XSPI_DCR1_MTYP_MASK         (7UL << 24) /* Mask of memory type bits */

#define XSPIM_PORT1   0
#define XSPIM_PORT2   1
#define XSPIM_MUX_OFF 0UL /* No multiplexed accesses */
#define XSPIM_MUX_ON  1UL /* XSPI1 and XSPI2 do multiplexed accesses on the same port */
/* If MUX_OFF: XSPI1 to Port 1, XSPI2 to Port 2
 * If MUX_ON: XSPI1 and XSPI2 muxed to Port 1, XSPI3 to Port 2*/
#define XSPIM_MODE_DIRECT 0UL
/* If MUX_OFF: XSPI1 to Port 2, XSPI2 to Port 1
 * If MUX_ON: XSPI1 and XSPI2 muxed to Port 2, XSPI3 to Port 1 */
#define XSPIM_MODE_SWAPPED (1UL << 1)

#define XSPI_CHIPSELECT_NCS1 0UL
#define XSPI_CHIPSELECT_NCS2 1UL

enum xspi_regs {
	xspi_cr = 0x0,
	xspi_dcr1 = 0x2,
	xspi_dcr2,
	xspi_dcr3,
	xspi_dcr4,
	xspi_sr = 0x8,
	xspi_fcr,
	xspi_dlr = 0x10,
	xspi_ar = 0x12,
	xspi_dr = 0x14,
	xspi_psmkr = 0x20,
	xspi_psmar = 0x22,
	xspi_pir = 0x24,
	xspi_ccr = 0x40,
	xspi_tcr = 0x42,
	xspi_ir = 0x44,
	xspi_abr = 0x48,
	xspi_lptr = 0x4c,
	xspi_wpccr = 0x50,
	xspi_wptcr = 0x52,
	xspi_wpir = 0x54,
	xspi_wpabr = 0x58,
	xspi_wccr = 0x60,
	xspi_wtcr = 0x62,
	xspi_wir = 0x64,
	xspi_wabr = 0x68,
	xspi_hlcr = 0x80,
	xspi_calfcr = 0x84,
	xspi_calmr = 0x86,
	xspi_calsor = 0x88,
	xspi_calsir = 0x8a,
};


typedef struct {
	s16 port;
	s8 pin;
} xspi_pin_t;


typedef struct {
	addr_t start;
	addr_t end; /* Exclusive */
	int encrypt;
} xspi_memregion_t; /* Encrypted memory region */


typedef struct {
	addr_t start; /* Address refers to the memory device, not cpu. */
	addr_t end;   /* Same as above. Exclusive */
	u32 cipher;
	u32 mode;
	u8 *key;
} xspi_memcrypt_args_t;


typedef struct {
	void *start;
	u32 size;
	volatile u32 *ctrl;
	struct {
		u16 sel; /* Clock mux (ipclk_xspi?sel) */
		u8 val;  /* Clock source (one of ipclk_sel_*) */
	} clksel;
	u16 divider_slow; /* Divider used for initialization - resulting clock must be under 50 MHz */
	u16 divider;      /* Divider used for normal operation - can be as fast as Flash can handle */
	u16 dev;
	u16 rst;
	xspi_pin_t resetPin; /* Hardware reset pin for device (set to -1 if unused) */
	u8 enabled;
	u8 spiPort;
	u8 chipSelect;
	u8 isHyperbus;
	u8 mceDev;
} xspi_ctrlParams_t;


extern const xspi_ctrlParams_t xspi_ctrlParams[XSPI_N_CONTROLLERS];

extern u32 xspi_memSize[XSPI_N_CONTROLLERS];


static inline void xspi_waitBusy(unsigned int minor)
{
	while ((*(xspi_ctrlParams[minor].ctrl + xspi_sr) & XSPI_SR_BUSY) != 0) {
		/* Wait for controller to become ready */
	}
}


/* Transfer data in indirect read or write mode using the FIFO.
 * `data`: output (if isRead == 0) or input (if isRead != 0) data buffer
 * `len`: length of data to transfer
 * `isRead`: == 0 if this is a write operation, != 0 otherwise */
int xspi_transferFifo(unsigned int minor, u8 *data, size_t len, u8 isRead);


/* Switch to higher clock speed (defined by `xspi_ctrlParams_t::divider`)*/
void xspi_setHigherClock(unsigned int minor);


/* Functions for regular-command SPI devices */


int xspi_regcom_init(unsigned int minor);


int xspi_regcom_sync(unsigned int minor);


ssize_t xspi_regcom_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout);


ssize_t xspi_regcom_write(unsigned int minor, addr_t offs, const void *buff, size_t len);


ssize_t xspi_regcom_write_mapped(unsigned int minor, addr_t offs, const void *buff, size_t len);


ssize_t xspi_regcom_erase(unsigned int minor, addr_t offs, size_t len, unsigned int flags);


/* Functions for HyperBus devices */


int xspi_hb_init(unsigned int minor);


int xspi_hb_sync(unsigned int minor);


ssize_t xspi_hb_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout);


ssize_t xspi_hb_write(unsigned int minor, addr_t offs, const void *buff, size_t len);


ssize_t xspi_hb_erase(unsigned int minor, addr_t offs, size_t len, unsigned int flags);

#endif
