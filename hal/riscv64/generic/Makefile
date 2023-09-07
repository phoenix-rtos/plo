#
# Makefile for Phoenix-RTOS loader (RISC-V 64 Generic HAL)
#
# Copyright 2024 Phoenix Systems
#
# %LICENSE%
#

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

PLO_COMMANDS = alias app call console copy dump echo go help kernel map mem phfs reboot script wait

include devices/ram-storage/Makefile
ifeq ($(TARGET_PROJECT),spike)
  include devices/tty-spike/Makefile
else
  include devices/uart-16550/Makefile
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, hal.o)
