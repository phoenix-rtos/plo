#
# Makefile for Phoenix-RTOS loader (ARMv8M HAL stm32n6)
#
# Copyright 2021, 2025 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS := $(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS := $(filter-out -Tdata% , $(LDFLAGS))

CFLAGS := $(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS += -mfloat-abi=soft

PLO_COMMANDS ?= alias app app-secure blob blob-secure call call-secure console copy devices dump echo erase go help kernel kernelimg kernel-sec \
  map mem otp phfs reboot script sigverif stop wait test-dev


PLO_ALLDEVICES := pipe-rtt ram-storage uart-stm32 flash-stm32xspi

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/stm32/n6/, _init.o stm32n6.o timer.o console.o otp.o pka.o hash.o)
