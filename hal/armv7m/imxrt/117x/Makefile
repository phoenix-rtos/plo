#
# Makefile for Phoenix-RTOS loader (ARMv7M HAL imxrt117x)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

LDFLAGS := $(filter-out -Tbss% , $(LDFLAGS))
LDFLAGS := $(filter-out -Tdata% , $(LDFLAGS))

CFLAGS := $(filter-out -mfloat-abi% , $(CFLAGS))
CFLAGS += -mfloat-abi=soft

PLO_COMMANDS ?= alias app blob bootcm4 bootrom call console copy dump echo erase go help kernel kernelimg \
  map mem mpu otp phfs reboot script stop wait

PLO_ALLDEVICES := pipe-rtt usbc-cdc uart-imxrt117x flash-imxrt

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/imxrt/117x/, _init.o imxrt.o timer.o console.o bootrom.o otp.o)
