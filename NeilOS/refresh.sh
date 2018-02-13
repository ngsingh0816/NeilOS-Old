#!/bin/sh

#  refresh.sh
#  NeilOS
#
#  Created by Neil Singh on 2/11/18.
#  Copyright © 2018 Neil Singh. All rights reserved.

cd NeilOS
rm NeilOS.img
cp orig_large.img NeilOS.img
hdiutil attach ./NeilOS.img
cp -f ./bootimg /Volumes/NeilOS/
rsync -au ../sysroot/. /Volumes/NeilOS/
hdiutil detach ./NeilOS.img
./vmware.sh
