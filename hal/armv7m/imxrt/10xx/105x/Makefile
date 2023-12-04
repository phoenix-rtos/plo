#
# Makefile for Phoenix-RTOS loader (ARMv7M HAL imxrt105x)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS+= -mfloat-abi=soft

PLO_COMMANDS := alias app call console copy dump echo erase go help kernel kernelimg map \
  mem mpu otp phfs reboot script wait

#include devices/pipe-rtt/Makefile
include devices/usbc-cdc/Makefile
include devices/uart-imxrt106x/Makefile
include devices/flash-imxrt/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/imxrt/10xx/105x/, _init.o imxrt.o)
