#
# Makefile for usbc-cdc
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

include devices/usbc/Makefile
include devices/usbphy-$(TARGET_SUBFAMILY)/Makefile

CFLAGS += -I../plo/devices/usbc-cdc

OBJS += $(addprefix $(PREFIX_O)devices/usbc-cdc/, cdc.o)
