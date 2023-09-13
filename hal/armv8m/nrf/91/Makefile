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

PLO_COMMANDS := alias app call console copy dump echo go help kernel kernelimg map mem phfs \
  reboot script wait

include devices/uart-nrf9160/Makefile
include devices/flash-nrf9160/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/nrf/91/, _init.o nrf91.o timer.o console.o)
