#
# Makefile for Phoenix-RTOS loader (ARMv8R HAL mps3-an536)
#
# Copyright 2024 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS := $(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS := $(filter-out -Tdata% , $(LDFLAGS))

CFLAGS += -Ihal/armv8r/mps3an536

# Limited set of commands because of memory constraints on QEMU
PLO_COMMANDS ?= alias app call console dump go help kernel map mem phfs script wait

PLO_ALLDEVICES := flash-mps3an536 uart-cmsdk-apb

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, _init.o console.o hal.o interrupts.o timer.o)
