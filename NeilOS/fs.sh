#!/bin/sh

#  fs.sh
#  
#
#  Created by Neil Singh on 6/14/17.
#

cd /Users/Neil/Downloads/ece391/ece391_share/work/anonymous/student-distrib
hdiutil attach ./mp3.img
cp -f ./bootimg /Volumes/NeilOS/
rsync -au ./filesystem/. /Volumes/NeilOS/
hdiutil detach /Volumes/NeilOS
