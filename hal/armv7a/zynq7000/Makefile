#
# Makefile for Phoenix-RTOS loader (ARMv7A HAL zynq7000)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS := $(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS := $(filter-out -Tdata% , $(LDFLAGS))

CFLAGS := $(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS := $(filter-out -mfpu% , $(CFLAGS))

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

PLO_COMMANDS ?= alias app bitstream blob call console copy devices dump echo erase go help jffs2 kernel \
  map mem part phfs reboot script stop test-ddr test-dev wait

PLO_ALLDEVICES := gpio-zynq usbc-cdc uart-zynq flash-zynq sdcard-zynq7000

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, _init.o hal.o zynq.o timer.o \
  interrupts.o console.o)
