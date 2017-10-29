#
# plo - operating system loader, version: 2.0
#
# Copyright 2012,2017 Phoenix Systems
# Copyright 2004 Pawel Pisarczyk
#

BCC ?= /Users/pawel/devel/dev86-0.16.17/bin/ncc
# on mac use: make BCC=/Users/ppisarczyk/devel/dev86-0.16.17/bin/ncc

CFLAGS = -ansi -I.
CFLAGS += -DCONSOLE_SERIAL

AS = as86
LD = ld86

SRCS = plo.c low.c plostd.c cmd.c timer.c serial.c msg.c phfs.c phoenixd.c disk.c
OBJS = _plo.o _lib.o _low.o $(SRCS:.c=.o)

all: plo

.c.o:
	$(BCC) -c $(CFLAGS) $<

.S.o:
	$(AS) -o $*.o $<

plo: $(OBJS)
	$(LD) -T0 -d -o plo.bin $(OBJS)
#	@./mkimg.sh ./plo.bin

clean:
	rm -f *.o *~ core *.s

