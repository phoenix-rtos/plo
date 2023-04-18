/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Exception handling
 *
 * Copyright 2017 Phoenix Systems
 * Author: Pawel Pisarczyk, Jakub Sejdak, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <hal/hal.h>

#define SIZE_FPUCTX (16 * sizeof(u32))

#ifdef CPU_IMXRT
#define EXCRET_PSP 0xffffffed
#else
#define EXCRET_PSP 0xfffffffd
#endif


typedef struct {
	u32 r0;
	u32 r1;
	u32 r2;
	u32 r3;
	u32 r12;
	u32 lr;
	u32 pc;
	u32 psr;
} cpu_hwContext_t;


typedef struct _exc_context_t {
	u32 psp;
	u32 r4;
	u32 r5;
	u32 r6;
	u32 r7;
	u32 r8;
	u32 r9;
	u32 r10;
	u32 r11;
	u32 excret;

	/* Saved by hardware */
	cpu_hwContext_t mspctx;
} exc_context_t;


__attribute__((section(".noxip"))) void hal_exceptionsDumpContext(char *buff, exc_context_t *ctx, int n)
{
	static const char *const mnemonics[] = {
		"0 #InitialSP", "1 #Reset", "2 #NMI", "3 #HardFault",
		"4 #MemMgtFault", "5 #BusFault", "6 #UsageFault", "7 #",
		"8 #", "9 #", "10 #", "11 #SVC",
		"12 #Debug", "13 #", "14 #PendSV", "15 #SysTick"
	};

	size_t i = 0;
	cpu_hwContext_t *hwctx;
	u32 msp = (u32)ctx + sizeof(*ctx);
	u32 psp = ctx->psp;

	/* If we came from userspace HW ctx in on psp stack */
	if (ctx->excret == EXCRET_PSP) {
		hwctx = (void *)ctx->psp;
		msp -= sizeof(cpu_hwContext_t);
		psp += sizeof(cpu_hwContext_t);
#ifdef CPU_IMXRT /* FIXME - check if FPU was enabled instead */
			psp += SIZE_FPUCTX;
#endif
	}
	else {
		hwctx = &ctx->mspctx;
#ifdef CPU_IMXRT
			msp += SIZE_FPUCTX;
#endif
	}

	n &= 0xf;

	hal_strcpy(buff, "\nException: ");
	hal_strcpy(buff += hal_strlen(buff), mnemonics[n]);
	hal_strcpy(buff += hal_strlen(buff), "\n");
	buff += hal_strlen(buff);

	i += hal_i2s(" r0=", &buff[i], hwctx->r0, 16, 1);
	i += hal_i2s("  r1=", &buff[i], hwctx->r1, 16, 1);
	i += hal_i2s("  r2=", &buff[i], hwctx->r2, 16, 1);
	i += hal_i2s("  r3=", &buff[i], hwctx->r3, 16, 1);

	i += hal_i2s("\n r4=", &buff[i], ctx->r4, 16, 1);
	i += hal_i2s("  r5=", &buff[i], ctx->r5, 16, 1);
	i += hal_i2s("  r6=", &buff[i], ctx->r6, 16, 1);
	i += hal_i2s("  r7=", &buff[i], ctx->r7, 16, 1);

	i += hal_i2s("\n r8=", &buff[i], ctx->r8, 16, 1);
	i += hal_i2s("  r9=", &buff[i], ctx->r9, 16, 1);
	i += hal_i2s(" r10=", &buff[i], ctx->r10, 16, 1);
	i += hal_i2s(" r11=", &buff[i], ctx->r11, 16, 1);

	i += hal_i2s("\nr12=", &buff[i], hwctx->r12, 16, 1);
	i += hal_i2s(" psr=", &buff[i], hwctx->psr, 16, 1);
	i += hal_i2s("  lr=", &buff[i], hwctx->lr, 16, 1);
	i += hal_i2s("  pc=", &buff[i], hwctx->pc, 16, 1);

	i += hal_i2s("\npsp=", &buff[i], psp, 16, 1);
	i += hal_i2s(" msp=", &buff[i], msp, 16, 1);
	i += hal_i2s(" exr=", &buff[i], ctx->excret, 16, 1);
	i += hal_i2s(" bfa=", &buff[i], *(u32 *)0xe000ed38, 16, 1);

	i += hal_i2s("\ncfs=", &buff[i], *(u32 *)0xe000ed28, 16, 1);

	buff[i++] = '\n';

	buff[i] = 0;
}


__attribute__((section(".noxip"))) void hal_exceptionsDispatch(unsigned int n, exc_context_t *ctx)
{
	static char buff[512];

	hal_exceptionsDumpContext(buff, ctx, n);
	hal_consolePrint(buff);

#ifdef NDEBUG
#ifdef CPU_STM32
	_stm32_nvicSystemReset();
#elif defined(CPU_IMXRT)
	_imxrt_nvicSystemReset();
#endif
#else
	hal_cpuHalt();
#endif
}
