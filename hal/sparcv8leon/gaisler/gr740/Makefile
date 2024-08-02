#
# Makefile for Phoenix-RTOS loader (SPARCV8 LEON4 GR740 HAL)
#
# Copyright 2024 Phoenix Systems
#
# %LICENSE%
#

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

PLO_COMMANDS ?= alias app blob call console copy dump echo go help jffs2 kernel map mem phfs reboot script stop wait test-dev

PLO_ALLDEVICES := uart-grlib

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/gaisler/$(TARGET_SUBFAMILY)/, _init.o hal.o gr740.o)
