/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * MCXN94x basic peripherals control functions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <board_config.h>

#include <hal/hal.h>
#include <lib/errno.h>
#include "n94x.h"

enum { flexcomm_istat = 1021, flexcomm_pselid };


enum {
	syscon_ahbmatprio = 4, syscon_cpu0stckcal = 14, syscon_cpu0nstckcal, syscon_cpu1stckcal,
	syscon_nmisrc = 18,
	syscon_presetctrl0 = 64, syscon_presetctrl1, syscon_presetctrl2, syscon_presetctrl3,
	syscon_presetctrlset0 = 72, syscon_presetctrlset1, syscon_presetctrlset2, syscon_presetctrlset3,
	syscon_presetctrlcrl0 = 80, syscon_presetctrlcrl1, syscon_presetctrlcrl2, syscon_presetctrlcrl3,
	syscon_ahbclkctrl0 = 128, syscon_ahbclkctrl1, syscon_ahbclkctrl2, syscon_ahbclkctrl3,
	syscon_ahbclkctrlset0 = 136, syscon_ahbclkctrlset1, syscon_ahbclkctrlset2, syscon_ahbclkctrlset3,
	syscon_ahbclkctrlclr0 = 144, syscon_ahbclkctrlclr1, syscon_ahbclkctrlclr2, syscon_ahbclkctrlclr3,
	syscon_systickclksel0 = 152, syscon_systickclksel1, syscon_tracesel,
	syscon_ctimer0clksel, syscon_ctimer1clksel, syscon_ctimer2clksel, syscon_ctimer3clksel,
	syscon_ctimer4clksel, syscon_clkoutset = 162, syscon_adc0clksel = 169, syscon_usb0clksel,
	syscon_fc0clksel = 172, syscon_fc1clksel, syscon_fc2clksel, syscon_fc3clksel, syscon_fc4clksel,
	syscon_fc5clksel, syscon_fc6clksel, syscon_fc7clksel, syscon_fc8clksel, syscon_fc9clksel,
	syscon_sctclksel = 188, syscon_systickclkdiv0 = 192, syscon_systickclkdiv1, syscon_traceclkdiv,
	syscon_tsiclksel = 212, syscon_sincfiltclksel = 216, syscon_slowclkdiv = 222, syscon_tsiclkdiv,
	syscon_ahbclkdiv, syscon_clkoutdiv, syscon_frohfdiv, syscon_wdt0clkdiv, syscon_adc0clkdiv = 229,
	syscon_usb0clkdiv, syscon_sctclkdiv = 237, syscon_pllclkdiv = 241, syscon_ctimer0clkdiv = 244,
	syscon_ctimer1clkdiv, syscon_ctimer2clkdiv, syscon_ctimer3clkdiv, syscon_ctimer4clkdiv,
	syscon_pll1clk0div, syscon_pll1clk1div, syscon_clkunlock, syscon_nvmctrl, syscon_romcr,
	syscon_smartdmaint = 261, syscon_adc1clksel = 281, syscon_adc1clkdiv, syscon_dac0clksel = 292,
	syscon_dac0clkdiv, syscon_dac1clksel, syscon_dac1clkdiv, syscon_dac2clksel, syscon_dac2clkdiv,
	syscon_flexspiclksel, syscon_flexspiclkdiv, syscon_pllclkdivsel = 331, syscon_i3c0fclksel,
	syscon_i3c0fclkstcsel, syscon_i3c0fclkstcdiv, syscon_i3c0fclksdiv, syscon_i3c0fclkdiv, syscon_i3c0fclkssel,
	syscon_micfilfclksel, syscon_micfilfclkdiv, syscon_usdhcclksel = 342, syscon_usdhcclkdiv,
	syscon_flexioclksel, syscon_flexioclkdiv, syscon_flexcan0clksel = 360, syscon_flexcan0clkdiv,
	syscon_flexcan1clksel, syscon_flexcan1clkdiv, syscon_enetrmiiclksel, syscon_enetrmiiclkdiv,
	syscon_enetptprefclksel, syscon_enetptprefclkdiv, syscon_enetphyintfsel, syscon_enetsbdflowctrl,
	syscon_ewm0clksel = 373, syscon_wdt1clksel, syscon_wdt1clkdiv, syscon_ostimerclksel,
	syscon_cmp0fclksel = 380, syscon_cmp0fclkdiv, syscon_cmp0rrclksel, syscon_rrclkdiv,
	syscon_cmp1fclksel, syscon_cmp1fclkdiv, syscon_cmp1rrclksel, syscon_cmp1rrclkdiv,
	syscon_cmp2fclksel, syscon_cmp2fclkdiv, syscon_cmp2rrclksel, syscon_cmp2rrclkdiv,
	syscon_cpuctrl = 512, syscon_cpboot, syscon_cpustat = 514, syscon_pcacctrl = 521,
	syscon_flexcomm0clkdiv = 532, syscon_flexcomm1clkdiv, syscon_flexcomm2clkdiv,
	syscon_flexcomm3clkdiv, syscon_flexcomm4clkdiv, syscon_flexcomm5clkdiv, syscon_flexcomm6clkdiv,
	syscon_flexcomm7clkdiv, syscon_flexcomm8clkdiv, syscon_flexcomm9clkdiv,
	syscon_sai0clksel = 544, syscon_sai1clksel, syscon_sai0clkdiv, syscon_sai1clkdiv,
	syscon_emvsim0clksel, syscon_emvsim1clksel, syscon_emvsim0clkdiv, syscon_emvsim1clkdiv,
	syscon_clockctrl = 646, syscon_i3c1fclksel = 716, syscon_i3c1fclkstcsel, syscon_i3c1fclkstcdiv,
	syscon_i3c1fclksdiv, syscon_i3c1fclkdiv, syscon_i3c1fclkssel, syscon_etbstatus = 724,
	syscon_etbcounterctrl, syscon_etbcounterreload, syscon_etbcountervalue, syscon_graycodelsb,
	syscon_graycodemsb, syscon_binarycodelsb, syscon_binarycodemsb, syscon_autoclkgateoverride = 897,
	syscon_autoclkgataoverridec = 907, syscon_pwm0subctl = 910, syscon_pwm1subctl,
	syscon_ctimerglobalstarten, syscon_eccenablectrl, syscon_jtagid = 1020, syscon_devicetype,
	syscon_deviceid0, syscon_dieid
};


static struct {
	volatile u32 *syscon;
	volatile u32 *flexcomm[10];
} n94x_common;


static int _mcxn94x_sysconGetRegs(int dev, volatile u32 **selr, volatile u32 **divr)
{
	if ((dev < pctl_rom) || (dev > pctl_i3c1stc)) {
		return -1;
	}

	*selr = NULL;
	*divr = NULL;

	switch (dev) {
		case pctl_flexspi:
			*selr = n94x_common.syscon + syscon_flexspiclksel;
			*divr = n94x_common.syscon + syscon_flexspiclkdiv;
			break;

		case pctl_adc0:
			*selr = n94x_common.syscon + syscon_adc0clksel;
			*divr = n94x_common.syscon + syscon_adc0clkdiv;
			break;

		case pctl_adc1:
			*selr = n94x_common.syscon + syscon_adc1clksel;
			*divr = n94x_common.syscon + syscon_adc1clkdiv;
			break;

		case pctl_dac0:
			*selr = n94x_common.syscon + syscon_dac0clksel;
			*divr = n94x_common.syscon + syscon_dac0clkdiv;
			break;

		case pctl_dac1:
			*selr = n94x_common.syscon + syscon_dac1clksel;
			*divr = n94x_common.syscon + syscon_dac1clkdiv;
			break;

		case pctl_dac2:
			*selr = n94x_common.syscon + syscon_dac2clksel;
			*divr = n94x_common.syscon + syscon_dac2clkdiv;
			break;

		case pctl_timer0:
			*selr = n94x_common.syscon + syscon_ctimer0clksel;
			*divr = n94x_common.syscon + syscon_ctimer0clkdiv;
			break;

		case pctl_timer1:
			*selr = n94x_common.syscon + syscon_ctimer1clksel;
			*divr = n94x_common.syscon + syscon_ctimer1clkdiv;
			break;

		case pctl_timer2:
			*selr = n94x_common.syscon + syscon_ctimer2clksel;
			*divr = n94x_common.syscon + syscon_ctimer2clkdiv;
			break;

		case pctl_timer3:
			*selr = n94x_common.syscon + syscon_ctimer3clksel;
			*divr = n94x_common.syscon + syscon_ctimer3clkdiv;
			break;

		case pctl_timer4:
			*selr = n94x_common.syscon + syscon_ctimer4clksel;
			*divr = n94x_common.syscon + syscon_ctimer4clkdiv;
			break;

		case pctl_sct:
			*selr = n94x_common.syscon + syscon_sctclksel;
			*divr = n94x_common.syscon + syscon_sctclkdiv;
			break;

		case pctl_ostimer:
			*selr = n94x_common.syscon + syscon_ostimerclksel;
			break;

		case pctl_ewm:
			*selr = n94x_common.syscon + syscon_ewm0clksel;
			break;

		case pctl_wwdt0:
			*divr = n94x_common.syscon + syscon_wdt0clkdiv;
			break;

		case pctl_wwdt1:
			*selr = n94x_common.syscon + syscon_wdt1clksel;
			*divr = n94x_common.syscon + syscon_wdt1clkdiv;
			break;

		case pctl_usb0fs:
			*selr = n94x_common.syscon + syscon_usb0clksel;
			*divr = n94x_common.syscon + syscon_usb0clkdiv;
			break;

		case pctl_evsim0:
			*selr = n94x_common.syscon + syscon_emvsim0clksel;
			*divr = n94x_common.syscon + syscon_emvsim0clkdiv;
			break;

		case pctl_evsim1:
			*selr = n94x_common.syscon + syscon_emvsim1clksel;
			*divr = n94x_common.syscon + syscon_emvsim1clkdiv;
			break;

		case pctl_cmp0:
			*selr = n94x_common.syscon + syscon_cmp0fclksel;
			*divr = n94x_common.syscon + syscon_cmp0fclkdiv;
			break;

		case pctl_cmp1:
			*selr = n94x_common.syscon + syscon_cmp1fclksel;
			*divr = n94x_common.syscon + syscon_cmp1fclkdiv;
			break;

		case pctl_cmp2:
			*selr = n94x_common.syscon + syscon_cmp2fclksel;
			*divr = n94x_common.syscon + syscon_cmp2fclkdiv;
			break;

		case pctl_cmp0rr:
			*selr = n94x_common.syscon + syscon_cmp0rrclksel;
			break;

		case pctl_cmp1rr:
			*selr = n94x_common.syscon + syscon_cmp1rrclksel;
			break;

		case pctl_cmp2rr:
			*selr = n94x_common.syscon + syscon_cmp2rrclksel;
			break;

		case pctl_fc0:
			*selr = n94x_common.syscon + syscon_fc0clksel;
			*divr = n94x_common.syscon + syscon_flexcomm0clkdiv;
			break;

		case pctl_fc1:
			*selr = n94x_common.syscon + syscon_fc1clksel;
			*divr = n94x_common.syscon + syscon_flexcomm1clkdiv;
			break;

		case pctl_fc2:
			*selr = n94x_common.syscon + syscon_fc2clksel;
			*divr = n94x_common.syscon + syscon_flexcomm2clkdiv;
			break;

		case pctl_fc3:
			*selr = n94x_common.syscon + syscon_fc3clksel;
			*divr = n94x_common.syscon + syscon_flexcomm3clkdiv;
			break;

		case pctl_fc4:
			*selr = n94x_common.syscon + syscon_fc4clksel;
			*divr = n94x_common.syscon + syscon_flexcomm4clkdiv;
			break;

		case pctl_fc5:
			*selr = n94x_common.syscon + syscon_fc5clksel;
			*divr = n94x_common.syscon + syscon_flexcomm5clkdiv;
			break;

		case pctl_fc6:
			*selr = n94x_common.syscon + syscon_fc6clksel;
			*divr = n94x_common.syscon + syscon_flexcomm6clkdiv;
			break;

		case pctl_fc7:
			*selr = n94x_common.syscon + syscon_fc7clksel;
			*divr = n94x_common.syscon + syscon_flexcomm7clkdiv;
			break;

		case pctl_fc8:
			*selr = n94x_common.syscon + syscon_fc8clksel;
			*divr = n94x_common.syscon + syscon_flexcomm8clkdiv;
			break;

		case pctl_fc9:
			*selr = n94x_common.syscon + syscon_fc9clksel;
			*divr = n94x_common.syscon + syscon_flexcomm9clkdiv;
			break;

		case pctl_flexcan0:
			*selr = n94x_common.syscon + syscon_flexcan0clksel;
			*divr = n94x_common.syscon + syscon_flexcan0clkdiv;
			break;

		case pctl_flexcan1:
			*selr = n94x_common.syscon + syscon_flexcan1clksel;
			*divr = n94x_common.syscon + syscon_flexcan1clkdiv;
			break;

		case pctl_flexio:
			*selr = n94x_common.syscon + syscon_flexioclksel;
			*divr = n94x_common.syscon + syscon_flexioclkdiv;
			break;

		case pctl_usdhc:
			*selr = n94x_common.syscon + syscon_usdhcclksel;
			*divr = n94x_common.syscon + syscon_usdhcclkdiv;
			break;

		case pctl_sinc:
			*selr = n94x_common.syscon + syscon_sincfiltclksel;
			break;

		case pctl_i3c0:
			*selr = n94x_common.syscon + syscon_i3c0fclksel;
			*divr = n94x_common.syscon + syscon_i3c0fclkdiv;
			break;

		case pctl_i3c1:
			*selr = n94x_common.syscon + syscon_i3c1fclksel;
			*divr = n94x_common.syscon + syscon_i3c1fclkdiv;
			break;

		case pctl_i3c0s:
			*selr = n94x_common.syscon + syscon_i3c0fclkssel;
			*divr = n94x_common.syscon + syscon_i3c0fclksdiv;
			break;

		case pctl_i3c1s:
			*selr = n94x_common.syscon + syscon_i3c1fclkssel;
			*divr = n94x_common.syscon + syscon_i3c1fclksdiv;
			break;

		case pctl_i3c0stc:
			*selr = n94x_common.syscon + syscon_i3c0fclkstcsel;
			*divr = n94x_common.syscon + syscon_i3c0fclkstcdiv;
			break;

		case pctl_i3c1stc:
			*selr = n94x_common.syscon + syscon_i3c1fclkstcsel;
			*divr = n94x_common.syscon + syscon_i3c1fclkstcdiv;
			break;

		case pctl_sai0:
			*selr = n94x_common.syscon + syscon_sai0clksel;
			*divr = n94x_common.syscon + syscon_sai0clkdiv;
			break;

		case pctl_sai1:
			*selr = n94x_common.syscon + syscon_sai1clksel;
			*divr = n94x_common.syscon + syscon_sai1clkdiv;
			break;

		/* enet TODO */

		case pctl_micfil:
			*selr = n94x_common.syscon + syscon_micfilfclksel;
			*divr = n94x_common.syscon + syscon_micfilfclkdiv;
			break;

		case pctl_tsi:
			*selr = n94x_common.syscon + syscon_tsiclksel;
			*divr = n94x_common.syscon + syscon_tsiclkdiv;
			break;

		default:
			break;
	}

	return 0;
}


static void _mcxn94x_sysconSetDevClkState(int dev, int enable)
{
	hal_cpuDataMemoryBarrier();
	if (enable != 0) {
		/* cmp0 and cmp1 fields are "reserved", let's try to contol them anyway */
		*(n94x_common.syscon + syscon_ahbclkctrlset0 + (dev / 32)) = 1 << (dev & 0x1f);
	}
	else {
		*(n94x_common.syscon + syscon_ahbclkctrlclr0 + (dev / 32)) = 1 << (dev & 0x1f);
	}
	hal_cpuDataMemoryBarrier();
}


int _mcxn94x_sysconSetDevClk(int dev, unsigned int sel, unsigned int div, int enable)
{
	volatile u32 *selr = NULL, *divr = NULL;

	if (_mcxn94x_sysconGetRegs(dev, &selr, &divr) < 0) {
		return -1;
	}

	/* Disable the clock */
	_mcxn94x_sysconSetDevClkState(dev, 0);

	if (selr != NULL) {
		*selr = sel & 0x7;
	}

	if (divr != NULL) {
		*divr = div & 0xff;

		/* Unhalt the divider */
		*divr &= ~(1 << 30);
	}

	_mcxn94x_sysconSetDevClkState(dev, enable);

	return 0;
}


static int _mcxn94x_sysconDevReset(int dev, int state)
{
	volatile u32 *reg = n94x_common.syscon + syscon_presetctrl0;

	if ((dev < pctl_rom) || (dev > pctl_sema42)) {
		return -1;
	}

	/* Select relevant AHB register */
	reg += dev / 32;

	/* Need to disable the clock before the reset */
	if (state != 0) {
		_mcxn94x_sysconSetDevClkState(dev, 0);
	}

	if (state != 0) {
		*(reg + (syscon_presetctrlset0 - syscon_presetctrl0)) = 1 << (dev & 0x1f);
	}
	else {
		*(reg + (syscon_presetctrlcrl0 - syscon_presetctrl0)) = 1 << (dev & 0x1f);
	}
	hal_cpuDataMemoryBarrier();

	return 0;
}


void _n94x_init(void)
{
	int i;
	static u32 flexcommConf[] = { FLEXCOMM0_SEL, FLEXCOMM1_SEL, FLEXCOMM2_SEL,
		FLEXCOMM3_SEL, FLEXCOMM4_SEL, FLEXCOMM5_SEL, FLEXCOMM6_SEL, FLEXCOMM7_SEL,
		FLEXCOMM8_SEL, FLEXCOMM9_SEL };

	n94x_common.syscon = (void *)0x40000000;
	n94x_common.flexcomm[0] = (void *)0x40092000;
	n94x_common.flexcomm[1] = (void *)0x40093000;
	n94x_common.flexcomm[2] = (void *)0x40094000;
	n94x_common.flexcomm[3] = (void *)0x40095000;
	n94x_common.flexcomm[4] = (void *)0x400b4000;
	n94x_common.flexcomm[5] = (void *)0x400b5000;
	n94x_common.flexcomm[6] = (void *)0x400b6000;
	n94x_common.flexcomm[7] = (void *)0x400b7000;
	n94x_common.flexcomm[8] = (void *)0x400b8000;
	n94x_common.flexcomm[9] = (void *)0x400b9000;

	/* Configure FlexComms */
	for (i = 0; i < sizeof(flexcommConf) / sizeof(*flexcommConf); ++i) {
		if (flexcommConf[i] == FLEXCOMM_NONE) {
			continue;
		}

		/* Enable FlexComms clock and select FRO_12M source */
		_mcxn94x_sysconSetDevClk(pctl_fc0 + i, 2, 0, 1);

		/* Select active interface */
		*(n94x_common.flexcomm[i] + flexcomm_pselid) = flexcommConf[i] & 0x7;
	}
	hal_cpuDataMemoryBarrier();

	/* Configure OSTIMER clock */
	/* Use xtal32k clock source, enable the clock, deassert reset */
	_mcxn94x_sysconSetDevClk(pctl_ostimer, 1, 0, 1);
	_mcxn94x_sysconDevReset(pctl_ostimer, 0);

	/* MCXTODO */
}
