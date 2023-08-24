#
# Makefile for Phoenix-RTOS loader (ARMv7M HAL imxrt)
#
# Copyright 2021, 2023 Phoenix Systems
#
# %LICENSE%
#


ifneq (, $(findstring imxrt106x, $(TARGET_SUBFAMILY)))
  include hal/armv7m/imxrt/10xx/106x/Makefile
  CFLAGS += -Ihal/armv7m/imxrt/10xx/106x
else ifneq (, $(findstring imxrt105x, $(TARGET_SUBFAMILY)))
  include hal/armv7m/imxrt/10xx/105x/Makefile
  CFLAGS += -Ihal/armv7m/imxrt/10xx/105x
else
  $(error Unsupported target)
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/imxrt/10xx/, timer.o console.o otp.o)
