CC = $(CROSS)gcc
LD = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy

DIR = $(TARGET_FAMILY)-$(TARGET_SUBFAMILY)

SRCS += $(DIR)/low.c $(DIR)/imxrt.c $(DIR)/serial.c $(DIR)/board_cmd.c $(DIR)/timer.c $(DIR)/phfs.c $(DIR)/flashdrv.c $(DIR)/rom_api.c $(DIR)/flashcfg.c $(DIR)/flash.c 
ASMS = $(DIR)/_startup.S

OBJS = $(SRCS:.c=.o) $(ASMS:.S=.o)

CFLAGS = -Og -DNDEBUG -g -Wall -Wstrict-prototypes -nostartfiles -nostdlib -mthumb -ffixed-r9 -fomit-frame-pointer -ffreestanding -Wno-bool-operation -fdata-sections -ffunction-sections -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16

LDFLAGS = -O1 -g -nostartfiles -nostdlib
LDFLAGS += -nostdlib -e _start --section-start .init=0 -Tbss=20000000 -z max-page-size=0x10 --gc-sections

LIBS = $(GCCLIB)

all: $(ARCH).hex $(ARCH).img

$(ARCH).hex: $(ARCH).elf
	$(OBJCOPY) -O ihex $(ARCH).elf $(ARCH).hex

$(ARCH).img: $(ARCH).elf
	$(OBJCOPY) -O binary $(ARCH).elf $(ARCH).img

$(ARCH).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $(ARCH).elf $(OBJS) $(LIBS)

