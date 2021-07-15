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


typedef u8 *va_list;

#define va_start(ap, parmN) ((void)((ap) = (va_list)((char *)(&parmN) + sizeof(parmN))))
#define va_arg(ap, type)    (*(type *)(((*(u8 **)&(ap)) += sizeof(type)) - (sizeof(type))))
#define va_end(ap)          ((void)0)


#endif
