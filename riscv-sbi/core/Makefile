#
# Makefile for Phoenix SBI
#
# Copyright 2024 Phoenix Systems
#
# %LICENSE%
#

include platform/$(TARGET_SUBFAMILY)/Makefile
include core/extensions/Makefile
include core/fdt/Makefile

OBJS += $(addprefix $(PREFIX_O)core/, _interrupts.o _start.o _string.o csr.o exceptions.o hart.o interrupts.o \
	list.o sbi.o spinlock.o string.o)
