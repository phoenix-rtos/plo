/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * FlexSPI Controller driver (i.MX RT117x)
 *
 * Copyright 2021-2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _FSPI_RT117X_H_
#define _FSPI_RT117X_H_


/* clang-format off */

enum { mcr0 = 0, mcr1, mcr2, ahbcr, inten, intr, lutkey, lutcr, ahbrxbuf0cr0, ahbrxbuf1cr0, ahbrxbuf2cr0, ahbrxbuf3cr0,
	ahbrxbuf4cr0, ahbrxbuf5cr0, ahbrxbuf6cr0, ahbrxbuf7cr0, flsha1cr0 = 24, flsha2cr0, flshb1cr0, flshb2cr0, flsha1cr1,
    flsha2cr1, flshb1cr1, flshb2cr1, flsha1cr2, flsha2cr2, flshb1cr2, flshb2cr2, flshcr4 = 37, ipcr0 = 40, ipcr1,
	ipcmd = 44, iprxfcr = 46, iptxfcr, dllacr, dllbcr, misccr4 = 52, misccr5, misccr6, misccr7, sts0 = 56, sts1, sts2,
    ahbspndsts, iprxfsts, iptxfsts, rfdr32 = 64, tfdr32 = 96, lut64 = 128, hmstr0cr = 256, hmstr1cr, hmstr2cr,
	hmstr3cr, hmstr4cr, hmstr5cr, hmstr6cr, hmstr7cr, haddrstart, haddrend, haddroffset };

/* clang-format on */


#define AHBRXBUF_CNT 8


static addr_t flexspi_ahbAddr(int instance)
{
	switch (instance) {
		case flexspi_instance1: return 0x30000000;
		case flexspi_instance2: return 0x60000000;
		default: return 0;
	}
}


static void *flexspi_getBase(int instance)
{
	switch (instance) {
		case flexspi_instance1: return (void *)0x400cc000;
		case flexspi_instance2: return (void *)0x400d0000;
		default: return NULL;
	}
}


__attribute__((section(".noxip"))) static void flexspi_clockConfig(flexspi_t *fspi)
{
	int gate, clk, div, mux, mfd, mfn, state;

	switch (fspi->instance) {
		case flexspi_instance1:
			clk = pctl_clk_flexspi1;
			gate = pctl_lpcg_flexspi1;
			div = 3; /* SYS_PLL2_CLK / (3 + 1) => 132 MHz */
			mux = 5; /* Select main clock: SYS_PLL2_CLK = 528 MHz */
			mfd = 0;
			mfn = 0;
			break;

		case flexspi_instance2:
			clk = pctl_clk_flexspi2;
			gate = pctl_lpcg_flexspi2;
			/* Copy defaults */
			_imxrt_getDevClock(clk, &div, &mux, &mfd, &mfn, &state);
			break;

		default:
			return;
	}

	_imxrt_setDirectLPCG(gate, 0);
	/* clkmhz = MHZ(mux) / (div + 1) * mfn / (mfd + 1) */
	_imxrt_setDevClock(clk, div, mux, mfd, mfn, 1);
	_imxrt_setDirectLPCG(gate, 1);
}


__attribute__((section(".noxip"))) static int flexspi_pinConfig(flexspi_t *fspi)
{
	unsigned int done, i;
	static const struct {
		int devMask;
		int isel, daisy;
		int mux, mode;
		int pad;
	} pin[] = {
		/* FlexSPI-1 A1/A2 */
		{ (flexspi_instance1 << 8) | flexspi_slBusA2, -1, -1, pctl_mux_gpio_sd_b2_04, 3, pctl_pad_gpio_sd_b2_04 },                                         /* SS1 */
		{ (flexspi_instance1 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspi1_dqs_fa, 2, pctl_mux_gpio_sd_b2_05, 1, pctl_pad_gpio_sd_b2_05 }, /* DQS */
		{ (flexspi_instance1 << 8) | flexspi_slBusA1, -1, -1, pctl_mux_gpio_sd_b2_06, 1, pctl_pad_gpio_sd_b2_06 },                                         /* SS0 */
		{ (flexspi_instance1 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspi1_sck_fa, 1, pctl_mux_gpio_sd_b2_07, 1, pctl_pad_gpio_sd_b2_07 }, /* SCLK */
		{ (flexspi_instance1 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspi1_fa_0, 1, pctl_mux_gpio_sd_b2_08, 1, pctl_pad_gpio_sd_b2_08 },   /* D0 */
		{ (flexspi_instance1 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspi1_fa_1, 1, pctl_mux_gpio_sd_b2_09, 1, pctl_pad_gpio_sd_b2_09 },   /* D1 */
		{ (flexspi_instance1 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspi1_fa_2, 1, pctl_mux_gpio_sd_b2_10, 1, pctl_pad_gpio_sd_b2_10 },   /* D2 */
		{ (flexspi_instance1 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspi1_fa_3, 1, pctl_mux_gpio_sd_b2_11, 1, pctl_pad_gpio_sd_b2_11 },   /* D3 */

		/* FlesSPI-1 B1/B2 */
		{ (flexspi_instance1 << 8) | flexspi_slBusB1 | flexspi_slBusB2, -1, -1, pctl_mux_gpio_sd_b1_05, 8, pctl_pad_gpio_sd_b1_05 },                       /* DQS */
		{ (flexspi_instance1 << 8) | flexspi_slBusB1 | flexspi_slBusB2, pctl_isel_flexspi1_sck_fb, 1, pctl_mux_gpio_sd_b2_04, 1, pctl_pad_gpio_sd_b2_04 }, /* SCLK */
		{ (flexspi_instance1 << 8) | flexspi_slBusB1, -1, -1, pctl_mux_gpio_sd_b2_05, 3, pctl_pad_gpio_sd_b2_05 },                                         /* SS0 */
		{ (flexspi_instance1 << 8) | flexspi_slBusB2, -1, -1, pctl_mux_gpio_sd_b1_03, 9, pctl_pad_gpio_sd_b1_03 },                                         /* SS1 */
		{ (flexspi_instance1 << 8) | flexspi_slBusB1 | flexspi_slBusB2, pctl_isel_flexspi1_fb_0, 1, pctl_mux_gpio_sd_b2_03, 1, pctl_pad_gpio_sd_b2_03 },   /* D0 */
		{ (flexspi_instance1 << 8) | flexspi_slBusB1 | flexspi_slBusB2, pctl_isel_flexspi1_fb_1, 1, pctl_mux_gpio_sd_b2_02, 1, pctl_pad_gpio_sd_b2_02 },   /* D1 */
		{ (flexspi_instance1 << 8) | flexspi_slBusB1 | flexspi_slBusB2, pctl_isel_flexspi1_fb_2, 1, pctl_mux_gpio_sd_b2_01, 1, pctl_pad_gpio_sd_b2_01 },   /* D2 */
		{ (flexspi_instance1 << 8) | flexspi_slBusB1 | flexspi_slBusB2, pctl_isel_flexspi1_fb_3, 1, pctl_mux_gpio_sd_b2_00, 1, pctl_pad_gpio_sd_b2_00 },   /* D3 */

		/* FlexSPI-2 A1/B2 */
		{ (flexspi_instance2 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspi2_fa_0, 0, pctl_mux_gpio_emc_b2_13, 4, pctl_pad_gpio_emc_b2_13 },   /* D0 */
		{ (flexspi_instance2 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspi2_fa_1, 0, pctl_mux_gpio_emc_b2_14, 4, pctl_pad_gpio_emc_b2_14 },   /* D1 */
		{ (flexspi_instance2 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspi2_fa_2, 0, pctl_mux_gpio_emc_b2_15, 4, pctl_pad_gpio_emc_b2_15 },   /* D2 */
		{ (flexspi_instance2 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspi2_fa_3, 0, pctl_mux_gpio_emc_b2_16, 4, pctl_pad_gpio_emc_b2_16 },   /* D3 */
		{ (flexspi_instance2 << 8) | flexspi_slBusA1 | flexspi_slBusA2, -1, -1, pctl_mux_gpio_emc_b2_17, 4, pctl_pad_gpio_emc_b2_17 },                       /* D4 */
		{ (flexspi_instance2 << 8) | flexspi_slBusA1 | flexspi_slBusA2, -1, -1, pctl_mux_gpio_emc_b2_18, 4, pctl_pad_gpio_emc_b2_18 },                       /* D5 */
		{ (flexspi_instance2 << 8) | flexspi_slBusA1 | flexspi_slBusA2, -1, -1, pctl_mux_gpio_emc_b2_19, 4, pctl_pad_gpio_emc_b2_19 },                       /* D6 */
		{ (flexspi_instance2 << 8) | flexspi_slBusA1 | flexspi_slBusA2, -1, -1, pctl_mux_gpio_emc_b2_20, 4, pctl_pad_gpio_emc_b2_20 },                       /* D7 */
		{ (flexspi_instance2 << 8) | flexspi_slBusA1 | flexspi_slBusA2, -1, -1, pctl_mux_gpio_emc_b2_12, 4, pctl_pad_gpio_emc_b2_12 },                       /* DQS */
		{ (flexspi_instance2 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspi2_sck_fa, 0, pctl_mux_gpio_emc_b2_10, 4, pctl_pad_gpio_emc_b2_10 }, /* SCLK */
		{ (flexspi_instance2 << 8) | flexspi_slBusA1, -1, -1, pctl_mux_gpio_emc_b2_11, 4, pctl_pad_gpio_emc_b2_11 },                                         /* SS0 */
		{ (flexspi_instance2 << 8) | flexspi_slBusA2, -1, -1, pctl_mux_gpio_ad_01, 9, pctl_pad_gpio_ad_01 },                                                 /* SS1 */

		/* FlexSPI-2 B1/B2 */
		{ (flexspi_instance2 << 8) | flexspi_slBusB1 | flexspi_slBusB2, -1, -1, pctl_mux_gpio_emc_b2_06, 4, pctl_pad_gpio_emc_b2_07 }, /* D0 */
		{ (flexspi_instance2 << 8) | flexspi_slBusB1 | flexspi_slBusB2, -1, -1, pctl_mux_gpio_emc_b2_05, 4, pctl_pad_gpio_emc_b2_06 }, /* D1 */
		{ (flexspi_instance2 << 8) | flexspi_slBusB1 | flexspi_slBusB2, -1, -1, pctl_mux_gpio_emc_b2_04, 4, pctl_pad_gpio_emc_b2_05 }, /* D2 */
		{ (flexspi_instance2 << 8) | flexspi_slBusB1 | flexspi_slBusB2, -1, -1, pctl_mux_gpio_emc_b2_03, 4, pctl_pad_gpio_emc_b2_04 }, /* D3 */
		{ (flexspi_instance2 << 8) | flexspi_slBusB1 | flexspi_slBusB2, -1, -1, pctl_mux_gpio_emc_b2_02, 4, pctl_pad_gpio_emc_b2_03 }, /* D4 */
		{ (flexspi_instance2 << 8) | flexspi_slBusB1 | flexspi_slBusB2, -1, -1, pctl_mux_gpio_emc_b2_01, 4, pctl_pad_gpio_emc_b2_01 }, /* D5 */
		{ (flexspi_instance2 << 8) | flexspi_slBusB1 | flexspi_slBusB2, -1, -1, pctl_mux_gpio_emc_b2_00, 4, pctl_pad_gpio_emc_b2_00 }, /* D6 */
		{ (flexspi_instance2 << 8) | flexspi_slBusB1 | flexspi_slBusB2, -1, -1, pctl_mux_gpio_emc_b1_41, 4, pctl_pad_gpio_emc_b1_41 }, /* D7 */
		{ (flexspi_instance2 << 8) | flexspi_slBusB1 | flexspi_slBusB2, -1, -1, pctl_mux_gpio_sd_b2_07, 8, pctl_pad_gpio_sd_b2_07 },   /* DQS */
		{ (flexspi_instance2 << 8) | flexspi_slBusB1 | flexspi_slBusB2, -1, -1, pctl_mux_gpio_emc_b2_09, 4, pctl_pad_gpio_emc_b2_09 }, /* SCLK */
		{ (flexspi_instance2 << 8) | flexspi_slBusB1, -1, -1, pctl_mux_gpio_emc_b2_08, 4, pctl_pad_gpio_emc_b2_08 },                   /* SS0 */
		{ (flexspi_instance2 << 8) | flexspi_slBusB2, -1, -1, pctl_mux_gpio_ad_00, 9, pctl_pad_gpio_ad_00 },                           /* SS1 */
	};

	for (i = 0, done = 0; i < sizeof(pin) / sizeof(pin[0]); ++i) {
		if ((pin[i].devMask & ((1 << (fspi->instance - 1)) << 8)) == 0)
			continue;

		if ((pin[i].devMask & fspi->slPortMask) == 0)
			continue;

		if (pin[i].isel >= 0)
			_imxrt_setIOisel(pin[i].isel, pin[i].daisy);

		if (pin[i].mux >= 0)
			_imxrt_setIOmux(pin[i].mux, pin[i].mode, 1); /* sion is enabled */

		if (pin[i].pad >= 0)
			_imxrt_setIOpad(pin[i].pad, 0, 1, 0, 1, 0, 0);

		done++;
	}

	return done ? EOK : -EINVAL;
}


#endif /* _FSPI_RT117X_H_ */
