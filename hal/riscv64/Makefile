#
# Makefile for Phoenix-RTOS loader (RISC-V 64 HAL)
#
# Copyright 2024 Phoenix Systems
#
# %LICENSE%
#

include hal/riscv64/$(TARGET_SUBFAMILY)/Makefile

CFLAGS += -Ihal/riscv64/$(TARGET_SUBFAMILY)

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, _init.o _interrupts.o _string.o \
  console.o dtb.o exceptions.o interrupts.o plic.o sbi.o string.o timer.o)
