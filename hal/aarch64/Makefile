#
# Makefile for Phoenix-RTOS loader (AArch64 HAL)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

ifneq (, $(findstring zynqmp, $(TARGET_SUBFAMILY)))
  include hal/aarch64/zynqmp/Makefile
  CFLAGS += -Ihal/aarch64/zynqmp
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, exceptions.o cache.o cpu.o string.o mmu.o)
