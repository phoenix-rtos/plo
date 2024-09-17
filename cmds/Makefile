#
# Makefile for plo cmds
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

PLO_ALLCOMMANDS = alias app bankswitch bitstream blob bootcm4 bootrom bridge call console \
  copy devices dump echo erase go help jffs2 kernel kernelimg lspci map mem mpu otp phfs \
  ptable reboot script stop test-dev test-ddr wait vbe

PLO_COMMANDS ?= $(PLO_ALLCOMMANDS)
PLO_APPLETS = $(filter $(PLO_ALLCOMMANDS), $(PLO_COMMANDS))

PLO_OBJS += $(foreach cmd, $(PLO_APPLETS), $(patsubst %.c, %.o, $(wildcard cmds/$(cmd).c)))

OBJS += $(addprefix $(PREFIX_O), $(PLO_OBJS))
