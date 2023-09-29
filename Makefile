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

LDGEN ?= $(CC)

CFLAGS += -I.
CPPFLAGS += -DVERSION=\"$(VERSION)\"

OBJS :=
include hal/$(TARGET_SUFF)/Makefile
include lib/Makefile
include devices/Makefile
include phfs/Makefile
include cmds/Makefile

# use explicit plo script dir, legacy value by default
PLO_SCRIPT_DIR ?= $(BUILD_DIR)

OBJS += $(addprefix $(PREFIX_O), _startc.o plo.o syspage.o)

# add optional per-project customizations - all WEAK symbols can be overridden
OBJS += $(addprefix $(PREFIX_O)/custom/, $(patsubst $(PROJECT_PATH)/%.c, %.o, $(wildcard $(PROJECT_PATH)/plo*.c)))

$(PREFIX_O)/custom/%.o: $(PROJECT_PATH)/%.c
	@mkdir -p $(@D)
	@printf "CC  %-24s\n" "$<"
	$(SIL)$(CC) -c $(CFLAGS) "$(abspath $<)" -o "$@"
	$(SIL)$(CC) -M  -MD -MP -MF $(PREFIX_O)/custom/$*.c.d -MT "$@" $(CFLAGS) $<


.PHONY: all base ram clean

.PRECIOUS: $(BUILD_DIR)%/.


all: base ram


base: $(PREFIX_PROG_STRIPPED)plo-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf $(PREFIX_PROG_STRIPPED)plo-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).img


ram: $(PREFIX_PROG_STRIPPED)plo-ram-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf $(PREFIX_PROG_STRIPPED)plo-ram-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).img


$(PLO_SCRIPT_DIR)/script.plo $(PLO_SCRIPT_DIR)/script-ram.plo: | $(PLO_SCRIPT_DIR)/.
	@echo "TOUCH $(@F)"
	$(SIL)touch $@


$(PREFIX_O)/script.o.plo: $(PREFIX_O)cmds/cmd.o $(PLO_SCRIPT_DIR)/script.plo | $(PREFIX_O)/.
	@echo "EMBED script.plo"
	$(SIL)$(OBJCOPY) --update-section .data=$(PLO_SCRIPT_DIR)/script.plo $(PREFIX_O)cmds/cmd.o --add-symbol script=.data:0 $@


$(PREFIX_O)/script-ram.o.plo: $(PREFIX_O)cmds/cmd.o $(PLO_SCRIPT_DIR)/script-ram.plo | $(PREFIX_O)/.
	@echo "EMBED script-ram.plo"
	$(SIL)$(OBJCOPY) --update-section .data=$(PLO_SCRIPT_DIR)/script-ram.plo $(PREFIX_O)cmds/cmd.o --add-symbol script=.data:0 $@


$(PREFIX_PROG)plo-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf: $(PREFIX_O)/$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).ld $(OBJS) $(PREFIX_O)/script.o.plo | $(PREFIX_PROG)/.
	@echo "LD  $(@F)"
	$(SIL)$(LD) $(CFLAGS) $(LDFLAGS) -Wl,-Map=$<.map -o $@ -Wl,-T,$^ -nostdlib -lgcc


$(PREFIX_PROG)plo-ram-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf: $(PREFIX_O)/$(TARGET_FAMILY)-$(TARGET_SUBFAMILY)-ram.ld $(OBJS) $(PREFIX_O)/script-ram.o.plo | $(PREFIX_PROG)/.
	@echo "LD  $(@F)"
	$(SIL)$(LD) $(CFLAGS) $(LDFLAGS) -Wl,-Map=$<.map -o $@ -Wl,-T,$^ -nostdlib -lgcc


$(PREFIX_PROG_STRIPPED)%.hex: $(PREFIX_PROG_STRIPPED)%.elf
	@echo "HEX $(@F)"
	$(SIL)$(OBJCOPY) -O ihex $< $@


$(PREFIX_PROG_STRIPPED)%.img: $(PREFIX_PROG_STRIPPED)%.elf
	@echo "BIN $(@F)"
	$(SIL)$(OBJCOPY) -O binary $< $@


-include $(PREFIX_O)/$(TARGET_FAMILY)-$(TARGET_SUBFAMILY)*ld.d
$(PREFIX_O)/$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).ld: | $(PREFIX_O)/.
	@echo "GEN $(@F)"
	$(SIL)$(LDGEN) $(LDSFLAGS) -MP -MF $@.d -MMD -D__LINKER__ -undef -xc -E -P ld/$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).ldt > $@
	$(SIL)$(SED) -i.tmp -e 's`.*\.o[ \t]*:`$@:`' $@.d && rm $@.d.tmp


$(PREFIX_O)/$(TARGET_FAMILY)-$(TARGET_SUBFAMILY)-%.ld: | $(PREFIX_O)/.
	@echo "GEN $(@F)"
	$(SIL)$(LDGEN) $(LDSFLAGS) -MP -MF $@.d -MMD -D__LINKER__ -D$* -undef -xc -E -P ld/$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).ldt > $@
	$(SIL)$(SED) -i.tmp -e 's`.*\.o[ \t]*:`$@:`' $@.d && rm $@.d.tmp


%/.:
	@echo "MKDIR $(@D)"
	$(SIL)mkdir -p "$(@D)"


clean:
	@echo "rm -rf $(BUILD_DIR)"

ifneq ($(filter clean,$(MAKECMDGOALS)),)
	$(shell rm -rf $(BUILD_DIR))
endif
