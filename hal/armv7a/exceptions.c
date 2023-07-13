/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Exception handling
 *
 * Copyright 2017, 2018, 2021 Phoenix Systems
 * Author: Pawel Pisarczyk, Jakub Sejdak, Aleksander Kaminski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


typedef struct _exc_context_t {
	u32 savesp;

	u32 dfsr;
	u32 dfar;
	u32 ifsr;
	u32 ifar;

	u32 psr;

	u32 r0;
	u32 r1;
	u32 r2;
	u32 r3;
	u32 r4;
	u32 r5;
	u32 r6;
	u32 r7;
	u32 r8;
	u32 r9;
	u32 r10;

	u32 fp;
	u32 ip;
	u32 sp;
	u32 lr;

	u32 pc;
} exc_context_t;


static const char digits[] = "0123456789abcdef";


static int exceptions_i2s(const char *prefix, char *s, unsigned int i, unsigned char b, char zero)
{
	char c;
	unsigned int l, k, m;

	m = hal_strlen(prefix);
	hal_memcpy(s, prefix, m);

	for (k = m, l = (unsigned int)-1; l; i /= b, l /= b) {
		if (!zero && !i)
			break;
		s[k++] = digits[i % b];
	}

	l = k--;

	while (k > m) {
		c = s[m];
		s[m++] = s[k];
		s[k--] = c;
	}

	return l;
}


void hal_exceptionsDumpContext(char *buff, exc_context_t *ctx, int n)
{
	static const char *const mnemonics[] = {
		"0 #Reset", "1 #Undef", "2 #Syscall", "3 #Prefetch",
		"4 #Abort", "5 #Reserved", "6 #FIRQ", "7 #IRQ"
	};
	size_t i = 0;

	n &= 0x7;

	hal_strcpy(buff, "\nException: ");
	hal_strcpy(buff += hal_strlen(buff), mnemonics[n]);
	hal_strcpy(buff += hal_strlen(buff), "\n");
	buff += hal_strlen(buff);

	i += exceptions_i2s(" r0=", &buff[i], ctx->r0, 16, 1);
	i += exceptions_i2s("  r1=", &buff[i], ctx->r1, 16, 1);
	i += exceptions_i2s("  r2=", &buff[i], ctx->r2, 16, 1);
	i += exceptions_i2s("  r3=", &buff[i], ctx->r3, 16, 1);

	i += exceptions_i2s("\n r4=", &buff[i], ctx->r4, 16, 1);
	i += exceptions_i2s("  r5=", &buff[i], ctx->r5, 16, 1);
	i += exceptions_i2s("  r6=", &buff[i], ctx->r6, 16, 1);
	i += exceptions_i2s("  r7=", &buff[i], ctx->r7, 16, 1);

	i += exceptions_i2s("\n r8=", &buff[i], ctx->r8, 16, 1);
	i += exceptions_i2s("  r9=", &buff[i], ctx->r9, 16, 1);
	i += exceptions_i2s(" r10=", &buff[i], ctx->r10, 16, 1);
	i += exceptions_i2s("  fp=", &buff[i], ctx->fp, 16, 1);

	i += exceptions_i2s("\n ip=", &buff[i], ctx->ip, 16, 1);
	i += exceptions_i2s("  sp=", &buff[i], (u32)ctx + 21 * 4, 16, 1);
	i += exceptions_i2s("  lr=", &buff[i], ctx->lr, 16, 1);
	i += exceptions_i2s("  pc=", &buff[i], ctx->pc, 16, 1);

	i += exceptions_i2s("\npsr=", &buff[i], ctx->psr, 16, 1);
	i += exceptions_i2s(" dfs=", &buff[i], ctx->dfsr, 16, 1);
	i += exceptions_i2s(" dfa=", &buff[i], ctx->dfar, 16, 1);
	i += exceptions_i2s(" ifs=", &buff[i], ctx->ifsr, 16, 1);

	i += exceptions_i2s("\nifa=", &buff[i], ctx->ifar, 16, 1);

	buff[i++] = '\n';

	buff[i] = 0;
}


void exceptions_dispatch(unsigned int n, exc_context_t *ctx)
{
	char buff[512];

	hal_interruptsDisableAll();

	hal_exceptionsDumpContext(buff, ctx, n);
	hal_consolePrint(buff);

#ifdef NDEBUG
	hal_cpuReboot();
#endif

	for (;;)
		hal_cpuHalt();
}
