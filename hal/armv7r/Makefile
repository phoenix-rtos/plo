#
# Makefile for Phoenix-RTOS loader (ARMv7-R HAL)
#
# Copyright 2025 Phoenix Systems
#
# %LICENSE%
#

ifneq (, $(findstring zynqmp, $(TARGET_SUBFAMILY)))
  include hal/armv7r/zynqmp/Makefile
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, _cache.o _exceptions.o _interrupts.o cpu.o exceptions.o mpu.o string.o)
