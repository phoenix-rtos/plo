#
# Makefile for plo/armv7m7-imxrt117x
#
# Copyright 2020 Phoenix Systems
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

PLO_COMMANDS = alias app console copy devs dump go help kernel map phfs script syspage timeout

include devices/uart-imxrt117x/Makefile
include devices/flash-imxrt/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, _startup.o hal.o imxrt.o timer.o console.o)
