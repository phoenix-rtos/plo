#
# Makefile for Phoenix-RTOS loader (RISC-V 64 Generic HAL)
#
# Copyright 2024 Phoenix Systems
#
# %LICENSE%
#

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

PLO_COMMANDS ?= alias app blob call console copy dump echo go help kernel map mem phfs reboot script stop wait

# tty-spike and uart-16550 registers under same major
PLO_ALLDEVICES := ram-storage tty-spike uart-16550

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, hal.o)
