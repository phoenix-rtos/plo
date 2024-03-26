#
# Makefile for plo devices
#
# Copyright 2020 Phoenix Systems
#
# %LICENSE%
#

OBJS += $(addprefix $(PREFIX_O)devices/, devs.o)

PLO_DEVICES ?= $(PLO_ALLDEVICES)

PLO_USED_DEVS := $(filter $(PLO_ALLDEVICES), $(PLO_DEVICES))
include $(foreach dev, $(PLO_USED_DEVS), devices/$(dev)/Makefile)
