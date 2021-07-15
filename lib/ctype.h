/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Character type - code derived from libphoenix
 *
 * Copyright 2017, 2021 Phoenix Systems
 * Author: Adrian Kepka, Hubert Buczynski, Gerard Swiderski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _LIB_CTYPE_H_
#define _LIB_CTYPE_H_


extern int lib_islower(int c);


extern int lib_isupper(int c);


extern int lib_isalpha(int c);


extern int lib_isdigit(int c);


extern int lib_isblank(int c);


extern int lib_isspace(int c);


extern int lib_isgraph(int c);


extern int lib_isprint(int c);


#endif
