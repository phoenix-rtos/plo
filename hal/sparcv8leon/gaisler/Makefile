#
# Makefile for Phoenix-RTOS loader (SPARCV8 LEON HAL)
#
# Copyright 2023 Phoenix Systems
#
# %LICENSE%
#

CFLAGS += -Ihal/sparcv8leon/gaisler/ -Ihal/sparcv8leon/gaisler/$(TARGET_SUBFAMILY)

include hal/sparcv8leon/gaisler/$(TARGET_SUBFAMILY)/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/gaisler/, console.o timer.o)

ifeq ($(TARGET_SUBFAMILY), gr712rc)
  OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/gaisler/, irqmp.o)
else ifeq ($(TARGET_SUBFAMILY), gr716)
  OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/gaisler/, irqamp.o)
else ifeq ($(TARGET_SUBFAMILY), gr740)
  OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/gaisler/, irqamp.o l2cache.o)
else ifeq ($(TARGET_SUBFAMILY), generic)
  OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/gaisler/, irqmp.o)
endif
