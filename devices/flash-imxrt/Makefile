#
# Makefile for flash-imxrt
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

CFLAGS += -I../plo/devices/flash-imxrt

#TODO: remove compilation phfs-flash from imxrt-flash
OBJS += $(addprefix $(PREFIX_O)devices/, phfs-flash.o)
OBJS += $(addprefix $(PREFIX_O)devices/flash-imxrt/, flashdrv.o flashcfg.o romapi-$(TARGET_SUBFAMILY).o)