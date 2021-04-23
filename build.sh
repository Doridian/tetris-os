#!/bin/sh
set -e

# HOW TO COMPILE libgcc soft-fp:
# Clone gcc, go into soft-fp folder, run this (gcc WILL throw errors, but it compiles enough to make it run):
# gcc -flto -c -O2 -msoft-float -m32 -fno-pie -fno-stack-protector -fcf-protection=none -fno-pic -march=i486 -mtune=i486 -I../config/i386/ -I../../include -I.. *.c
# ar -crv libsoft-fp.a *.o

make clean
make iso
cp boot.iso  ../mister/fat/ao486/tetris.vhd
