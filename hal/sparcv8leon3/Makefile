#
# Makefile for Phoenix-RTOS loader (SPARCV8 LEON3 HAL)
#
# Copyright 2022 Phoenix Systems
#
# %LICENSE%
#

include hal/sparcv8leon3/$(TARGET_SUBFAMILY)/Makefile

CFLAGS += -Ihal/sparcv8leon3/$(TARGET_SUBFAMILY)

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, cpu.o string.o interrupts.o exceptions.o _init.o _interrupts.o _traps.o)
