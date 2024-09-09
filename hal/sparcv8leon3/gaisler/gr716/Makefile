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

PLO_COMMANDS ?= alias app blob call console copy dump echo go help kernel map mem phfs reboot script stop wait

PLO_ALLDEVICES := gpio-gr716 uart-grlib flash-gr716

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/gaisler/$(TARGET_SUBFAMILY)/, _init.o hal.o gr716.o)
