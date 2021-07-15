#
# Makefile for Phoenix-RTOS loader (ARMv7M HAL imxrt10xx)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#


INIT_RAM = 0x0
INIT_FLASH = 0x70000000

BSS=-Tbss=20009000
LDFLAGS+=--defsym _plo_bss=0x20009000

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS+= -mfloat-abi=soft

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

PLO_COMMANDS = alias app call console copy devs dump echo go help kernel map mpu phfs script wait

include devices/usbc-cdc/Makefile
include devices/uart-imxrt106x/Makefile
include devices/flash-imxrt/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/imxrt/10xx/, _init.o hal.o imxrt.o timer.o console.o)
