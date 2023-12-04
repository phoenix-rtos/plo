/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Interaction with user (prompt, ask, display message)
 *
 * Copyright 2022 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "lib.h"


int lib_promptConfirm(const char *message, const char *answer, time_t timeout)
{
	char c;
	unsigned int pos = 0;

	/*
	 * Use "%s" format in `message` to provide a hint of what to type
	 * from optional `answer string.
	 */
	lib_printf(message, answer);

	for (;;) {
		lib_consoleGetc(&c, timeout);

		if (c == 3) {
			/* ^C -- abort */
			return -EINTR;
		}

		if (answer != NULL && *answer != '\0') {
			if (answer[pos] == c) {
				lib_consolePutc(c);
				pos++;

				if (answer[pos] == '\0') {
					/* User confirmed by entering answer */
					return 1;
				}

				continue;
			}

			while (pos-- > 0) {
				lib_consolePuts("\b \b");
			}
		}

		break;
	}

	/* Any other input, or not confirmed answer */
	return 0;
}
