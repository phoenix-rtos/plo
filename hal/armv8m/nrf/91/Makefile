#
# Makefile for Phoenix-RTOS loader (ARMv8M HAL nrf9160)
#
# Copyright 2022 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS := $(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS := $(filter-out -Tdata% , $(LDFLAGS))

CFLAGS := $(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS += -mfloat-abi=soft

PLO_COMMANDS ?= alias app blob call console copy devices dump echo go help kernel kernelimg map \
  mem phfs reboot script stop wait

PLO_ALLDEVICES := uart-nrf9160 flash-nrf9160

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/nrf/91/, _init.o nrf91.o timer.o console.o)
