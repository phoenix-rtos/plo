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


#define MAX_SCRIPT_LINES_NB 64


extern void script_init(void);


extern void script_run(void);


#endif
