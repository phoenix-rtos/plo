/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * RV64 CSR definitions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CSR_H_
#define _CSR_H_

/* Unprivileged CSRs */

#define CSR_CYCLE 0xc00u
#define CSR_TIME  0xc01u

/* Supervisor CSRs */

#define CSR_SSTATUS 0x100u
#define CSR_SIE     0x104u
#define CSR_STVEC   0x105u

#define CSR_SENVCFG 0x10au

#define CSR_SSCRATCH 0x140u
#define CSR_SEPC     0x141u
#define CSR_SCAUSE   0x142u
#define CSR_STVAL    0x143u
#define CSR_SIP      0x144u


#endif
