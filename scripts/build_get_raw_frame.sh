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
echo "# Building Get Raw Frame...                                                            #"
echo "########################################################################################"
NORMAL_TEXT

uname -a

TARGET_BUILD_FOLDER=../build

mkdir $TARGET_BUILD_FOLDER
mkdir $TARGET_BUILD_FOLDER/get_raw_frame

rm $TARGET_BUILD_FOLDER/get_raw_frame/get_raw_frame
cd ../src/host/libpixyusb2_examples/get_raw_frame
make
mv ./get_raw_frame ../../../../build/get_raw_frame

if [ -f ../../../../build/get_raw_frame/get_raw_frame ]; then
  GREEN_TEXT
  printf "SUCCESS "
else
  RED_TEXT
  printf "FAILURE "
fi
echo ""
