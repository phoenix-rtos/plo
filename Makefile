#
# Makefile for plo (structured)
#
# Copyright 2020-2022 Phoenix Systems
#
# %LICENSE%
#

SIL ?= @
MAKEFLAGS += --no-print-directory
VERSION="1.21 rev: $(shell git rev-parse --short HEAD)"

KERNEL=1

include ../phoenix-rtos-build/Makefile.common
include ../phoenix-rtos-build/Makefile.$(TARGET_SUFF)

CFLAGS += $(BOARD_CONFIG) -DVERSION=\"$(VERSION)\"
CFLAGS += -I../plo

include hal/$(TARGET_SUFF)/Makefile
include lib/Makefile
include devices/Makefile
include phfs/Makefile
include cmds/Makefile


OBJS += $(addprefix $(PREFIX_O), _startc.o plo.o syspage.o)


.PHONY: all base ram clean


all: base ram


base: $(PREFIX_PROG_STRIPPED)plo-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf  $(PREFIX_PROG_STRIPPED)plo-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).img


ram: $(PREFIX_PROG_STRIPPED)plo-ram-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf $(PREFIX_PROG_STRIPPED)plo-ram-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).img


$(BUILD_DIR)/script.plo $(BUILD_DIR)/ramscript.plo:
	@echo "TOUCH $(@F)"
	$(SIL)touch $@


$(PREFIX_O)/script.o.plo: $(PREFIX_O)cmds/cmd.o $(BUILD_DIR)/script.plo
	@mkdir -p $(@D)
	@echo "EMBED script.plo"
	$(SIL)$(OBJCOPY) --update-section .data=$(BUILD_DIR)/script.plo $(PREFIX_O)cmds/cmd.o --add-symbol script=.data:0 $@


$(PREFIX_O)/ramscript.o.plo: $(PREFIX_O)cmds/cmd.o $(BUILD_DIR)/ramscript.plo
	@mkdir -p $(@D)
	@echo "EMBED ramscript.plo"
	$(SIL)$(OBJCOPY) --update-section .data=$(BUILD_DIR)/ramscript.plo $(PREFIX_O)cmds/cmd.o --add-symbol script=.data:0 $@


$(PREFIX_PROG)plo-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf: $(PREFIX_O)/$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).ld $(OBJS) $(PREFIX_O)/script.o.plo
	@mkdir -p $(@D)
	@echo "LD  $(@F)"
	$(SIL)$(LD) $(LDFLAGS) -Map=$<.map -o $@ -T $^ $(GCCLIB)


$(PREFIX_PROG)plo-ram-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf: $(PREFIX_O)/$(TARGET_FAMILY)-$(TARGET_SUBFAMILY)-ram.ld $(OBJS) $(PREFIX_O)/ramscript.o.plo
	@mkdir -p $(@D)
	@echo "LD  $(@F)"
	$(SIL)$(LD) $(LDFLAGS) -Map=$<.map -o $@ -T $^ $(GCCLIB)


$(PREFIX_PROG_STRIPPED)%.hex: $(PREFIX_PROG_STRIPPED)%.elf
	@echo "HEX $(@F)"
	$(SIL)$(OBJCOPY) -O ihex $< $@


$(PREFIX_PROG_STRIPPED)%.img: $(PREFIX_PROG_STRIPPED)%.elf
	@echo "BIN $(@F)"
	$(SIL)$(OBJCOPY) -O binary $< $@


$(PREFIX_O)/$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).ld:
	@echo "GEN $(@F)"
	$(SIL)$(CC) $(LDSFLAGS) -D__LINKER__ -undef -xc -E -P ld/$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).ldt > $@


$(PREFIX_O)/$(TARGET_FAMILY)-$(TARGET_SUBFAMILY)-%.ld:
	@echo "GEN $(@F)"
	$(SIL)$(CC) $(LDSFLAGS) -D__LINKER__ -D$* -undef -xc -E -P ld/$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).ldt > $@


clean:
	@echo "rm -rf $(BUILD_DIR)"

ifneq ($(filter clean,$(MAKECMDGOALS)),)
	$(shell rm -rf $(BUILD_DIR))
endif
