#
# Makefile for Phoenix-RTOS loader (ARMv8M HAL stm32u3)
#
# Copyright 2021, 2025 Phoenix Systems
# Copyright 2026 Apator Metrix
#
# %LICENSE%
#

LDFLAGS := $(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS := $(filter-out -Tdata% , $(LDFLAGS))

CFLAGS := $(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS += -mfloat-abi=soft

ifneq (,$(FLASH_SIZE))
  CFLAGS += -DFLASH_PROGRAM_BANK_SIZE=$(FLASH_SIZE)
endif
ifneq (,$(RAM_SIZE))
  CFLAGS += -DRAM_BANK_SIZE=$(RAM_SIZE)
endif

PLO_COMMANDS ?= alias app bankswitch blob call console copy devices dump echo go help kernelimg \
  map mem phfs reboot script stop wait

PLO_ALLDEVICES := flash-stm32 uart-stm32 ram-storage

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/stm32/u3/, _init.o stm32u3.o)
