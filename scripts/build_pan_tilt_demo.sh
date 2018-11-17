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
echo "# Building Pan Tilt Demo...                                                            #"
echo "########################################################################################"
NORMAL_TEXT

uname -a

TARGET_BUILD_FOLDER=../build

mkdir $TARGET_BUILD_FOLDER
mkdir $TARGET_BUILD_FOLDER/pan_tilt_demo

rm $TARGET_BUILD_FOLDER/pan_tilt_demo/pan_tilt__demo
cd ../src/host/libpixyusb2_examples/pan_tilt_demo
make
mv ./pan_tilt_demo ../../../../build/pan_tilt_demo

if [ -f ../../../../build/pan_tilt_demo/pan_tilt_demo ]; then
  GREEN_TEXT
  printf "SUCCESS "
else
  RED_TEXT
  printf "FAILURE "
fi
echo ""
