/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * UART 16450 driver
 *
 * Copyright 2012 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "errors.h"
#include "low.h"
#include "plostd.h"
#include "timer.h"
#include "serial.h"


typedef struct {
	u16 active;
	u16 base;
	u16 irq;
	u8 rbuff[RBUFFSZ];
	u16 rb;
	u16 rp;
	u8 sbuff[SBUFFSZ];
	u16 sp;
	u16 se;
} serial_t;


serial_t serials[NPORTS];


int serial_isr(u16 irq, void *data)
{
	serial_t *serial = data;
	u8 iir;

	for (;;) {
		if ((iir = low_inb(serial->base + REG_IIR)) & IIR_IRQPEND)
			break;

		/* Receive */
		if ((iir & IIR_DR) == IIR_DR) {
			serial->rbuff[serial->rp] = low_inb(serial->base + REG_RBR);
			serial->rp = (++serial->rp % RBUFFSZ);
			if (serial->rp == serial->rb) {
				serial->rb = (++serial->rb % RBUFFSZ);
			}
		}

		/* Transmit */
		if ((iir & IIR_THRE) == IIR_THRE) {
			serial->sp = (++serial->sp % SBUFFSZ);
			if (serial->sp != serial->se)
				low_outb(serial->base + REG_THR, serial->sbuff[serial->sp]);
		}
	}
	return IRQ_HANDLED;
}


int serial_read(unsigned int pn, u8 *buff, u16 len, u16 timeout)
{
	serial_t *serial;
	unsigned int l;
	unsigned int cnt;

	if (pn >= NPORTS)
		return ERR_ARG;
	serial = &serials[pn];
	if (!serial->active)
		return ERR_ARG;

	/* Wait for data */
	if (!timer_wait(timeout, TIMER_VALCHG, &serial->rp, serial->rb))
		return ERR_SERIAL_TIMEOUT;

	low_cli();

	if (serial->rp > serial->rb)
		l = min(serial->rp - serial->rb, len);
	else
		l = min(RBUFFSZ - serial->rb, len);

	low_memcpy(buff, &serial->rbuff[serial->rb], l);
	cnt = l;
	if ((len > l) && (serial->rp < serial->rb)) {
		low_memcpy(buff + l, &serial->rbuff[0], min(len - l, serial->rp));
		cnt += min(len - l, serial->rp);
	}
	serial->rb = ((serial->rb + cnt) % RBUFFSZ);

	low_sti();
	return cnt;
}


int serial_write(unsigned int pn, u8 *buff, u16 len)
{
	serial_t *serial;
	unsigned int l;
	unsigned int cnt;

	if (pn >= NPORTS)
		return ERR_ARG;
	serial = &serials[pn];
	if (!serial->active)
		return ERR_ARG;

	low_cli();

	if (serial->sp > serial->se)
		l = min(serial->sp - serial->se, len);
	else
		l = min(SBUFFSZ - serial->se, len);

	low_memcpy(&serial->sbuff[serial->se], buff, l);
	cnt = l;
	if ((len > l) && (serial->se >= serial->sp)) {
		low_memcpy(serial->sbuff, buff + l, min(len - l, serial->sp));
		cnt += min(len - l, serial->sp);
	}

	/* Initialize sending process */
	if (serial->se == serial->sp)
		low_outb(serial->base, serial->sbuff[serial->sp]);

	serial->se = ((serial->se + cnt) % SBUFFSZ);
	low_sti();

	return cnt;
}


void serial_initone(unsigned int pn, u16 base, u16 irq, u16 speed, serial_t *serial)
{
	serial->base = base;
	serial->irq = irq;
	serial->rb = 0;
	serial->rp = 0;
	serial->sp = (u16)-1;
	serial->se = 0;

	if (low_inb(serial->base + REG_IIR) == 0xff) {
		serial->active = 0;
		return;
	}
	serial->active = 1;

	low_cli();
	low_irqinst(serial->irq, serial_isr, (void *)serial);
	low_maskirq(serial->irq, 0);

	/* Set speed */
	low_outb(serial->base + REG_LCR, LCR_DLAB);
	low_outb(serial->base + REG_LSB, speed);
	low_outb(serial->base + REG_MSB, 0);

	/* Set data format */
	low_outb(serial->base + REG_LCR, LCR_D8N1);

	/* Enable hardware interrupts */
	low_outb(serial->base + REG_MCR, MCR_OUT2);

	/* Set interrupt mask */
	low_outb(serial->base + REG_IMR, IMR_THRE | IMR_DR);

	low_sti();
	return;
}


void serial_init(u16 speed)
{
	serial_initone(0, COM1_BASE, COM1_IRQ, speed, &serials[0]);
	serial_initone(1, COM2_BASE, COM2_IRQ, speed, &serials[1]);
	return;
}


void serial_done(void)
{
	unsigned int k;

	for (k = 0; k < NPORTS; k++) {
		low_cli();
		low_maskirq(serials[k].irq, 1);
		low_irquninst(serials[k].irq);
		low_sti();
		return;
	}
}
