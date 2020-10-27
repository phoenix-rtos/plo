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

#TARGET ?= ia32-generic
#TARGET ?= armv7m3-stm32l152xd
#TARGET ?= armv7m3-stm32l152xe
#TARGET ?= armv7m4-stm32l4x6
#TARGET ?= armv7m7-imxrt105x
TARGET ?= armv7m7-imxrt106x
#TARGET ?= armv7m7-imxrt117x
#TARGET ?= armv7a7-imx6ull
#TARGET ?= riscv64-spike


include ../phoenix-rtos-build/Makefile.common
include ../phoenix-rtos-build/Makefile.$(TARGET_SUFF)

ifeq ($(TARGET_SUBFAMILY), imxrt106x)
	INIT=.init=70000000
	BSS=-Tbss=20011000

	LDFLAGS:=$(filter-out -Tbss% , $(LDFLAGS))
       LDFLAGS+=--defsym=plo_bss=0x20011000
	LDFLAGS:=$(filter-out -Tdata% , $(LDFLAGS))
else
	INIT=.init=0
endif

CFLAGS += -I../plo/$(TARGET)


OBJS = $(PREFIX_O)plo.o $(PREFIX_O)plostd.o $(PREFIX_O)phoenixd.o $(PREFIX_O)msg.o $(PREFIX_O)phfs.o $(PREFIX_O)cmd.o


all: $(PREFIX_PROG_STRIPPED)plo-$(TARGET).elf $(PREFIX_PROG_STRIPPED)plo-$(TARGET).hex $(PREFIX_PROG_STRIPPED)plo-$(TARGET).img



include $(TARGET)/Makefile

$(BUILD_DIR)/script.plo:
	@printf "TOUCH script.plo\n"
	$(SIL)touch $(BUILD_DIR)/script.plo


$(PREFIX_O)/script.o.plo: $(PREFIX_O)script.o $(BUILD_DIR)/script.plo
	@mkdir -p $(@D)
	@printf "EMBED script.plo\n"
	$(SIL)$(OBJCOPY) --update-section .data=$(BUILD_DIR)/script.plo $(PREFIX_O)script.o --add-symbol script=.data:0 $(PREFIX_O)script.o.plo


$(PREFIX_PROG)plo-$(TARGET).elf: $(OBJS) $(PREFIX_O)/script.o.plo
	@(printf "LD  %-24s\n" "$(@F)");
	$(SIL)$(LD) $(LDFLAGS) -e _start --section-start $(INIT) $(BSS) -o $(PREFIX_PROG)plo-$(TARGET).elf $(OBJS) $(PREFIX_O)/script.o.plo $(GCCLIB)



$(PREFIX_PROG_STRIPPED)plo-$(TARGET).hex: $(PREFIX_PROG_STRIPPED)plo-$(TARGET).elf
	@(printf "HEX %s\n" "$(@F)");
	$(SIL)$(OBJCOPY) -O ihex $(PREFIX_PROG_STRIPPED)plo-$(TARGET).elf $(PREFIX_PROG_STRIPPED)plo-$(TARGET).hex


$(PREFIX_PROG_STRIPPED)plo-$(TARGET).img: $(PREFIX_PROG_STRIPPED)plo-$(TARGET).elf
	@(printf "BIN %s\n" "$(@F)");
	$(SIL)$(OBJCOPY) -O binary $(PREFIX_PROG_STRIPPED)plo-$(TARGET).elf $(PREFIX_PROG_STRIPPED)plo-$(TARGET).img


.PHONY: clean
clean:
	@echo "rm -rf $(BUILD_DIR)"

ifneq ($(filter clean,$(MAKECMDGOALS)),)
	$(shell rm -rf $(BUILD_DIR))
endif
