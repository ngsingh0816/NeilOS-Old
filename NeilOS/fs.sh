#!/bin/sh

#  fs.sh
#
#
#  Created by Neil Singh on 6/14/17.
#

cd NeilOS
hdiutil attach ./NeilOS.img
cp -f ./bootimg /Volumes/NeilOS/
rsync -au ../sysroot/. /Volumes/NeilOS/
hdiutil detach /Volumes/NeilOS
./vmware.sh
