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

PLO_COMMANDS ?= alias app blob call console copy dump echo erase go help kernel kernelimg map \
  mem mpu otp phfs reboot script stop wait

# pipe-rtt is disabled due to small amount of space in DTCM; RTT_ENABLED is not defined for this architecture
PLO_ALLDEVICES := usbc-cdc uart-imxrt106x flash-imxrt

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/imxrt/10xx/105x/, _init.o imxrt.o)
