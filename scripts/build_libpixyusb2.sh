#!/bin/bash
echo "Starting build..."

set -x

TARGET_BUILD_FOLDER=../build

mkdir $TARGET_BUILD_FOLDER
mkdir $TARGET_BUILD_FOLDER/libpixyusb2

echo "Starting build..."
rm $TARGET_BUILD_FOLDER/libpixyusb/libpixy2.a
cd ../src/host/libpixyusb2/src
make
mv ./lib/libpixy2.a ../../../../build/libpixyusb2
