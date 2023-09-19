/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ACPI detection and configuration
 *
 * Copyright 2023 Phoenix Systems
 * Author: Andrzej Stalke
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */
#ifndef _ACPI_H_
#define _ACPI_H_
#include <hal/hal.h>

void hal_acpiInit(hal_syspage_t *hs);

#endif
