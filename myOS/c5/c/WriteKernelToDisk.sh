#!/bin/bash
#Author:gzh

gcc -m32 -c -o ./kernel/main.o ./kernel/main.c && \
ld ./kernel/main.o -N -Ttext=0xc0001500 -e main -m elf_i386 -o ./kernel/kernel.bin && \
strip --remove-section=.note.gnu.property ./kernel/kernel.bin && \
dd if=./kernel/kernel.bin of=/usr/share/bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc
