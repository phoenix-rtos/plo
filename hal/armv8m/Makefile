#
# Makefile for Phoenix-RTOS loader (ARMv8M HAL)
#
# Copyright 2022 Phoenix Systems
#
# %LICENSE%
#

ifneq (, $(findstring nrf, $(TARGET_SUBFAMILY)))
  include hal/armv8m/nrf/Makefile
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, cpu.o interrupts.o mpu.o string.o)
