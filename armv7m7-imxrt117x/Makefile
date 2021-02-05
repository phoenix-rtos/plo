#
# Makefile for plo/armv7m7-imxrt117x
#
# Copyright 2020 Phoenix Systems
#
# %LICENSE%
#

INIT_RAM = 0x20250000
INIT_FLASH = 0x70000000

BSS=-Tbss=20011000
LDFLAGS+=--defsym=plo_bss=0x20011000

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS+= -mfloat-abi=soft

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

OBJS += $(addprefix $(PREFIX_O)$(TARGET)/, low.o imxrt.o serial.o cmd.o timer.o flashdrv.o rom_api.o flashcfg.o phfs-flash.o syspage.o)
STARTUP = $(addprefix $(PREFIX_O)$(TARGET)/, _startup_flash.o)
STARTUP_RAM = $(addprefix $(PREFIX_O)$(TARGET)/, _startup_ram.o)
