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


/* Set to 1 to print text descriptions of exceptions for architecture extensions */
#define EXTENSION_DESCRIPTIONS 0

typedef struct _exc_context_t {
	u64 esr;
	u64 far;
	u64 psr;
	u64 pc;
	u64 x[31];
	u64 sp;
} exc_context_t;


static const char digits[] = "0123456789abcdef";


static int exceptions_i2s(const char *prefix, char *buf, u64 num, unsigned char base, char zero)
{
	char c;
	u64 l;
	size_t k, m;

	m = hal_strlen(prefix);
	hal_memcpy(buf, prefix, m);

	for (k = m, l = (u64)-1; l != 0; num /= base, l /= base) {
		if ((zero == 0) && (num == 0)) {
			break;
		}

		buf[k++] = digits[num % base];
	}

	l = k--;

	while (k > m) {
		c = buf[m];
		buf[m++] = buf[k];
		buf[k--] = c;
	}

	return l;
}


static const char *exceptionClassStr(u8 excClass)
{
	switch (excClass) {
		case 0b000000:
			return "Unknown reason";
		case 0b000001:
			return "Trapped WFI/WFE";
		case 0b000011:
			return "Trapped MCR/MRC access (cp15)";
		case 0b000100:
			return "Trapped MCRR/MRRC access (cp15)";
		case 0b000101:
			return "Trapped MCR/MRC access (cp14)";
		case 0b000110:
			return "Trapped LDC/STC access";
		case 0b000111:
			return "Trapped SME, SVE, Advanced SIMD or floating-point functionality due to CPACR_ELx.FPEN";
		case 0b001100:
			return "Trapped MRRC access (cp14)";
		case 0b001110:
			return "Illegal Execution state";
		case 0b010001:
			return "SVC (AA32)";
		case 0b010100:
			return "Trapped MSRR/MRRS/SYS (AA64)";
		case 0b010101:
			return "SVC (AA64)";
		case 0b011000:
			return "Trapped MSR/MRS/SYS (AA64)";
		case 0b100000:
			return "Instruction Abort (EL0)";
		case 0b100001:
			return "Instruction Abort (EL1)";
		case 0b100010:
			return "PC alignment fault";
		case 0b100100:
			return "Data Abort (EL0)";
		case 0b100101:
			return "Data Abort (EL1)";
		case 0b100110:
			return "SP alignment fault";
		case 0b101000:
			return "Trapped floating-point exception (AA32)";
		case 0b101100:
			return "Trapped floating-point exception (AA64)";
		case 0b101111:
			return "SError exception";
		case 0b110000:
			return "Breakpoint (EL0)";
		case 0b110001:
			return "Breakpoint (EL1)";
		case 0b110010:
			return "Software Step (EL0)";
		case 0b110011:
			return "Software Step (EL1)";
		case 0b110100:
			return "Watchpoint (EL0)";
		case 0b110101:
			return "Watchpoint (EL1)";
		case 0b111000:
			return "BKPT (AA32)";
		case 0b111100:
			return "BRK (AA64)";
#if EXTENSION_DESCRIPTIONS
		case 0b001010:
			return "(FEAT_LS64) Trapped execution of an LD64B or ST64B* instruction";
		case 0b001101:
			return "(FEAT_BTI) Branch Target Exception";
		case 0b011001:
			return "(FEAT_SVE) Access to SVE functionality trapped";
		case 0b011011:
			return "(FEAT_TME) Exception from an access to a TSTART instruction...";
		case 0b011100:
			return "(FEAT_FPAC) Exception from a PAC Fail";
		case 0b011101:
			return "(FEAT_SME) Access to SME functionality trapped";
		case 0b100111:
			return "(FEAT_MOPS) Memory Operation Exception";
		case 0b101101:
			return "(FEAT_GCS) GCS exception";
		case 0b111101:
			return "(FEAT_EBEP) PMU exception";
#endif
		default:
			return "Reserved";
	}
}


void hal_exceptionsDumpContext(char *buff, exc_context_t *ctx, int n)
{
	size_t i = 0, j;
	u8 excClass = (ctx->esr >> 26) & 0x3f;
	const char *toAdd;

	toAdd = "\nException #";
	hal_strcpy(&buff[i], toAdd);
	i += hal_strlen(toAdd);
	buff[i++] = '0' + excClass / 10;
	buff[i++] = '0' + excClass % 10;
	buff[i++] = ':';
	buff[i++] = ' ';
	toAdd = exceptionClassStr(excClass);
	hal_strcpy(&buff[i], toAdd);
	i += hal_strlen(toAdd);

	char prefix[6] = "    =";
	for (j = 0; j < 29; j++) {
		prefix[0] = ((j % 4) == 0) ? '\n' : ' ';
		if (j < 10) {
			prefix[1] = ' ';
			prefix[2] = 'x';
		}
		else {
			prefix[1] = 'x';
			prefix[2] = '0' + (j / 10);
		}

		prefix[3] = '0' + (j % 10);
		i += exceptions_i2s(prefix, &buff[i], ctx->x[j], 16, 1);
	}

	i += exceptions_i2s("  fp=", &buff[i], ctx->x[29], 16, 1);
	i += exceptions_i2s("  lr=", &buff[i], ctx->x[30], 16, 1);
	i += exceptions_i2s("  sp=", &buff[i], ctx->sp, 16, 1);

	i += exceptions_i2s("\npsr=", &buff[i], ctx->psr, 16, 1);
	i += exceptions_i2s("  pc=", &buff[i], ctx->pc, 16, 1);
	i += exceptions_i2s(" esr=", &buff[i], ctx->esr, 16, 1);
	i += exceptions_i2s(" far=", &buff[i], ctx->far, 16, 1);

	buff[i++] = '\n';
	buff[i] = '\0';
}


void exceptions_dispatch(unsigned int n, exc_context_t *ctx)
{
	char buff[1024];

	hal_interruptsDisableAll();

	hal_exceptionsDumpContext(buff, ctx, n);
	hal_consolePrint(buff);

#ifdef NDEBUG
	hal_cpuReboot();
#endif

	for (;;) {
		hal_cpuHalt();
	}
}
