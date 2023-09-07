/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Exception handling
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


typedef struct _exc_context_t {
	u64 savesp;

	u64 ra;
	u64 sp;
	u64 gp;
	u64 tp;

	u64 t0;
	u64 t1;
	u64 t2;

	u64 s0;
	u64 s1;

	u64 a0;
	u64 a1;
	u64 a2;
	u64 a3;
	u64 a4;
	u64 a5;
	u64 a6;
	u64 a7;

	u64 s2;
	u64 s3;
	u64 s4;
	u64 s5;
	u64 s6;
	u64 s7;
	u64 s8;
	u64 s9;
	u64 s10;
	u64 s11;

	u64 t3;
	u64 t4;
	u64 t5;
	u64 t6;

	u64 sstatus;
	u64 sepc;
	u64 sbadaddr;
	u64 scause;
} exc_context_t;


void hal_exceptionsDumpContext(char *buff, exc_context_t *ctx, int n)
{
	unsigned int i = 0;

	static const char *mnemonics[] = {
		"0 Instruction address missaligned", "1 Instruction access fault", "2 Illegal instruction", "3 Breakpoint",
		"4 Reserved", "5 Load access fault", "6 AMO address misaligned", "7 Store/AMO access fault",
		"8 Environment call", "9 Reserved", "10 Reserved", "11 Reserved",
		"12 Instruction page fault", "13 Load page fault", "14 Reserved", "15 Store/AMO page fault"
	};

	n &= 0xf;

	hal_strcpy(buff, "\nException: ");
	hal_strcpy(buff += hal_strlen(buff), mnemonics[n]);
	hal_strcpy(buff += hal_strlen(buff), "\n");
	buff += hal_strlen(buff);

	i += hal_i2s("zero: ", &buff[i], 0, 16, 1);
	i += hal_i2s("  ra : ", &buff[i], (u64)ctx->ra, 16, 1);
	i += hal_i2s("   sp : ", &buff[i], (u64)ctx->sp, 16, 1);
	i += hal_i2s("   gp : ", &buff[i], (u64)ctx->gp, 16, 1);
	buff[i++] = '\n';

	i += hal_i2s(" tp : ", &buff[i], (u64)ctx->tp, 16, 1);
	i += hal_i2s("  t0 : ", &buff[i], (u64)ctx->t0, 16, 1);
	i += hal_i2s("   t1 : ", &buff[i], (u64)ctx->t1, 16, 1);
	i += hal_i2s("   t2 : ", &buff[i], (u64)ctx->t2, 16, 1);
	buff[i++] = '\n';

	i += hal_i2s(" s0 : ", &buff[i], (u64)ctx->s0, 16, 1);
	i += hal_i2s("  s1 : ", &buff[i], (u64)ctx->s1, 16, 1);
	i += hal_i2s("   a0 : ", &buff[i], (u64)ctx->a0, 16, 1);
	i += hal_i2s("   a1 : ", &buff[i], (u64)ctx->a1, 16, 1);
	buff[i++] = '\n';

	i += hal_i2s(" a2 : ", &buff[i], (u64)ctx->a2, 16, 1);
	i += hal_i2s("  a3 : ", &buff[i], (u64)ctx->a3, 16, 1);
	i += hal_i2s("   a4 : ", &buff[i], (u64)ctx->a4, 16, 1);
	i += hal_i2s("   a5 : ", &buff[i], (u64)ctx->a5, 16, 1);
	buff[i++] = '\n';

	i += hal_i2s(" a6 : ", &buff[i], (u64)ctx->a6, 16, 1);
	i += hal_i2s("  a7 : ", &buff[i], (u64)ctx->a7, 16, 1);
	i += hal_i2s("   s2 : ", &buff[i], (u64)ctx->s2, 16, 1);
	i += hal_i2s("   s3 : ", &buff[i], (u64)ctx->s3, 16, 1);
	buff[i++] = '\n';

	i += hal_i2s(" s4 : ", &buff[i], (u64)ctx->s4, 16, 1);
	i += hal_i2s("  s5 : ", &buff[i], (u64)ctx->s5, 16, 1);
	i += hal_i2s("   s6 : ", &buff[i], (u64)ctx->s6, 16, 1);
	i += hal_i2s("   s7 : ", &buff[i], (u64)ctx->s7, 16, 1);
	buff[i++] = '\n';

	i += hal_i2s(" s8 : ", &buff[i], (u64)ctx->s8, 16, 1);
	i += hal_i2s("  s9 : ", &buff[i], (u64)ctx->s9, 16, 1);
	i += hal_i2s("  s10 : ", &buff[i], (u64)ctx->s10, 16, 1);
	i += hal_i2s("  s11 : ", &buff[i], (u64)ctx->s11, 16, 1);
	buff[i++] = '\n';

	i += hal_i2s(" t3 : ", &buff[i], (u64)ctx->t3, 16, 1);
	i += hal_i2s("  t4 : ", &buff[i], (u64)ctx->t4, 16, 1);
	i += hal_i2s("   t5 : ", &buff[i], (u64)ctx->t5, 16, 1);
	i += hal_i2s("   t6 : ", &buff[i], (u64)ctx->t6, 16, 1);
	buff[i++] = '\n';

	i += hal_i2s(" sstatus : ", &buff[i], (u64)ctx->sstatus, 16, 1);
	i += hal_i2s(" sepc : ", &buff[i], (u64)ctx->sepc, 16, 1);
	i += hal_i2s(" sbaddaddr : ", &buff[i], (u64)ctx->sbadaddr, 16, 1);
	i += hal_i2s(" scause : ", &buff[i], (u64)ctx->scause, 16, 1);

	buff[i++] = '\n';

	buff[i] = 0;
}


void exceptions_dispatch(unsigned int n, exc_context_t *ctx)
{
	char buff[512];

	hal_exceptionsDumpContext(buff, ctx, n);
	hal_consolePrint(buff);

#ifdef NDEBUG
	hal_cpuReboot();
#endif

	for (;;) {
		hal_cpuHalt();
	}
}
