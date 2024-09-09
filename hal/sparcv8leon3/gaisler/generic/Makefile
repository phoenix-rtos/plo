#
# Makefile for Phoenix-RTOS loader (SPARCV8 LEON3 Generic HAL)
#
# Copyright 2024 Phoenix Systems
#
# %LICENSE%
#

PLO_COMMANDS ?= alias app blob call console copy dump echo go help jffs2 kernel map mem phfs reboot script stop wait test-dev

PLO_ALLDEVICES := uart-grlib ram-storage

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/gaisler/$(TARGET_SUBFAMILY)/, _init.o hal.o generic.o)
