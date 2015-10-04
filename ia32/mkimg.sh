#!/bin/sh

kernel="../../phoenix-rtos-kernel/phoenix-ia32-qemu.elf"

bin="plo.bin"
binsz=`ls -al $bin | awk '{ print $5 }'`

nsec=`expr $binsz / 512`
imgsz=`expr $nsec \* 512`

echo "$bin size: $binsz"
echo "boot image size: $imgsz"

# Calculate padding size
if [ $binsz -gt $imgsz ]; then
	padsz=`expr $imgsz + 512 - $binsz`
else
	padsz=0
fi

if [ $padsz -lt 0 ]; then
	padsz=0
fi

echo "Adding $padsz zeros to $bin"
if ! dd if=/dev/zero of=plo.bin seek=$binsz bs=1 count=$padsz >/dev/null 2>&1; then
	echo "Can't correct $bin size!"
	exit 1
fi

echo "Copying kernel"
dd if=$kernel of=plo.bin seek=64
