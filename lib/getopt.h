/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * getopt - command arguments parser
 *
 * Copyright 2022 Phoenix Systems
 * Author: Jan Sikorski, Krystian Wasik
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _LIB_GETOPT_H_
#define _LIB_GETOPT_H_


extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;


/* Resets getopt "opt*" globals to initial state */
extern void lib_getoptReset(void);


/* Parses command-line arguments */
extern int lib_getopt(int argc, char *const argv[], const char *optstring);


#endif
