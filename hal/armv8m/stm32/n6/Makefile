#
# Makefile for Phoenix-RTOS loader (ARMv8M HAL stm32n6)
#
# Copyright 2021, 2025 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS := $(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS := $(filter-out -Tdata% , $(LDFLAGS))

CFLAGS := $(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS += -mfloat-abi=soft

PLO_COMMANDS ?= alias app blob call console copy devices dump echo go help kernelimg \
  map mem phfs reboot script stop wait

PLO_ALLDEVICES := pipe-rtt ram-storage

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/stm32/n6/, _init.o stm32n6.o timer.o console.o)
