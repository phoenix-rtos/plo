/*
 * Phoenix SBI
 *
 * Platform config
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _SBI_CONFIG_H_
#define _SBI_CONFIG_H_


#if defined(__CPU_GR765)
#include "ld/gr765.ldt"
#elif defined(__CPU_GRFPGA)
#include "ld/grfpga.ldt"
#elif defined(__CPU_GENERIC)
#include "ld/generic.ldt"
#else
#error "Unsupported TARGET"
#endif

#endif
