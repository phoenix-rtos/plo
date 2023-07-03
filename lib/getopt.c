/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * getopt - command arguments parser
 *
 * Copyright 2018-2022 Phoenix Systems
 * Author: Jan Sikorski, Krystian Wasik
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "lib.h"
#include <hal/string.h>


/* globals */
char *optarg;
int optind;
int opterr;
int optopt;


static struct {
	int optwhere;
} getopt_common;


void lib_getoptReset(void)
{
	optarg = NULL;
	optind = 1;
	opterr = 1;
	optopt = 0;
	getopt_common.optwhere = 1;
}


int lib_getopt(int argc, char *const argv[], const char *optstring)
{
	char opt;
	char *optspec;
	int leading_colon;

	if (optind <= 0) {
		lib_getoptReset();
	}

	if ((argc == 0) || (argv == NULL) || (optind >= argc)) {
		return -1;
	}

	if ((argv[optind] == NULL) || (argv[optind][0] != '-') || (argv[optind][1] == '\0')) {
		return -1;
	}

	if ((argv[optind][1] == '-') && (argv[optind][2] == '\0')) {
		optind++;
		return -1;
	}

	opt = argv[optind][getopt_common.optwhere++];
	leading_colon = ((*optstring == ':') ? 1 : 0);

	optspec = hal_strchr(optstring, opt);
	if (optspec == NULL) {
		/* Unrecognized option */
		optopt = opt;
		if ((opterr != 0) && (leading_colon == 0)) {
			lib_printf("%s: illegal option -- %c\n", argv[0], opt);
		}
		if (argv[optind][getopt_common.optwhere] == '\0') {
			optind++;
			getopt_common.optwhere = 1;
		}
		return '?';
	}

	if (*(++optspec) == ':') {
		if (argv[optind][getopt_common.optwhere] != '\0') {
			/* Argument in the same argv */
			optarg = &argv[optind][getopt_common.optwhere];
		}
		else if (*(++optspec) == ':') {
			/* Optional argument */
			optarg = NULL;
		}
		else if (optind + 1 < argc) {
			/* Argument in the next argv */
			optarg = argv[optind + 1];
			optind++;
		}
		else {
			/* Argument not provided */
			optopt = opt;
			optind++;
			getopt_common.optwhere = 1;
			if (leading_colon != 0) {
				return ':';
			}
			else {
				if (opterr != 0) {
					lib_printf("%s: option requires an argument -- %c\n", argv[0], opt);
				}
				return '?';
			}
		}
	}
	else if (argv[optind][getopt_common.optwhere] != '\0') {
		/* More options in the same argv */
		return opt;
	}

	optind++;
	getopt_common.optwhere = 1;

	return opt;
}
