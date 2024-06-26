#
# Makefile for Phoenix-RTOS loader (ARMv7M HAL imxrt106x)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))

CFLAGS:=$(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS+= -mfloat-abi=soft

PLO_COMMANDS ?= alias app blob bootrom bridge call console copy devices dump echo erase go \
  help kernel kernelimg map mem mpu otp phfs reboot script stop wait

PLO_ALLDEVICES := pipe-rtt usbc-cdc uart-imxrt106x flash-imxrt

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/imxrt/10xx/106x/, _init.o imxrt.o bootrom.o)
