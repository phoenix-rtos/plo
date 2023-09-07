/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Enumerate PCI devices and read config data
 *
 * Copyright 2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <hal/ia32/pci.h>
#include <lib/lib.h>

#include "cmd.h"


struct PCIDevice {
	u16 vendorID;
	u16 deviceID;
	u8 classCode;
	u8 subclass;
	u8 headerType;
};


static void cmd_lspciInfo(void)
{
	lib_printf("enumerates PCI devices and read config data");
}


static void print_usage(const char *name)
{
	lib_printf(
		"Usage: %s <options>\n"
		"\t-d bus:dev:func  dump pci device config data\n"
		"\t-h               prints help\n",
		name);
}


static const char *pciClassName(u8 classId)
{
	static const char *pciClassNames[] = {
		"Undefined",
		"Mass Storage Controller",
		"Network Controller",
		"Display Controller",
		"Multimedia Controller",
		"Memory Controller",
		"Bridge Device",
		"Simple Communication Controller",
		"Base System Peripheral",
		"Input Device Controller",
		"Docking Station",
		"Processor",
		"Serial Bus Controller",
		"Wireless Controller",
		"Intelligent I/O Controller",
		"Satellite Communication Controller",
		"Encryption Controller",
		"Data Acquisition and Signal Processing Controller",
		"Processing Accelerator",
		"Non-Essential Instrumentation",
	};

	if (classId < (sizeof(pciClassNames) / sizeof(pciClassNames[0]))) {
		return pciClassNames[classId];
	}
	else if (classId == 0x40) {
		return "Co-Processor";
	}
	else if (classId == 0xff) {
		return "Unassigned (Vendor specific)";
	}
	else {
		return NULL;
	}
}


static void readPCIDevice(u8 bus, u8 dev, u8 func, struct PCIDevice *pciDevice)
{
	u32 cfg = hal_pciRead32(bus, dev, func, 0);
	pciDevice->vendorID = (u16)cfg;
	pciDevice->deviceID = (u16)(cfg >> 16);

	cfg = hal_pciRead32(bus, dev, func, 2);
	pciDevice->subclass = (u8)(cfg >> 16);
	pciDevice->classCode = (u8)(cfg >> 24);

	cfg = hal_pciRead32(bus, dev, func, 3);
	pciDevice->headerType = (u8)(cfg >> 16);
}


static void listDevices(void)
{
	struct PCIDevice pciDevice;
	unsigned int bus;
	unsigned int dev;
	unsigned int func;
	const char *name;

	lib_printf("\n\033[1mBUS\tDEVICE\tFUNC\tVID\tDID\tCLASS\tDESCRIPTION\033[0m\n");

	for (bus = 0; bus < 256u; ++bus) {
		for (dev = 0; dev < 32u; ++dev) {
			for (func = 0; func < 8u; ++func) {
				readPCIDevice(bus, dev, func, &pciDevice);

				if (pciDevice.vendorID == 0xffff) {
					continue;
				}

				lib_printf("0x%02x\t0x%02x\t0x%x\t0x%04x\t0x%04x\t0x%02x%02x\t",
					bus, dev, func, pciDevice.vendorID, pciDevice.deviceID, pciDevice.classCode, pciDevice.subclass);

				name = pciClassName(pciDevice.classCode);
				lib_printf("%.*s\n", 30, (name != NULL) ? name : "Unknown device class");

				if ((func == 0) && ((pciDevice.headerType & 0x80) == 0)) {
					break;
				}
			}
		}
	}
}


static void dumpConfig(u8 bus, u8 dev, u8 func)
{
	size_t offs;
	u32 val;

	lib_printf("\033[1mOFFSET | CONFIG DATA\033[0m\n");

	for (offs = 0; offs < 256 / sizeof(val); ++offs) {
		if ((offs & 3) == 0) {
			lib_printf("0x%02x   | ", offs * sizeof(val));
		}

		val = hal_pciRead32(bus, dev, func, offs);
		lib_printf("0x%08x%c", val, ((offs & 3) == 3) ? '\n' : ' ');
	}
}


static int parseBDF(char *str, u8 *bus, u8 *dev, u8 *func)
{
	unsigned int val;
	char *ptr = str;

	if ((ptr == NULL) || (*ptr == '\0')) {
		return -EINVAL;
	}

	val = lib_strtoul(ptr, &ptr, 0);
	if ((*ptr != ':') || (val > 0xff)) {
		return -EINVAL;
	}
	*bus = val;

	ptr++;
	val = lib_strtoul(ptr, &ptr, 0);
	if ((*ptr != ':') || (val > 0x1f)) {
		return -EINVAL;
	}
	*dev = val;

	ptr++;
	val = lib_strtoul(ptr, &ptr, 0);
	if ((*ptr != '\0') || (val > 7)) {
		return -EINVAL;
	}
	*func = val;

	return 0;
}


static int cmd_lspciMain(int argc, char *argv[])
{
	int opt;
	int optDumpRegs = 0;
	u8 bus, dev, func;

	lib_printf("\n");

	for (;;) {
		opt = lib_getopt(argc, argv, "d:h");
		if (opt == -1) {
			break;
		}

		if (opt == 'd') {
			if ((optDumpRegs == 0) && (parseBDF(optarg, &bus, &dev, &func) == 0)) {
				optDumpRegs = 1;
				continue;
			}
			lib_printf("%s: invalid arguments\n", argv[0]);
		}

		print_usage(argv[0]);
		return CMD_EXIT_FAILURE;
	}

	if (hal_pciDetect() < 0) {
		log_error("\n%s: PCI bus is unavailable", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	if (optDumpRegs != 0) {
		dumpConfig(bus, dev, func);
	}
	else {
		listDevices();
	}

	return CMD_EXIT_SUCCESS;
}


__attribute__((constructor)) static void cmd_lspciReg(void)
{
	const static cmd_t app_cmd = {
		.name = "lspci", .run = cmd_lspciMain, .info = cmd_lspciInfo
	};

	cmd_reg(&app_cmd);
}
