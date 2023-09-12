#
# Makefile for flash-imxrt
#
# Copyright 2021-2022 Phoenix Systems
#
# %LICENSE%
#

FLEXSPI_OBJS := fspi/fspi.o

ifneq (, $(findstring imxrt117x, $(TARGET_SUBFAMILY)))
  FLEXSPI_OBJS += flashdrv.o nor/nor.o nor/nor_mx.o
else ifneq (, $(findstring imxrt106x, $(TARGET_SUBFAMILY)))
  FLEXSPI_OBJS += flashdrv.o nor/nor.o nor/nor_mx.o
else ifneq (, $(findstring imxrt105x, $(TARGET_SUBFAMILY)))
  FLEXSPI_OBJS += flashdrv.o nor/nor.o nor/nor_mx.o hyperbus/hyper.o
endif

OBJS += $(addprefix $(PREFIX_O)devices/flash-imxrt/, $(FLEXSPI_OBJS))
