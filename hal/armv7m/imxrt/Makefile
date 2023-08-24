#
# Makefile for Phoenix-RTOS loader (ARMv7M HAL imxrt)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#


ifneq (, $(findstring imxrt117x, $(TARGET_SUBFAMILY)))
  include hal/armv7m/imxrt/117x/Makefile
  CFLAGS += -Ihal/armv7m/imxrt/117x
else ifneq (, $(findstring imxrt10, $(TARGET_SUBFAMILY)))
  include hal/armv7m/imxrt/10xx/Makefile
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/imxrt/, hal.o)
