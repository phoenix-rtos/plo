#
# Makefile for Phoenix-RTOS loader (ARMv7A HAL i.MX 6ULL)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

INIT_RAM = 0x907000
INIT_FLASH = 0x0

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))
LDFLAGS+=-Wl,--defsym,_plo_bss=__bss_start

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS:=$(filter-out -mfpu% , $(CFLAGS))

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

PLO_COMMANDS ?= alias app blob call console copy devices dump echo erase go help jffs2 kernel map mem phfs \
  script stop test-dev test-ddr wait

PLO_ALLDEVICES := nand-imx6ull flash-imx6ull uart-imx6ull usbc-cdc

ifdef PLO_NAND
  $(warning Usage of PLO_NAND is deprecated, devices should be passed explicitly)
  PLO_DEVCFG_DEPRECATED = y
endif

ifdef PLO_NOR
  $(warning Usage of PLO_NOR is deprecated, devices should be passed explicitly)
  PLO_DEVCFG_DEPRECATED = y
endif

ifdef PLO_DEVCFG_DEPRECATED
  PLO_ALLDEVICES := $(filter-out nand-imx6ull flash-imx6ull, $(PLO_ALLDEVICES))
  ifeq ($(PLO_NAND), y)
    PLO_ALLDEVICES += nand-imx6ull
  endif

  ifeq ($(PLO_NOR), y)
    PLO_ALLDEVICES += flash-imx6ull
  endif
endif


ifeq ($(PLO_NOR_BOOT), y)
  CFLAGS += -DPLO_NOR_BOOT
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, _init.o imx6ull.o hal.o timer.o \
  interrupts.o console.o)
