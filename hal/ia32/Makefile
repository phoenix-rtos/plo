#
# Makefile for Phoenix-RTOS loader (IA-32 HAL)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

CFLAGS += -DVADDR_KERNEL_BASE=$(VADDR_KERNEL_BASE)
CFLAGS += -Ihal/ia32

PLO_COMMANDS ?= alias app blob call console copy dump echo go help kernel lspci map mem phfs script reboot stop syspage wait

PLO_ALLDEVICES := disk-bios tty-bios uart-16550

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, _exceptions.o _init.o _interrupts.o cpu.o exceptions.o hal.o interrupts.o memory.o pci.o string.o timer.o acpi.o)

ifeq ($(CONSOLE), serial)
  OBJS += $(PREFIX_O)hal/$(TARGET_SUFF)/console-serial.o
else
  OBJS += $(PREFIX_O)hal/$(TARGET_SUFF)/console-vga.o
endif
