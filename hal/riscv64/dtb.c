/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * DTB parser
 *
 * Copyright 2018, 2020, 2024 Phoenix Systems
 * Author: Pawel Pisarczyk, Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "dtb.h"

#include <errno.h>
#include <hal/string.h>

extern void _end(void);


struct _fdt_header_t {
	u32 magic;
	u32 totalsize;
	u32 off_dt_struct;
	u32 off_dt_strings;
	u32 off_mem_rsvmap;
	u32 version;
	u32 last_comp_version;
	u32 boot_cpuid_phys;
	u32 size_dt_strings;
	u32 size_dt_struct;
};


static struct {
	struct _fdt_header_t *fdth;

	void *start;

	char *model;
	char *compatible;

	size_t ncpus;
	struct {
		u32 reg;
		char *compatible;
		char *mmu;
		char *isa;
		u32 clock;

		struct {
			char *compatible;
		} intctl;
	} cpus[8];

	struct {
		size_t nreg;
		u64 *reg;
	} memory;

	struct {
		struct {
			int exist;
			u32 *reg;
		} intctl;
	} soc;

} dtb_common;


char *dtb_getString(u32 i)
{
	return (char *)((void *)dtb_common.fdth + ntoh32(dtb_common.fdth->off_dt_strings) + i);
}


void dtb_parseSystem(void *dtb, u32 si, u32 l)
{
	if (hal_strcmp(dtb_getString(si), "model") == 0) {
		dtb_common.model = dtb;
	}
	else if (hal_strcmp(dtb_getString(si), "compatible") == 0) {
		dtb_common.compatible = dtb;
	}
}


void dtb_parseCPU(void *dtb, u32 si, u32 l)
{
	if (hal_strcmp(dtb_getString(si), "compatible") == 0) {
		dtb_common.cpus[dtb_common.ncpus].compatible = dtb;
	}
	else if (hal_strcmp(dtb_getString(si), "riscv,isa") == 0) {
		dtb_common.cpus[dtb_common.ncpus].isa = dtb;
	}
	else if (hal_strcmp(dtb_getString(si), "mmu-type") == 0) {
		dtb_common.cpus[dtb_common.ncpus].mmu = dtb;
	}
	else if (hal_strcmp(dtb_getString(si), "clock-frequency") == 0) {
		dtb_common.cpus[dtb_common.ncpus].clock = ntoh32(*(u32 *)dtb);
	}
}


void dtb_parseInterruptController(void *dtb, u32 si, u32 l)
{
}


void dtb_parseSOCInterruptController(void *dtb, u32 si, u32 l)
{
	dtb_common.soc.intctl.exist = 1;
}


void dtb_parseMemory(void *dtb, u32 si, u32 l)
{
	if (hal_strcmp(dtb_getString(si), "reg") == 0) {
		dtb_common.memory.nreg = l / 16;
		dtb_common.memory.reg = dtb;
	}
}


void dtb_parse(void *dtb)
{
	extern char _start;
	unsigned int d = 0;
	u32 token, si;
	size_t l;
	enum {
		stateIdle,
		stateSystem,
		stateCPU,
		stateCPUInterruptController,
		stateMemory,
		stateSOC,
		stateSOCInterruptController
	} state = stateIdle;

	dtb_common.fdth = (struct _fdt_header_t *)dtb;

	if (dtb_common.fdth->magic != ntoh32(0xd00dfeed)) {
		return;
	}

	dtb = (void *)dtb_common.fdth + ntoh32(dtb_common.fdth->off_dt_struct);
	dtb_common.soc.intctl.exist = 0;
	dtb_common.ncpus = 0;

	for (;;) {
		token = ntoh32(*(u32 *)dtb);
		dtb += 4;

		/* FDT_NODE_BEGIN */
		if (token == 1) {
			if (!d && (*(char *)dtb == 0)) {
				state = stateSystem;
			}
			else if ((d == 1) && (hal_strncmp(dtb, "memory@", 7) == 0)) {
				state = stateMemory;
			}
			else if ((d == 2) && (hal_strncmp(dtb, "cpu@", 4) == 0)) {
				state = stateCPU;
			}
			else if ((state == stateCPU) && (hal_strncmp(dtb, "interrupt-controller", 20) == 0)) {
				state = stateCPUInterruptController;
			}
			else if ((d == 1) && (hal_strncmp(dtb, "soc", 3) == 0)) {
				state = stateSOC;
			}
			else if ((state == stateSOC) && ((hal_strncmp(dtb, "interrupt-controller@", 21) == 0) || (hal_strncmp(dtb, "plic@", 5) == 0))) {
				state = stateSOCInterruptController;
			}

			dtb += ((hal_strlen(dtb) + 3) & ~3);
			d++;
		}

		/* FDT_PROP */
		else if (token == 3) {
			l = ntoh32(*(u32 *)dtb);
			l = ((l + 3) & ~3);

			dtb += 4;
			si = ntoh32(*(u32 *)dtb);
			dtb += 4;

			switch (state) {
				case stateSystem:
					dtb_parseSystem(dtb, si, l);
					break;

				case stateMemory:
					dtb_parseMemory(dtb, si, l);
					break;

				case stateCPU:
					dtb_parseCPU(dtb, si, l);
					break;

				case stateCPUInterruptController:
					dtb_parseInterruptController(dtb, si, l);
					break;

				case stateSOCInterruptController:
					dtb_parseSOCInterruptController(dtb, si, l);
					break;

				default:
					break;
			}

			dtb += l;
		}

		/* FDT_NODE_END */
		else if (token == 2) {
			switch (state) {
				case stateCPU:
					dtb_common.ncpus++;
				case stateMemory:
					state = stateSystem;
					break;

				case stateCPUInterruptController:
					state = stateCPU;
					break;

				case stateSOCInterruptController:
					state = stateSOC;
					break;

				default:
					break;
			}
			d--;
		}
		else if (token == 9) {
			break;
		}
	}

	dtb_common.start = &_start;
}


int dtb_getPLIC(void)
{
	return dtb_common.soc.intctl.exist;
}
