#
# Makefile for Phoenix-RTOS loader (ARMv7M HAL stm32l4x6)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

INIT_RAM = 0x08000000
INIT_FLASH = 0x08000000

BSS=-Tbss=0x20010000
LDFLAGS+=--defsym _plo_bss=0x20010000

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS+= -mfloat-abi=soft

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

PLO_COMMANDS = alias app call console copy dump echo go help kernelimg map mpu phfs script wait

include devices/uart-stm32l4x6/Makefile
include devices/flash-stm32/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/stm32/l4/, _init.o stm32l4.o timer.o console.o)
