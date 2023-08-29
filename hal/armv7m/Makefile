#
# Makefile for Phoenix-RTOS loader (ARMv7M HAL)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

ifneq (, $(findstring imxrt, $(TARGET_SUBFAMILY)))
  include hal/armv7m/imxrt/Makefile
endif

ifneq (, $(findstring stm32, $(TARGET_SUBFAMILY)))
  include hal/armv7m/stm32/Makefile
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, cpu.o exceptions.o interrupts.o mpu.o string.o)
