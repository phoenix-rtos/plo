/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Low - level routines
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "imxrt.h"
#include "config.h"
#include "flashdrv.h"
#include "../low.h"
#include "../plostd.h"
#include "../errors.h"
#include "../serial.h"


#define SIZE_INTERRUPTS 167


typedef struct {
    void *data;
    int (*isr)(u16, void *);
} intr_handler_t;


struct{
    intr_handler_t irqs[SIZE_INTERRUPTS];
} low_common;


u16  _plo_timeout = 3;
char _welcome_str[] = WELCOME;
char _plo_command[CMD_SIZE] = DEFAULT_CMD;



/* Init and deinit functions */

void low_init(void)
{
    int i;

    _imxrt_init();

    for (i = 0; i < SIZE_INTERRUPTS; ++i) {
        low_common.irqs[i].data = NULL;
        low_common.irqs[i].isr = NULL;
    }
}


void low_done(void)
{
    //TODO
}



/* Functions modify registers */

u8 low_inb(u16 addr)
{
    //TODO
    return 0;
}


void low_outb(u16 addr, u8 b)
{
    //TODO
}


u32 low_ind(u16 addr)
{
    //TODO
    return 0;
}


void low_outd(u16 addr, u32 d)
{
    //TODO
}


/* Functions make operations on memory */

void low_setfar(u16 segm, u16 offs, u16 v)
{
    //TODO
}


u16 low_getfar(u16 segm, u16 offs)
{
    //TODO
    return 0;
}


void low_setfarbabs(u32 addr, u8 v)
{
    //TODO
}


u8 low_getfarbabs(u32 addr)
{
    //TODO
    return 0;
}


void low_setfarabs(u32 addr, u32 v)
{
    //TODO
}


u32 low_getfarabs(u32 addr)
{
    //TODO
    return 0;
}


void low_copyto(u16 segm, u16 offs, void *src, unsigned int l)
{
    //TODO
}


void low_copyfrom(u16 segm, u16 offs, void *dst, unsigned int l)
{
    //TODO
}


void low_memcpy(void *dst, const void *src, unsigned int l)
{
    int i;
    char* dst_ = dst;
    const char* src_ = src;

    for(i = 0; i < l; ++i)
        *(dst_ + i) = *(src_ + i);
}


void low_copytoabs(u32 addr, void *src, unsigned int l)
{
    //TODO
}


void low_copyfromabs(u32 addr, void *dst, unsigned int l)
{
    //TODO
}


u16 low_getcs(void)
{
    //TODO
    return 0;
}


int low_mmcreate(void)
{
    //TODO
    return 0;
}


int low_mmget(unsigned int n, low_mmitem_t *mmitem)
{
    //TODO
    return 0;
}


int low_launch(void)
{
    //TODO
    return 0;
}



/* Opeartions on interrupts */

void low_cli(void)
{
    asm("cpsid if");
}


void low_sti(void)
{
    asm("cpsie if");
}


int low_irqdispatch(u16 irq)
{
    if (low_common.irqs[irq].isr == NULL)
        return -1;

    low_common.irqs[irq].isr(irq, low_common.irqs[irq].data);

    return 0;
}


void low_maskirq(u16 n, u8 v)
{
    //TODO
}


int low_irqinst(u16 irq, int (*isr)(u16, void *), void *data)
{
    if (irq > SIZE_INTERRUPTS)
        return ERR_ARG;

    low_cli();
    low_common.irqs[irq].isr = isr;
    low_common.irqs[irq].data = data;

    _imxrt_nvicSetPriority(irq - 0x10, 1);
    _imxrt_nvicSetIRQ(irq - 0x10, 1);
    low_sti();

    return 0;
}


int low_irquninst(u16 irq)
{
    low_cli();
    _imxrt_nvicSetIRQ(irq - 0x10, 0);
    low_sti();

    return 0;
}


void low_setattr(char attr)
{
    switch (attr) {
    case ATTR_DEBUG:
        serial_safewrite(UART_CONSOLE, (u8 *)"\033[0m\033[32m", 9);
        break;
    case ATTR_USER:
        serial_safewrite(UART_CONSOLE, (u8 *)"\033[0m", 4);
        break;
    case ATTR_INIT:
        serial_safewrite(UART_CONSOLE, (u8 *)"\033[0m\033[35m", 9);
        break;
    case ATTR_LOADER:
        serial_safewrite(UART_CONSOLE, (u8 *)"\033[0m\033[1m", 8);
        break;
    case ATTR_ERROR:
        serial_safewrite(UART_CONSOLE, (u8 *)"\033[0m\033[31m", 9);
        break;
    }

    return;
}


void low_putc(const char ch)
{
    serial_write(UART_CONSOLE, (u8 *)&ch, 1);
}


void low_getc(char *c, char *sc)
{
    while (serial_read(UART_CONSOLE, (u8 *)c, 1, 500) <= 0)
        ;
    *sc = 0;

    /* Translate backspace */
    if (*c == 127)
        *c = 8;

    /* Simple parser for VT100 commands */
    else if (*c == 27) {
        while (serial_read(UART_CONSOLE, (u8 *)c, 1, 500) <= 0)
            ;

        switch (*c) {
        case 91:
            while (serial_read(UART_CONSOLE, (u8 *)c, 1, 500) <= 0)
                ;

            switch (*c) {
            case 'A':             /* UP */
                *sc = 72;
                break;
            case 'B':             /* DOWN */
                *sc = 80;
                break;
            }
            break;
        }
        *c = 0;
    }
}


int low_keypressed(void)
{
    return !serial_rxEmpty(UART_CONSOLE);
}

