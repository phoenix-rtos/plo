#
# Makefile for Phoenix-RTOS loader (SPARCV8 LEON3 GR716 HAL)
#
# Copyright 2022 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))
LDFLAGS:=$(filter-out --section-start% , $(LDFLAGS))

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS:=$(filter-out -mfpu% , $(CFLAGS))

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)

PLO_COMMANDS := alias app call console copy dump echo go help kernel map mem phfs reboot script wait

include devices/gpio-gr716/Makefile
include devices/uart-gr716/Makefile
include devices/flash-gr716/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, hal.o gr716.o timer.o console.o)
