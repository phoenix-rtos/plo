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

#define LDO_VOUT_1V   2
#define LDO_VOUT_1V05 3
#define LDO_VOUT_1V1  4
#define LDO_VOUT_1V15 5
#define LDO_VOUT_1V2  6
#define LDO_VOUT_1V25 7


/* clang-format off */
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
	syscon_cmp0fclksel = 380, syscon_cmp0fclkdiv, syscon_cmp0rrclksel, syscon_cmp0rrclkdiv,
	syscon_cmp1fclksel, syscon_cmp1fclkdiv, syscon_cmp1rrclksel, syscon_cmp1rrclkdiv,
	syscon_cmp2fclksel, syscon_cmp2fclkdiv, syscon_cmp2rrclksel, syscon_cmp2rrclkdiv,
	syscon_cpuctrl = 512, syscon_cpboot, syscon_cpustat = 514, syscon_lpcacctrl = 521,
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


enum {
	scg_verid = 0, scg_param, scg_trimlock, scg_csr = 4, scg_rccr, scg_sosccsr = 64,
	scg_sosccfg = 66, scg_sirccsr = 128, scg_sirctcfg = 131, scg_sirctrim, scg_sircstat = 134,
	scg_firccsr = 192, scg_firccfg = 194, scg_firctrim, scg_fircstat = 198, scg_rosccsr = 256,
	scg_apllcsr = 320, scg_apllctrl, scg_apllstat, scg_apllndiv, scg_apllmdiv, scg_apllpdiv,
	scg_aplllockcnfg, scg_apllsscgstat, scg_apllsscg0, scg_apllsscg1, scg_apllovrd = 381,
	scg_spllcsr = 384, scg_spllctrl, scg_spllstat, scg_spllndiv, scg_spllmdiv, scg_spllpdiv,
	scg_spllockcnfg, scg_spllsscgstat, scg_spllsscg0, scg_spllsscg1, scg_spllovrd = 445,
	scg_upllcsr = 448, scg_ldocsr = 512
};


enum {
	port_verid = 0, port_gpclr = 4, port_gpchr, port_config = 8, port_edfr = 16, port_edier,
	port_edcr, port_calib0 = 24, port_calib1, port_pcr0 = 32, port_pcr1, port_pcr2, port_pcr3,
	port_pcr4, port_pcr5, port_pcr6, port_pcr7, port_pcr8, port_pcr9, port_pcr10, port_pcr11,
	port_pcr12, port_pcr13, port_pcr14, port_pcr15, port_pcr16, port_pcr17, port_pcr18,
	port_pcr19, port_pcr20, port_pcr21, port_pcr22, port_pcr23, port_pcr24, port_pcr25,
	port_pcr26, port_pcr27, port_pcr28, port_pcr29, port_pcr30, port_pcr31
};


enum {
	inputmux_sct0inmux0 = 0, inputmux_sct0inmux1, inputmux_sct0inmux2, inputmux_sct0inmux3,
	inputmux_sct0inmux4, inputmux_sct0inmux5, inputmux_sct0inmux6, inputmux_sct0inmux7,
	inputmux_ctimer0cap0, inputmux_ctimer0cap1, inputmux_ctimer0cap2, inputmux_ctimer0cap3,
	inputmux_timer0trig, inputmux_ctimer1cap0 = 16, inputmux_ctimer1cap1, inputmux_ctimer1cap2,
	inputmux_ctimer1cap3, inputmux_timer1trig, inputmux_ctimer2cap0 = 24, inputmux_ctimer2cap1,
	inputmux_ctimer2cap2, inputmux_ctimer2cap3, inputmux_timer2trig, inputmux_smartdmaarchbinmux0 = 40,
	inputmux_smartdmaarchbinmux1, inputmux_smartdmaarchbinmux2, inputmux_smartdmaarchbinmux3,
	inputmux_smartdmaarchbinmux4, inputmux_smartdmaarchbinmux5, inputmux_smartdmaarchbinmux6,
	inputmux_smartdmaarchbinmux7, inputmux_pintsel0, inputmux_pintsel1, inputmux_pintsel2,
	inputmux_pintsel3, inputmux_pintsel4, inputmux_pintsel5, inputmux_pintsel6, inputmux_pintsel7,
	inputmux_freqmesref = 96, inputmux_freqmeastar, inputmux_ctimer3cap0 = 105, inputmux_ctimer3cap1,
	inputmux_ctimer3cap2, inputmux_ctimer3cap3, inputmux_timer3trig, inputmux_ctimer4cap0 = 112,
	inputmux_ctimer4cap1, inputmux_ctimer4cap2, inputmux_ctimer4cap3, inputmux_timer4trig,
	inputmux_cmp0trig = 152, inputmux_adc0trig0 = 160, inputmux_adc0trig1, inputmux_adc0trig2,
	inputmux_adc0trig3, inputmux_adc1trig0 = 176, inputmux_adc1trig1, inputmux_adc1trig2,
	inputmux_adc1trig3, inputmux_dac0trig = 192, inputmux_dac1trig = 200, inputmux_dac2trig = 208,
	inputmux_enc0trig = 216, inputmux_enc0home, inputmux_enc0index, inputmux_enc0phaseb,
	inputmux_enc0phasea, inputmux_enc1trig = 224, inputmux_enc1home, inputmux_enc1index,
	inputmux_enc1phaseb, inputmux_enc1phasea, inputmux_flexpwm0sm0extsync = 232,
	inputmux_flexpwm0sm1extsync, inputmux_flexpwm0sm2extsync, inputmux_flexpwm0sm3extsync,
	inputmux_flexpwm0sm0exta, inputmux_flexpwm0sm1exta, inputmux_flexpwm0sm2exta,
	inputmux_flexpwm0sm3exta, inputmux_flexpwm0extforce, inputmux_flexpwm0fault0,
	inputmux_flexpwm0fault1, inputmux_flexpwm0fault2, inputmux_flexpwm0fault3,
	inputmux_flexpwm1sm0extsync = 248, inputmux_flexpwm1sm1extsync, inputmux_flexpwm1sm2extsync,
	inputmux_flexpwm1sm3extsync, inputmux_flexpwm1sm0exta, inputmux_flexpwm1sm1exta,
	inputmux_flexpwm1sm2exta, inputmux_flexpwm1sm3exta, inputmux_flexpwm1extforce,
	inputmux_flexpwm1fault0, inputmux_flexpwm1fault1, inputmux_flexpwm1fault2,
	inputmux_flexpwm1fault3, inputmux_pwm0extclk = 264, inputmux_pwm1extclk,
	inputmux_evtgtrig0 = 272, inputmux_evtgtrig1, inputmux_evtgtrig2, inputmux_evtgtrig3,
	inputmux_evtgtrig4, inputmux_evtgtrig5, inputmux_evtgtrig6, inputmux_evtgtrig7,
	inputmux_evtgtrig8, inputmux_evtgtrig9, inputmux_evtgtrig10, inputmux_evtgtrig11,
	inputmux_evtgtrig12, inputmux_evtgtrig13, inputmux_evtgtrig14, inputmux_evtgtrig15,
	inputmux_usbfstrig, inputmux_tsitrig = 296, inputmux_exttrig0 = 304, inputmux_exttrig1,
	inputmux_exttrig2, inputmux_exttrig3, inputmux_exttrig4, inputmux_exttrig5, inputmux_exttrig6,
	inputmux_exttrig7, inputmux_cmp1trig = 312, inputmux_cmp2trig = 320,
	inputmux_sincfilterch0 = 328, inputmux_sincfilterch1, inputmux_sincfilterch2,
	inputmux_sincfilterch3, inputmux_sincfilterch4, inputmux_opamp0trig = 352,
	inputmux_opamp1trig, inputmux_opamp2trig, inputmux_flexcomm0trig = 360,
	inputmux_flexcomm1trig = 368, inputmux_flexcomm2trig = 376, inputmux_flexcomm3trig = 384,
	inputmux_flexcomm4trig = 392, inputmux_flexcomm5trig = 400, inputmux_flexcomm6trig = 408,
	inputmux_flexcomm7trig = 416, inputmux_flexcomm8trig = 424, inputmux_flexcomm9trig = 436,
	inputmux_flexiotrig0 = 440, inputmux_flexiotrig1, inputmux_flexiotrig2, inputmux_flexiotrig3,
	inputmux_flexiotrig4, inputmux_flexiotrig5, inputmux_flexiotrig6, inputmux_flexiotrig7,
	inputmux_dma0reqenable0 = 448, inputmux_dma0reqenable0set, inputmux_dma0reqenable0clr,
	inputmux_dma0reqenable0tog, inputmux_dma0reqenable1, inputmux_dma0reqenable1set,
	inputmux_dma0reqenable1clr, inputmux_dma0reqenable1tog, inputmux_dma0reqenable2,
	inputmux_dma0reqenable2set, inputmux_dma0reqenable2clr,inputmux_dma0reqenable2tog,
	inputmux_dma0reqenable3, inputmux_dma0reqenable3set, inputmux_dma0reqenable3clr,
	inputmux_dma1reqenable0 = 480, inputmux_dma1reqenable0set, inputmux_dma1reqenable0clr,
	inputmux_dma1reqenable0tog, inputmux_dma1reqenable1, inputmux_dma1reqenable1set,
	inputmux_dma1reqenable1clr, inputmux_dma1reqenable1tog, inputmux_dma1reqenable2,
	inputmux_dma1reqenable2set, inputmux_dma1reqenable2clr, inputmux_dma1reqenable2tog,
	inputmux_dma1reqenable3, inputmux_dma1reqenable3set, inputmux_dma1reqenable3clr
};


enum {
	vbat_verid = 0, vbat_statusa = 4, vbat_statusb, vbat_irqena, vbat_irqenb, vbat_wakena,
	vbat_wakenb, vbat_wakecfg = 14, vbat_oscctla = 64, vbat_oscctlb, vbat_osccfga,
	vbat_osccfgb, vbat_osclcka = 70, vbat_osclckb, vbat_oscclke, vbat_froctla = 128,
	vbat_froctlb, vbat_frolcka = 134, vbat_frolckb, vbat_froclke, vbat_ldoctla = 192,
	vbat_ldoctlb, vbat_ldolcka = 198, vbat_ldolckb, vbat_ldoramc, vbat_ldotimer0 = 204,
	vbat_ldotimer1 = 206, vbat_wakeup0a = 448, vbat_wakeup0b, vbat_wakeup1a, vbat_wakeup1b,
	vbat_waklcka = 510, vbat_waklckb
};


enum {
	spc_verid = 0, spc_sc = 4, spc_cntrl, spc_lpreqcfg = 7, spc_pdstatus0 = 12, spc_pdstatus1,
	spc_sramctl = 16, spc_activecfg = 64, spc_activecgfg1, spc_lpcfg, spc_lpcfg1, spc_lpwkupdelay = 72,
	spc_activevdelay, spc_vdstat = 76, spc_vdcorecfg, spc_vdsyscfg, spc_iocfg, spc_evdcfg,
	spc_glitchdetectsc, spc_coreldocfg = 192, spc_sysldocfg = 256, spc_dcdccfg = 320, spc_dcdcburstcfg
};


enum {
	fmu_fstat = 0, fmu_fcnfg, fmu_fctlr, fmu_fcc0b0, fmu_fcc0b1, fmu_fcc0b2, fmu_fcc0b3, fmu_fcc0b4,
	fmu_fcc0b5, fmu_fcc0b6, fmu_fcc0b7
};


enum {
	itrc_status = 0, itrc_status1, itrc_outsel, itrc_outsel1 = 24, itrc_outsel2 = 46,
	itrc_swevent0 = 94, itrc_swevent1
};


enum {
	gdet_conf0 = 0, gdet_conf1, gdet_enable1, gdet_conf2, gdet_conf3, gdet_conf4, gdet_conf5,
	gdet_reset = 4010, gdet_test, gdet_dlyctrl = 4016
};
/* clang-format on */


static struct {
	volatile u32 *port[6];
	volatile u32 *inputmux;
	volatile u32 *syscon;
	volatile u32 *flexcomm[10];
	volatile u32 *vbat;
	volatile u32 *scg;
	volatile u32 *spc;
	volatile u32 *fmu;
	volatile u32 *gdet0;
	volatile u32 *gdet1;
	volatile u32 *itrc;
} n94x_common;


int _mcxn94x_portPinConfig(int pin, int mux, int options)
{
	int port = pin / 32;

	pin %= 32;

	if (port >= (sizeof(n94x_common.port) / sizeof(*n94x_common.port))) {
		return -EINVAL;
	}

	*(n94x_common.port[port] + port_pcr0 + pin) = (((mux & 0xf) << 8) | (options & 0x307f));

	return 0;
}


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
			*divr = n94x_common.syscon + syscon_cmp0rrclkdiv;
			break;

		case pctl_cmp1rr:
			*selr = n94x_common.syscon + syscon_cmp1rrclksel;
			*divr = n94x_common.syscon + syscon_cmp1rrclkdiv;
			break;

		case pctl_cmp2rr:
			*selr = n94x_common.syscon + syscon_cmp2rrclksel;
			*divr = n94x_common.syscon + syscon_cmp2rrclkdiv;
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
	int reg = dev >> 5;
	int bit = dev & 0x1f;

	hal_cpuDataMemoryBarrier();
	if (enable != 0) {
		/* Some fields are "reserved", let's try to control them anyway */
		*(n94x_common.syscon + syscon_ahbclkctrlset0 + reg) = 1u << bit;
	}
	else {
		*(n94x_common.syscon + syscon_ahbclkctrlclr0 + reg) = 1u << bit;
	}
	hal_cpuDataMemoryBarrier();
}


int _mcxn94x_sysconSetDevClk(int dev, unsigned int sel, unsigned int div, int enable)
{
	volatile u32 *selr = NULL, *divr = NULL;

	if (_mcxn94x_sysconGetRegs(dev, &selr, &divr) < 0) {
		return -EINVAL;
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


int _mcxn94x_sysconDevReset(int dev, int state)
{
	int reg = dev / 32;
	int bit = dev & 0x1f;

	if ((dev < pctl_rom) || (dev > pctl_sema42)) {
		return -EINVAL;
	}

	if (state != 0) {
		*(n94x_common.syscon + syscon_presetctrlset0 + reg) = 1 << bit;
	}
	else {
		*(n94x_common.syscon + syscon_presetctrlcrl0 + reg) = 1 << bit;
	}
	hal_cpuDataMemoryBarrier();

	return 0;
}


u64 _mcxn94x_sysconGray2Bin(u64 gray)
{
	u64 ret;

	*(n94x_common.syscon + syscon_graycodelsb) = gray & 0xffffffff;
	*(n94x_common.syscon + syscon_graycodemsb) = gray >> 32;
	hal_cpuDataMemoryBarrier();

	ret = *(n94x_common.syscon + syscon_binarycodelsb);
	ret |= ((u64)*(n94x_common.syscon + syscon_binarycodemsb)) << 32;

	return ret;
}


static void _mcxn94x_scgTrimUnlock(void)
{
	*(n94x_common.scg + scg_trimlock) = 0x5a5a0001;
	hal_cpuDataMemoryBarrier();
}


static void _mcxn94x_scgTrimLock(void)
{
	hal_cpuDataMemoryBarrier();
	*(n94x_common.scg + scg_trimlock) = 0x5a5a0000;
}


static void _mcxn94x_scgLdoConfig(u8 vout)
{
	u32 t;

	_mcxn94x_scgTrimUnlock();
	t = *(n94x_common.scg + scg_ldocsr) & ~0x1f;
	*(n94x_common.scg + scg_ldocsr) = t | ((vout & 0x7) << 1) | 1;
	hal_cpuDataMemoryBarrier();
	while ((*(n94x_common.scg + scg_ldocsr) & (1u << 31)) == 0) {
	}
	_mcxn94x_scgTrimLock();
}


static void _mcxn94x_clockConfigOsc32khz(u8 extal, u8 cap, u8 gain)
{
	static const u32 mode = 0; /* Normal mode */
	u32 t;

	/* Setup oscillator startup time first (0.5 s) */
	t = *(n94x_common.vbat + vbat_osccfga) & ~(0x7 << 9);
	*(n94x_common.vbat + vbat_osccfga) = t | (0x4 << 9);
	hal_cpuDataMemoryBarrier();
	*(n94x_common.vbat + vbat_osccfgb) = ~(*(n94x_common.vbat + vbat_osccfga));
	hal_cpuDataMemoryBarrier();

	/* Set oscillator to normal mode - skipping init mode seems to work just fine */
	*(n94x_common.vbat + vbat_oscctla) = (mode << 16) | ((extal & 0xf) << 12) | ((cap & 0xf) << 8) | (1 << 7) | ((gain & 0x3) << 2) | 1;
	hal_cpuDataMemoryBarrier();
	*(n94x_common.vbat + vbat_oscctlb) = ~(*(n94x_common.vbat + vbat_oscctla));
	hal_cpuDataMemoryBarrier();
	while ((*(n94x_common.vbat + vbat_statusa) & (1 << 5)) == 0) {
	}

	*(n94x_common.vbat + vbat_oscclke) |= 0xf;
	hal_cpuDataMemoryBarrier();
	*(n94x_common.vbat + vbat_osclcka) |= 1;
	*(n94x_common.vbat + vbat_osclckb) &= ~1;
	hal_cpuDataMemoryBarrier();
}


static void _mcxn94x_clockConfigExtClk(u32 freq)
{
	u8 range;

	if (freq < (20 * 1000 * 1000)) {
		range = 0;
	}
	else if (freq < (30 * 1000 * 1000)) {
		range = 1;
	}
	else if (freq < (50 * 1000 * 1000)) {
		range = 2;
	}
	else {
		range = 3;
	}

	/* Clear errors if present */
	*(n94x_common.scg + scg_sosccsr) = (1 << 26);

	/* Set range and internal reference */
	*(n94x_common.scg + scg_sosccfg) = (range << 4) | (1 << 2);

	/* Unlock SOSCCSR */
	*(n94x_common.scg + scg_sosccsr) &= ~(1 << 23);
	hal_cpuDataMemoryBarrier();

	/* Enable clock and monitor */
	*(n94x_common.scg + scg_sosccsr) |= (1 << 17) | 1;
	hal_cpuDataMemoryBarrier();

	/* Wait for clock to stabilize */
	while ((*(n94x_common.scg + scg_sosccsr) & (1 << 24)) == 0) {
	}
}


static void _mcxn94x_configBandGap(void)
{
	u32 t;

	/* Wait if busy */
	while ((*(n94x_common.spc + spc_sc) & 1) != 0) {
	}

	/* Enable band-gap */
	t = *(n94x_common.spc + spc_activecfg) & ~(0x3 << 20);
	*(n94x_common.spc + spc_activecfg) = t | (1 << 20);
	hal_cpuDataMemoryBarrier();
}


static void _mcxn94x_configDCDC(void)
{
	u32 t;

	/* For now only normal strength, OD voltage */

	/* Wait if busy */
	while ((*(n94x_common.spc + spc_sc) & 1) != 0) {
	}

	/* Select normal strength */
	t = *(n94x_common.spc + spc_activecfg) & ~(0x3 << 8);
	*(n94x_common.spc + spc_activecfg) = t | (0x2 << 8);
	hal_cpuDataMemoryBarrier();

	/* Select OD voltage (1.2 V) */
	t = *(n94x_common.spc + spc_activecfg) & ~(0x3 << 10);
	*(n94x_common.spc + spc_activecfg) = t | (0x3 << 10);
	hal_cpuDataMemoryBarrier();

	/* Wait if busy */
	while ((*(n94x_common.spc + spc_sc) & 1) != 0) {
	}
}


static void _mcxn94x_configLDO(void)
{
	u32 t;

	/* For now only normal strength, OD voltage */

	/* Wait if busy */
	while ((*(n94x_common.spc + spc_sc) & 1) != 0) {
	}

	/* Select normal strength */
	*(n94x_common.spc + spc_activecfg) |= 1;
	hal_cpuDataMemoryBarrier();

	/* Select OD voltage (1.2 V) */
	t = *(n94x_common.spc + spc_activecfg) & ~(0x3 << 2);
	*(n94x_common.spc + spc_activecfg) = t | (0x3 << 2);
	hal_cpuDataMemoryBarrier();
}


static void _mcxn94x_configFlashWS(u8 ws)
{
	u32 t = *(n94x_common.fmu + fmu_fctlr) & ~(0xf);
	*(n94x_common.fmu + fmu_fctlr) = t | (ws & 0xf);
	hal_cpuDataMemoryBarrier();
}


static void _mcxn94x_configSramVoltage(u8 vsm)
{
	/* Set voltage */
	*(n94x_common.spc + spc_sramctl) = (vsm & 0x3);
	hal_cpuDataMemoryBarrier();

	/* Request change */
	*(n94x_common.spc + spc_sramctl) |= (1 << 30);
	hal_cpuDataMemoryBarrier();

	/* Wait for completion */
	while ((*(n94x_common.spc + spc_sramctl) & (1u << 31)) == 0) {
	}

	*(n94x_common.spc + spc_sramctl) &= ~(1 << 30);
	hal_cpuDataMemoryBarrier();
}


static u8 _mcxn94x_clockGetSeli(u32 m)
{
	u32 a;

	if (m >= 8000) {
		a = 1;
	}
	else if (m >= 122) {
		a = 8000 / m;
	}
	else {
		a = (2 * (m / 4)) + 3;
	}

	return (a > 63) ? 63 : a;
}


static u8 _mcxn94x_clockGetSelp(u32 m)
{
	u32 a = (m >> 2) + 1;
	return (a > 31) ? 31 : a;
}


static void _mcxn94x_clockConfigPLL0(u8 source, u8 mdiv, u8 pdiv, u8 ndiv)
{
	u8 seli = _mcxn94x_clockGetSeli(mdiv);
	u8 selp = _mcxn94x_clockGetSelp(mdiv);

	/* Power off during config */
	*(n94x_common.scg + scg_apllcsr) &= ~((1 << 1) | 1);
	hal_cpuDataMemoryBarrier();

	/* Configure parameters */
	*(n94x_common.scg + scg_apllctrl) = ((source & 0x3) << 25) | ((selp & 0x1f) << 10) | ((seli & 0x3f) << 4);
	*(n94x_common.scg + scg_apllndiv) = ndiv;
	hal_cpuDataMemoryBarrier();
	*(n94x_common.scg + scg_apllndiv) = ndiv | (1u << 31);
	*(n94x_common.scg + scg_apllpdiv) = pdiv;
	hal_cpuDataMemoryBarrier();
	*(n94x_common.scg + scg_apllpdiv) = pdiv | (1u << 31);
	*(n94x_common.scg + scg_apllmdiv) = mdiv;
	hal_cpuDataMemoryBarrier();
	*(n94x_common.scg + scg_apllmdiv) = mdiv | (1u << 31);
	hal_cpuDataMemoryBarrier();

	/* Config lock time */
	_mcxn94x_scgTrimUnlock();
	*(n94x_common.scg + scg_aplllockcnfg) = ((SOSC_FREQ / ndiv) * 500) + 300;
	_mcxn94x_scgTrimLock();

	/* Enable PLL0 */
	*(n94x_common.scg + scg_apllcsr) |= (1 << 1) | 1;
	hal_cpuDataMemoryBarrier();

	/* Wait for lock */
	while ((*(n94x_common.scg + scg_apllcsr) & (1 << 24)) == 0) {
	}

	/* Disable clock monitor */
	*(n94x_common.scg + scg_apllcsr) &= ~(1 << 16);
}


void _mcxn94x_disableGDET(void)
{
	u32 t;

	/* Disable aGDET trigger to the CHIP_RESET */
	t = *(n94x_common.itrc + itrc_outsel + (4 * 2)) & ~(0x3 << 18);
	*(n94x_common.itrc + itrc_outsel + (4 * 2)) = t | (0x2 << 18);
	t = *(n94x_common.itrc + itrc_outsel + (4 * 2) + 1) & ~(0x3 << 18);
	*(n94x_common.itrc + itrc_outsel + (4 * 2) + 1) = t | (0x2 << 18);
	hal_cpuDataMemoryBarrier();

	/* Disable aGDET interrupt and reset */
	*(n94x_common.spc + spc_activecfg) |= (1 << 12);
	*(n94x_common.spc + spc_glitchdetectsc) &= ~(1 << 16);
	hal_cpuDataMemoryBarrier();
	*(n94x_common.spc + spc_glitchdetectsc) = 0x3c;
	hal_cpuDataMemoryBarrier();
	*(n94x_common.spc + spc_glitchdetectsc) |= (1 << 16);

	/* Disable dGDET trigger to the CHIP_RESET */
	t = *(n94x_common.itrc + itrc_outsel + (4 * 2)) & ~0x3;
	*(n94x_common.itrc + itrc_outsel + (4 * 2)) = t | 0x2;
	t = *(n94x_common.itrc + itrc_outsel + (4 * 2) + 1) & ~0x3;
	*(n94x_common.itrc + itrc_outsel + (4 * 2) + 1) = t | 0x2;
	hal_cpuDataMemoryBarrier();

	*(n94x_common.gdet0 + gdet_enable1) = 0;
	*(n94x_common.gdet1 + gdet_enable1) = 0;
	hal_cpuDataMemoryBarrier();
}


void _mcxn94x_init(void)
{
	int i;
	static u32 flexcommConf[] = { FLEXCOMM0_SEL, FLEXCOMM1_SEL, FLEXCOMM2_SEL,
		FLEXCOMM3_SEL, FLEXCOMM4_SEL, FLEXCOMM5_SEL, FLEXCOMM6_SEL, FLEXCOMM7_SEL,
		FLEXCOMM8_SEL, FLEXCOMM9_SEL };

	n94x_common.syscon = (void *)0x40000000;
	n94x_common.port[0] = (void *)0x40116000;
	n94x_common.port[1] = (void *)0x40117000;
	n94x_common.port[2] = (void *)0x40118000;
	n94x_common.port[3] = (void *)0x40119000;
	n94x_common.port[4] = (void *)0x4011a000;
	n94x_common.port[5] = (void *)0x40042000;
	n94x_common.inputmux = (void *)0x40006000;
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
	n94x_common.vbat = (void *)0x40059000;
	n94x_common.scg = (void *)0x40044000;
	n94x_common.spc = (void *)0x40045000;
	n94x_common.fmu = (void *)0x40043000;
	n94x_common.gdet0 = (void *)0x40024000;
	n94x_common.gdet1 = (void *)0x40025000;
	n94x_common.itrc = (void *)0x40026000;

	_mcxn94x_disableGDET();

	/* Enable Mailbox */
	_mcxn94x_sysconSetDevClkState(pctl_mailbox, 1);

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

	/* Disable RAM ECC to free up RAMH bank */
	*(n94x_common.syscon + syscon_eccenablectrl) = 0;

	/* Enable PORTn clocks */
	for (i = 0; i < sizeof(n94x_common.port) / sizeof(*n94x_common.port) - 1; ++i) {
		/* PORT5 is reserved according to doc, let's enable it anyway */
		/* No sel/div for these, sel/div doesn't matter */
		_mcxn94x_sysconSetDevClk(pctl_port0 + i, 0, 0, 1);
	}

	/* Enable the flash cache LPCAC */
	*(n94x_common.syscon + syscon_lpcacctrl) &= ~1;

	/* Enable LDO */
	_mcxn94x_scgLdoConfig(LDO_VOUT_1V1);

	/* Configure 32 KHz crystal pins */
	_mcxn94x_portPinConfig(pctl_pin_p5_0, 0, MCX_PIN_FAST | MCX_PIN_INPUT_BUFFER_ENABLE);
	_mcxn94x_portPinConfig(pctl_pin_p5_1, 0, MCX_PIN_FAST | MCX_PIN_INPUT_BUFFER_ENABLE);

	/* Enable 32 KHz oscillator */
	_mcxn94x_clockConfigOsc32khz(ROSC_EXTALCAP_PF / 2, ROSC_CAP_PF / 2, ROSC_AMP_GAIN);

	/* Configure 24 MHz oscillator pins */
	_mcxn94x_portPinConfig(pctl_pin_p1_30, 0, MCX_PIN_FAST | MCX_PIN_INPUT_BUFFER_ENABLE);
	_mcxn94x_portPinConfig(pctl_pin_p1_31, 0, MCX_PIN_FAST | MCX_PIN_INPUT_BUFFER_ENABLE);

	/* Enable oscillator */
	_mcxn94x_clockConfigExtClk(SOSC_FREQ);

	/* Config power supply */
	_mcxn94x_configBandGap();
	_mcxn94x_configDCDC();
	_mcxn94x_configLDO();
	_mcxn94x_configSramVoltage(0x3);

	/* Config flash waitstates */
	_mcxn94x_configFlashWS(3);

	/* Enable PPL0 @150 MHz, SOSC source Fin = SOSC_FREQ = 24 MHz */
	/* Fout = Fin * m / (2 * p * n) */
	_mcxn94x_clockConfigPLL0(0, 25, 1, 2);

	/* Select PLL0 as a main clock */
	*(n94x_common.scg + scg_rccr) = (*(n94x_common.scg + scg_rccr) & ~(0xf << 24)) | 5 << 24;
	hal_cpuDataMemoryBarrier();

	/* Wait for clock to change */
	while (((*(n94x_common.scg + scg_csr) >> 24) & 0xf) != 5) {
	}

	/* Set AHB clock divider to clk / 1 */
	*(n94x_common.syscon + syscon_ahbclkdiv) = 0;
	hal_cpuDataMemoryBarrier();
}
