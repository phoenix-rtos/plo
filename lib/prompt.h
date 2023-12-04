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

#ifndef _LIB_PROMPT_H_
#define _LIB_PROMPT_H_


/* Prompt the user with a message to confirm next action by typing an answer */
extern int lib_promptConfirm(const char *message, const char *answer, time_t timeout);


#endif
