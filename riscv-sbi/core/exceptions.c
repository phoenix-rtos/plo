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

#include "csr.h"
#include "hart.h"
#include "string.h"
#include "types.h"

#include "devices/console.h"


typedef struct _exc_context_t {
	u64 ra; /* x1 */
	u64 sp; /* x2 */
	u64 gp; /* x3 */
	u64 tp; /* x4 */

	u64 t0; /* x5 */
	u64 t1; /* x6 */
	u64 t2; /* x7 */

	u64 s0; /* x8 */
	u64 s1; /* x9 */

	u64 a0; /* x10 */
	u64 a1; /* x11 */
	u64 a2; /* x12 */
	u64 a3; /* x13 */
	u64 a4; /* x14 */
	u64 a5; /* x15 */
	u64 a6; /* x16 */
	u64 a7; /* x17 */

	u64 s2;  /* x18 */
	u64 s3;  /* x19 */
	u64 s4;  /* x20 */
	u64 s5;  /* x21 */
	u64 s6;  /* x22 */
	u64 s7;  /* x23 */
	u64 s8;  /* x24 */
	u64 s9;  /* x25 */
	u64 s10; /* x26 */
	u64 s11; /* x27 */

	u64 t3; /* x28 */
	u64 t4; /* x29 */
	u64 t5; /* x30 */
	u64 t6; /* x31 */

	u64 mstatus;
	u64 mepc;
	u64 mcause;
	u64 mtval;
} __attribute__((packed, aligned(8))) exc_context_t;


static void exceptions_DumpContext(char *buff, exc_context_t *ctx, int n)
{
	unsigned int i = 0;

	static const char *mnemonics[] = {
		"0 Instruction address missaligned", "1 Instruction access fault", "2 Illegal instruction", "3 Breakpoint",
		"4 Reserved", "5 Load access fault", "6 AMO address misaligned", "7 Store/AMO access fault",
		"8 Environment call", "9 Reserved", "10 Reserved", "11 Reserved",
		"12 Instruction page fault", "13 Load page fault", "14 Reserved", "15 Store/AMO page fault"
	};

	n &= 0xf;

	sbi_strcpy(buff, "\nException: ");
	sbi_strcpy(buff += sbi_strlen(buff), mnemonics[n]);
	sbi_strcpy(buff += sbi_strlen(buff), "\n");
	buff += sbi_strlen(buff);

	i += sbi_i2s("zero: ", &buff[i], 0, 16, 1);
	i += sbi_i2s("  ra : ", &buff[i], (u64)ctx->ra, 16, 1);
	i += sbi_i2s("   sp : ", &buff[i], (u64)ctx->sp, 16, 1);
	i += sbi_i2s("   gp : ", &buff[i], (u64)ctx->gp, 16, 1);
	buff[i++] = '\n';

	i += sbi_i2s(" tp : ", &buff[i], (u64)ctx->tp, 16, 1);
	i += sbi_i2s("  t0 : ", &buff[i], (u64)ctx->t0, 16, 1);
	i += sbi_i2s("   t1 : ", &buff[i], (u64)ctx->t1, 16, 1);
	i += sbi_i2s("   t2 : ", &buff[i], (u64)ctx->t2, 16, 1);
	buff[i++] = '\n';

	i += sbi_i2s(" s0 : ", &buff[i], (u64)ctx->s0, 16, 1);
	i += sbi_i2s("  s1 : ", &buff[i], (u64)ctx->s1, 16, 1);
	i += sbi_i2s("   a0 : ", &buff[i], (u64)ctx->a0, 16, 1);
	i += sbi_i2s("   a1 : ", &buff[i], (u64)ctx->a1, 16, 1);
	buff[i++] = '\n';

	i += sbi_i2s(" a2 : ", &buff[i], (u64)ctx->a2, 16, 1);
	i += sbi_i2s("  a3 : ", &buff[i], (u64)ctx->a3, 16, 1);
	i += sbi_i2s("   a4 : ", &buff[i], (u64)ctx->a4, 16, 1);
	i += sbi_i2s("   a5 : ", &buff[i], (u64)ctx->a5, 16, 1);
	buff[i++] = '\n';

	i += sbi_i2s(" a6 : ", &buff[i], (u64)ctx->a6, 16, 1);
	i += sbi_i2s("  a7 : ", &buff[i], (u64)ctx->a7, 16, 1);
	i += sbi_i2s("   s2 : ", &buff[i], (u64)ctx->s2, 16, 1);
	i += sbi_i2s("   s3 : ", &buff[i], (u64)ctx->s3, 16, 1);
	buff[i++] = '\n';

	i += sbi_i2s(" s4 : ", &buff[i], (u64)ctx->s4, 16, 1);
	i += sbi_i2s("  s5 : ", &buff[i], (u64)ctx->s5, 16, 1);
	i += sbi_i2s("   s6 : ", &buff[i], (u64)ctx->s6, 16, 1);
	i += sbi_i2s("   s7 : ", &buff[i], (u64)ctx->s7, 16, 1);
	buff[i++] = '\n';

	i += sbi_i2s(" s8 : ", &buff[i], (u64)ctx->s8, 16, 1);
	i += sbi_i2s("  s9 : ", &buff[i], (u64)ctx->s9, 16, 1);
	i += sbi_i2s("  s10 : ", &buff[i], (u64)ctx->s10, 16, 1);
	i += sbi_i2s("  s11 : ", &buff[i], (u64)ctx->s11, 16, 1);
	buff[i++] = '\n';

	i += sbi_i2s(" t3 : ", &buff[i], (u64)ctx->t3, 16, 1);
	i += sbi_i2s("  t4 : ", &buff[i], (u64)ctx->t4, 16, 1);
	i += sbi_i2s("   t5 : ", &buff[i], (u64)ctx->t5, 16, 1);
	i += sbi_i2s("   t6 : ", &buff[i], (u64)ctx->t6, 16, 1);
	buff[i++] = '\n';

	i += sbi_i2s(" mstatus : ", &buff[i], (u64)ctx->mstatus, 16, 1);
	i += sbi_i2s(" mepc : ", &buff[i], (u64)ctx->mepc, 16, 1);
	i += sbi_i2s(" mcause : ", &buff[i], (u64)ctx->mcause, 16, 1);

	buff[i++] = '\n';

	buff[i] = 0;
}


static void __attribute__((noreturn)) exceptions_defaultHandler(unsigned int n, exc_context_t *ctx)
{
	char buff[1100];

	exceptions_DumpContext(buff, ctx, n);
	console_print(buff);

	hart_halt();
}


static void exceptions_redirect(unsigned int n, u64 insn, exc_context_t *ctx)
{
	u64 prevMode = (ctx->mstatus & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT;
	u64 mstatus = ctx->mstatus;

	/* Update S-mode CSRs */
	csr_write(CSR_SEPC, ctx->mepc);
	csr_write(CSR_SCAUSE, ctx->mcause);
	csr_write(CSR_STVAL, insn);

	/* Update exception context */
	ctx->mepc = csr_read(CSR_STVEC);

	mstatus &= ~(MSTATUS_MPP | MSTATUS_SPP | MSTATUS_SPIE);
	/* Set MPP to S-mode */
	mstatus |= (PRV_S << MSTATUS_MPP_SHIFT);

	/* Set SPP accordingly */
	if (prevMode == PRV_S) {
		mstatus |= MSTATUS_SPP;
	}

	/* Update SPIE */
	if ((ctx->mstatus & MSTATUS_SIE) != 0) {
		mstatus |= MSTATUS_SPIE;
	}

	/* Clear SIE */
	mstatus &= ~MSTATUS_SIE;

	ctx->mstatus = mstatus;
}


static u64 exceptions_getRegval(unsigned int regNum, exc_context_t *ctx)
{
	if (regNum == 0) {
		return 0;
	}

	return *((u64 *)((addr_t)ctx + (regNum - 1) * sizeof(u64)));
}


static void exceptions_setRegval(unsigned int regNum, u64 regVal, exc_context_t *ctx)
{
	if (regNum != 0) {
		*((u64 *)((addr_t)ctx + (regNum - 1) * sizeof(u64))) = regVal;
	}
}


static int exceptions_illSystem(unsigned int n, exc_context_t *ctx)
{
	/* Assuming we trapped on CSR instruction (not checked) */
	u64 insn = ctx->mtval;
	u32 csr = (insn >> 20) & 0xfffu;
	u64 csrVal, newCsrVal;
	u64 prevMode = (ctx->mstatus & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT;
	u32 rs1 = (insn >> 15) & 0x1fu;
	u64 rs1Val = exceptions_getRegval(rs1, ctx);
	u32 write = rs1;
	u32 rd = (insn >> 7) & 0x1fu;

	if (prevMode == PRV_M) {
		/* Trapped on CSR access from M-mode, halt */
		exceptions_defaultHandler(n, ctx);
	}

	if ((csr == CSR_FFLAGS) || (csr == CSR_FRM) || (csr == CSR_FCSR)) {
		/* Redirect FP CSR writes to S-Mode */
		return 0;
	}

	if (csr_emulateRead(csr, &csrVal) < 0) {
		exceptions_defaultHandler(n, ctx);
	}

	/* Check funct3 */
	switch ((insn >> 12) & 0x7) {
		case 1:
			/* CSRRW */
			newCsrVal = rs1Val;
			write = 1;
			break;

		case 2:
			/* CSRRS */
			newCsrVal = csrVal | rs1Val;
			break;

		case 3:
			/* CSRRC */
			newCsrVal = csrVal & ~rs1Val;
			break;

		case 5:
			/* CSRRWI */
			newCsrVal = rs1;
			write = 1;
			break;

		case 6:
			/* CSRRSI */
			newCsrVal = csrVal | rs1;
			break;

		case 7:
			/* CSRRCI */
			newCsrVal = csrVal & ~rs1;
			break;

		default:
			exceptions_defaultHandler(n, ctx);
			break;
	}

	if ((write != 0) && (csr_emulateWrite(csr, newCsrVal) < 0)) {
		exceptions_defaultHandler(n, ctx);
	}

	exceptions_setRegval(rd, csrVal, ctx);

	/* Skip instruction */
	ctx->mepc += 4;

	return 1;
}


static void exceptions_illegalHandler(unsigned int n, exc_context_t *ctx)
{
	u64 insn = ctx->mtval;
	u64 prevMode = (ctx->mstatus & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT;
	unsigned long unpriv_insn;

	int handled = 0;
	/* Check failing instruction */
	if (((insn & 3) == 3) && (((insn & 0x7c) >> 2) == 0x1c)) {
		/* Non-compressed, SYSTEM opcode */
		handled = exceptions_illSystem(n, ctx);
	}

	if (handled == 0) {
		if (prevMode == PRV_M) {
			/* Trapped on illegal instruction in M-mode */
			exceptions_defaultHandler(n, ctx);
		}
		else {
			if (insn == 0) {
				/* mepc has virtual address, have to load using MPRV */
				MPRV_LOAD(lhu, insn, ctx->mepc);
				/* Check lowest to bits to determine if we are dealing with compressed insn
				 * Noncompressed RV instructions have bits [1:0] = 11.
				 */
				if ((insn & 3) == 3) {
					/* Noncompressed, load 2nd part */
					MPRV_LOAD(lhu, unpriv_insn, ctx->mepc + 2);
					insn = (unpriv_insn << 16) | insn;
				}
			}
			exceptions_redirect(n, insn, ctx);
		}
	}
}


void exceptions_dispatch(unsigned int n, exc_context_t *ctx)
{
	if (n == MCAUSE_ILLEGAL) {
		exceptions_illegalHandler(n, ctx);
	}
	else {
		exceptions_defaultHandler(n, ctx);
	}
}
