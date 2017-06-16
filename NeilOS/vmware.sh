#!/bin/sh

qemu-img convert -o compat6 -O vmdk -f raw mp3.img mp3.vmdk
