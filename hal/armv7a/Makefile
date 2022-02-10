#
# Makefile for Phoenix-RTOS loader (ARMv7A HAL)
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

ifneq (, $(findstring zynq7000, $(TARGET_SUBFAMILY)))
    include hal/armv7a/zynq7000/Makefile
    CFLAGS += -I../plo/hal/armv7a/zynq7000
else ifneq (, $(findstring imx6ull, $(TARGET_SUBFAMILY)))
   include hal/armv7a/imx6ull/Makefile
    CFLAGS += -I../plo/hal/armv7a/imx6ull
endif

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/, exceptions.o cpu.o string.o)
