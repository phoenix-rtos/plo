#
# Makefile for flash-imx6ull
#
# Copyright 2021-2023 Phoenix Systems
#
# %LICENSE%
#

FLEXSPI_OBJS := qspi/qspi.o

FLEXSPI_OBJS += flashdrv.o nor/nor.o nor/nor_mx.o

OBJS += $(addprefix $(PREFIX_O)devices/flash-imx6ull/, $(FLEXSPI_OBJS))
