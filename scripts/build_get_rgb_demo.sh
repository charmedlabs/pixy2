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
echo "# Building Get RGB Demo...                                                             #"
echo "########################################################################################"
NORMAL_TEXT

uname -a

TARGET_BUILD_FOLDER=../build

mkdir $TARGET_BUILD_FOLDER
mkdir $TARGET_BUILD_FOLDER/get_rgb_demo

rm $TARGET_BUILD_FOLDER/get_rgb_demo/get_rgb_demo
cd ../src/host/libpixyusb2_examples/get_rgb_demo
make
mv ./get_rgb_demo ../../../../build/get_rgb_demo

if [ -f ../../../../build/get_rgb_demo/get_rgb_demo ]; then
  GREEN_TEXT
  printf "SUCCESS "
else
  RED_TEXT
  printf "FAILURE "
fi
echo ""
