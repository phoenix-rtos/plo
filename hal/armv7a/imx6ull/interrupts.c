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

#define SIZE_INTERRUPTS 159


typedef struct {
	int (*f)(unsigned int, void *);
	void *data;
} intr_handler_t;


enum {
	/* Distributor register map */

	/* 1024 reserved */ gicd_ctlr = 0x400, gicd_typer, gicd_iidr, /* 29 reserved */ gicd_igroupr0 = 0x420, /* 16 registers */
	/* 16 reserved */ gicd_isenabler0 = 0x440, /* 16 registers */ /* 16 reserved */ gicd_icenabler0 = 0x460, /* 16 registers */
	/* 16 reserved */ gicd_ispendr0 = 0x480, /* 16 registers */ /* 16 reserved */ gicd_icpendr0 = 0x4a0, /* 16 registers */
	/* 16 reserved */ gicd_isactiver0 = 0x4c0, /* 16 registers */ /* 16 reserved */ gicd_icactiver0 = 0x4e0, /* 16 registers */
	/* 16 reserved */ gicd_ipriorityr0 = 0x500, /* 64 registers */ /* 128 reserved */ gicd_itargetsr0 = 0x600, /* 64 registers */
	/* 128 reserved */ gicd_icfgr0 = 0x700, /* 32 registers */ /* 32 reserved */ gicd_ppisr = 0x740, gicd_spisr0, /* 15 registers */
	/* 112 reserved */ gicd_sgir = 0x7c0, /* 3 reserved */ gicd_cpendsgir = 0x7c4, /* 4 registers */ gicd_spendsgir = 0x7c8, /* 4 registers */
	/* 40 reserved */ gicd_pidr4 = 0x7f4, gicd_pidr5, gicd_pidr6, gicd_pidr7, gicd_pidr0, gicd_pidr1, gicd_pidr2, gicd_pidr3,
	gicd_cidr0, gicd_cidr1, gicd_cidr2, gicd_cidr3,

	/* GIC virtual CPU interface register map */

	gicv_ctlr = 0x800, gicv_pmr, gicv_bpr, gicv_iar, gicv_eoir, gicv_rpr, gicv_hppir, gicv_abpr, gicv_aiar, gicv_aeoir, gicv_ahppir
	/* 41 reserved */, gicv_apr0 = 0x834, /* 3 reserved */ gicv_nsapr0 = 0x838, /* 6 reserved */ gicv_ciidr = 0x83f,
	/* 960 reserved */ gicv_dir = 0xc00 };


struct {
	volatile u32 *gic;
	intr_handler_t handlers[SIZE_INTERRUPTS];
} interrupts_common;


void hal_interruptsEnable(unsigned int irqn)
{
	if (irqn >= SIZE_INTERRUPTS) {
		return;
	}

	*(interrupts_common.gic + gicd_isenabler0 + (irqn >> 5)) = 1 << (irqn & 0x1f);
}


void hal_interruptsDisable(unsigned int irqn)
{
	if (irqn >= SIZE_INTERRUPTS) {
		return;
	}

	*(interrupts_common.gic + gicd_icenabler0 + (irqn >> 5)) = 1 << (irqn & 0x1f);
}


void interrupts_dispatch(void)
{
	u32 n, iar;

	iar = *(interrupts_common.gic + gicv_iar);

	/* Check that INT_ID is not 1023 or 1022 (spurious interrupt) */
	if ((iar & 0x03fe) >= 0x03fe)
		return;

	n = iar & 0x1ff;

	if (n >= SIZE_INTERRUPTS)
		return;

	if (interrupts_common.handlers[n].f != NULL)
		interrupts_common.handlers[n].f(n, interrupts_common.handlers[n].data);

	/* Update End of Interrupt register with original value from iar */
	*(interrupts_common.gic + gicv_eoir) = iar;

	return;
}


static void interrupts_setConf(unsigned int irqn, u32 conf)
{
	u32 t;

	t = *(interrupts_common.gic + gicd_icfgr0 + (irqn >> 4)) & ~(0x3 << ((irqn & 0xf) << 1));
	*(interrupts_common.gic + gicd_icfgr0 + (irqn >> 4)) = t | ((conf & 0x3) << ((irqn & 0xf) << 1));
}


static void interrupts_setPriority(unsigned int irqn, u32 priority)
{
	u32 mask = *(interrupts_common.gic + gicd_ipriorityr0 + (irqn >> 2)) & ~(0xff << ((irqn & 0x3) << 3));

	*(interrupts_common.gic + gicd_ipriorityr0 + (irqn >> 2)) = mask | ((priority & 0xff) << ((irqn & 0x3) << 3));
}


static inline u32 interrupts_getPriority(unsigned int irqn)
{
	return (*(interrupts_common.gic + gicd_ipriorityr0 + (irqn >> 2)) >> ((irqn & 0x3) << 3)) & 0xff;
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
		interrupts_setPriority(n, 0xa);
		interrupts_setConf(n, 0x3);
		hal_interruptsEnable(n);
	}

	hal_interruptsEnableAll();

	return 0;
}


static void interrupts_initAddr(void)
{
	asm volatile(" \
		mrc p15, 4, %0, c15, c0, 0"
		: "=r" (interrupts_common.gic));
}


void interrupts_init(void)
{
	u32 i, t, priority;

	interrupts_initAddr();

	*(interrupts_common.gic + gicd_ctlr) &= ~1;

	interrupts_setPriority(0, 0xff);
	priority = interrupts_getPriority(0);

	for (i = 32; i <= SIZE_INTERRUPTS; ++i) {
		*(interrupts_common.gic + gicd_icenabler0 + (i >> 5)) = 1 << (i & 0x1f);
		interrupts_setConf(i, 0);
		interrupts_setPriority(i, priority >> 1);
		t = *(interrupts_common.gic + gicd_itargetsr0 + (i >> 2)) & ~(0xff << ((i & 0x3) << 3));
		*(interrupts_common.gic + gicd_itargetsr0 + (i >> 2)) = t | (1 << ((i & 0x3) << 3));
		*(interrupts_common.gic + gicd_igroupr0 + (i >> 5)) &= ~(1 << (i & 0x1f));
	}

	*(interrupts_common.gic + gicd_ctlr) |= 1;
	*(interrupts_common.gic + gicv_ctlr) &= ~1;

	for (i = 0; i < 32; ++i) {
		if (i > 15)
			interrupts_setConf(i, 0);
		*(interrupts_common.gic + gicd_icenabler0) = 1 << i;
		interrupts_setPriority(i, priority >> 1);
		*(interrupts_common.gic + gicd_igroupr0) &= ~(1 << i);
	}

	*(interrupts_common.gic + gicv_ctlr) |= 1;
	*(interrupts_common.gic + gicv_bpr) = 0;
	*(interrupts_common.gic + gicv_pmr) = 0xff;

	hal_interruptsEnableAll();
}
