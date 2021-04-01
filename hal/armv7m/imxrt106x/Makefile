#
# Makefile for plo/armv7m7-imxrt106x
#
# Copyright 2020 Phoenix Systems
#
# %LICENSE%
#

INIT_RAM = 0x0
INIT_FLASH = 0x70000000

BSS=-Tbss=20010000
LDFLAGS+=--defsym=plo_bss=0x20010000

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS+= -mfloat-abi=soft

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, _startup.o hal.o imxrt.o serial.o cmd.o timer.o flashdrv.o rom_api.o flashcfg.o phfs-flash.o syspage.o usbphy.o)
