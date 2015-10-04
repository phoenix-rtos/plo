/* 
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Common types
 *
 * Copyright 2001, 2005 Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * Phoenix-RTOS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Phoenix-RTOS kernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Phoenix-RTOS kernel; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _TYPES_H_
#define _TYPES_H_


typedef unsigned char u8;
typedef unsigned int  u16;
typedef unsigned long u32;
typedef long          s32;


/* stdarg definitions */
typedef u8 *va_list;

#define va_start(ap, parmN) ((void)((ap) = (va_list)((char *)(&parmN) + sizeof(parmN))))
#define va_arg(ap, type) (*(type *)(((*(char **)&(ap)) += sizeof(type)) - (sizeof(type))))
#define va_end(ap) ((void)0)


#endif
