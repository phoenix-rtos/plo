#
# Makefile for Phoenix-RTOS loader (SPARCV8 LEON3 GR712RC HAL)
#
# Copyright 2023 Phoenix Systems
#
# %LICENSE%
#

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

PLO_COMMANDS := alias app call console copy dump echo go help kernel map mem phfs reboot script wait

include devices/uart-grlib/Makefile
include devices/ram-storage/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/gaisler/$(TARGET_SUBFAMILY)/, _init.o hal.o gr712rc.o)
