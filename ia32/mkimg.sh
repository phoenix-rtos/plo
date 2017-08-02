#!/bin/sh

function add2img {
	sector=`expr $offs \* 2`
	printf "Copying %s (%d, 0x%x)\n" $1 $offs $sector

	sz=`du -k $1 | awk '{ print $1 }'`
	dd if=$1 of=plo.bin seek=$offs bs=1024 >/dev/null 2>&1
	offs=`expr $offs + $sz + 1`
}

plo="plo.bin"
sz=`du -k $plo | awk '{ print $1 }'`
echo "Loader size: $sz blocks"

echo "Adding padding to $plo"
padsz=32
dd if=/dev/zero of=$plo seek=$sz bs=1024 count=$padsz >/dev/null 2>&1

offs=32
add2img "../../phoenix-rtos-kernel/phoenix-ia32-qemu.elf"
add2img "../../libphoenix/test/test_mmap"
add2img "../../libphoenix/test/test_malloc"
add2img "../../libphoenix/test/test_threads"
add2img "../../libphoenix/test/test_str2num"

#add2img "../../phoenix-rtos-devices/tty/ttypc/ttypc"
#add2img "../../phoenix-rtos-filesystems/meterfs/meterfs"
