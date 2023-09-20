#
# Makefile for Phoenix-RTOS loader (SPARCV8 LEON3 GR716 HAL)
#
# Copyright 2022 Phoenix Systems
#
# %LICENSE%
#

comma := ,

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Wl$(comma)--section-start% , $(LDFLAGS))

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

PLO_COMMANDS := alias app call console copy dump echo go help kernel map mem phfs reboot script wait

include devices/gpio-gr716/Makefile
include devices/uart-grlib/Makefile
ifeq ($(TARGET_PROJECT), mini)
  include devices/flash-gr716/Makefile
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/gaisler/$(TARGET_SUBFAMILY)/, _init.o hal.o gr716.o)
