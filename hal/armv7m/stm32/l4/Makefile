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

PLO_COMMANDS := alias app bankswitch call console copy dump echo go help kernelimg map mem mpu \
  phfs reboot script wait

#include devices/pipe-rtt/Makefile
include devices/uart-stm32l4x6/Makefile
include devices/flash-stm32/Makefile
include devices/ram-storage/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/stm32/l4/, _init.o stm32l4.o timer.o console.o)
