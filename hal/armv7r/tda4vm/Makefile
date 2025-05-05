#
# Makefile for Phoenix-RTOS loader (ARMv7-R HAL TDA4VM)
#
# Copyright 2025 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS := $(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS := $(filter-out -Tdata% , $(LDFLAGS))

CFLAGS := $(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS := $(filter-out -mfpu% , $(CFLAGS))

CFLAGS += -Ihal/armv7r/tda4vm

PLO_COMMANDS ?= alias app blob call console copy devices dump echo erase go help jffs2 kernel \
  map mem mpu phfs reboot script stop test-dev wait

PLO_ALLDEVICES := ram-storage uart-16550

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, _init.o hal.o tda4vm.o timer.o \
  interrupts.o console.o)
