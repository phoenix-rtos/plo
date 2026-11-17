#
# Makefile for Phoenix-RTOS loader (SPARCV8 LEON GR716 HAL)
#
# Copyright 2026 Phoenix Systems
#
# SPDX-License-Identifier: BSD-3-Clause
#

comma := ,

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Wl$(comma)--section-start% , $(LDFLAGS))

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

PLO_COMMANDS ?= alias app blob call console copy devices dump echo go help kernel map mem phfs reboot \
  script stop wait

PLO_ALLDEVICES := gpio-grlib uart-grlib flash-spimctrl

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/gaisler/$(TARGET_SUBFAMILY)/, _init.o hal.o gr716.o)
