#
# Makefile for Phoenix-RTOS loader (ARMv7M HAL)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

ifneq (, $(findstring imxrt, $(TARGET_SUBFAMILY)))
	include hal/armv7m/imxrt/Makefile
	OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, mpu.o string.o)
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, cpu.o interrupts.o)
