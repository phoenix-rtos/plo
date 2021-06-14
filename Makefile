#
# Makefile for plo (structured)
#
# Copyright 2020 Phoenix Systems
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


all: $(PREFIX_PROG_STRIPPED)plo-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf  $(PREFIX_PROG_STRIPPED)plo-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).img\
     $(PREFIX_PROG_STRIPPED)plo-ram-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf $(PREFIX_PROG_STRIPPED)plo-ram-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).img



$(BUILD_DIR)/script.plo:
	@printf "TOUCH script.plo\n"
	$(SIL)touch $(BUILD_DIR)/script.plo

$(PREFIX_O)/script.o.plo: $(PREFIX_O)cmds/cmd.o $(BUILD_DIR)/script.plo
	@mkdir -p $(@D)
	@printf "EMBED script.plo\n"
	$(SIL)$(OBJCOPY) --update-section .data=$(BUILD_DIR)/script.plo $(PREFIX_O)cmds/cmd.o --add-symbol script=.data:0 $(PREFIX_O)script.o.plo


$(PREFIX_PROG)plo-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf: $(OBJS) $(PREFIX_O)/script.o.plo
	@mkdir -p $(@D)
	@(printf "LD  %-24s\n" "$(@F)");
	$(SIL)$(LD) $(LDFLAGS) -e _start --section-start .init=$(INIT_FLASH) $(BSS) -o $(PREFIX_PROG)plo-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf $(OBJS) $(PREFIX_O)/script.o.plo $(GCCLIB)

$(PREFIX_PROG)plo-ram-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf: $(OBJS) $(PREFIX_O)/script.o.plo
	@mkdir -p $(@D)
	@(printf "LD  %-24s\n" "$(@F)");
	$(SIL)$(LD) $(LDFLAGS) -e _start --section-start .init=$(INIT_RAM) $(BSS) -o $(PREFIX_PROG)plo-ram-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf $(OBJS) $(PREFIX_O)/script.o.plo $(GCCLIB)


$(PREFIX_PROG_STRIPPED)%.hex: $(PREFIX_PROG_STRIPPED)%.elf
	@(printf "HEX %s\n" "$(@F)");
	$(SIL)$(OBJCOPY) -O ihex $< $@


$(PREFIX_PROG_STRIPPED)%.img: $(PREFIX_PROG_STRIPPED)%.elf
	@(printf "BIN %s\n" "$(@F)");
	$(SIL)$(OBJCOPY) -O binary $< $@


.PHONY: clean
clean:
	@echo "rm -rf $(BUILD_DIR)"

ifneq ($(filter clean,$(MAKECMDGOALS)),)
	$(shell rm -rf $(BUILD_DIR))
endif
