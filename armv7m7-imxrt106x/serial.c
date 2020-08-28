/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * i.MXRT1064 Serial driver
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczyński
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "config.h"
#include "imxrt.h"
#include "../serial.h"
#include "../errors.h"
#include "../timer.h"
#include "../low.h"


#define CONCATENATE(x, y) x##y
#define PIN2MUX(x) CONCATENATE(pctl_mux_gpio_, x)
#define PIN2PAD(x) CONCATENATE(pctl_pad_gpio_, x)

#define UART_MAX_CNT 7

#define UART1_POS 0
#define UART2_POS (UART1_POS + UART1)
#define UART3_POS (UART2_POS + UART2)
#define UART4_POS (UART3_POS + UART3)
#define UART5_POS (UART4_POS + UART4)
#define UART6_POS (UART5_POS + UART5)
#define UART7_POS (UART6_POS + UART6)
#define UART8_POS (UART7_POS + UART7)

#define UART_CNT (UART1 + UART2 + UART3 + UART4 + UART5 + UART6 + UART7 + UART8)


#define BUFFER_SIZE 0x200


typedef struct {
    volatile u32 *base;
    u16 irq;

    u16 rxFifoSz;
    u16 txFifoSz;

    u8 rxBuff[BUFFER_SIZE];
    u16 rxHead;
    u16 rxTail;

    u8 txBuff[BUFFER_SIZE];
    u16 txHead;
    u16 txTail;
    u8 tFull;
} serial_t;


struct {
    serial_t serials[UART_CNT];
} serial_common;


const int serialConfig[] = { UART1, UART2, UART3, UART4, UART5, UART6, UART7, UART8 };


const int serialPos[] = { UART1_POS, UART2_POS, UART3_POS, UART4_POS, UART5_POS, UART6_POS,
    UART7_POS, UART8_POS };


enum { veridr = 0, paramr, globalr, pincfgr, baudr, statr, ctrlr, datar, matchr, modirr, fifor, waterr };


static inline int serial_getRXcount(serial_t *serial)
{
    return (*(serial->base + waterr) >> 24) & 0xff;
}


static inline int serial_getTXcount(serial_t *serial)
{
    return (*(serial->base + waterr) >> 8) & 0xff;
}


int serial_rxEmpty(unsigned int pn)
{
    serial_t *serial;
    --pn;

    if (pn > UART_MAX_CNT || !serialConfig[pn])
        return ERR_ARG;

    serial = &serial_common.serials[serialPos[pn]];

    return serial->rxHead == serial->rxTail;
}


int serial_handleIntr(u16 irq, void *buff)
{
    serial_t *serial = (serial_t *)buff;

    if (serial == NULL)
        return 0;

    /* Receive */
    while (serial_getRXcount(serial)) {
        serial->rxBuff[serial->rxHead] = *(serial->base + datar);
        serial->rxHead = (serial->rxHead + 1) % BUFFER_SIZE;
        if (serial->rxHead == serial->rxTail) {
            serial->rxTail = (serial->rxTail + 1) % BUFFER_SIZE;
        }
    }

    /* Transmit */
    while (serial_getTXcount(serial) < serial->txFifoSz && !((*(serial->base + statr) >> 22) & 0x1))
    {
        serial->txHead = (serial->txHead + 1) % BUFFER_SIZE;
        if (serial->txHead != serial->txTail) {
            *(serial->base + datar) = serial->txBuff[serial->txHead];
            serial->tFull = 0;
        }
        else {
            *(serial->base + ctrlr) &= ~(1 << 23);
            break;
        }
    }

    return 0;
}


int serial_read(unsigned int pn, u8 *buff, u16 len, u16 timeout)
{
    serial_t *serial;
    u16 l, cnt;

    --pn;

    if (pn > UART_MAX_CNT || !serialConfig[pn])
        return ERR_ARG;

    serial = &serial_common.serials[serialPos[pn]];

    if (!timer_wait(timeout, TIMER_VALCHG, &serial->rxHead, serial->rxTail))
        return ERR_SERIAL_TIMEOUT;

    low_cli();

    if (serial->rxHead > serial->rxTail)
        l = min(serial->rxHead - serial->rxTail, len);
    else
        l = min(BUFFER_SIZE - serial->rxTail, len);

    low_memcpy(buff, &serial->rxBuff[serial->rxTail], l);
    cnt = l;
    if ((len > l) && (serial->rxHead < serial->rxTail)) {
        low_memcpy(buff + l, &serial->rxBuff[0], min(len - l, serial->rxHead));
        cnt += min(len - l, serial->rxHead);
    }
    serial->rxTail = ((serial->rxTail + cnt) % BUFFER_SIZE);

    low_sti();

    return cnt;
}


int serial_write(unsigned int pn, const u8 *buff, u16 len)
{
    serial_t *serial;
    u16 l, cnt = 0;

    --pn;

    if (pn > UART_MAX_CNT || !serialConfig[pn])
        return ERR_ARG;

    serial = &serial_common.serials[serialPos[pn]];

    while (serial->txHead == serial->txTail && serial->tFull)
        ;

    low_cli();
    if (serial->txHead > serial->txTail)
        l = min(serial->txHead - serial->txTail, len);
    else
        l = min(BUFFER_SIZE - serial->txTail, len);

    low_memcpy(&serial->txBuff[serial->txTail], buff, l);
    cnt = l;
    if ((len > l) && (serial->txTail >= serial->txHead)) {
        low_memcpy(serial->txBuff, buff + l, min(len - l, serial->txHead));
        cnt += min(len - l, serial->txHead);
    }

    /* Initialize sending */
    if (serial->txTail == serial->txHead)
        *(serial->base + datar) = serial->txBuff[serial->txHead];

    serial->txTail = ((serial->txTail + cnt) % BUFFER_SIZE);

    if (serial->txTail == serial->txHead)
        serial->tFull = 1;

    *(serial->base + ctrlr) |= 1 << 23;

    low_sti();

    return cnt;
}


int serial_safewrite(unsigned int pn, const u8 *buff, u16 len)
{
    int l;

    for (l = 0; len;) {
        if ((l = serial_write(pn, buff, len)) < 0)
            return ERR_MSG_IO;
        buff += l;
        len -= l;
    }
    return 0;
}



static int serial_muxVal(int mux)
{
    switch (mux) {
        case pctl_mux_gpio_b1_12:
        case pctl_mux_gpio_b1_13:
            return 1;

        case pctl_mux_gpio_b0_08:
        case pctl_mux_gpio_b0_09:
            return 3;

        case pctl_mux_gpio_sd_b1_00:
        case pctl_mux_gpio_sd_b1_01:
            return 4;

    }

    return 2;
}


static u32 calculate_baudrate(int baud)
{
    int osr, sbr, bestSbr = 0, bestOsr = 0, bestErr = 1000, t;

    if (!baud)
        return 0;

    for (osr = 3; osr < 32; ++osr) {
        sbr = UART_CLK / (baud * (osr + 1));
        sbr &= 0xfff;
        t = UART_CLK / (sbr * (osr + 1));

        if (t > baud)
            t = ((t - baud) * 1000) / baud;
        else
            t = ((baud - t) * 1000) / baud;

        if (t < bestErr) {
            bestErr = t;
            bestOsr = osr;
            bestSbr = sbr;
        }

        /* Finish if error is < 1% */
        if (bestErr < 10)
            break;
    }

    return (bestOsr << 24) | ((bestOsr <= 6) << 17) | bestSbr;
}


static int serial_getIsel(int mux, int *isel, int *val)
{
    switch (mux) {
        case pctl_mux_gpio_ad_b1_02: *isel = pctl_isel_lpuart2_tx; *val = 1; break;
        case pctl_mux_gpio_sd_b1_11: *isel = pctl_isel_lpuart2_tx; *val = 0; break;
        case pctl_mux_gpio_ad_b1_03: *isel = pctl_isel_lpuart2_rx; *val = 1; break;
        case pctl_mux_gpio_sd_b1_10: *isel = pctl_isel_lpuart2_rx; *val = 0; break;
        case pctl_mux_gpio_emc_13:   *isel = pctl_isel_lpuart3_tx; *val = 1; break;
        case pctl_mux_gpio_ad_b1_06: *isel = pctl_isel_lpuart3_tx; *val = 0; break;
        case pctl_mux_gpio_b0_08:    *isel = pctl_isel_lpuart3_tx; *val = 2; break;
        case pctl_mux_gpio_emc_14:   *isel = pctl_isel_lpuart3_rx; *val = 1; break;
        case pctl_mux_gpio_ad_b1_07: *isel = pctl_isel_lpuart3_rx; *val = 0; break;
        case pctl_mux_gpio_b0_09:    *isel = pctl_isel_lpuart3_rx; *val = 2; break;
        case pctl_mux_gpio_emc_15:   *isel = pctl_isel_lpuart3_cts_b; *val = 0; break;
        case pctl_mux_gpio_ad_b1_04: *isel = pctl_isel_lpuart3_cts_b; *val = 1; break;
        case pctl_mux_gpio_emc_19:   *isel = pctl_isel_lpuart4_tx; *val = 1; break;
        case pctl_mux_gpio_b1_00:    *isel = pctl_isel_lpuart4_tx; *val = 2; break;
        case pctl_mux_gpio_sd_b1_00: *isel = pctl_isel_lpuart4_tx; *val = 0; break;
        case pctl_mux_gpio_emc_20:   *isel = pctl_isel_lpuart4_rx; *val = 1; break;
        case pctl_mux_gpio_b1_01:    *isel = pctl_isel_lpuart4_rx; *val = 2; break;
        case pctl_mux_gpio_sd_b1_01: *isel = pctl_isel_lpuart4_rx; *val = 0; break;
        case pctl_mux_gpio_emc_23:   *isel = pctl_isel_lpuart5_tx; *val = 0; break;
        case pctl_mux_gpio_b1_12:    *isel = pctl_isel_lpuart5_tx; *val = 1; break;
        case pctl_mux_gpio_emc_24:   *isel = pctl_isel_lpuart5_rx; *val = 0; break;
        case pctl_mux_gpio_b1_13:    *isel = pctl_isel_lpuart5_rx; *val = 1; break;
        case pctl_mux_gpio_emc_25:   *isel = pctl_isel_lpuart6_tx; *val = 0; break;
        case pctl_mux_gpio_ad_b0_12: *isel = pctl_isel_lpuart6_tx; *val = 1; break;
        case pctl_mux_gpio_emc_26:   *isel = pctl_isel_lpuart6_rx; *val = 0; break;
        case pctl_mux_gpio_ad_b0_03: *isel = pctl_isel_lpuart6_rx; *val = 1; break;
        case pctl_mux_gpio_emc_31:   *isel = pctl_isel_lpuart7_tx; *val = 1; break;
        case pctl_mux_gpio_sd_b1_08: *isel = pctl_isel_lpuart7_tx; *val = 0; break;
        case pctl_mux_gpio_emc_32:   *isel = pctl_isel_lpuart7_rx; *val = 1; break;
        case pctl_mux_gpio_sd_b1_09: *isel = pctl_isel_lpuart7_rx; *val = 0; break;
        case pctl_mux_gpio_emc_38:   *isel = pctl_isel_lpuart8_tx; *val = 2; break;
        case pctl_mux_gpio_ad_b1_10: *isel = pctl_isel_lpuart8_tx; *val = 1; break;
        case pctl_mux_gpio_sd_b0_04: *isel = pctl_isel_lpuart8_tx; *val = 0; break;
        case pctl_mux_gpio_emc_39:   *isel = pctl_isel_lpuart8_rx; *val = 2; break;
        case pctl_mux_gpio_ad_b1_11: *isel = pctl_isel_lpuart8_rx; *val = 1; break;
        case pctl_mux_gpio_sd_b0_05: *isel = pctl_isel_lpuart8_rx; *val = 0; break;
        default: return -1;
    }

    return 0;
}


static void serial_initPins(void)
{
    int i, isel, val;
    static const int muxes[] = {
#if UART1
        PIN2MUX(UART1_TX_PIN), PIN2MUX(UART1_RX_PIN), PIN2MUX(UART1_RTS_PIN), PIN2MUX(UART1_CTS_PIN),
#endif
#if UART2
        PIN2MUX(UART2_TX_PIN), PIN2MUX(UART2_RX_PIN), PIN2MUX(UART2_RTS_PIN), PIN2MUX(UART2_CTS_PIN),
#endif
#if UART3
        PIN2MUX(UART3_TX_PIN), PIN2MUX(UART3_RX_PIN), PIN2MUX(UART3_RTS_PIN), PIN2MUX(UART3_CTS_PIN),
#endif
#if UART4
        PIN2MUX(UART4_TX_PIN), PIN2MUX(UART4_RX_PIN), PIN2MUX(UART4_RTS_PIN), PIN2MUX(UART4_CTS_PIN),
#endif
#if UART5
        PIN2MUX(UART5_TX_PIN), PIN2MUX(UART5_RX_PIN), PIN2MUX(UART5_RTS_PIN), PIN2MUX(UART5_CTS_PIN),
#endif
#if UART6
        PIN2MUX(UART6_TX_PIN), PIN2MUX(UART6_RX_PIN), PIN2MUX(UART6_RTS_PIN), PIN2MUX(UART6_CTS_PIN),
#endif
#if UART7
        PIN2MUX(UART7_TX_PIN), PIN2MUX(UART7_RX_PIN), PIN2MUX(UART7_RTS_PIN), PIN2MUX(UART7_CTS_PIN),
#endif
#if UART8
        PIN2MUX(UART8_TX_PIN), PIN2MUX(UART8_RX_PIN), PIN2MUX(UART8_RTS_PIN), PIN2MUX(UART8_CTS_PIN)
#endif
    };

    for (i = 0; i < sizeof(muxes) / sizeof(muxes[0]); ++i) {
        _imxrt_setIOmux(muxes[i], 0, serial_muxVal(muxes[i]));



        if (serial_getIsel(muxes[i], &isel, &val) < 0)
            continue;

        _imxrt_setIOisel(isel, val);
    }
}


/* TODO: set baudrate using 'baud' argument */
void serial_init(u32 baud, u32 *st)
{
    u32 t;
    int i, dev;
    serial_t *serial;

    static const u32 fifoSzLut[] = { 1, 4, 8, 16, 32, 64, 128, 256 };
    static const struct {
        volatile u32 *base;
        int dev;
        unsigned irq;
    } info[] = {
        { UART1_BASE, UART1_CLK, UART1_IRQ },
        { UART2_BASE, UART2_CLK, UART2_IRQ },
        { UART3_BASE, UART3_CLK, UART3_IRQ },
        { UART4_BASE, UART4_CLK, UART4_IRQ },
        { UART5_BASE, UART5_CLK, UART5_IRQ },
        { UART6_BASE, UART6_CLK, UART6_IRQ },
        { UART7_BASE, UART7_CLK, UART7_IRQ },
        { UART8_BASE, UART8_CLK, UART8_IRQ }
    };

    *st = 115200;
    serial_initPins();



    _imxrt_ccmSetMux(clk_mux_uart, 0);
    _imxrt_ccmSetDiv(clk_div_uart, 0);



    for (i = 0, dev = 0; dev < sizeof(serialConfig) / sizeof(serialConfig[0]); ++dev) {
        if (!serialConfig[dev])
            continue;

        serial = &serial_common.serials[i++];
        serial->base = info[dev].base;
        serial->rxHead = 0;
        serial->txHead = 0;
        serial->rxTail = 0;

        serial->txTail = 0;
        serial->tFull = 0;

        /* Disable TX and RX */
        *(serial->base + ctrlr) &= ~((1 << 19) | (1 << 18));

        /* Reset all internal logic and registers, except the Global Register */
        *(serial->base + globalr) |= 1 << 1;
        imxrt_dataBarrier();
        *(serial->base + globalr) &= ~(1 << 1);
        imxrt_dataBarrier();

        /* Disable input trigger */
        *(serial->base + pincfgr) &= ~3;

        /* Set 115200 default baudrate */
        t = *(serial->base + baudr) & ~((0x1f << 24) | (1 << 17) | 0xfff);
        *(serial->base + baudr) = t | calculate_baudrate(115200);

        /* Set 8 bit and no parity mode */
        *(serial->base + ctrlr) &= ~0x117;

        /* One stop bit */
        *(serial->base + baudr) &= ~(1 << 13);

        *(serial->base + waterr) = 0;

        /* Enable FIFO */
        *(serial->base + fifor) |= (1 << 7) | (1 << 3);
        *(serial->base + fifor) |= 0x3 << 14;

        /* Clear all status flags */
        *(serial->base + statr) |= 0xc01fc000;

        serial->rxFifoSz = fifoSzLut[*(serial->base + fifor) & 0x7];
        serial->txFifoSz = fifoSzLut[(*(serial->base + fifor) >> 4) & 0x7];

        /* Enable receiver interrupt */
        *(serial->base + ctrlr) |= 1 << 21;

        /* Enable TX and RX */
        *(serial->base + ctrlr) |= (1 << 19) | (1 << 18);

        _imxrt_setDevClock(info[dev].dev, clk_state_run);

        low_irqinst(info[dev].irq, serial_handleIntr, (void *)serial);
    }

    return;
}


void serial_done(void)
{
    int i, dev;
    serial_t *serial;

    for (i = 0, dev = 0; dev < sizeof(serialConfig) / sizeof(serialConfig[0]); ++dev) {
        if (!serialConfig[dev])
            continue;

        serial = &serial_common.serials[i++];

        /* disable TX and RX */
        *(serial->base + ctrlr) &= ~((1 << 19) | (1 << 18));
        *(serial->base + ctrlr) &= ~((1 << 23) | (1 << 21));

        low_irquninst(serial->irq);
    }
}
