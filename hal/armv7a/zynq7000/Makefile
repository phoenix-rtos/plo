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

PLO_COMMANDS ?= alias app blob bitstream call console copy dump echo erase go help jffs2 kernel map mem \
  phfs reboot script stop test-ddr test-dev wait

PLO_ALLDEVICES := gpio-zynq7000 usbc-cdc uart-zynq7000 flash-zynq7000 sdcard-zynq7000

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, _init.o hal.o zynq.o timer.o \
  interrupts.o console.o)
