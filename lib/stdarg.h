/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Variable arguments handling
 *
 * Copyright 2021 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Copyright 2006 Radoslaw F. Wawrzusiak
 * Author: Pawel Pisarczyk, Radoslaw F. Wawrzusiak, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _LIB_STDARG_H_
#define _LIB_STDARG_H_

#include <hal/hal.h>


typedef __builtin_va_list va_list;
#define va_start(ap, parmN) __builtin_va_start(ap, parmN)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)
#define va_end(ap)          __builtin_va_end(ap)


#endif
