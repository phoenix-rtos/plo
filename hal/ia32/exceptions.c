/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Exception handling
 *
 * Copyright 2012, 2016, 2021 Phoenix Systems
 * Copyright 2001, 2005-2006 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


/* Number of exceptions */
#define SIZE_EXCEPTIONS 32


typedef struct {
	u32 dr0;
	u32 dr1;
	u32 dr2;
	u32 dr3;
	u32 dr4;
	u32 dr5;
	u32 edi;
	u32 esi;
	u32 ebp;
	u32 edx;
	u32 ecx;
	u32 ebx;
	u32 eax;
	u16 gs;
	u16 fs;
	u16 es;
	u16 ds;
	u32 err;
	u32 eip;
	u32 cs;
	u32 eflags;
	u32 esp;
	u32 ss;
} __attribute__((packed)) exc_context_t;


typedef void (*exc_handler_t)(unsigned int, exc_context_t *);


struct {
	exc_handler_t handlers[SIZE_EXCEPTIONS];
} exceptions_common;


/* Sets exception stub in IDT */
extern int interrupts_setIDTEntry(unsigned int n, void (*base)(void), unsigned short sel, unsigned char flags);


/* Exception stubs */
extern void _exceptions_exc0(void);
extern void _exceptions_exc1(void);
extern void _exceptions_exc2(void);
extern void _exceptions_exc3(void);
extern void _exceptions_exc4(void);
extern void _exceptions_exc5(void);
extern void _exceptions_exc6(void);
extern void _exceptions_exc7(void);
extern void _exceptions_exc8(void);
extern void _exceptions_exc9(void);
extern void _exceptions_exc10(void);
extern void _exceptions_exc11(void);
extern void _exceptions_exc12(void);
extern void _exceptions_exc13(void);
extern void _exceptions_exc14(void);
extern void _exceptions_exc15(void);
extern void _exceptions_exc16(void);
extern void _exceptions_exc17(void);
extern void _exceptions_exc18(void);
extern void _exceptions_exc19(void);
extern void _exceptions_exc20(void);
extern void _exceptions_exc21(void);
extern void _exceptions_exc22(void);
extern void _exceptions_exc23(void);
extern void _exceptions_exc24(void);
extern void _exceptions_exc25(void);
extern void _exceptions_exc26(void);
extern void _exceptions_exc27(void);
extern void _exceptions_exc28(void);
extern void _exceptions_exc29(void);
extern void _exceptions_exc30(void);
extern void _exceptions_exc31(void);


/* Dumps exception context to buffer */
static void hal_exceptionsDumpContext(char *buff, unsigned int n, exc_context_t *ctx)
{
	/* clang-format off */
	static const char *const mnemonics[] = {
		"0 #DE",  "1 #DB",  "2 #NMI", "3 #BP",      "4 #OF",  "5 #BR",  "6 #UD",  "7 #NM",
		"8 #DF",  "9 #",    "10 #TS", "11 #NP",     "12 #SS", "13 #GP", "14 #PF", "15 #",
		"16 #MF", "17 #AC", "18 #MC", "19 #XM/#XF", "20 #VE", "21 #",   "22 #",   "23 #",
		"24 #",   "25 #",   "26 #",   "27 #",       "28 #",   "29 #",   "30 #SX", "31 #"
	};
	/* clang-format on */
	unsigned int cr2, i = 0;

	__asm__ volatile(
		"movl %%cr2, %%eax; "
	: "=a" (cr2));

	hal_strcpy(buff, "\nException: ");
	hal_strcpy(buff += hal_strlen(buff), mnemonics[n]);
	buff += hal_strlen(buff);

	i += hal_i2s("\neax=", buff + i, ctx->eax, 16, 1);
	i += hal_i2s("  cs=", buff + i, ctx->cs, 16, 1);
	i += hal_i2s(" eip=", buff + i, ctx->eip, 16, 1);
	i += hal_i2s(" eflgs=", buff + i, ctx->eflags, 16, 1);

	i += hal_i2s("\nebx=", buff + i, ctx->ebx, 16, 1);
	i += hal_i2s("  ss=", buff + i, ctx->ss, 16, 1);
	i += hal_i2s(" esp=", buff + i, ctx->esp, 16, 1);
	i += hal_i2s(" ebp=", buff + i, ctx->ebp, 16, 1);

	i += hal_i2s("\necx=", buff + i, ctx->ecx, 16, 1);
	i += hal_i2s("  ds=", buff + i, ctx->ds, 16, 1);
	i += hal_i2s(" esi=", buff + i, ctx->esi, 16, 1);
	i += hal_i2s("  fs=", buff + i, ctx->fs, 16, 1);

	i += hal_i2s("\nedx=", buff + i, ctx->edx, 16, 1);
	i += hal_i2s("  es=", buff + i, ctx->es, 16, 1);
	i += hal_i2s(" edi=", buff + i, ctx->edi, 16, 1);
	i += hal_i2s("  gs=", buff + i, ctx->gs, 16, 1);

	i += hal_i2s("\ndr0=", buff + i, ctx->dr0, 16, 1);
	i += hal_i2s(" dr1=", buff + i, ctx->dr1, 16, 1);
	i += hal_i2s(" dr2=", buff + i, ctx->dr2, 16, 1);
	i += hal_i2s(" dr3=", buff + i, ctx->dr3, 16, 1);

	i += hal_i2s("\ndr4=", buff + i, ctx->dr4, 16, 1);
	i += hal_i2s(" dr5=", buff + i, ctx->dr5, 16, 1);
	i += hal_i2s(" cr2=", buff + i, cr2, 16, 1);
	buff[i] = '\0';
}


/* Default exception handler */
static void exceptions_defaultHandler(unsigned int n, exc_context_t *ctx)
{
	char buff[512];

	hal_exceptionsDumpContext(buff, n, ctx);
	hal_consolePrint("\033[1m");
	hal_consolePrint(buff);
	hal_consolePrint("\033[0m");
	hal_interruptsDisableAll();

#ifdef NDEBUG
	hal_cpuReboot();
#endif

	for (;;) {
		hal_cpuHalt();
	}
}


void hal_exceptionsInit(void)
{
	unsigned int i;

	/* Set exception stubs */
	interrupts_setIDTEntry(0, _exceptions_exc0, 0x08, 0x0e);
	interrupts_setIDTEntry(1, _exceptions_exc1, 0x08, 0x0e);
	interrupts_setIDTEntry(2, _exceptions_exc2, 0x08, 0x0e);
	interrupts_setIDTEntry(3, _exceptions_exc3, 0x08, 0x0e);
	interrupts_setIDTEntry(4, _exceptions_exc4, 0x08, 0x0e);
	interrupts_setIDTEntry(5, _exceptions_exc5, 0x08, 0x0e);
	interrupts_setIDTEntry(6, _exceptions_exc6, 0x08, 0x0e);
	interrupts_setIDTEntry(7, _exceptions_exc7, 0x08, 0x0e);
	interrupts_setIDTEntry(8, _exceptions_exc8, 0x08, 0x0e);
	interrupts_setIDTEntry(9, _exceptions_exc9, 0x08, 0x0e);
	interrupts_setIDTEntry(10, _exceptions_exc10, 0x08, 0x0e);
	interrupts_setIDTEntry(11, _exceptions_exc11, 0x08, 0x0e);
	interrupts_setIDTEntry(12, _exceptions_exc12, 0x08, 0x0e);
	interrupts_setIDTEntry(13, _exceptions_exc13, 0x08, 0x0e);
	interrupts_setIDTEntry(14, _exceptions_exc14, 0x08, 0x0e);
	interrupts_setIDTEntry(15, _exceptions_exc15, 0x08, 0x0e);
	interrupts_setIDTEntry(16, _exceptions_exc16, 0x08, 0x0e);
	interrupts_setIDTEntry(17, _exceptions_exc17, 0x08, 0x0e);
	interrupts_setIDTEntry(18, _exceptions_exc18, 0x08, 0x0e);
	interrupts_setIDTEntry(19, _exceptions_exc19, 0x08, 0x0e);
	interrupts_setIDTEntry(20, _exceptions_exc20, 0x08, 0x0e);
	interrupts_setIDTEntry(21, _exceptions_exc21, 0x08, 0x0e);
	interrupts_setIDTEntry(22, _exceptions_exc22, 0x08, 0x0e);
	interrupts_setIDTEntry(23, _exceptions_exc23, 0x08, 0x0e);
	interrupts_setIDTEntry(24, _exceptions_exc24, 0x08, 0x0e);
	interrupts_setIDTEntry(25, _exceptions_exc25, 0x08, 0x0e);
	interrupts_setIDTEntry(26, _exceptions_exc26, 0x08, 0x0e);
	interrupts_setIDTEntry(27, _exceptions_exc27, 0x08, 0x0e);
	interrupts_setIDTEntry(28, _exceptions_exc28, 0x08, 0x0e);
	interrupts_setIDTEntry(29, _exceptions_exc29, 0x08, 0x0e);
	interrupts_setIDTEntry(30, _exceptions_exc30, 0x08, 0x0e);
	interrupts_setIDTEntry(31, _exceptions_exc31, 0x08, 0x0e);

	/* Set exception handlers */
	for (i = 0; i < SIZE_EXCEPTIONS; i++)
		exceptions_common.handlers[i] = exceptions_defaultHandler;
}
