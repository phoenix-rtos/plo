/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ZynqMP DDR4 initialization
 * Based on ZynqMP FSBL code by Xilinx, licensed under MIT license.
 *
 * Copyright 2025 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "zynqmp.h"
#include "zynqmp_regs.h"
#include "config.h"

#include <board_config.h>

#define DDRC_BASE_ADDRESS    0xfd070000
#define DDR_PHY_BASE_ADDRESS 0xfd080000

struct {
	volatile u32 *ddrc;
	volatile u32 *ddr_phy;
} ddrInit_common;

static struct {
	unsigned offset;
	u32 mask;
	u32 value;
} ddrcInitValues[] = {
	{ ddrc_mstr, 0xe30fbe3d, 0x81040010 },
	{ ddrc_mrctrl0, 0x8000f03f, 0x00000030 },
	{ ddrc_derateen, 0x000003f3, 0x00000200 },
	{ ddrc_derateint, 0xffffffff, 0x00800000 },
	{ ddrc_pwrctl, 0x0000007f, 0x00000000 },
	{ ddrc_pwrtmg, 0x00ffff1f, 0x00408210 },
	{ ddrc_rfshctl0, 0x00f1f1f4, 0x00210000 },
	{ ddrc_rfshctl1, 0x0fff0fff, 0x00000000 },
	{ ddrc_rfshctl3, 0x00000073, 0x00000001 },
	{ ddrc_rfshtmg, 0x0fff83ff, 0x007f8089 },
	{ ddrc_ecccfg0, 0x00000017, 0x00000010 },
	{ ddrc_ecccfg1, 0x00000003, 0x00000000 },
	{ ddrc_crcparctl1, 0x3f000391, 0x10000200 },
	{ ddrc_crcparctl2, 0x01ff1f3f, 0x0040051f },
	{ ddrc_init0, 0xc3ff0fff, 0x00020102 },
	{ ddrc_init1, 0x01ff7f0f, 0x00020000 },
	{ ddrc_init2, 0x0000ff0f, 0x00002205 },
	{ ddrc_init3, 0xffffffff, 0x07300301 },
	{ ddrc_init4, 0xffffffff, 0x00200200 },
	{ ddrc_init5, 0x00ff03ff, 0x00210004 },
	{ ddrc_init6, 0xffffffff, 0x000006c0 },
	{ ddrc_init7, 0xffff0000, 0x08190000 },
	{ ddrc_dimmctl, 0x0000003f, 0x00000010 },
	{ ddrc_rankctl, 0x00000fff, 0x0000066f },
	{ ddrc_dramtmg0, 0x7f3f7f3f, 0x11102311 },
	{ ddrc_dramtmg1, 0x001f1f7f, 0x00040419 },
	{ ddrc_dramtmg2, 0x3f3f3f3f, 0x0708060d },
	{ ddrc_dramtmg3, 0x3ff3f3ff, 0x0050400c },
	{ ddrc_dramtmg4, 0x1f0f0f1f, 0x08030409 },
	{ ddrc_dramtmg5, 0x0f0f3f1f, 0x06060403 },
	{ ddrc_dramtmg6, 0x0f0f000f, 0x01010004 },
	{ ddrc_dramtmg7, 0x00000f0f, 0x00000606 },
	{ ddrc_dramtmg8, 0x7f7f7f7f, 0x03030d06 },
	{ ddrc_dramtmg9, 0x40070f3f, 0x0002030b },
	{ ddrc_dramtmg11, 0x7f1f031f, 0x1107010e },
	{ ddrc_dramtmg12, 0x00030f1f, 0x00020608 },
	{ ddrc_zqctl0, 0xf7ff03ff, 0x81000040 },
	{ ddrc_zqctl1, 0x3fffffff, 0x0201908b },
	{ ddrc_dfitmg0, 0x1fbfbf3f, 0x048b820b },
	{ ddrc_dfitmg1, 0xf31f0f0f, 0x00030304 },
	{ ddrc_dfilpcfg0, 0x0ff1f1f1, 0x07000101 },
	{ ddrc_dfilpcfg1, 0x000000f1, 0x00000021 },
	{ ddrc_dfiupd0, 0xc3ff03ff, 0x00400003 },
	{ ddrc_dfiupd1, 0x00ff00ff, 0x00c800ff },
	{ ddrc_dfimisc, 0x00000007, 0x00000000 },
	{ ddrc_dfitmg2, 0x00003f3f, 0x00000909 },
	{ ddrc_dbictl, 0x00000007, 0x00000001 },
	{ ddrc_addrmap0, 0x0000001f, 0x0000001f },
	{ ddrc_addrmap1, 0x001f1f1f, 0x001f0909 },
	{ ddrc_addrmap2, 0x0f0f0f0f, 0x01010100 },
	{ ddrc_addrmap3, 0x0f0f0f0f, 0x01010101 },
	{ ddrc_addrmap4, 0x00000f0f, 0x00000f0f },
	{ ddrc_addrmap5, 0x0f0f0f0f, 0x070f0707 },
	{ ddrc_addrmap6, 0x8f0f0f0f, 0x0f070707 },
	{ ddrc_addrmap7, 0x00000f0f, 0x00000f0f },
	{ ddrc_addrmap8, 0x00001f1f, 0x00001f01 },
	{ ddrc_addrmap9, 0x0f0f0f0f, 0x07070707 },
	{ ddrc_addrmap10, 0x0f0f0f0f, 0x07070707 },
	{ ddrc_addrmap11, 0x0000000f, 0x00000007 },
	{ ddrc_odtcfg, 0x0f1f0f7c, 0x06000600 },
	{ ddrc_odtmap, 0x00003333, 0x00000001 },
	{ ddrc_sched, 0x7fff3f07, 0x01002001 },
	{ ddrc_perflpr1, 0xff00ffff, 0x08000040 },
	{ ddrc_perfwr1, 0xff00ffff, 0xffff3dc9 },
	{ ddrc_dqmap0, 0xffffffff, 0x00000000 },
	{ ddrc_dqmap1, 0xffffffff, 0x00000000 },
	{ ddrc_dqmap2, 0xffffffff, 0x00000000 },
	{ ddrc_dqmap3, 0xffffffff, 0x00000000 },
	{ ddrc_dqmap4, 0x0000ffff, 0x00000000 },
	{ ddrc_dqmap5, 0x00000001, 0x00000001 },
	{ ddrc_dbg0, 0x00000011, 0x00000000 },
	{ ddrc_dbgcmd, 0x80000033, 0x00000000 },
	{ ddrc_swctl, 0x00000001, 0x00000000 },
	{ ddrc_pccfg, 0x00000111, 0x00000001 },
	{ ddrc_pcfgr_0, 0x000073ff, 0x0000200f },
	{ ddrc_pcfgw_0, 0x000073ff, 0x0000200f },
	{ ddrc_pctrl_0, 0x00000001, 0x00000001 },
	{ ddrc_pcfgqos0_0, 0x0033000f, 0x0020000b },
	{ ddrc_pcfgqos1_0, 0x07ff07ff, 0x00000000 },
	{ ddrc_pcfgr_1, 0x000073ff, 0x0000200f },
	{ ddrc_pcfgw_1, 0x000073ff, 0x0000200f },
	{ ddrc_pctrl_1, 0x00000001, 0x00000001 },
	{ ddrc_pcfgqos0_1, 0x03330f0f, 0x02000b03 },
	{ ddrc_pcfgqos1_1, 0x07ff07ff, 0x00000000 },
	{ ddrc_pcfgr_2, 0x000073ff, 0x0000200f },
	{ ddrc_pcfgw_2, 0x000073ff, 0x0000200f },
	{ ddrc_pctrl_2, 0x00000001, 0x00000001 },
	{ ddrc_pcfgqos0_2, 0x03330f0f, 0x02000b03 },
	{ ddrc_pcfgqos1_2, 0x07ff07ff, 0x00000000 },
	{ ddrc_pcfgr_3, 0x000073ff, 0x0000200f },
	{ ddrc_pcfgw_3, 0x000073ff, 0x0000200f },
	{ ddrc_pctrl_3, 0x00000001, 0x00000001 },
	{ ddrc_pcfgqos0_3, 0x0033000f, 0x00100003 },
	{ ddrc_pcfgqos1_3, 0x07ff07ff, 0x0000004f },
	{ ddrc_pcfgwqos0_3, 0x0033000f, 0x00100003 },
	{ ddrc_pcfgwqos1_3, 0x000007ff, 0x0000004f },
	{ ddrc_pcfgr_4, 0x000073ff, 0x0000200f },
	{ ddrc_pcfgw_4, 0x000073ff, 0x0000200f },
	{ ddrc_pctrl_4, 0x00000001, 0x00000001 },
	{ ddrc_pcfgqos0_4, 0x0033000f, 0x00100003 },
	{ ddrc_pcfgqos1_4, 0x07ff07ff, 0x0000004f },
	{ ddrc_pcfgwqos0_4, 0x0033000f, 0x00100003 },
	{ ddrc_pcfgwqos1_4, 0x000007ff, 0x0000004f },
	{ ddrc_pcfgr_5, 0x000073ff, 0x0000200f },
	{ ddrc_pcfgw_5, 0x000073ff, 0x0000200f },
	{ ddrc_pctrl_5, 0x00000001, 0x00000001 },
	{ ddrc_pcfgqos0_5, 0x0033000f, 0x00100003 },
	{ ddrc_pcfgqos1_5, 0x07ff07ff, 0x0000004f },
	{ ddrc_pcfgwqos0_5, 0x0033000f, 0x00100003 },
	{ ddrc_pcfgwqos1_5, 0x000007ff, 0x0000004f },
	{ ddrc_sarbase0, 0x000001ff, 0x00000000 },
	{ ddrc_sarsize0, 0x000000ff, 0x00000000 },
	{ ddrc_sarbase1, 0x000001ff, 0x00000010 },
	{ ddrc_sarsize1, 0x000000ff, 0x0000000f },
	{ ddrc_dfitmg0_shadow, 0x1fbfbf3f, 0x07828002 },
};


static struct {
	unsigned offset;
	u32 value;
} ddrPhyInitValues[] = {
	{ ddr_phy_pgcr0, 0x07001e00 },
	{ ddr_phy_pgcr2, 0x00f0fc08 },
	{ ddr_phy_pgcr3, 0x55aa5480 },
	{ ddr_phy_pgcr5, 0x010100f4 },
	{ ddr_phy_ptr0, 0x41a20d10 },
	{ ddr_phy_ptr1, 0xcd141275 },
	{ ddr_phy_pllcr0, 0x01100000 },
	{ ddr_phy_dsgcr, 0x02a04161 },
	{ ddr_phy_gpr0, 0x00000000 },
	{ ddr_phy_gpr1, 0x000000de },
	{ ddr_phy_dcr, 0x0800040c },
	{ ddr_phy_dtpr0, 0x07220f08 },
	{ ddr_phy_dtpr1, 0x28200008 },
	{ ddr_phy_dtpr2, 0x000f0300 },
	{ ddr_phy_dtpr3, 0x83000800 },
	{ ddr_phy_dtpr4, 0x01112b07 },
	{ ddr_phy_dtpr5, 0x00320f08 },
	{ ddr_phy_dtpr6, 0x00000e0f },
	{ ddr_phy_rdimmgcr0, 0x08400020 },
	{ ddr_phy_rdimmgcr1, 0x00000c80 },
	{ ddr_phy_rdimmcr0, 0x00000000 },
	{ ddr_phy_rdimmcr1, 0x00000200 },
	{ ddr_phy_mr0, 0x00000630 },
	{ ddr_phy_mr1, 0x00000301 },
	{ ddr_phy_mr2, 0x00000020 },
	{ ddr_phy_mr3, 0x00000200 },
	{ ddr_phy_mr4, 0x00000000 },
	{ ddr_phy_mr5, 0x000006c0 },
	{ ddr_phy_mr6, 0x00000819 },
	{ ddr_phy_mr11, 0x00000000 },
	{ ddr_phy_mr12, 0x0000004d },
	{ ddr_phy_mr13, 0x00000008 },
	{ ddr_phy_mr14, 0x0000004d },
	{ ddr_phy_mr22, 0x00000000 },
	{ ddr_phy_dtcr0, 0x800091c7 },
	{ ddr_phy_dtcr1, 0x00010236 },
	{ ddr_phy_catr0, 0x00141054 },
	{ ddr_phy_dqsdr0, 0x00088000 },
	{ ddr_phy_bistlsr, 0x12340800 },
	{ ddr_phy_riocr5, 0x00000005 },
	{ ddr_phy_aciocr0, 0x30000028 },
	{ ddr_phy_aciocr2, 0x0a000000 },
	{ ddr_phy_aciocr3, 0x00000009 },
	{ ddr_phy_aciocr4, 0x0a000000 },
	{ ddr_phy_iovcr0, 0x0300b0ce },
	{ ddr_phy_vtcr0, 0xf9032019 },
	{ ddr_phy_vtcr1, 0x07f001e3 },
	{ ddr_phy_acbdlr1, 0x00000000 },
	{ ddr_phy_acbdlr2, 0x00000000 },
	{ ddr_phy_acbdlr6, 0x00000000 },
	{ ddr_phy_acbdlr7, 0x00000000 },
	{ ddr_phy_acbdlr8, 0x00000000 },
	{ ddr_phy_acbdlr9, 0x00000000 },
	{ ddr_phy_zqcr, 0x008aaa58 },
	{ ddr_phy_zq0pr0, 0x000079dd },
	{ ddr_phy_zq0or0, 0x01e10210 },
	{ ddr_phy_zq0or1, 0x01e10000 },
	{ ddr_phy_zq1pr0, 0x00087bdb },
	{ ddr_phy_dx0gcr0, 0x40800604 },
	{ ddr_phy_dx0gcr1, 0x00007fff },
	{ ddr_phy_dx0gcr3, 0x3f000008 },
	{ ddr_phy_dx0gcr4, 0x0e00b03c },
	{ ddr_phy_dx0gcr5, 0x09094f4f },
	{ ddr_phy_dx0gcr6, 0x09092b2b },
	{ ddr_phy_dx1gcr0, 0x40800604 },
	{ ddr_phy_dx1gcr1, 0x00007fff },
	{ ddr_phy_dx1gcr3, 0x3f000008 },
	{ ddr_phy_dx1gcr4, 0x0e00b03c },
	{ ddr_phy_dx1gcr5, 0x09094f4f },
	{ ddr_phy_dx1gcr6, 0x09092b2b },
	{ ddr_phy_dx2gcr0, 0x40800604 },
	{ ddr_phy_dx2gcr1, 0x00007fff },
	{ ddr_phy_dx2gcr3, 0x3f000008 },
	{ ddr_phy_dx2gcr4, 0x0e00b03c },
	{ ddr_phy_dx2gcr5, 0x09094f4f },
	{ ddr_phy_dx2gcr6, 0x09092b2b },
	{ ddr_phy_dx3gcr0, 0x40800604 },
	{ ddr_phy_dx3gcr1, 0x00007fff },
	{ ddr_phy_dx3gcr3, 0x3f000008 },
	{ ddr_phy_dx3gcr4, 0x0e00b03c },
	{ ddr_phy_dx3gcr5, 0x09094f4f },
	{ ddr_phy_dx3gcr6, 0x09092b2b },
	{ ddr_phy_dx4gcr0, 0x40800604 },
	{ ddr_phy_dx4gcr1, 0x00007fff },
	{ ddr_phy_dx4gcr2, 0x00000000 },
	{ ddr_phy_dx4gcr3, 0x3f000008 },
	{ ddr_phy_dx4gcr4, 0x0e00b004 },
	{ ddr_phy_dx4gcr5, 0x09094f4f },
	{ ddr_phy_dx4gcr6, 0x09092b2b },
	{ ddr_phy_dx5gcr0, 0x40800604 },
	{ ddr_phy_dx5gcr1, 0x00007fff },
	{ ddr_phy_dx5gcr2, 0x00000000 },
	{ ddr_phy_dx5gcr3, 0x3f000008 },
	{ ddr_phy_dx5gcr4, 0x0e00b03c },
	{ ddr_phy_dx5gcr5, 0x09094f4f },
	{ ddr_phy_dx5gcr6, 0x09092b2b },
	{ ddr_phy_dx6gcr0, 0x40800604 },
	{ ddr_phy_dx6gcr1, 0x00007fff },
	{ ddr_phy_dx6gcr2, 0x00000000 },
	{ ddr_phy_dx6gcr3, 0x3f000008 },
	{ ddr_phy_dx6gcr4, 0x0e00b004 },
	{ ddr_phy_dx6gcr5, 0x09094f4f },
	{ ddr_phy_dx6gcr6, 0x09092b2b },
	{ ddr_phy_dx7gcr0, 0x40800604 },
	{ ddr_phy_dx7gcr1, 0x00007fff },
	{ ddr_phy_dx7gcr2, 0x00000000 },
	{ ddr_phy_dx7gcr3, 0x3f000008 },
	{ ddr_phy_dx7gcr4, 0x0e00b03c },
	{ ddr_phy_dx7gcr5, 0x09094f4f },
	{ ddr_phy_dx7gcr6, 0x09092b2b },
	{ ddr_phy_dx8gcr0, 0x80803660 },
	{ ddr_phy_dx8gcr1, 0x55556000 },
	{ ddr_phy_dx8gcr2, 0xaaaaaaaa },
	{ ddr_phy_dx8gcr3, 0x0029a4a4 },
	{ ddr_phy_dx8gcr4, 0x0c00b000 },
	{ ddr_phy_dx8gcr5, 0x09094f4f },
	{ ddr_phy_dx8gcr6, 0x09092b2b },
	{ ddr_phy_dx8sl0osc, 0x2a019ffe },
	{ ddr_phy_dx8sl0pllcr0, 0x01100000 },
	{ ddr_phy_dx8sl0dqsctl, 0x01264300 },
	{ ddr_phy_dx8sl0dxctl2, 0x00041800 },
	{ ddr_phy_dx8sl0iocr, 0x70800000 },
	{ ddr_phy_dx8sl1osc, 0x2a019ffe },
	{ ddr_phy_dx8sl1pllcr0, 0x01100000 },
	{ ddr_phy_dx8sl1dqsctl, 0x01264300 },
	{ ddr_phy_dx8sl1dxctl2, 0x00041800 },
	{ ddr_phy_dx8sl1iocr, 0x70800000 },
	{ ddr_phy_dx8sl2osc, 0x2a019ffe },
	{ ddr_phy_dx8sl2pllcr0, 0x01100000 },
	{ ddr_phy_dx8sl2dqsctl, 0x01264300 },
	{ ddr_phy_dx8sl2dxctl2, 0x00041800 },
	{ ddr_phy_dx8sl2iocr, 0x70800000 },
	{ ddr_phy_dx8sl3osc, 0x2a019ffe },
	{ ddr_phy_dx8sl3pllcr0, 0x01100000 },
	{ ddr_phy_dx8sl3dqsctl, 0x01264300 },
	{ ddr_phy_dx8sl3dxctl2, 0x00041800 },
	{ ddr_phy_dx8sl3iocr, 0x70800000 },
	{ ddr_phy_dx8sl4osc, 0x15019ffe },
	{ ddr_phy_dx8sl4pllcr0, 0x21100000 },
	{ ddr_phy_dx8sl4dqsctl, 0x01266300 },
	{ ddr_phy_dx8sl4dxctl2, 0x00041800 },
	{ ddr_phy_dx8sl4iocr, 0x70400000 },
	{ ddr_phy_dx8slbdqsctl, 0x012643c4 },
};


static int wait_with_timeout(volatile u32 *ptr, u32 mask, u32 expected)
{
	/* Use cycle counter built into the CPU - we don't have TTC running yet.
	 * The timeout is 600M cycles (~1 s in default configuration)
	 * it should never be hit unless something is catastrophically wrong. */
	u64 start = sysreg_read(pmccntr_el0);
	while ((*ptr & mask) != expected) {
		if ((sysreg_read(pmccntr_el0) - start) > (600uL * 1000 * 1000)) {
			return 0;
		}
	}

	return 1;
}


int _zynqmp_ddrInit(void)
{
	int i;
	u32 val, initial_tREFPRD;
	u32 pll_retry = 10, pll_locked = 0;

	ddrInit_common.ddrc = (void *)DDRC_BASE_ADDRESS;
	ddrInit_common.ddr_phy = (void *)DDR_PHY_BASE_ADDRESS;

	_zynqmp_devReset(ctl_reset_fpd_ddr_block, 1);
	for (i = 0; i < (sizeof(ddrcInitValues) / sizeof(ddrcInitValues[0])); i++) {
		val = *(ddrInit_common.ddrc + ddrcInitValues[i].offset) & ddrcInitValues[i].mask;
		*(ddrInit_common.ddrc + ddrcInitValues[i].offset) = val | ddrcInitValues[i].value;
	}

	_zynqmp_devReset(ctl_reset_fpd_ddr_block, 0);
	_zynqmp_devReset(ctl_reset_fpd_ddr_apm, 0);
	for (i = 0; i < (sizeof(ddrPhyInitValues) / sizeof(ddrPhyInitValues[0])); i++) {
		*(ddrInit_common.ddr_phy + ddrPhyInitValues[i].offset) = ddrPhyInitValues[i].value;
	}

	while ((pll_retry > 0) && (pll_locked == 0)) {
		*(ddrInit_common.ddr_phy + ddr_phy_pir) = 0x00040010;
		*(ddrInit_common.ddr_phy + ddr_phy_pir) = 0x00040011;

		if (wait_with_timeout(ddrInit_common.ddr_phy + ddr_phy_pgsr0, 0x1, 0x1) == 0) {
			return -1;
		}

		pll_locked = (*(ddrInit_common.ddr_phy + ddr_phy_pgsr0) >> 31) & 1;
		pll_locked &= (*(ddrInit_common.ddr_phy + ddr_phy_dx0gsr0) >> 16) & 1;
		pll_locked &= (*(ddrInit_common.ddr_phy + ddr_phy_dx2gsr0) >> 16) & 1;
		pll_locked &= (*(ddrInit_common.ddr_phy + ddr_phy_dx4gsr0) >> 16) & 1;
		pll_locked &= (*(ddrInit_common.ddr_phy + ddr_phy_dx6gsr0) >> 16) & 1;
		pll_retry--;
	}

	*(ddrInit_common.ddr_phy + ddr_phy_gpr1) = *(ddrInit_common.ddr_phy + ddr_phy_gpr1) | (pll_retry << 16);
	if (pll_locked == 0) {
		return -2;
	}

	*(ddrInit_common.ddr_phy + ddr_phy_pir) = 0x00040063;

	/* PHY BRINGUP SEQ */
	if (wait_with_timeout(ddrInit_common.ddr_phy + ddr_phy_pgsr0, 0x0000000f, 0x0000000f) == 0) {
		return -3;
	}
	*(ddrInit_common.ddr_phy + ddr_phy_pir) |= 0x00000001;

	/* poll for PHY initialization to complete */
	if (wait_with_timeout(ddrInit_common.ddr_phy + ddr_phy_pgsr0, 0x000000ff, 0x0000001f) == 0) {
		return -4;
	}

	*(ddrInit_common.ddrc + ddrc_dfimisc) = 0x00000001;
	*(ddrInit_common.ddrc + ddrc_swctl) = 0x00000001;
	if (wait_with_timeout(ddrInit_common.ddrc + ddrc_stat, 0x0000000f, 0x00000001) == 0) {
		return -5;
	}

	*(ddrInit_common.ddr_phy + ddr_phy_pgcr1) |= 0x00000040;
	*(ddrInit_common.ddr_phy + ddr_phy_pir) = 0x0004fe01;

	do {
		val = *(ddrInit_common.ddr_phy + ddr_phy_pgsr0);
		if ((val & 0xffc0000) != 0) {
			return -6;
		}
	} while (val != 0x80000fff);


	/* Run Vref training in static read mode*/
	*(ddrInit_common.ddr_phy + ddr_phy_dtcr0) = 0x100091c7;

	initial_tREFPRD = *(ddrInit_common.ddr_phy + ddr_phy_pgcr2) & 0x0003ffff;
	*(ddrInit_common.ddr_phy + ddr_phy_pgcr2) = (*(ddrInit_common.ddr_phy + ddr_phy_pgcr2) & 0x3ffff) | initial_tREFPRD;

	*(ddrInit_common.ddr_phy + ddr_phy_pgcr3) |= 0x00000018;
	*(ddrInit_common.ddr_phy + ddr_phy_dx8sl0dxctl2) |= 0x00000030;
	*(ddrInit_common.ddr_phy + ddr_phy_dx8sl1dxctl2) |= 0x00000030;
	*(ddrInit_common.ddr_phy + ddr_phy_dx8sl2dxctl2) |= 0x00000030;
	*(ddrInit_common.ddr_phy + ddr_phy_dx8sl3dxctl2) |= 0x00000030;
	*(ddrInit_common.ddr_phy + ddr_phy_dx8sl4dxctl2) |= 0x00000030;

	*(ddrInit_common.ddr_phy + ddr_phy_pir) = 0x00060001;
	do {
		val = *(ddrInit_common.ddr_phy + ddr_phy_pgsr0);
		if ((val & 0xffc0000) != 0) {
			return -7;
		}
	} while ((val & 0x80004001) != 0x80004001);

	/* Vref training is complete */
	*(ddrInit_common.ddr_phy + ddr_phy_pgcr3) &= ~0x00000018;
	*(ddrInit_common.ddr_phy + ddr_phy_dx8sl0dxctl2) &= ~0x00000030;
	*(ddrInit_common.ddr_phy + ddr_phy_dx8sl1dxctl2) &= ~0x00000030;
	*(ddrInit_common.ddr_phy + ddr_phy_dx8sl2dxctl2) &= ~0x00000030;
	*(ddrInit_common.ddr_phy + ddr_phy_dx8sl3dxctl2) &= ~0x00000030;
	*(ddrInit_common.ddr_phy + ddr_phy_dx8sl4dxctl2) &= ~0x00000030;
	/* Vref training is complete, disabling static read mode */
	*(ddrInit_common.ddr_phy + ddr_phy_dtcr0) = 0x800091c7;
	*(ddrInit_common.ddr_phy + ddr_phy_pgcr2) = (*(ddrInit_common.ddr_phy + ddr_phy_pgcr2) & 0x3ffff) | initial_tREFPRD;

	*(ddrInit_common.ddr_phy + ddr_phy_pir) = 0x0000c001;
	do {
		val = *(ddrInit_common.ddr_phy + ddr_phy_pgsr0);
		if ((val & 0xffc0000) != 0) {
			return -8;
		}
	} while ((val & 0x80000c01) != 0x80000c01);

	*(ddrInit_common.ddrc + ddrc_zqctl0) = 0x01000040;
	*(ddrInit_common.ddrc + ddrc_rfshctl3) = 0x00000000;
	*(ddrInit_common.ddr_phy + ddr_phy_pgcr1) &= ~0x00000040;
	return 0;
}
