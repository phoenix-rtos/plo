CROSS ?= arm-phoenix-

CC = $(CROSS)gcc
AR = $(CROSS)ar
LD = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy

LFLAGS=-g -nostartfiles -nostdlib -T./vybrid6.ld
CFLAGS=-g -O1 -nostartfiles -nostdlib -mcpu=cortex-a5 -mtune=cortex-a5 -fomit-frame-pointer -ffreestanding -c

SRCS = plo.c low.c plostd.c cmd.c timer.c serial.c iap.c msg.c phfs.c phoenixd.c exceptions.c flash.c bootrom_data.c
SRCS += mmcblk/io/mmcblk_sdhc.c mmcblk/mmcblk_sd.c mmcblk/mmcblk.c
OBJS =  _startup.o  $(SRCS:.c=.o)


all: arm-plo.hex arm-plo.img
	

.S.o:
	$(CC) $(CFLAGS) $<

exceptions.o: exceptions.c
	$(CC) $(CFLAGS) exceptions.c

arm-plo.hex: arm-plo.elf
	$(OBJCOPY) -O ihex arm-plo.elf arm-plo.hex

arm-plo.img: arm-plo.elf
	$(OBJCOPY) -O binary arm-plo.elf arm-plo.img

arm-plo.elf: $(OBJS)
	$(LD) $(LFLAGS) -o arm-plo.elf $(OBJS)

clean:
	rm *.o *.elf *.hex
