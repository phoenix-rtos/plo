#
# Makefile for Phoenix SBI
#
# Copyright 2024 Phoenix Systems
#
# %LICENSE%
#

ifneq ($(filter $(TARGET_SUBFAMILY),grfpga gr765),)
  include platform/gaisler/$(TARGET_SUBFAMILY)/Makefile
else
  include platform/$(TARGET_SUBFAMILY)/Makefile
endif

include core/extensions/Makefile
include core/fdt/Makefile

OBJS += $(addprefix $(PREFIX_O)core/, _interrupts.o _start.o _string.o csr.o exceptions.o hart.o interrupts.o \
	list.o sbi.o spinlock.o string.o)
