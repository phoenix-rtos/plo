#
# Makefile for Phoenix-RTOS loader (ARMv7-R HAL)
#
# Copyright 2025 Phoenix Systems
#
# %LICENSE%
#

ifneq (, $(findstring zynqmp, $(TARGET_SUBFAMILY)))
  include hal/armv7r/zynqmp/Makefile
else ifneq (, $(findstring tda4vm, $(TARGET_SUBFAMILY)))
  include hal/armv7r/tda4vm/Makefile
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, _cache.o _exceptions.o _interrupts.o cpu.o exceptions.o mpu.o string.o)
