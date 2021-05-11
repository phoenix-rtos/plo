/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Standard functions
 *
 * Copyright 2012 Phoenix Systems
 * Copyright 2001, 2005, 2020 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _PLOSTD_H_
#define _PLOSTD_H_

#include "types.h"

/* User interface */
#define PROMPT       "(plo)% "

#define LINESZ       80
#define HISTSZ       8


/* Console character attributes */
#define ATTR_NONE    0
#define ATTR_DEBUG   2
#define ATTR_USER    7
#define ATTR_INIT    13
#define ATTR_LOADER  15
#define ATTR_ERROR   4


extern int plostd_ishex(const char *s);

extern int plostd_isalnum(char c);

extern unsigned int plostd_strlen(const char *s);

extern int plostd_strcmp(const char *s1, const char *s2);

extern int plostd_strncmp(const char *s1, const char *s2, u32 n);

extern char *plostd_itoa(unsigned int i, char *buff);

extern char *plostd_itoah(u8 *ip, u8 is, char *buff, int lz);

extern unsigned int plostd_ahtoi(const char *s);

extern char *plostd_ltoa(unsigned long n, unsigned int base, char *buff);

extern unsigned long plostd_atol(const char *s);

extern void plostd_puts(const char *s);

extern void plostd_setattr(char attr);

extern void plostd_printf(char attr, const char *, ...);


#endif
