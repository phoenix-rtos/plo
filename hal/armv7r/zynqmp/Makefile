#
# Makefile for Phoenix-RTOS loader (ARMv7-R HAL zynqmp)
#
# Copyright 2025 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS := $(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS := $(filter-out -Tdata% , $(LDFLAGS))

CFLAGS := $(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS := $(filter-out -mfpu% , $(CFLAGS))

CFLAGS += -Ihal/armv7r/zynqmp

PLO_COMMANDS ?= alias app bitstream blob call console copy devices dump echo erase go help jffs2 kernel \
  map mem mpu part phfs reboot script stop test-ddr test-dev wait

PLO_ALLDEVICES := ram-storage gpio-zynq uart-zynq flash-zynq

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, _init.o hal.o zynqmp.o timer.o \
  ddr_init.o interrupts.o console.o)
