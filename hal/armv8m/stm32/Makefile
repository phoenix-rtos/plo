#
# Makefile for Phoenix-RTOS loader (ARMv8M HAL stm32)
#
# Copyright 2025 Phoenix Systems
#
# %LICENSE%
#

ifneq (, $(findstring stm32n6, $(TARGET_SUBFAMILY)))
  include hal/armv8m/stm32/n6/Makefile
  CFLAGS += -I../plo/hal/armv8m/stm32/n6
else ifneq (, $(findstring stm32u3, $(TARGET_SUBFAMILY)))
  include hal/armv8m/stm32/u3/Makefile
  CFLAGS += -I../plo/hal/armv8m/stm32/u3
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/stm32/, hal.o console.o timer.o)
