#
# Makefile for plo cmds
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

PLO_ALLCOMMANDS = alias app bankswitch bitstream bootcm4 bootrom call console copy dump echo \
	erase go help kernel kernelimg map mpu otp phfs reboot script test-ddr test-dev wait
PLO_COMMANDS ?= $(PLO_ALLCOMMANDS)
PLO_APPLETS = $(filter $(PLO_ALLCOMMANDS), $(PLO_COMMANDS))

PLO_OBJS += $(foreach cmd, $(PLO_APPLETS), $(patsubst %.c, %.o, $(wildcard cmds/$(cmd).c)))

OBJS += $(addprefix $(PREFIX_O), $(PLO_OBJS))
