/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * comamnds script parser
 *
 * Copyright 2020 Phoenix Systems
 * Authors: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef SCRIPT_H_
#define SCRIPT_H_

#include "types.h"



extern void script_init(void);


extern void script_run(void);


extern int script_expandAlias(char **name);


#endif
