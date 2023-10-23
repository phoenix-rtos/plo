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

PLO_COMMANDS := alias app bitstream call console copy dump echo erase go help jffs2 kernel map mem \
  phfs reboot script test-ddr test-dev wait

include devices/gpio-zynq7000/Makefile
include devices/usbc-cdc/Makefile
include devices/uart-zynq7000/Makefile
include devices/flash-zynq7000/Makefile
include devices/sdcard-zynq7000/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, _init.o hal.o zynq.o timer.o \
  interrupts.o console.o)
