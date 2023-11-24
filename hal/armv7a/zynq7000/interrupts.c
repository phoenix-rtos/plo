/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * MPCore exception and interrupt handling
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


#define MPCORE_BASE_ADDRESS 0xf8f00000
#define SIZE_INTERRUPTS     95
#define SPI_FIRST_IRQID     32


enum {
	/* Interrupt interface registers */
	cicr = 0x40, cpmr, cbpr, ciar, ceoir, crpr, chpir, cabpr,
	/* Distributor registers */
	ddcr = 0x400, dictr, diidr, disr0 = 0x420, /* 2 registers */	diser0 = 0x440, /* 2 registers */ dicer0 = 0x460, /* 2 registers */
	dispr0 = 0x480, /* 2 registers */ dicpr0 = 0x4a0, /* 2 registers */ dabr0 = 0x4c0, /* 2 registers */ dipr0 = 0x500, /* 24 registers */
	diptr0 = 0x600, /* 24 registers */ dicfr0 = 0x700, /* 6 registers */ ppi_st = 0x740, spi_st0, spi_st1, dsgir = 0x7c0, pidr4 = 0x7f4,
	pidr5, pidr6, pidr7, pidr0, pidr1, pidr2, pidr3, cidr0, cidr1, cidr2, cidr3
};


/* Type of interrupt's configuration */
enum {
	reserved = 0, high_lvl = 1, rising_edge = 3
};


typedef struct {
	int (*f)(unsigned int, void *);
	void *data;
} intr_handler_t;


struct {
	volatile u32 *mpcore;
	intr_handler_t handlers[SIZE_INTERRUPTS];
} interrupts_common;


/* Required configuration for SPI (Shared Peripheral Interrupts IRQID[32:95]) */
static const u8 spiConf[] = {/* IRQID: 32-39 */ rising_edge, rising_edge, high_lvl, high_lvl, reserved, high_lvl, high_lvl, high_lvl,
                             /* IRQID: 40-47 */ high_lvl, rising_edge, high_lvl, high_lvl, high_lvl, high_lvl, high_lvl, high_lvl,
                             /* IRQID: 48-55 */ high_lvl, high_lvl, high_lvl, high_lvl, high_lvl, high_lvl, high_lvl, rising_edge,
                             /* IRQID: 56-63 */ high_lvl, high_lvl, high_lvl, high_lvl, high_lvl, rising_edge, rising_edge, rising_edge,
                             /* IRQID: 64-71 */ rising_edge, rising_edge, rising_edge, rising_edge, rising_edge, high_lvl, high_lvl, high_lvl,
                             /* IRQID: 72-79 */ high_lvl, high_lvl, high_lvl, high_lvl, high_lvl, high_lvl, rising_edge, high_lvl,
                             /* IRQID: 80-87 */ high_lvl, high_lvl, high_lvl, high_lvl, high_lvl, high_lvl, high_lvl, high_lvl,
                             /* IRQID: 88-95 */ high_lvl, high_lvl, high_lvl, high_lvl, rising_edge, reserved, reserved, reserved };



void hal_interruptsEnable(unsigned int irqn)
{
	if (irqn < SPI_FIRST_IRQID || irqn >= SIZE_INTERRUPTS) {
		return;
	}

	*(interrupts_common.mpcore + diser0 + (irqn >> 5)) = 1 << (irqn & 0x1f);
}


void hal_interruptsDisable(unsigned int irqn)
{
	if (irqn < SPI_FIRST_IRQID || irqn >= SIZE_INTERRUPTS) {
		return;
	}

	*(interrupts_common.mpcore + dicer0 + (irqn >> 5)) = 1 << (irqn & 0x1f);
}


static void interrupts_setConf(unsigned int irqn, u32 conf)
{
	u32 mask;

	if (irqn < SPI_FIRST_IRQID || irqn >= SIZE_INTERRUPTS || conf == reserved)
		return;

	mask = *(interrupts_common.mpcore + dicfr0 + (irqn >> 4)) & ~(0x3 << ((irqn & 0xf) << 1));
	*(interrupts_common.mpcore + dicfr0 + (irqn >> 4)) = mask | ((conf & 0x3) << ((irqn & 0xf) << 1));
}


static void interrupts_setCPU(unsigned int irqn, u32 cpuID)
{
	u32 mask;

	if (irqn < SPI_FIRST_IRQID || irqn >= SIZE_INTERRUPTS)
		return;

	mask = *(interrupts_common.mpcore + diptr0 + (irqn >> 2)) & ~(0x3 << ((irqn & 0x3) << 3));
	*(interrupts_common.mpcore + diptr0 + (irqn >> 2)) = mask | ((cpuID & 0x3) << ((irqn & 0x3) << 3));
}


static void interrupts_setPriority(unsigned int irqn, u32 priority)
{
	u32 mask = *(interrupts_common.mpcore + dipr0 + (irqn >> 2)) & ~(0xff << ((irqn & 0x3) << 3));

	*(interrupts_common.mpcore + dipr0 + (irqn >> 2)) = mask | ((priority & 0xff) << ((irqn & 0x3) << 3));
}


static inline u32 interrupts_getPriority(unsigned int irqn)
{
	return (*(interrupts_common.mpcore + dipr0 + (irqn >> 2)) >> ((irqn & 0x3) << 3)) & 0xff;
}


void interrupts_dispatch(void)
{
	u32 n = *(interrupts_common.mpcore + ciar) & 0x3ff;

	if (n >= SIZE_INTERRUPTS)
		return;

	if (interrupts_common.handlers[n].f != NULL)
		interrupts_common.handlers[n].f(n, interrupts_common.handlers[n].data);

	*(interrupts_common.mpcore + ceoir) = n;

	return;
}


int hal_interruptsSet(unsigned int n, int (*f)(unsigned int, void *), void *data)
{
	if (n >= SIZE_INTERRUPTS)
		return -1;

	hal_interruptsDisableAll();
	interrupts_common.handlers[n].data = data;
	interrupts_common.handlers[n].f = f;

	if (f == NULL) {
		hal_interruptsDisable(n);
	}
	else {
		interrupts_setPriority(n, 0xa); /* each of the irqs has the same priority */
		interrupts_setCPU(n, 0x1);      /* CPU 0 handle all irqs                  */
		hal_interruptsEnable(n);
	}

	hal_interruptsEnableAll();

	return 0;
}


void interrupts_init(void)
{
	int i;

	interrupts_common.mpcore = (void *)MPCORE_BASE_ADDRESS;

	for (i = 0; i < SIZE_INTERRUPTS; ++i) {
		interrupts_common.handlers[i].data = NULL;
		interrupts_common.handlers[i].f = NULL;
	}

	/* Initialize Distributor of the mpcore
	 * enable_secure = 0 */
	*(interrupts_common.mpcore + ddcr) &= ~0x3;

	/* Set default priorities - 10 for the SGI (IRQID: 0 - 15), PPI (IRQID: 16 - 31), SPI (IRQID: 32 - 95) */
	for (i = 0; i < SIZE_INTERRUPTS; ++i)
		interrupts_setPriority(i, 0xa);

	/* Set required configuration and CPU_0 as a default processor */
	for (i = SPI_FIRST_IRQID; i < SIZE_INTERRUPTS; ++i) {
		interrupts_setConf(i, spiConf[i - SPI_FIRST_IRQID]);
		interrupts_setCPU(i, 0x1);
	}

	/* Disable interrupts */
	*(interrupts_common.mpcore + dicer0) = 0xffffffff;
	*(interrupts_common.mpcore + dicer0 + 1) = 0xffffffff;
	*(interrupts_common.mpcore + dicer0 + 2) = 0xffffffff;

	/* enable_secure = 1 */
	*(interrupts_common.mpcore + ddcr) |= 0x3;

	*(interrupts_common.mpcore + cicr) &= ~0x3;

	/* Initialize CPU Interface of the mpcore
	 * set the maximum priority mask */
	*(interrupts_common.mpcore + cpmr) = (*(interrupts_common.mpcore + cpmr) & ~0x1f) | 0x1f;

	/* EnableS = 1; EnableNS = 1; AckCtl = 1; FIQEn = 0 */
	*(interrupts_common.mpcore + cicr) = (*(interrupts_common.mpcore + cicr) & ~0x7) | 0x7;

	hal_interruptsEnableAll();
}
