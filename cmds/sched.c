/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Configure scheduler
 *
 * Copyright 2026 Phoenix Systems
 * Author: Jakub Klimek
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cmd.h"
#include "elf.h"

#include <lib/lib.h>
#include <hal/hal.h>
#include <phfs/phfs.h>
#include <syspage.h>


static void cmd_schedInfo(void)
{
	lib_printf("configures scheduler, usage: sched [window1Size;window2Size...]");
}


static syspage_sched_cycle_t *cmd_parseCycle(char *desc, unsigned char windowCnt)
{
	syspage_sched_cycle_t *cycle;

	unsigned char id;
	unsigned int i = 0;
	char *cur = desc;
	unsigned long duration = 0;

	while (*cur != '\0') {
		if (*cur == ':') {
			i++;
		}
		cur++;
	}

	cycle = syspage_alloc(sizeof(*cycle) + sizeof(*cycle->windows) * i);
	if (cycle == NULL) {
		log_error("\nCannot allocate memory for scheduler configuration");
		return NULL;
	}
	cycle->len = i;

	cur = desc;
	id = lib_strtoul(cur, &cur, 0);
	if (id > windowCnt) {
		log_error("\nInvalid scheduler cycle description: window ID exceeds window count");
		return NULL;
	}
	if (*cur == ':') {
		/* No background window, use system window */
		cycle->bgId = 0U;
		cur = desc;
	}
	else if ((*cur == '\0') || (*cur == ';')) {
		cycle->bgId = id;
		if (*cur == ';') {
			cur++;
		}
	}
	else {
		log_error("\nInvalid scheduler cycle description");
		return NULL;
	}
	for (i = 0; *cur != '\0'; i++) {
		if (i >= cycle->len) {
			log_error("\nInvalid scheduler cycle description");
			return NULL;
		}
		cycle->windows[i].id = lib_strtoul(cur, &cur, 0);
		if (cycle->windows[i].id > windowCnt) {
			log_error("\nInvalid scheduler cycle description: window ID exceeds window count");
			return NULL;
		}
		if (*cur != ':') {
			log_error("\nInvalid scheduler cycle description");
			return NULL;
		}
		duration += lib_strtoul(cur + 1, &cur, 0);
		if (duration == 0U) {
			log_error("\nInvalid scheduler cycle: window duration cannot be zero");
			return NULL;
		}
		cycle->windows[i].stop = duration;
		if (*cur == ';') {
			cur++;
		}
		else if (*cur != '\0') {
			log_error("\nInvalid scheduler cycle description");
			return NULL;
		}
	}
	return cycle;
}


static int cmd_sched(int argc, char *argv[])
{
	unsigned int i;
	syspage_sched_t *config;

	/* Parse command arguments */
	if (argc < 3) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	config = syspage_alloc(sizeof(syspage_sched_t) + sizeof(*config->cycles) * (argc - 2));
	if (config == NULL) {
		log_error("\n%s: Cannot allocate memory for scheduler configuration", argv[0]);
		return CMD_EXIT_FAILURE;
	}
	config->flags = 0U;

	/* ARG_0: command name */

	/* ARG_1: window count */
	config->windowCnt = lib_strtoul(argv[1], NULL, 10);

	/* ARG_2...N: cycles descriptions */
	config->cycleCnt = argc - 2;

	for (i = 0; i < config->cycleCnt; ++i) {
		config->cycles[i] = cmd_parseCycle(argv[i + 2], config->windowCnt);
		if (config->cycles[i] == NULL) {
			log_error("\n%s: Cannot parse scheduler cycle description", argv[0]);
			return CMD_EXIT_FAILURE;
		}
	}
	if (syspage_schedulerConfigSet(config) != EOK) {
		log_error("\n%s: Scheduler is already configured!", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t sched_cmd __attribute__((section("commands"), used)) = {
	.name = "sched", .run = cmd_sched, .info = cmd_schedInfo
};
