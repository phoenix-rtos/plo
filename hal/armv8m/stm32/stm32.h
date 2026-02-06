/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * STM32 ARMv8-M MCU specific header dispatch 
 *
 * Copyright 2026 Apator Metrix
 * Author: Mateusz Karcz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#if defined(__CPU_STM32N6)
#include "n6/stm32n6.h"
#elif defined(__CPU_STM32U5)
#include "u5/stm32u5.h"
#else
#error "Unsupported platform"
#endif
