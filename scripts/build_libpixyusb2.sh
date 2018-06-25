#!/bin/bash

function WHITE_TEXT {
  printf "\033[1;37m"
}
function NORMAL_TEXT {
  printf "\033[0m"
}
function GREEN_TEXT {
  printf "\033[1;32m"
}
function RED_TEXT {
  printf "\033[1;31m"
}

WHITE_TEXT
echo "########################################################################################"
echo "# Building libpixyusb2...                                                              #"
echo "########################################################################################"
NORMAL_TEXT

uname -a

TARGET_BUILD_FOLDER=../build

mkdir $TARGET_BUILD_FOLDER
mkdir $TARGET_BUILD_FOLDER/libpixyusb2

echo "Starting build..."
rm $TARGET_BUILD_FOLDER/libpixyusb/libpixy2.a
cd ../src/host/libpixyusb2/src
make
mv ./lib/libpixy2.a ../../../../build/libpixyusb2

if [ -f ../../../../build/libpixyusb2/libpixy2.a ]; then
  GREEN_TEXT
  printf "SUCCESS "
else
  RED_TEXT
  printf "FAILURE "
fi
NORMAL_TEXT
echo ""
