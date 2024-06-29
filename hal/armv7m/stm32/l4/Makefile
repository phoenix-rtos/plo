#
# Makefile for Phoenix-RTOS loader (ARMv7M HAL stm32l4x6)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS := $(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS := $(filter-out -Tdata% , $(LDFLAGS))

CFLAGS := $(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS += -mfloat-abi=soft

PLO_COMMANDS ?= alias app bankswitch blob call console copy devices dump echo go help kernelimg \
  map mem mpu phfs reboot script stop wait

PLO_ALLDEVICES := pipe-rtt uart-stm32l4x6 flash-stm32 ram-storage

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/stm32/l4/, _init.o stm32l4.o timer.o console.o)
