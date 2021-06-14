#
# Makefile for Phoenix-RTOS loader (ARMv7M HAL imxrt)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#


ifneq (, $(findstring imxrt117x, $(TARGET_SUBFAMILY)))
	include hal/armv7m/imxrt/117x/Makefile
	CFLAGS += -I../plo/hal/armv7m/imxrt/117x
else ifneq (, $(findstring imxrt106x, $(TARGET_SUBFAMILY)))
	include hal/armv7m/imxrt/10xx/Makefile
	CFLAGS += -I../plo/hal/armv7m/imxrt/10xx
else ifneq (, $(findstring imxrt105x, $(TARGET_SUBFAMILY)))
	include hal/armv7m/imxrt/10xx/Makefile
	CFLAGS += -I../plo/hal/armv7m/imxrt/10xx
endif
