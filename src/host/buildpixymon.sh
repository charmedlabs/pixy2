#!/bin/bash

set -x

unamestr=`uname`
if [[ "$unamestr" == 'Linux' ]]; then
   platform='linux'     
else
   platform='mac'
fi

echo Building for $platform

cd src/host/pixymon

if [[ "$platform" == 'linux' ]]; then
   qmake pixymon.pro
   make -w
   cd ../../..
   cp src/host/pixymon/PixyMon .
   strip PixyMon
   cp src/host/pixymon/pixyflash.bin.hdr .

   # CLEAN UP
   rm -rdf src
   rm buildpixymon.sh
fi

if [[ "$platform" == 'mac' ]]; then
   qmake pixymon.pro
   make -w
   cd ../../..
   cp -rf src/host/pixymon/PixyMon.app .
   strip PixyMon.app/Contents/MacOS/PixyMon
   cp src/host/pixymon/pixyflash.bin.hdr PixyMon.app/Contents/MacOS

   # CLEAN UP
   rm -rdf src
   rm buildpixymon.sh
fi
