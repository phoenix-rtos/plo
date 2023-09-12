/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * FlexSPI Controller driver (i.MX RT106x)
 *
 * Copyright 2021-2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _FSPI_RT106X_H_
#define _FSPI_RT106X_H_


/* clang-format off */

enum { mcr0 = 0, mcr1, mcr2, ahbcr, inten, intr, lutkey, lutcr, ahbrxbuf0cr0, ahbrxbuf1cr0, ahbrxbuf2cr0, ahbrxbuf3cr0,
	flsha1cr0 = 24, flsha2cr0, flshb1cr0, flshb2cr0, flsha1cr1, flsha2cr1, flshb1cr1, flshb2cr1, flsha1cr2, flsha2cr2,
	flshb1cr2, flshb2cr2, flshcr4 = 37, ipcr0 = 40, ipcr1, ipcmd = 44, iprxfcr = 46, iptxfcr, dllacr, dllbcr, sts0 = 56,
	sts1, sts2, ahbspndsts, iprxfsts, iptxfsts, rfdr32 = 64, tfdr32 = 96, lut64 = 128 };

/* clang-format on */

#define AHBRXBUF_CNT 4


static addr_t flexspi_ahbAddr(int instance)
{
	switch (instance) {
		case flexspi_instance1: return 0x60000000;
		case flexspi_instance2: return 0x70000000;
		default: return 0;
	}
}


static void *flexspi_getBase(int instance)
{
	switch (instance) {
		case flexspi_instance1: return (void *)0x402a8000;
		case flexspi_instance2: return (void *)0x402a4000;
		default: return NULL;
	}
}


__attribute__((section(".noxip"))) static void flexspi_clockConfig(flexspi_t *fspi)
{
	if (fspi->instance == flexspi_instance1) {
		_imxrt_ccmControlGate(pctl_clk_flexspi, clk_state_off);
		_imxrt_ccmInitUsb1Pfd(clk_pfd0, 13);  /* PLL3_PDF0=664.6MHz */
		_imxrt_ccmSetDiv(clk_div_flexspi, 4); /* div5 */
		_imxrt_ccmSetMux(clk_mux_flexspi, 3); /* PLL3 PFD0 */
		_imxrt_ccmControlGate(pctl_clk_flexspi, clk_state_run_wait);
	}
	else if (fspi->instance == flexspi_instance2) {
		_imxrt_ccmControlGate(pctl_clk_flexspi2, clk_state_off);
		_imxrt_ccmInitUsb1Pfd(clk_pfd0, 13);   /* PLL3_PDF0=664.6MHz */
		_imxrt_ccmSetDiv(clk_div_flexspi2, 4); /* div5 */
		_imxrt_ccmSetMux(clk_mux_flexspi2, 1); /* PLL3 PFD0 */
		_imxrt_ccmControlGate(pctl_clk_flexspi2, clk_state_run_wait);
	}
}


__attribute__((section(".noxip"))) static int flexspi_pinConfig(flexspi_t *fspi)
{
	unsigned int done, i;
	static const struct {
		int typ;
		int devMask;
		int isel, daisy;
		int mux, mode;
		int pad;
	} pin[] = {
		/* FlexSPI instance 1 A1|A2 bus */
		{ 0, (flexspi_instance1 << 8) | flexspi_slBusA1, -1, -1, pctl_mux_gpio_sd_b1_06, 1, pctl_pad_gpio_sd_b1_06 },                                        /* SS0 */
		{ 2, (flexspi_instance1 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspia_dqs, 0, pctl_mux_gpio_sd_b1_05, 1, pctl_pad_gpio_sd_b1_05 },   /* DQS */
		{ 1, (flexspi_instance1 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspia_sck, 0, pctl_mux_gpio_sd_b1_07, 1, pctl_pad_gpio_sd_b1_07 },   /* SCLK */
		{ 1, (flexspi_instance1 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspia_data0, 0, pctl_mux_gpio_sd_b1_08, 1, pctl_pad_gpio_sd_b1_08 }, /* D0 */
		{ 1, (flexspi_instance1 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspia_data1, 0, pctl_mux_gpio_sd_b1_09, 1, pctl_pad_gpio_sd_b1_09 }, /* D1 */
		{ 1, (flexspi_instance1 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspia_data2, 0, pctl_mux_gpio_sd_b1_10, 1, pctl_pad_gpio_sd_b1_10 }, /* D2 */
		{ 1, (flexspi_instance1 << 8) | flexspi_slBusA1 | flexspi_slBusA2, pctl_isel_flexspia_data3, 0, pctl_mux_gpio_sd_b1_11, 1, pctl_pad_gpio_sd_b1_11 }, /* D3 */

		/* FlexSPI instance 1 B1|B2 bus (slave) */
		{ 1, (flexspi_instance1 << 8) | flexspi_slBusB1 | flexspi_slBusB2, -1, -1, pctl_mux_gpio_sd_b1_04, 1, pctl_pad_gpio_sd_b1_04 },                      /* SCLK */
		{ 1, (flexspi_instance1 << 8) | flexspi_slBusB1 | flexspi_slBusB2, pctl_isel_flexspib_data0, 0, pctl_mux_gpio_sd_b1_03, 1, pctl_pad_gpio_sd_b1_03 }, /* D0/D4 */
		{ 1, (flexspi_instance1 << 8) | flexspi_slBusB1 | flexspi_slBusB2, pctl_isel_flexspib_data1, 0, pctl_mux_gpio_sd_b1_02, 1, pctl_pad_gpio_sd_b1_02 }, /* D1/D5 */
		{ 1, (flexspi_instance1 << 8) | flexspi_slBusB1 | flexspi_slBusB2, pctl_isel_flexspib_data2, 0, pctl_mux_gpio_sd_b1_01, 1, pctl_pad_gpio_sd_b1_01 }, /* D2/D6 */
		{ 1, (flexspi_instance1 << 8) | flexspi_slBusB1 | flexspi_slBusB2, pctl_isel_flexspib_data3, 0, pctl_mux_gpio_sd_b1_00, 1, pctl_pad_gpio_sd_b1_00 }, /* D3/D7 */

		/* FlexSPI instance 2 A1 ed by internal flash W25Q32, unspecified muxes/pads/pins */
		{ 1, (flexspi_instance2 << 8) | flexspi_slBusA1, -1, -1, -1, 0, -1 }, /* SS0 */
		{ 1, (flexspi_instance2 << 8) | flexspi_slBusA1, -1, -1, -1, 0, -1 }, /* SCLK */
		{ 2, (flexspi_instance2 << 8) | flexspi_slBusA1, -1, 2, -1, 0, -1 },  /* DQS */
		{ 1, (flexspi_instance2 << 8) | flexspi_slBusA1, -1, 2, -1, 0, -1 },  /* D0 */
		{ 1, (flexspi_instance2 << 8) | flexspi_slBusA1, -1, 0, -1, 0, -1 },  /* D1 */
		{ 1, (flexspi_instance2 << 8) | flexspi_slBusA1, -1, 0, -1, 0, -1 },  /* D2 */
		{ 1, (flexspi_instance2 << 8) | flexspi_slBusA1, -1, 2, -1, 0, -1 },  /* D3 */
	};

	for (i = 0, done = 0; i < sizeof(pin) / sizeof(pin[0]); ++i) {
		if ((pin[i].devMask & ((1 << (fspi->instance - 1)) << 8)) == 0) {
			continue;
		}

		if ((pin[i].devMask & fspi->slPortMask) == 0) {
			continue;
		}

		if (pin[i].isel >= 0) {
			_imxrt_setIOisel(pin[i].isel, pin[i].daisy);
		}

		if (pin[i].mux >= 0) {
			_imxrt_setIOmux(pin[i].mux, pin[i].mode, 1); /* sion is enabled */
		}

		if (pin[i].pad >= 0) {
			if (pin[i].typ == 0) {
				_imxrt_setIOpad(pin[i].pad, 0, 0, 0, 1, 0, 3, 6, 1);
			}
			else if (pin[i].typ == 1) { /* DATA */
				_imxrt_setIOpad(pin[i].pad, 0, 0, 0, 0, 0, 3, 6, 1);
			}
			else if (pin[i].typ == 2) { /* DQS */
				_imxrt_setIOpad(pin[i].pad, 1, 0, 1, 1, 0, 3, 6, 1);
			}
		}

		done++;
	}

	if (fspi->instance == flexspi_instance2) {
		/* MIMXRT1064 (W25Q32 nor flash internally connected) setup read from bootloader */

		*((volatile u32 *)0x401f86ac) = 0x0;    /* SS0 MUX: */
		*((volatile u32 *)0x401f8704) = 0x10f1; /* SS0 PAD: */

		*((volatile u32 *)0x401f867c) = 0x10;   /* SCLK MUX: */
		*((volatile u32 *)0x401f86d4) = 0x10f1; /* SCLK PAD: */
		*((volatile u32 *)0x401f8750) = 0x2;    /* SCLK SEL: */

		*((volatile u32 *)0x401f8664) = 0x0;    /* D0 MUX: */
		*((volatile u32 *)0x401f86bc) = 0x10f1; /* D0 PAD: */
		*((volatile u32 *)0x401f8730) = 0x2;    /* D0 SEL: */

		*((volatile u32 *)0x401f86a0) = 0x0;    /* D1 MUX: */
		*((volatile u32 *)0x401f86f8) = 0x10f1; /* D1 PAD: */
		*((volatile u32 *)0x401f8734) = 0x0;    /* D1 SEL: */

		*((volatile u32 *)0x401f869c) = 0x0;    /* D2 MUX: */
		*((volatile u32 *)0x401f86f4) = 0x10f1; /* D2 PAD: */
		*((volatile u32 *)0x401f8738) = 0x0;    /* D2 SEL: */

		*((volatile u32 *)0x401f8684) = 0x0;    /* D3 MUX: */
		*((volatile u32 *)0x401f86dc) = 0x10f1; /* D3 PAD: */
		*((volatile u32 *)0x401f873c) = 0x2;    /* D3 SEL: */

		*((volatile u32 *)0x401f8680) = 0x10;    /* DQS_A MUX: */
		*((volatile u32 *)0x401f86d8) = 0x130f1; /* DQS_A PAD: */
		*((volatile u32 *)0x401f872c) = 0x2;     /* DQS_A SEL: */

		*((volatile u32 *)0x401f867c) = 0x10; /* MUX? */
		*((volatile u32 *)0x401f86b0) = 0;    /* MUX? */
	}

	return done ? EOK : -EINVAL;
}


#endif /* _FSPI_RT106X_H_ */
