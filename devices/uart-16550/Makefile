#
# Makefile for uart-16550
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

OBJS += $(addprefix $(PREFIX_O)devices/uart-16550/, uart-16550.o)

ifeq ($(TARGET_FAMILY), ia32)
  OBJS += $(PREFIX_O)devices/uart-16550/uarthw-pc.o
else ifeq ($(TARGET_FAMILY), riscv64)
  OBJS += $(PREFIX_O)devices/uart-16550/uarthw-riscv.o
endif
