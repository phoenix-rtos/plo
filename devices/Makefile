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
PLO_UNKNOWN_DEVS := $(filter-out $(PLO_ALLDEVICES), $(PLO_DEVICES))

ifneq ($(PLO_UNKNOWN_DEVS),)
  $(error Unrecognised entries in PLO_DEVICES: $(PLO_UNKNOWN_DEVS))
endif

include $(foreach dev, $(PLO_USED_DEVS), devices/$(dev)/Makefile)
