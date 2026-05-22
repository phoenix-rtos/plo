#
# Makefile for Phoenix-RTOS loader (AArch64 HAL zynqmp)
#
# Copyright 2024 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS := $(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS := $(filter-out -Tdata% , $(LDFLAGS))

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

PLO_COMMANDS ?= alias app bitstream blob call console copy devices dump echo erase go help jffs2 kernel \
  map mem phfs reboot script stop test-ddr test-dev wait

PLO_ALLDEVICES := ram-storage gpio-zynq uart-zynq flash-zynq

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, _init.o hal.o zynqmp.o  \
        ddr_init.o interrupts.o )

ifneq (, $(findstring virt, $(TARGET_PROJECT)))
  OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, timer-virt.o console-virt.o )
else
  OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, timer.o console.o )
endif
