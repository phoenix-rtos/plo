#
# Makefile for Phoenix-RTOS loader (ARMv7M HAL imxrt10xx)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS+= -mfloat-abi=soft

GCCLIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

PLO_COMMANDS = alias app bootrom call console copy dump echo erase go help kernel kernelimg map mpu otp phfs reboot script wait

include devices/usbc-cdc/Makefile
include devices/uart-imxrt106x/Makefile
include devices/flash-flexspi/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/imxrt/10xx/, _init.o imxrt.o timer.o console.o bootrom.o otp.o)
