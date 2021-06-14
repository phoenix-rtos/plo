#
# Makefile for Phoenix-RTOS loader (ARMv7A HAL zynq7000)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

INIT_RAM = 0x0
INIT_FLASH = 0x0

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))
LDFLAGS+=--defsym _plo_bss=__bss_start

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS:=$(filter-out -mfpu% , $(CFLAGS))

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

PLO_COMMANDS = alias app bitstream call console copy devs dump echo go help kernel map phfs script syspage wait

include devices/gpio-zynq7000/Makefile
include devices/usbc-cdc/Makefile
include devices/uart-zynq7000/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, _startup.o hal.o zynq.o timer.o\
		interrupts.o console.o)
