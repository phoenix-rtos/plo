/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * connect/bridge two serial devices (USB, UART or PIPE)
 *
 * Copyright 2024 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <devices/devs.h>

#include "cmd.h"

typedef struct {
	int major, minor;
} pair_t;


static int getPair(pair_t *p, char *str)
{
	char *endptr;
	p->major = lib_strtol(str, &endptr, 0);
	if (((*endptr != ':') && (*endptr != '.')) || (str == endptr) || (p->major < 0)) {
		return -1;
	}
	str = ++endptr;
	p->minor = lib_strtol(str, &endptr, 0);
	return ((*endptr != '\0') || (str == endptr) || (p->minor < 0)) ? -1 : 0;
}


static int validatePair(pair_t *p)
{
	int majors[] = { DEV_UART, DEV_USB, DEV_PIPE };
	unsigned int ctx = 0;
	unsigned int major = 0;
	unsigned int minor = 0;
	int i, res = -1;

	if ((p->major < 0) && (p->minor < 0)) {
		return -1;
	}

	for (i = 0; i < sizeof(majors) / sizeof(majors[0]); i++) {
		if (p->major == majors[i]) {
			res = 0;
			break;
		}
	}

	while (res == 0) {
		const dev_t *dev = devs_iterNext(&ctx, &major, &minor);
		if (dev == DEVS_ITER_STOP) {
			res = -1;
			break;
		}

		if ((dev != NULL) && (p->major == major) && (p->minor == minor)) {
			res = 0;
			break;
		}
	}

	return res;
}


static void bufprint(const char *prefix, void *data, size_t len)
{
	u8 *ptr = data;
	size_t todo = len;
	lib_printf("%s: ", prefix);
	while (todo-- != 0) {
		lib_printf("%02X ", *(ptr++));
	}
	lib_printf("\n");
}


static void cmd_bridgeInfo(void)
{
	lib_printf("bridges two serial devices (UART, USB or PIPE)");
}


static void print_usage(const char *name)
{
	lib_printf(
		"Usage: %s <options>\n"
		"\tConnects two serial devices (A)TX/RX <--> RX/TX(B)\n"
		"\t-a <major:minor>  device A is required (L prefix enables A dump)\n"
		"\t-b <major:minor>  device B is required (L prefix enables B dump)\n"
		"\t-A <baud>         optional baud speed of device A\n"
		"\t-B <baud>         optional baud speed of device B\n"
		"\t-c <major:minor>  use device C as monitor device\n"
		"\t-C                use console as default device '-c'\n",
		name);
}


static int cmd_bridge(int argc, char *argv[])
{
	u8 buf[64];
	ssize_t res;
	int tmp, spda = -1, spdb = -1;
	int loga = 0, logb = 0, con = 0;
	pair_t deva = { -1, -1 };
	pair_t devb = { -1, -1 };
	pair_t devc = { -1, -1 };
	char *endptr;

	lib_printf("\n");

	if (argc == 1) {
		print_usage(argv[0]);
		return CMD_EXIT_FAILURE;
	}

	for (;;) {
		int opt = lib_getopt(argc, argv, "a:A:b:B:c:C");
		if (opt == -1) {
			break;
		}

		switch (opt) {
			case 'a':
				if ((optarg[0] | 0x20) == 'l') {
					optarg++;
					loga = 1;
				}
				if (getPair(&deva, optarg) < 0) {
					log_error("%s: Invalid arguments", argv[0]);
					return CMD_EXIT_FAILURE;
				}
				break;

			case 'b':
				if ((optarg[0] | 0x20) == 'l') {
					optarg++;
					logb = 1;
				}
				if (getPair(&devb, optarg) < 0) {
					log_error("%s: Invalid arguments", argv[0]);
					return CMD_EXIT_FAILURE;
				}
				break;

			case 'C':
				con = 1;
				break;

			case 'c':
				con = 1;
				if (optarg != NULL) {
					if (getPair(&devc, optarg) < 0) {
						log_error("%s: Invalid arguments", argv[0]);
						return CMD_EXIT_FAILURE;
					}
				}
				break;

			case 'A':
				tmp = (int)lib_strtol(optarg, &endptr, 10);
				if ((*endptr != '\0') || (tmp < 0)) {
					log_error("%s: Invalid arguments", argv[0]);
					return CMD_EXIT_FAILURE;
				}
				spda = tmp;
				break;

			case 'B':
				tmp = (int)lib_strtol(optarg, &endptr, 10);
				if ((*endptr != '\0') || (tmp < 0)) {
					log_error("%s: Invalid arguments", argv[0]);
					return CMD_EXIT_FAILURE;
				}
				spdb = tmp;
				break;

			case 'h':
			default:
				print_usage(argv[0]);
				return CMD_EXIT_FAILURE;
		}
	}

	if ((validatePair(&deva) < 0) || (validatePair(&devb) < 0) ||
		((devc.major >= 0) && (validatePair(&devc) < 0))) {

		log_error("%s: Invalid device pair need to be UART, USB or PIPE\n", argv[0]);
		print_usage(argv[0]);
		return CMD_EXIT_FAILURE;
	}

	if (spda >= 0) {
		devs_control(deva.major, deva.minor, DEV_CONTROL_SETBAUD, &spda);
	}

	if (spdb >= 0) {
		devs_control(devb.major, devb.minor, DEV_CONTROL_SETBAUD, &spdb);
	}

	if (con != 0) {
		lib_printf("Bridged devices: A (%d:%d @ %d) <---> (%d:%d @ %d) B\n",
			deva.major, deva.minor, spda, devb.major, devb.minor, spdb);
		lib_printf("Press '!' to terminate connection\n");
	}

	for (;;) {
		if (con != 0) {
			u8 c = 0;
			if ((devc.major < 0) || (devc.minor < 0)) {
				lib_consoleGetc((char *)&c, 0);
			}
			else {
				devs_read(devc.major, devc.minor, 0, &c, 1, 0);
			}
			if (c == '!') {
				break;
			}
		}

		res = devs_read(deva.major, deva.minor, 0, buf, sizeof(buf), 0);
		if (res > 0) {
			devs_write(devb.major, devb.minor, 0, buf, res);
			if (loga != 0) {
				bufprint("A->B", buf, res);
			}
		}

		res = devs_read(devb.major, devb.minor, 0, buf, sizeof(buf), 0);
		if (res > 0) {
			devs_write(deva.major, deva.minor, 0, buf, res);
			if (logb != 0) {
				bufprint("B->A", buf, res);
			}
		}
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t bridge_cmd __attribute__((section("commands"), used)) = {
	.name = "bridge", .run = cmd_bridge, .info = cmd_bridgeInfo
};
