#!/bin/sh

qemu-img convert -o compat6 -O vmdk -f raw NeilOS.img NeilOS.vmdk
