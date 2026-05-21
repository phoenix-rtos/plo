/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Create partition
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


#define ACC_MAPS_MAX 32


static void cmd_portInfo(void)
{
	lib_printf("creates named port, usage: port <name> <recvPart1;recvPart2...> <sendPart1;sendPart2...>");
}


static size_t cmd_listParse(char *list, char sep)
{
	size_t nb = 0;

	while (*list != '\0') {
		if (*list == sep) {
			*list = '\0';
			++nb;
		}
		list++;
	}

	return ++nb;
}


static int cmd_partNamesToBitmask(char *list, char sep, unsigned int *mask)
{
	size_t nb = cmd_listParse(list, sep);
	char *name = list;
	size_t i;
	syspage_part_t *part;
	int res;

	*mask = 0;

	for (i = 0; i < nb; i++) {
		res = syspage_partResolve(name, &part);
		if (res != EOK) {
			log_error("\nNo such partition to access a named port: %s", name);
			return -EINVAL;
		}
		*mask |= (1UL << part->id);
		name += hal_strlen(name) + 1;
	}
	return EOK;
}


static int cmd_port(int argc, char *argv[])
{
	int res, argvID = 0;

	char *recvParts, *sendParts;

	const char *name;
	unsigned int mask;
	syspage_named_port_t *port;

	/* Parse command arguments */
	if (argc != 4) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	/* ARG_0: command name */

	/* ARG_1: port name */
	argvID = 1;
	name = argv[argvID++];

	/* ARG_2: receiving parts */
	recvParts = argv[argvID++];

	/* ARG_3: sending parts */
	sendParts = argv[argvID++];

	port = syspage_namedPortAdd();
	if (port == NULL) {
		log_error("\nCannot allocate memory for %s", name);
		return -ENOMEM;
	}

	port->name = syspage_alloc(hal_strlen(name) + 1);
	if (port->name == NULL) {
		log_error("\nCannot allocate memory for %s", name);
		return -ENOMEM;
	}
	hal_strcpy(port->name, name);

	res = cmd_partNamesToBitmask(recvParts, ';', &mask);
	if (res < 0) {
		return res;
	}
	port->recvMask = mask;
	res = cmd_partNamesToBitmask(sendParts, ';', &mask);
	if (res < 0) {
		return res;
	}
	port->sendMask = mask;

	return CMD_EXIT_SUCCESS;
}


static const cmd_t port_cmd __attribute__((section("commands"), used)) = {
	.name = "port", .run = cmd_port, .info = cmd_portInfo
};
