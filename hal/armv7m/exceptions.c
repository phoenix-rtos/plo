/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Exception handling
 *
 * Copyright 2017, 2023 Phoenix Systems
 * Author: Pawel Pisarczyk, Jakub Sejdak, Aleksander Kaminski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <hal/hal.h>

#define SIZE_FPUCTX (16 * sizeof(u32))

#if defined(__CPU_IMXRT106X) || defined(__CPU_IMXRT105X) || defined(__CPU_IMXRT117X)
#define EXCRET_PSP 0xffffffed
#else
#define EXCRET_PSP 0xfffffffd
#endif


struct cpuContext {
	u32 r0;
	u32 r1;
	u32 r2;
	u32 r3;
	u32 r12;
	u32 lr;
	u32 pc;
	u32 psr;
};


struct excContext {
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
	struct cpuContext mspctx;
};


__attribute__((section(".noxip"))) void hal_exceptionsDumpContext(char *buff, struct excContext *ctx, int n)
{
	static const char *const mnemonics[] = {
		"0 #InitialSP", "1 #Reset", "2 #NMI", "3 #HardFault",
		"4 #MemMgtFault", "5 #BusFault", "6 #UsageFault", "7 #",
		"8 #", "9 #", "10 #", "11 #SVC",
		"12 #Debug", "13 #", "14 #PendSV", "15 #SysTick"
	};

	struct cpuContext *hwctx;
	u32 msp = (u32)ctx + sizeof(*ctx);
	u32 psp = ctx->psp;
	const u32 fpucheck = (*(u32 *)0xe000ed88) & ((3uL << 20) | (3uL << 22));
	char *ptr = buff;

	n &= 0xf;

	/* If we came from userspace HW ctx in on psp stack */
	if (ctx->excret == EXCRET_PSP) {
		hwctx = (void *)ctx->psp;
		msp -= sizeof(*hwctx);
		psp += sizeof(*hwctx);
		if (fpucheck != 0) {
			psp += SIZE_FPUCTX;
		}
	}
	else {
		hwctx = &ctx->mspctx;
		if (fpucheck != 0) {
			msp += SIZE_FPUCTX;
		}
	}

	hal_strcpy(ptr, "\nException: ");
	ptr += hal_strlen(ptr);

	hal_strcpy(ptr, mnemonics[n]);
	ptr += hal_strlen(ptr);

	ptr += hal_i2s("\n r0=", ptr, hwctx->r0, 16, 1);
	ptr += hal_i2s("  r1=", ptr, hwctx->r1, 16, 1);
	ptr += hal_i2s("  r2=", ptr, hwctx->r2, 16, 1);
	ptr += hal_i2s("  r3=", ptr, hwctx->r3, 16, 1);

	ptr += hal_i2s("\n r4=", ptr, ctx->r4, 16, 1);
	ptr += hal_i2s("  r5=", ptr, ctx->r5, 16, 1);
	ptr += hal_i2s("  r6=", ptr, ctx->r6, 16, 1);
	ptr += hal_i2s("  r7=", ptr, ctx->r7, 16, 1);

	ptr += hal_i2s("\n r8=", ptr, ctx->r8, 16, 1);
	ptr += hal_i2s("  r9=", ptr, ctx->r9, 16, 1);
	ptr += hal_i2s(" r10=", ptr, ctx->r10, 16, 1);
	ptr += hal_i2s(" r11=", ptr, ctx->r11, 16, 1);

	ptr += hal_i2s("\nr12=", ptr, hwctx->r12, 16, 1);
	ptr += hal_i2s(" psr=", ptr, hwctx->psr, 16, 1);
	ptr += hal_i2s("  lr=", ptr, hwctx->lr, 16, 1);
	ptr += hal_i2s("  pc=", ptr, hwctx->pc, 16, 1);

	ptr += hal_i2s("\npsp=", ptr, psp, 16, 1);
	ptr += hal_i2s(" msp=", ptr, msp, 16, 1);
	ptr += hal_i2s(" exr=", ptr, ctx->excret, 16, 1);
	ptr += hal_i2s(" bfa=", ptr, *(u32 *)0xe000ed38, 16, 1);

	*(ptr++) = '\n';
	*ptr = '\0';
}


__attribute__((section(".noxip"))) void hal_exceptionsDispatch(unsigned int n, struct excContext *ctx)
{
	static char buff[512];

	hal_exceptionsDumpContext(buff, ctx, n);
	hal_consolePrint(buff);

#ifdef NDEBUG
	hal_cpuReboot();
#endif

	for (;;) {
		hal_cpuHalt();
	}
}
