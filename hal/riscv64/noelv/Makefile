#
# Makefile for Phoenix-RTOS loader (RISC-V 64 NOEL-V HAL)
#
# Copyright 2024 Phoenix Systems
#
# %LICENSE%
#

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

PLO_COMMANDS ?= alias app call console copy dump echo go help kernel map mem phfs reboot script wait

PLO_ALLDEVICES := ram-storage uart-grlib

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, console.o noelv.o hal.o)
