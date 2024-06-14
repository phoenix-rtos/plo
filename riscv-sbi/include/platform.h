/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * Platform layer
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _SBI_PLATFORM_H_
#define _SBI_PLATFORM_H_


/* Called very early in the boot sequence, must not depend on any other subsystems.
 * May be used to initialize early CPU features, such as caches. */
void platform_earlyInit(void);


#endif
