#
# Makefile for plo devices
#
# Copyright 2020 Phoenix Systems
#
# %LICENSE%
#


CFLAGS += -I../plo/devices

include devices/usbc-cdc/Makefile

OBJS += $(addprefix $(PREFIX_O)devices/, phfs-usb.o phfs-serial.o)
