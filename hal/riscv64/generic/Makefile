#
# Makefile for Phoenix-RTOS loader (RISC-V 64 Generic HAL)
#
# Copyright 2024 Phoenix Systems
#
# %LICENSE%
#

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

PLO_COMMANDS ?= alias app call console copy dump echo go help kernel map mem phfs reboot script wait

# TODO: Consider how conflicting devices should be handled
PLO_ALLDEVICES := ram-storage
ifeq ($(TARGET_PROJECT),spike)
  PLO_ALLDEVICES += tty-spike
else
  PLO_ALLDEVICES += uart-16550
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, hal.o)
