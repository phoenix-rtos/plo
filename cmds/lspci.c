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


static void readPCIDevice(hal_pciAddr_t *bdf, struct PCIDevice *pciDevice)
{
	u32 cfg = hal_pciRead32(bdf, HAL_PCI_REG_PCI_ID / sizeof(u32));
	pciDevice->vendorID = (u16)cfg;
	pciDevice->deviceID = (u16)(cfg >> 16u);

	cfg = hal_pciRead32(bdf, HAL_PCI_REG_CLASS / sizeof(u32));
	pciDevice->subclass = (u8)(cfg >> 16u);
	pciDevice->classCode = (u8)(cfg >> 24u);

	cfg = hal_pciRead32(bdf, HAL_PCI_REG_HEADER_TYPE / sizeof(u32));
	pciDevice->headerType = (u8)(cfg >> 16u);
}


static void listDevices(void)
{
	struct PCIDevice pciDevice;
	hal_pciAddr_t bdf;
	const char *name;

	lib_printf("\n\033[1mBUS\tDEVICE\tFUNC\tVID\tDID\tCLASS\tDESCRIPTION\033[0m\n");

	for (bdf.bus = 0u; bdf.bus < HAL_PCI_NUM_DEVICES; ++bdf.bus) {
		for (bdf.device = 0u; bdf.device < HAL_PCI_NUM_DEVICES; ++bdf.device) {
			for (bdf.function = 0u; bdf.function < HAL_PCI_NUM_FUNCS; ++bdf.function) {
				readPCIDevice(&bdf, &pciDevice);

				if (pciDevice.vendorID == 0xffffu) {
					continue;
				}

				lib_printf("0x%02x\t0x%02x\t0x%x\t0x%04x\t0x%04x\t0x%02x%02x\t",
					bdf.bus, bdf.device, bdf.function,
					pciDevice.vendorID, pciDevice.deviceID,
					pciDevice.classCode, pciDevice.subclass);

				name = pciClassName(pciDevice.classCode);
				lib_printf("%.*s\n", 30, (name != NULL) ? name : "Unknown device class");

				if ((bdf.function == 0u) && ((pciDevice.headerType & 0x80u) == 0u)) {
					break;
				}
			}
		}
	}
}


static void dumpConfig(hal_pciAddr_t *bdf)
{
	size_t offs;
	u32 val;

	lib_printf("\033[1mOFFSET | CONFIG DATA\033[0m\n");

	for (offs = 0; offs < 256 / sizeof(val); ++offs) {
		if ((offs & 3) == 0) {
			lib_printf("0x%02x   | ", offs * sizeof(val));
		}

		val = hal_pciRead32(bdf, offs);
		lib_printf("0x%08x%c", val, ((offs & 3) == 3) ? '\n' : ' ');
	}
}


static int parseBDF(char *str, hal_pciAddr_t *bdf)
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
	bdf->bus = val;

	ptr++;
	val = lib_strtoul(ptr, &ptr, 0);
	if ((*ptr != ':') || (val > 0x1f)) {
		return -EINVAL;
	}
	bdf->device = val;

	ptr++;
	val = lib_strtoul(ptr, &ptr, 0);
	if ((*ptr != '\0') || (val > 7)) {
		return -EINVAL;
	}
	bdf->function = val;

	return 0;
}


static int cmd_lspciMain(int argc, char *argv[])
{
	int opt;
	int optDumpRegs = 0;
	hal_pciAddr_t bdf = { 0 };

	lib_printf("\n");

	for (;;) {
		opt = lib_getopt(argc, argv, "d:h");
		if (opt == -1) {
			break;
		}

		if (opt == 'd') {
			if ((optDumpRegs == 0) && (parseBDF(optarg, &bdf) == 0)) {
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
		dumpConfig(&bdf);
	}
	else {
		listDevices();
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t lspci_cmd __attribute__((section("commands"), used)) = {
	.name = "lspci", .run = cmd_lspciMain, .info = cmd_lspciInfo
};
