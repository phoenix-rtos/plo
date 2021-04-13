#
# Makefile for plo/armv7a9-zynq7000
#
# Copyright 2020 Phoenix Systems
#
# %LICENSE%
#

INIT_RAM = 0x0
INIT_FLASH = 0x0

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))
LDFLAGS+=--defsym _plo_bss=__bss_start

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS:=$(filter-out -mfpu% , $(CFLAGS))

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

include devices/gpio-zynq7000/Makefile
include devices/usbc-cdc/Makefile
include devices/uart-zynq7000/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, _startup.o hal.o zynq.o timer.o\
		cmd.o interrupts.o)
