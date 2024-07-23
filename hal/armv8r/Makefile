#
# Makefile for Phoenix-RTOS loader (ARMv8M HAL)
#
# Copyright 2022 Phoenix Systems
#
# %LICENSE%
#

ifneq (, $(findstring mps3an536, $(TARGET_SUBFAMILY)))
  include hal/armv8r/mps3an536/Makefile
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, _cache.o _exceptions.o _interrupts.o cpu.o exceptions.o string.o)
