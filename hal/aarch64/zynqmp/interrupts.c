/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GICv2 compatible interrupt controller driver
 *
 * Copyright 2021, 2024 Phoenix Systems
 * Author: Hubert Buczynski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


#define SIZE_INTERRUPTS 188
#define SPI_FIRST_IRQID 32

#define DEFAULT_PRIORITY 0x80

enum {
	/* Distributor registers */
	gicd_ctlr = 0x0,
	gicd_typer,
	gicd_iidr,
	gicd_igroupr0 = 0x20,     /* 6 registers */
	gicd_isenabler0 = 0x40,   /* 6 registers */
	gicd_icenabler0 = 0x60,   /* 6 registers */
	gicd_ispendr0 = 0x80,     /* 6 registers */
	gicd_icpendr0 = 0xa0,     /* 6 registers */
	gicd_isactiver0 = 0xc0,   /* 6 registers */
	gicd_icactiver0 = 0xe0,   /* 6 registers */
	gicd_ipriorityr0 = 0x100, /* 48 registers */
	gicd_itargetsr0 = 0x200,  /* 48 registers */
	gicd_icfgr0 = 0x300,      /* 12 registers */
	gicd_ppisr = 0x340,
	gicd_spisr0, /* 5 registers */
	gicd_sgir = 0x3c0,
	gicd_cpendsgir0 = 0x3c4, /* 4 registers */
	gicd_spendsgir0 = 0x3c8, /* 4 registers */
	gicd_pidr4 = 0x3f4,      /* 4 registers */
	gicd_pidr0 = 0x3f8,      /* 4 registers */
	gicd_cidr0 = 0x3fc,      /* 4 registers */
	/* CPU interface registers */
	gicc_ctlr = 0x4000,
	gicc_pmr,
	gicc_bpr,
	gicc_iar,
	gicc_eoir,
	gicc_rpr,
	gicc_hppir,
	gicc_abpr,
	gicc_aiar,
	gicc_aeoir,
	gicc_ahppir,
	gicc_apr0 = 0x4034,
	gicc_nsapr0 = 0x4038,
	gicc_iidr = 0x403f,
	gicc_dir = 0x8000,
};


/* Type of interrupt's configuration */
enum {
	gicv2_cfg_reserved = 0,
	gicv2_cfg_high_level = 1,
	gicv2_cfg_rising_edge = 3
};


typedef struct {
	int (*f)(unsigned int, void *);
	void *data;
} intr_handler_t;


struct {
	volatile u32 *gic;
	intr_handler_t handlers[SIZE_INTERRUPTS];
} interrupts_common;


void hal_interruptsEnable(unsigned int irqn)
{
	unsigned int irq_reg = irqn / 32;
	unsigned int irq_offs = irqn % 32;

	if (irqn < SPI_FIRST_IRQID || irqn >= SIZE_INTERRUPTS) {
		return;
	}

	*(interrupts_common.gic + gicd_isenabler0 + irq_reg) = 1u << irq_offs;
	hal_cpuDataMemoryBarrier();
}


void hal_interruptsDisable(unsigned int irqn)
{
	unsigned int irq_reg = irqn / 32;
	unsigned int irq_offs = irqn % 32;

	if (irqn < SPI_FIRST_IRQID || irqn >= SIZE_INTERRUPTS) {
		return;
	}

	*(interrupts_common.gic + gicd_icenabler0 + irq_reg) = 1u << irq_offs;
	hal_cpuDataMemoryBarrier();
}


static void interrupts_setConf(unsigned int irqn, u32 conf)
{
	unsigned int irq_reg = irqn / 16;
	unsigned int irq_offs = (irqn % 16) * 2;
	u32 mask;

	if (irqn < SPI_FIRST_IRQID || irqn >= SIZE_INTERRUPTS || conf == gicv2_cfg_reserved) {
		return;
	}

	mask = *(interrupts_common.gic + gicd_icfgr0 + irq_reg) & ~(0x3 << irq_offs);
	*(interrupts_common.gic + gicd_icfgr0 + irq_reg) = mask | ((conf & 0x3) << irq_offs);
}


static void interrupts_setCPU(unsigned int irqn, u32 cpuID)
{
	unsigned int irq_reg = irqn / 4;
	unsigned int irq_offs = (irqn % 4) * 8;
	u32 mask;

	if (irqn < SPI_FIRST_IRQID || irqn >= SIZE_INTERRUPTS) {
		return;
	}

	mask = *(interrupts_common.gic + gicd_itargetsr0 + irq_reg) & ~(0xff << irq_offs);
	*(interrupts_common.gic + gicd_itargetsr0 + irq_reg) = mask | ((cpuID & 0xff) << irq_offs);
}


static void interrupts_setPriority(unsigned int irqn, u32 priority)
{
	unsigned int irq_reg = irqn / 4;
	unsigned int irq_offs = (irqn % 4) * 8;
	u32 mask = *(interrupts_common.gic + gicd_ipriorityr0 + irq_reg) & ~(0xff << irq_offs);

	*(interrupts_common.gic + gicd_ipriorityr0 + irq_reg) = mask | ((priority & 0xff) << irq_offs);
}


static inline u32 interrupts_getPriority(unsigned int irqn)
{
	unsigned int irq_reg = irqn / 4;
	unsigned int irq_offs = (irqn % 4) * 8;

	return (*(interrupts_common.gic + gicd_ipriorityr0 + irq_reg) >> irq_offs) & 0xff;
}


void interrupts_dispatch(void)
{
	u32 n = *(interrupts_common.gic + gicc_iar) & 0x3ff;

	if (n >= SIZE_INTERRUPTS) {
		return;
	}

	if (interrupts_common.handlers[n].f != NULL) {
		interrupts_common.handlers[n].f(n, interrupts_common.handlers[n].data);
	}

	*(interrupts_common.gic + gicc_eoir) = n;

	return;
}


int hal_interruptsSet(unsigned int n, int (*f)(unsigned int, void *), void *data)
{
	if (n >= SIZE_INTERRUPTS) {
		return -1;
	}

	hal_interruptsDisableAll();
	interrupts_common.handlers[n].data = data;
	interrupts_common.handlers[n].f = f;

	if (f == NULL) {
		hal_interruptsDisable(n);
	}
	else {
		interrupts_setPriority(n, DEFAULT_PRIORITY); /* each of the irqs has the same priority */
		interrupts_setCPU(n, 0x1);                   /* CPU 0 handle all irqs                  */
		hal_interruptsEnable(n);
	}

	hal_interruptsEnableAll();

	return 0;
}


static int _zynqmp_interrupts_classify(unsigned int irqn)
{
	/* ZynqMP specific: most interrupts are high level, some are reserved.
	 * PL to PS interrupts can be either high level or rising edge, here we configure
	 * lower half as high level, upper half as rising edge */
	if ((irqn < 40) || ((irqn >= 129) && (irqn <= 135))) {
		return gicv2_cfg_reserved;
	}
	else if ((irqn >= 136) && (irqn <= 143)) {
		return gicv2_cfg_rising_edge;
	}
	else {
		return gicv2_cfg_high_level;
	}
}


void interrupts_init(void)
{
	unsigned int i;

	/* Note: CBAR_EL1 register is set incorrectly in QEMU, so we have to hardcode GIC address */
	interrupts_common.gic = (void *)GIC_BASE_ADDRESS;

	for (i = 0; i < SIZE_INTERRUPTS; ++i) {
		interrupts_common.handlers[i].data = NULL;
		interrupts_common.handlers[i].f = NULL;
	}

	/* Clear pending and disable interrupts */
	for (i = 0; i < (SIZE_INTERRUPTS + 31) / 32; i++) {
		*(interrupts_common.gic + gicd_icenabler0 + i) = 0xffffffff;
		*(interrupts_common.gic + gicd_icpendr0 + i) = 0xffffffff;
		*(interrupts_common.gic + gicd_icactiver0 + i) = 0xffffffff;
	}

	for (i = 0; i < 4; i++) {
		*(interrupts_common.gic + gicd_cpendsgir0 + i) = 0xffffffff;
	}

	/* Disable distributor */
	*(interrupts_common.gic + gicd_ctlr) &= ~0x3;

	/* Set default priorities - 128 for the SGI (IRQID: 0 - 15), PPI (IRQID: 16 - 31), SPI (IRQID: 32 - 188) */
	for (i = 0; i < SIZE_INTERRUPTS; ++i) {
		interrupts_setPriority(i, DEFAULT_PRIORITY);
	}

	/* Set required configuration and CPU_0 as a default processor */
	for (i = SPI_FIRST_IRQID; i < SIZE_INTERRUPTS; ++i) {
		interrupts_setConf(i, _zynqmp_interrupts_classify(i));
		interrupts_setCPU(i, 0x1);
	}

	/* enable_secure = 1 */
	*(interrupts_common.gic + gicd_ctlr) |= 0x3;

	/* Initialize CPU Interface of the gic
	 * Set the maximum priority mask and binary point */
	*(interrupts_common.gic + gicc_bpr) = 3;
	*(interrupts_common.gic + gicc_pmr) = 0xff;

	/* EnableGrp0 = 1; EnableGrp1 = 1; AckCtl = 1; FIQEn = 1 in secure mode
	 * EnableGrp1 = 1 in non-secure mode, other bits are ignored */
	*(interrupts_common.gic + gicc_ctlr) = *(interrupts_common.gic + gicc_ctlr) | 0xf;

	hal_interruptsEnableAll();
}
