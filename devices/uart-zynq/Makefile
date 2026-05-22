#
# Makefile for uart-zynq
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

ifneq (, $(findstring virt, $(TARGET_PROJECT)))
  OBJS += $(addprefix $(PREFIX_O)devices/uart-zynq/, uart-virt.o)
else
  OBJS += $(addprefix $(PREFIX_O)devices/uart-zynq/, uart.o)
endif
