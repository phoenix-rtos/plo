#
# Makefile for Phoenix-RTOS loader (ARMv8M HAL)
#
# Copyright 2022 Phoenix Systems
#
# %LICENSE%
#

ifneq (, $(findstring nrf, $(TARGET_SUBFAMILY)))
  include hal/armv8m/nrf/Makefile
else ifneq (, $(findstring mcx, $(TARGET_SUBFAMILY)))
  include hal/armv8m/mcx/Makefile
else ifneq (, $(findstring stm32, $(TARGET_SUBFAMILY)))
  include hal/armv8m/stm32/Makefile
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, cpu.o interrupts.o mpu.o string.o)
