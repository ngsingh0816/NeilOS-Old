#!/bin/sh

rm NeilOS.img
cp orig_large.img NeilOS.img
cd ..
./NeilOS/fs.sh
