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

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS:=$(filter-out -mfpu% , $(CFLAGS))

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

OBJS += $(addprefix $(PREFIX_O)$(TARGET_FAMILY)-$(TARGET_SUBFAMILY)/, _startup.o low.o zynq.o serial.o timer.o syspage.o cmd.o interrupts.o usbphy.o gpio.o)
