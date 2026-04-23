#
# Makefile for Phoenix-RTOS loader (ARMv8M HAL stm32)
#
# Copyright 2025, 2026 Phoenix Systems
#
# %LICENSE%
#


ifneq (, $(findstring n6, $(TARGET_SUBFAMILY)))
  include hal/armv8m/stm32/n6/Makefile
  CFLAGS += -I../plo/hal/armv8m/stm32/n6
endif

ifneq (, $(findstring h5, $(TARGET_SUBFAMILY)))
  include hal/armv8m/stm32/h5/Makefile
  CFLAGS += -I../plo/hal/armv8m/stm32/h5
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/stm32/, hal.o)
