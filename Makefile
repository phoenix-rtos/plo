#
# Makefile for phoenix-rtos-devices
#
# Copyright 2018, 2019 Phoenix Systems
#
# %LICENSE%
#

SIL ?= @
MAKEFLAGS += --no-print-directory

#TARGET ?= ia32-generic
#TARGET ?= armv7m3-stm32l152xd
#TARGET ?= armv7m3-stm32l152xe
#TARGET ?= armv7m4-stm32l4x6
#TARGET ?= armv7m7-imxrt105x
#TARGET ?= armv7m7-imxrt106x
#TARGET ?= armv7m7-imxrt117x
#TARGET ?= armv7a7-imx6ull
TARGET ?= ia32-generic
#TARGET ?= riscv64-spike

include ../phoenix-rtos-build/Makefile.common
include ../phoenix-rtos-build/Makefile.$(TARGET_SUFF)

ARCH = $(TARGET_FAMILY)-$(TARGET_SUBFAMILY)-plo
SRCS += plo.c plostd.c cmd.c

.PHONY: clean
clean:
	rm -f *.elf *.hex *.img *.o $(TARGET_FAMILY)-$(TARGET_SUBFAMILY)/*.o

ifneq ($(filter clean,$(MAKECMDGOALS)),)
	$(shell rm -f *.elf *.hex *.img *.o $(TARGET_FAMILY)-$(TARGET_SUBFAMILY)/*.o)
endif

T1 := $(filter-out clean all,$(MAKECMDGOALS))
ifneq ($(T1),)
	include $(T1)/Makefile
.PHONY: $(T1)
$(T1):
	@echo >/dev/null
else
	include $(TARGET_FAMILY)-$(TARGET_SUBFAMILY)/Makefile
endif
