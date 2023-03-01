/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * FlexSPI Controller driver (i.MX RT105x)
 *
 * Copyright 2021-2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _FSPI_RT105X_H_
#define _FSPI_RT105X_H_


/* clang-format off */

enum { mcr0 = 0, mcr1, mcr2, ahbcr, inten, intr, lutkey, lutcr, ahbrxbuf0cr0, ahbrxbuf1cr0, ahbrxbuf2cr0, ahbrxbuf3cr0,
	flsha1cr0 = 24, flsha2cr0, flshb1cr0, flshb2cr0, flsha1cr1, flsha2cr1, flshb1cr1, flshb2cr1, flsha1cr2, flsha2cr2,
	flshb1cr2, flshb2cr2, flshcr4 = 37, ipcr0 = 40, ipcr1, ipcmd = 44, iprxfcr = 46, iptxfcr, dllacr, dllbcr, sts0 = 56,
	sts1, sts2, ahbspndsts, iprxfsts, iptxfsts, rfdr32 = 64, tfdr32 = 96, lut64 = 128 };

/* clang-format on */

#define AHBRXBUF_CNT 4


static inline addr_t flexspi_ahbAddr(int instance)
{
	return (instance == flexspi_instance1) ? 0x60000000 : 0;
}

static inline void *flexspi_getBase(int instance)
{
	return instance == flexspi_instance1 ? (void *)0x402a8000 : NULL;
}


__attribute__((section(".noxip"))) static void flexspi_clockConfig(flexspi_t *fspi)
{
	_imxrt_ccmControlGate(pctl_clk_flexspi, clk_state_off);
	_imxrt_ccmSetDiv(clk_div_flexspi, 1); /* div2 -> 130 MHz */
	_imxrt_ccmSetMux(clk_mux_flexspi, 3); /* PLL3 PFD0 */
	_imxrt_ccmInitUsb1Pfd(clk_pfd0, 33);  /* PLL3_PDF0=261.818MHz */
	_imxrt_ccmControlGate(pctl_clk_flexspi, clk_state_run_wait);
}


__attribute__((section(".noxip"))) static int flexspi_pinConfig(flexspi_t *fspi)
{
	/* MIMXRT1052 (IS25WP064 nor flash) pin setup as decoded from bootloader image */
	*((volatile u32 *)0x401f81ec) = 0x11;   /* MUX FLEXSPIA_SS0_B */
	*((volatile u32 *)0x401f83dc) = 0x10f1; /* PIN FLEXSPIA_SS0_B */

	*((volatile u32 *)0x401f81e8) = 0x11;    /* MUX FLEXSPIA_DQS */
	*((volatile u32 *)0x401f84a4) = 0x0;     /* DAISY FLEXSPIA_DQS */
	*((volatile u32 *)0x401f83d8) = 0x130f1; /* PIN FLEXSPIA_DQS */

	*((volatile u32 *)0x401f81f0) = 0x11;   /* MUX FLEXSPIA_SCLK */
	*((volatile u32 *)0x401f84c8) = 0x0;    /* DAISY FLEXSPIA_SCLK */
	*((volatile u32 *)0x401f83e0) = 0x10f1; /* PIN FLEXSPIA_SCLK */

	*((volatile u32 *)0x401f81f4) = 0x11;   /* MUX FLEXSPIA_DATA00 */
	*((volatile u32 *)0x401f84a8) = 0x0;    /* DAISY FLEXSPIA_DATA00 */
	*((volatile u32 *)0x401f83e4) = 0x10f1; /* PIN FLEXSPIA_DATA00 */

	*((volatile u32 *)0x401f81f8) = 0x11;   /* MUX FLEXSPIA_DATA01 */
	*((volatile u32 *)0x401f84ac) = 0x0;    /* DAISY FLEXSPIA_DATA01 */
	*((volatile u32 *)0x401f83e8) = 0x10f1; /* PIN FLEXSPIA_DATA01 */

	*((volatile u32 *)0x401f81fc) = 0x11;   /* MUX FLEXSPIA_DATA02 */
	*((volatile u32 *)0x401f84b0) = 0x0;    /* DAISY FLEXSPIA_DATA02 */
	*((volatile u32 *)0x401f83ec) = 0x10f1; /* PIN FLEXSPIA_DATA02 */

	*((volatile u32 *)0x401f8200) = 0x11;   /* MUX FLEXSPIA_DATA03 */
	*((volatile u32 *)0x401f84b4) = 0x0;    /* DAISY FLEXSPIA_DATA0 */
	*((volatile u32 *)0x401f83f0) = 0x10f1; /* PIN FLEXSPIA_DATA03 */

#if 0
	/* FLEXSPIA + FLEXSPIB pin setup for S29QL512 nor hyper flash */
	*((volatile u32 *)0x401f81e4) = 0x11;   /* MUX FLEXSPIB_SCLK */
	*((volatile u32 *)0x401f84b8) = 0x0;    /* DAISY */
	*((volatile u32 *)0x401f83d4) = 0x10f1; /* PIN FLEXSPIB_SCLK */

	*((volatile u32 *)0x401f81e0) = 0x11;   /* MUX FLEXSPIB_DATA00 */
	*((volatile u32 *)0x401f83d0) = 0x10f1; /* PIN FLEXSPIB_DATA00 */

	*((volatile u32 *)0x401f81dc) = 0x11;   /* MUX FLEXSPIB_DATA01 */
	*((volatile u32 *)0x401f84bc) = 0x0;    /* DAISY */
	*((volatile u32 *)0x401f83cc) = 0x10f1; /* PIN FLEXSPIB_DATA01 */

	*((volatile u32 *)0x401f81d8) = 0x11;   /* MUX FLEXSPIB_DATA00 */
	*((volatile u32 *)0x401f84c0) = 0x0;    /* DAISY */
	*((volatile u32 *)0x401f83c8) = 0x10f1; /* PIN FLEXSPIB_DATA02 */

	*((volatile u32 *)0x401f81d4) = 0x11;   /* MUX FLEXSPIB_DATA00 */
	*((volatile u32 *)0x401f84c4) = 0x0;    /* DAISY */
	*((volatile u32 *)0x401f83c4) = 0x10f1; /* PIN FLEXSPIB_DATA03 */

#endif


	return EOK;
}


#endif /* _FSPI_RT105X_H_ */
