#
# Makefile for plo (structured)
#
# Copyright 2020 Phoenix Systems
#
# %LICENSE%
#

SIL ?= @
MAKEFLAGS += --no-print-directory

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


OBJS = $(PREFIX_O)plo.o $(PREFIX_O)plostd.o $(PREFIX_O)cmd.o


all: $(PREFIX_PROG_STRIPPED)plo-$(TARGET).elf


include $(TARGET)/Makefile


$(PREFIX_PROG)plo-$(TARGET).elf: $(OBJS)
	@(printf "LD  %-24s\n" "$(@F)");
	$(SIL)$(LD) $(LDFLAGS) -e _start --section-start .init=0 -o $(PREFIX_PROG)plo-$(TARGET).elf $(OBJS) $(GCCLIB)

#$(ARCH).hex: $(ARCH).elf
#	$(OBJCOPY) -O ihex $(ARCH).elf $(ARCH).hex
#
#$(ARCH).img: $(ARCH).elf
#	$(OBJCOPY) -O binary $(ARCH).elf $(ARCH).img


.PHONY: clean
clean:
	@echo "rm -rf $(BUILD_DIR)"

ifneq ($(filter clean,$(MAKECMDGOALS)),)
	$(shell rm -rf $(BUILD_DIR))
endif
