#
# Makefile for plo (structured)
#
# Copyright 2020 Phoenix Systems
#
# %LICENSE%
#

SIL ?= @
MAKEFLAGS += --no-print-directory

KERNEL=1

include ../phoenix-rtos-build/Makefile.common
include ../phoenix-rtos-build/Makefile.$(TARGET_SUFF)

include lib/Makefile
include devices/Makefile
include hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/Makefile

CFLAGS += $(BOARD_CONFIG)
CFLAGS += -I../plo/hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)
CFLAGS += -I$(BUILD_DIR)

OBJS += $(addprefix $(PREFIX_O), plo.o plostd.o phoenixd.o msg.o phfs.o cmd.o syspage.o script.o)


all: $(PREFIX_PROG_STRIPPED)plo-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf  $(PREFIX_PROG_STRIPPED)plo-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).img\
     $(PREFIX_PROG_STRIPPED)plo-ram-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf $(PREFIX_PROG_STRIPPED)plo-ram-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).img


$(PREFIX_O)script.o: $(BUILD_DIR)/script.plo.h

$(BUILD_DIR)/script.plo:
	@printf "TOUCH $(@F)\n"
	$(SIL)touch "$@"

$(BUILD_DIR)/script.plo.h: $(BUILD_DIR)/script.plo
	@mkdir -p $(@D)
	@printf "CREATE $(@F)\n"
	@printf "" > "$@";\
	if [ -s "$<" ]; then\
		while read -r line; do\
			if [ $${#line} -gt 0 ]; then\
				printf '"%s",\n' "$$line" >> "$@";\
			fi;\
		done < "$<";\
	fi

$(PREFIX_PROG)plo-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf: $(OBJS) $(STARTUP)
	@mkdir -p $(@D)
	@printf "LD  %-24s\n" "$(@F)"
	$(SIL)$(LD) $(LDFLAGS) -e _start --section-start .init=$(INIT_FLASH) $(BSS) -o $@ $(OBJS) $(GCCLIB)

$(PREFIX_PROG)plo-ram-$(TARGET_FAMILY)-$(TARGET_SUBFAMILY).elf: $(OBJS) $(STARTUP_RAM)
	@mkdir -p $(@D)
	@printf "LD  %-24s\n" "$(@F)"
	$(SIL)$(LD) $(LDFLAGS) -e _start --section-start .init=$(INIT_RAM) $(BSS) -o $@ $(OBJS) $(GCCLIB)


$(PREFIX_PROG_STRIPPED)%.hex: $(PREFIX_PROG_STRIPPED)%.elf
	@printf "HEX %s\n" "$(@F)"
	$(SIL)$(OBJCOPY) -O ihex $< $@


$(PREFIX_PROG_STRIPPED)%.img: $(PREFIX_PROG_STRIPPED)%.elf
	@printf "BIN %s\n" "$(@F)";
	$(SIL)$(OBJCOPY) -O binary $< $@


.PHONY: clean
clean:
	@echo "rm -rf $(BUILD_DIR)"

ifneq ($(filter clean,$(MAKECMDGOALS)),)
	$(shell rm -rf $(BUILD_DIR))
endif
