#
# Makefile for Phoenix-RTOS loader (ARMv7M HAL imxrt117x)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

INIT_RAM = 0x20250000
INIT_FLASH = 0x30000000

BSS=-Tbss=20011000
LDFLAGS+=--defsym _plo_bss=0x20011000

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS+= -mfloat-abi=soft

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

PLO_COMMANDS = alias app call console copy dump echo go help kernel map mpu phfs script wait

include devices/uart-imxrt117x/Makefile
include devices/flash-imxrt/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/imxrt/117x/, _init.o hal.o imxrt.o timer.o console.o)
