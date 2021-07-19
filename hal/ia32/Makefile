#
# Makefile for Phoenix-RTOS loader (IA-32 HAL)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

CFLAGS += -DVADDR_KERNEL_BASE=$(VADDR_KERNEL_BASE)
CFLAGS += -I../plo/hal/ia32
LDFLAGS += --defsym _plo_bss=__bss_start

INIT_FLASH := 0x7c00
PLO_COMMANDS := alias app call console copy dump echo go help kernel map phfs script syspage wait

include devices/disk-bios/Makefile
include devices/tty-bios/Makefile
include devices/uart-16550/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, _exceptions.o _init.o _interrupts.o cpu.o exceptions.o hal.o interrupts.o memory.o string.o timer.o)

ifeq ($(CONSOLE), serial)
  OBJS += $(PREFIX_O)hal/$(TARGET_SUFF)/console-serial.o
else
  OBJS += $(PREFIX_O)hal/$(TARGET_SUFF)/console-vga.o
endif
