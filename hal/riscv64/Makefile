#
# Makefile for Phoenix-RTOS loader (RISC-V 64 HAL)
#
# Copyright 2024 Phoenix Systems
#
# %LICENSE%
#

ifneq ($(filter $(TARGET_SUBFAMILY),grfpga gr765),)
  include hal/$(TARGET_SUFF)/gaisler/Makefile
  CFLAGS += -Ihal/$(TARGET_SUFF)/gaisler/$(TARGET_SUBFAMILY)
else
  include hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/Makefile
  CFLAGS += -Ihal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)
endif


OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, _init.o _interrupts.o _string.o \
  dtb.o exceptions.o interrupts.o plic.o sbi.o string.o timer.o)
