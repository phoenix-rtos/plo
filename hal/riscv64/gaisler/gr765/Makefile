#
# Makefile for Phoenix-RTOS loader (RISC-V 64 GR765 HAL)
#
# Copyright 2025 Phoenix Systems
#
# %LICENSE%
#

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

PLO_COMMANDS ?= alias app blob call console copy dump echo erase go help jffs2 kernel map mem phfs reboot script test-dev wait

PLO_ALLDEVICES := uart-grlib ram-storage flash-nandfctrl2

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/gaisler/$(TARGET_SUBFAMILY)/, gr765.o)
