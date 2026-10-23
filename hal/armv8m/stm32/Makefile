#
# Makefile for Phoenix-RTOS loader (ARMv8M HAL stm32)
#
# Copyright 2025, 2026 Phoenix Systems
#
# %LICENSE%
#

ifneq (, $(findstring stm32n6, $(TARGET_SUBFAMILY)))
  include hal/armv8m/stm32/n6/Makefile
  CFLAGS += -I../plo/hal/armv8m/stm32/n6
else ifneq (, $(findstring stm32u3, $(TARGET_SUBFAMILY)))
  include hal/armv8m/stm32/u3/Makefile
  CFLAGS += -I../plo/hal/armv8m/stm32/u3
else ifneq (, $(findstring stm32h5, $(TARGET_SUBFAMILY)))
  include hal/armv8m/stm32/h5/Makefile
  CFLAGS += -I../plo/hal/armv8m/stm32/h5
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/stm32/, hal.o console.o timer.o)
