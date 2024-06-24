#
# Makefile for Phoenix-RTOS loader (ARMv8M HAL mcxn94x)
#
# Copyright 2024 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS := $(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS := $(filter-out -Tdata% , $(LDFLAGS))

CFLAGS := $(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS += -mfloat-abi=soft

PLO_COMMANDS ?= alias app blob call console copy dump echo go help kernel kernelimg map mem phfs \
  reboot script wait

PLO_ALLDEVICES := uart-mcxn94x flash-mcxn94x ram-storage

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/mcx/n94x/, _init.o n94x.o timer.o console.o)
