#!/bin/sh

function add2img {
	echo "Copying $1 (offs=$offs)"
	sz=`du -k $1 | awk '{ print $1 }'`
	dd if=$1 of=plo.bin seek=$offs >/dev/null 2>&1
	offs=`expr $offs + $sz`
}

plo="plo.bin"
sz=`du $plo | awk '{ print $1 }'`
echo "Loader size: $sz blocks"

echo "Adding padding to $plo"
padsz=64
dd if=/dev/zero of=$plo seek=$sz bs=1024 count=$padsz >/dev/null 2>&1

offs=64
add2img "../../phoenix-rtos-kernel/phoenix-ia32-qemu.elf"
add2img "../../phoenix-rtos-filesystems/meterfs/meterfs"
