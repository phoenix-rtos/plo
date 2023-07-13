/*
 * Phoenix-RTOS
 *
 * Operating system kernel
 *
 * Low-level initialization for iMX6ULL processor
 *
 * Copyright 2018, 2021 Phoenix Systems
 * Author: Jan Sikorski, Aleksander Kaminski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "imx6ull.h"
#include "../cpu.h"


/* clang-format off */
/* CCM registers */
enum { ccm_ccr = 0, ccm_ccdr, ccm_csr, ccm_ccsr, ccm_cacrr, ccm_cbcdr, ccm_cbcmr,
	ccm_cscmr1, ccm_cscmr2, ccm_cscdr1, ccm_cs1cdr, ccm_cs2cdr, ccm_cdcdr, ccm_chsccdr,
	ccm_cscdr2, ccm_cscdr3, ccm_cdhipr = ccm_cscdr3 + 3, ccm_clpcr = ccm_cdhipr + 3,
	ccm_cisr, ccm_cimr, ccm_ccosr, ccm_cgpr, ccm_ccgr0, ccm_ccgr1, ccm_ccgr2, ccm_ccgr3,
	ccm_ccgr4, ccm_ccgr5, ccm_ccgr6, ccm_cmeor = ccm_ccgr6 + 2 };


enum { ccm_analog_pll_arm = 0, ccm_analog_pll_arm_set, ccm_analog_pll_arm_clr, ccm_analog_pll_arm_tog, ccm_analog_pll_usb1,
	ccm_analog_pll_usb1_set, ccm_analog_pll_usb1_clr, ccm_analog_pll_usb1_tog, ccm_analog_pll_usb2, ccm_analog_pll_usb2_set,
	ccm_analog_pll_usb2_clr, ccm_analog_pll_usb2_tog, ccm_analog_pll_sys, ccm_analog_pll_sys_set, ccm_analog_pll_sys_clr,
	ccm_analog_pll_sys_tog, ccm_analog_pll_sys_ss, ccm_analog_pll_sys_num = ccm_analog_pll_sys_ss + 4,
	ccm_analog_pll_sys_denom = ccm_analog_pll_sys_num + 4, ccm_analog_pll_audio = ccm_analog_pll_sys_denom + 4,
	ccm_analog_pll_audio_set, ccm_analog_pll_audio_clr, ccm_analog_pll_audio_tog, ccm_analog_pll_audio_num,
	ccm_analog_pll_audio_denom = ccm_analog_pll_audio_num + 4, ccm_analog_pll_video = ccm_analog_pll_audio_denom + 4,
	ccm_analog_pll_video_set, ccm_analog_pll_video_clr, ccm_analog_pll_video_tog, ccm_analog_pll_video_num,
	ccm_analog_pll_video_denom = ccm_analog_pll_video_num + 4, ccm_analog_pll_enet = ccm_analog_pll_video_denom + 4,
	ccm_analog_pll_enet_set, ccm_analog_pll_enet_clr, ccm_analog_pll_enet_tog, ccm_analog_pfd_480, ccm_analog_pfd_480_set,
	ccm_analog_pfd_480_clr, ccm_analog_pfd_480_tog, ccm_analog_pfd_528, ccm_analog_pfd_528_set, ccm_analog_pfd_528_clr,
	ccm_analog_pfd_528_tog,
	ccm_analog_misc0 = 84, ccm_analog_misc0_set, ccm_analog_misc0_clr, ccm_analog_misc0_tog, ccm_analog_misc1,
	ccm_analog_misc1_set, ccm_analog_misc1_clr, ccm_analog_misc1_tog, ccm_analog_misc2, ccm_analog_misc2_set,
	ccm_analog_misc2_clr, ccm_analog_misc2_tog };


/* IOMUX - GPR */
enum {
	/* IOMUXC_GPR_GPR0 */
	gpr_dmareq0 = 0, gpr_dmareq1, gpr_dmareq2, gpr_dmareq3, gpr_dmareq4, gpr_dmareq5,
	gpr_dmareq6, gpr_dmareq7, gpr_dmareq8, gpr_dmareq9, gpr_dmareq10, gpr_dmareq11,
	gpr_dmareq12, gpr_dmareq13, gpr_dmareq14, gpr_dmareq15, gpr_dmareq16, gpr_dmareq17,
	gpr_dmareq18, gpr_dmareq19, gpr_dmareq20, gpr_dmareq21, gpr_dmareq22,

	/* IOMUXC_GPR_GPR1 */
	gpr_act_cs0 = 32, gpr_addrs0, gpr_act_cs1 = 35, gpr_addrs1, gpr_act_cs2 = 38,
	gpr_addrs2, gpr_act_cs3 = 41, gpr_addrs3, gpr_gint = 44, gpr_enet1_clk,
	gpr_enet2_clk, gpr_usb_exp, gpr_add_ds, gpr_enet1_tx, gpr_enet2_tx, gpr_sai1_mclk,
	gpr_sai2_mclk, gpr_sai3_mclk, gpr_exc_mon, gpr_tzasc1, gpr_arma7_atb, gpr_armv7_ahb,

	/* IOMUXC_GPR_GPR2 */
	gpr_pxp_powersaving = 64, gpr_pxp_shutdown, gpr_pxp_deepsleep, gpr_pxp_lightsleep,
	gpr_lcdif1_powersaving, gpr_lcdif1_shutdown, gpr_lcdif1_deepsleep, gpr_lcdif1_lightsleep,
	gpr_lcdif2_powersaving, gpr_lcdif2_shutdown, gpr_lcdif2_deepsleep, gpr_lcdif2_lightsleep,
	gpr_l2_powersaving, gpr_l2_shutdown, gpr_l2_deepsleep, gpr_l2_lightsleep, gpr_mqs_clk_div,
	gpr_mqs_sw_rst = 88, gpr_mqs_en, gpr_mqs_oversample, gpr_dram_rst_bypass, gpr_dram_rst,
	gpr_dram_cke0, gpr_dram_cke1, gpr_dram_cke_bypass,

	/* IOMUXC_GPR_GPR3 */
	gpr_ocram_ctl, gpr_core_dbg = 109, gpr_ocram_status = 112,

	/* IOMUXC_GPR_GPR4 */
	gpr_sdma_stop_req = 128, gpr_can1_stop_req, gpr_can2_stop_req, gpr_enet1_stop_req,
	gpr_enet2_stop_req, gpr_sai1_stop_req, gpr_sai2_stop_req, gpr_sai3_stop_req,
	gpr_enet_ipg, gpr_sdma_stop_ack = 144, gpr_can1_stop_ack, gpr_can2_stop_ack,
	gpr_enet1_stop_ack, gpr_enet2_stop_ack, gpr_sai1_stop_ack, gpr_sai2_stop_ack,
	gpr_sai3_stop_ack, gpr_arm_wfi = 158, gpr_arm_wfe,

	/* IOMUXC_GPR_GPR5 */
	gpr_wdog1 = 166, gpr_wdog2, gpr_wdog3 = 180, gpr_gpt2_capn1 = 183, gpr_gpt2_capn2,
	gpr_enet1_event3n, gpr_enet2_event3n, gpr_vref_gpt1 = 188, gpr_vref_gpt2,
	gpr_ref_epit1, gpr_ref_epit2,

	/* IOMUXC_GPR_GPR9 */
	gpr_tzasc1_byp = 288,

	/* IOMUXC_GPR_GPR10 */
	gpr_dbg_en = 320, gpr_dbg_clk_en, gpr_sec_err_resp, gpr_ocram_tz_en = 330,
	gpr_ocram_tz_addr,

	/* IOMUXC_GPR_GPR14 */
	gpr_sm1 = 448, gpr_sm2
};



/* IOMUX - GRP */
enum {
	grp_addds = 0x124, grp_ddrmode_ctl, grp_b0ds, grp_ddrpk, grp_ctlds, grp_b1ds,
	grp_ddrhys, grp_ddrpke, grp_ddrmode, grp_ddr_type
};


/* Multi Mode DDR Controller */
enum {
	mmdc_mdctl = 0, mmdc_mdpdc, mmdc_mdotc, mmdc_mdcfg0, mmdc_mdcfg1, mmdc_mdcfg2, mmdc_mdmisc, mmdc_mdscr,
	mmdc_mdref,
	mmdc_mdrwd = 0xb, mmdc_mdor, mmdc_mdmmr, mmdc_mdcfg3lp, mmdc_mdmr4, mmdc_mdasp,

	mmdc_maarcr = 0x100, mmdc_mapsr, mmdc_maexidr0, mmdc_maexidr1, mmdc_madpcr0, mmdc_madpcr1, mmdc_madpsr0,
	mmdc_madpsr1, mmdc_madpsr2, mmdc_madpsr3, mmdc_madpsr4, mmdc_madpsr5, mmdc_masbs0, mmdc_masbs1,
	mmdc_magenp = 0x110,

	mmdc_mpzqhwctrl = 0x200, mmdc_mpzqswctrl, mmdc_mpwlgcr, mmdc_mpwldectrl0, mmdc_mpwldectrl1, mmdc_mpwldlst,
	mmdc_mpodtctrl, mmdc_mprddqby0dl, mmdc_mprddqby1dl,
	mmdc_mpwrdqby0dl = 0x20b, mmdc_mpwrdqby1dl, mmdc_mpwrdqby2dl,
	mmdc_mpwrdqby3dl, mmdc_mpdgctrl0, mmdc_mpdgctrl1, mmdc_mpdgdlst0, mmdc_mprddlctl, mmdc_mprddlst, mmdc_mpwrdlctl,
	mmdc_mpwrdlst, mmdc_mpsdctrl, mmdc_mpzqlp2ctl, mmdc_mprddlhwctl, mmdc_mpwrdlhwctl, mmdc_mprddlhwst0,
	mmdc_mpwrdlhwst0 = 0x21c, mmdc_mpwlhwerr = 0x21e, mmdc_mpdghwst0, mmdc_mpdghwst1, mmdc_mpdghwst2, mmdc_mpdghwst3,
	mmdc_mppdcmpr1, mmdc_mppdcmpr2, mmdc_mpswdar0, mmdc_mpswdrdr0, mmdc_mpswdrdr1, mmdc_mpswdrdr2, mmdc_mpswdrdr3,
	mmdc_mpswdrdr4, mmdc_mpswdrdr5, mmdc_mpswdrdr6, mmdc_mpswdrdr7, mmdc_mpmur0, mmdc_mpwrcadl, mmdc_mpdccr
};


/* WDOG registers */
enum { wdog_wcr = 0, wdog_wsr, wdog_wrsr, wdog_wicr, wdog_wmcr };


/* Reserved slots */
static const char ccm_reserved[] = { clk_asrc + 1, clk_ipsync_ip2apb_tzasc1_ipg + 1, clk_pxp + 1,
	clk_mmdc_core_aclk_fast_core_p0 + 1, clk_iomux_snvs_gpr + 1, clk_usdhc2 + 1 };
/* clang-format on */


struct {
	volatile u32 *ccm;
	volatile u32 *ccm_analog;

	volatile u32 *iomux;
	volatile u32 *iomux_gpr;
	volatile u32 *iomux_snvs;

	volatile u32 *mmdc;

	volatile u16 *wdog;
} imx6ull_common;


void _imx6ull_softRst(void)
{
	/* assert SRS signal by writing 0 to bit 4 and 1 to bit 2 (WDOG enable) */
	*(imx6ull_common.wdog + wdog_wcr) = (1 << 2);
	hal_cpuDataMemoryBarrier();
	for (;;) {
	}
}


static int imx6ull_isValidDev(int dev)
{
	int i;

	if (dev < clk_aips_tz1 || dev > clk_pwm7)
		return 0;

	for (i = 0; i < sizeof(ccm_reserved) / sizeof(ccm_reserved[0]); ++i) {
		if (dev == ccm_reserved[i])
			return 0;
	}

	return 1;
}


int imx6ull_setDevClock(int dev, unsigned int state)
{
	int ccgr, flag, mask;
	u32 r;

	if (!imx6ull_isValidDev(dev))
		return -1;

	ccgr = dev / 16;
	flag = (state & 3) << (2 * (dev % 16));
	mask = 3 << (2 * (dev % 16));

	r = *(imx6ull_common.ccm + ccm_ccgr0 + ccgr);
	*(imx6ull_common.ccm + ccm_ccgr0 + ccgr) = (r & ~mask) | flag;

	return 0;
}


int imx6ull_setIOmux(int mux, char sion, char mode)
{
	volatile u32 *base = imx6ull_common.iomux;

	if (mux >= mux_boot_mode0 && mux <= mux_tamper9) {
		mux = (mux - mux_boot_mode0);
		base = imx6ull_common.iomux_snvs;
	}
	else if (mux < mux_jtag_mod || mux > mux_csi_d7)
		return -1;

	*(base + mux) = (!!sion << 4) | (mode & 0xf);

	return 0;
}


int imx6ull_setIOpad(int pad, char hys, char pus, char pue, char pke, char ode, char speed, char dse, char sre)
{
	u32 t;
	volatile u32 *base = imx6ull_common.iomux;

	if (pad >= pad_test_mode && pad <= pad_tamper9) {
		pad = pad - pad_test_mode + 12;
		base = imx6ull_common.iomux_gpr;
	}
	else if (pad < pad_jtag_mod || pad > pad_csi_d7)
		return -1;

	t = (!!hys << 16) | ((pus & 0x3) << 14) | (!!pue << 13) | (!!pke << 12);
	t |= (!!ode << 11) | ((speed & 0x3) << 6) | ((dse & 0x7) << 3) | !!sre;
	*(base + pad) = t;

	return 0;
}


int imx6ull_setIOisel(int isel, char daisy)
{
	if (isel < isel_anatop || isel > isel_usdhc2_wp)
		return -1;

	*(imx6ull_common.iomux + isel) = daisy & 0x7;

	return 0;
}


static void imx6ull_ddrInit(void)
{
	*(imx6ull_common.mmdc + mmdc_mdctl) |= 0x80000000;

	/* Enable DDR clock */
	*(imx6ull_common.ccm + ccm_ccgr3) |= 0x0f300000;

	/* Config IOMUX */
	*(imx6ull_common.iomux + grp_ddr_type) = 0x000c0000;
	*(imx6ull_common.iomux + grp_ddrpke) = 0x00000000;

	*(imx6ull_common.iomux + pad_dram_sdclk0p) = 0x00000030;
	*(imx6ull_common.iomux + pad_dram_cas_b) = 0x00000030;
	*(imx6ull_common.iomux + pad_dram_rasb) = 0x00000030;

	*(imx6ull_common.iomux + grp_addds) = 0x00000030;

	*(imx6ull_common.iomux + pad_dram_reset) = 0x00000030;
	*(imx6ull_common.iomux + pad_dram_sdba2) = 0x00000000;
	*(imx6ull_common.iomux + pad_dram_odt0) = 0x00000030;
	*(imx6ull_common.iomux + pad_dram_odt1) = 0x00000030;

	*(imx6ull_common.iomux + grp_ctlds) = 0x00000030;
	*(imx6ull_common.iomux + grp_ddrmode_ctl) = 0x00020000;

	*(imx6ull_common.iomux + pad_dram_sdqs0p) = 0x00000030;
	*(imx6ull_common.iomux + pad_dram_sdqs1p) = 0x00000030;

	*(imx6ull_common.iomux + grp_ddrmode) = 0x00020000;
	*(imx6ull_common.iomux + grp_b0ds) = 0x00000030;
	*(imx6ull_common.iomux + grp_b1ds) = 0x00000030;

	*(imx6ull_common.iomux + pad_dram_dqm0) = 0x00000030;
	*(imx6ull_common.iomux + pad_dram_dqm1) = 0x00000030;

	/* Config DDR control register */
	*(imx6ull_common.mmdc + mmdc_mdscr) = 0x00008000;
	*(imx6ull_common.mmdc + mmdc_mpzqhwctrl) = 0xa1390003;
	*(imx6ull_common.mmdc + mmdc_mpwldectrl0) = 0x00150019;
	*(imx6ull_common.mmdc + mmdc_mpdgctrl0) = 0x41550153;
	*(imx6ull_common.mmdc + mmdc_mprddlctl) = 0x40403a3e;
	*(imx6ull_common.mmdc + mmdc_mpwrdlctl) = 0x40402f2a;

	*(imx6ull_common.mmdc + mmdc_mprddqby0dl) = 0x33333333;
	*(imx6ull_common.mmdc + mmdc_mprddqby1dl) = 0x33333333;

	*(imx6ull_common.mmdc + mmdc_mpwrdqby0dl) = 0xf3333333;
	*(imx6ull_common.mmdc + mmdc_mpwrdqby1dl) = 0xf3333333;

	*(imx6ull_common.mmdc + mmdc_mpdccr) = 0x00944009;
	*(imx6ull_common.mmdc + mmdc_mpmur0) = 0x00000800;

	*(imx6ull_common.mmdc + mmdc_mdpdc) = 0x0002002d;
	*(imx6ull_common.mmdc + mmdc_mdotc) = 0x1b333030;
	*(imx6ull_common.mmdc + mmdc_mdcfg0) = 0x676b52f3;
	*(imx6ull_common.mmdc + mmdc_mdcfg1) = 0xb66d0b63;
	*(imx6ull_common.mmdc + mmdc_mdcfg2) = 0x01ff00db;
	*(imx6ull_common.mmdc + mmdc_mdmisc) = 0x00201740;
	*(imx6ull_common.mmdc + mmdc_mdscr) = 0x00008000;
	*(imx6ull_common.mmdc + mmdc_mdrwd) = 0x000026d2;
	*(imx6ull_common.mmdc + mmdc_mdor) = 0x006b1023;
	*(imx6ull_common.mmdc + mmdc_mdasp) = 0x00000047;

	*(imx6ull_common.mmdc + mmdc_mdctl) = 0x82180000;
	*(imx6ull_common.mmdc + mmdc_mppdcmpr2) = 0x00400000;

	*(imx6ull_common.mmdc + mmdc_mdscr) = 0x2008032;
	*(imx6ull_common.mmdc + mmdc_mdscr) = 0x8033;
	*(imx6ull_common.mmdc + mmdc_mdscr) = 0x48031;
	*(imx6ull_common.mmdc + mmdc_mdscr) = 0x15208030;
	*(imx6ull_common.mmdc + mmdc_mdscr) = 0x4008040;

	*(imx6ull_common.mmdc + mmdc_mdref) = 0x800;

	*(imx6ull_common.mmdc + mmdc_mpodtctrl) = 0x227;
	*(imx6ull_common.mmdc + mmdc_mdpdc) = 0x2552d;
	*(imx6ull_common.mmdc + mmdc_mapsr) = 0x11006;
	*(imx6ull_common.mmdc + mmdc_mdscr) = 0x0;
}


static void imx6ull_pllInit(void)
{
/* This code bases on initialization PLLs and clocks from phoenix-rtos-kernel (imx6ull/_init.S).
 * It occurred that it doesn't work as expected. PLL should be locked after initialization but it doesn't.
 * PLO will used clock and PLL configuration set by bootROM.
 * NOTE: TODO - PLL and clocks configuration should be checked! */
#if 0

	/* Enable USB2 PLL - 480 MHz */
	*(imx6ull_common.ccm_analog + ccm_analog_pll_usb2) |= 0x3000;

	/* Wait until USB2 PLL is locked */
	while ((*(imx6ull_common.ccm_analog + ccm_analog_pll_usb2) & (1 << 31)))
		;

	/* Clear PLL bypass */
	*(imx6ull_common.ccm_analog + ccm_analog_pll_usb2) &= ~(1 << 16);
	/* Enable USB clock */
	*(imx6ull_common.ccm_analog + ccm_analog_pll_usb2) |= (1 << 6);


	/* Enable ENET PLL (both 50 MHz) */
	*(imx6ull_common.ccm_analog + ccm_analog_pll_enet) = 0x102005;

	/* Wait until ENET PLL is unlocked */
	while ((*(imx6ull_common.ccm_analog + ccm_analog_pll_enet) & (1 << 31)))
		;
#endif
}


extern void imx6ull_setQSPIClockSource(unsigned char source, unsigned char divider)
{
	u32 reg;

	reg = *(imx6ull_common.ccm + ccm_cscmr1);

	/* Set divider. */
	reg &= ~(7u << 26);
	reg |= ((divider - 1u) & 0x7u) << 26;

	/* Set source. */
	reg &= ~(7u << 7);
	reg |= (source & 0x7u) << 7;

	*(imx6ull_common.ccm + ccm_cscmr1) = reg;
}


void imx6ull_init(void)
{
	u32 reg, tmp;

	imx6ull_common.ccm = (void *)0x020c4000;
	imx6ull_common.ccm_analog = (void *)0x020c8000;
	imx6ull_common.mmdc = (void *)0x021b0000;

	imx6ull_common.iomux = (void *)0x020e0000;
	imx6ull_common.iomux_gpr = (void *)0x020e4000;
	imx6ull_common.iomux_snvs = (void *)0x02290000;

	imx6ull_common.wdog = (void *)0x020bc000;

	imx6ull_pllInit();

	/* Set ARM clock to 792 MHz; set ARM clock divider to 1 */
	*(imx6ull_common.ccm + ccm_cacrr) = 0;
	hal_cpuDataSyncBarrier();

	/* Enable EPITs clocks */
	*(imx6ull_common.ccm + ccm_ccgr1) |= 0x00005000;

	/* Set ENFC clock to 198 MHz */
	/* First disable all output clocks */
	reg = *(imx6ull_common.ccm + ccm_ccgr4);
	tmp = reg;
	reg &= ~((3u << 30) | (3u << 28) | (3u << 26) | (3u << 24) | (3u << 12));
	*(imx6ull_common.ccm + ccm_ccgr4) = reg;

	/* Configure ENFC clock */
	reg = *(imx6ull_common.ccm + ccm_cs2cdr);
	reg &= ~((63u << 21) | (7u << 18) | (7u << 15)); /* Clear ENFC clock selector and dividers */
	reg |= (3u << 15);                               /* Set ENFC_CLK_SEL to PLL2 PFD2 (396 MHz) */
	reg |= (1u << 18);                               /* Set ENFC_PRED divider to 2 */
	*(imx6ull_common.ccm + ccm_cs2cdr) = reg;

	/* Restore output clocks state */
	*(imx6ull_common.ccm + ccm_ccgr4) = tmp;

	imx6ull_ddrInit();
}
