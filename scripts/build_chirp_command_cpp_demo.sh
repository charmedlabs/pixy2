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
echo "# Building Chirp Command CPP Demo...                                                   #"
echo "########################################################################################"
NORMAL_TEXT

uname -a

TARGET_BUILD_FOLDER=../build

mkdir $TARGET_BUILD_FOLDER
mkdir $TARGET_BUILD_FOLDER/chirp_command_cpp_demo

rm $TARGET_BUILD_FOLDER/chirp_command_cpp_demo/chirp_command_cpp_demo
cd ../src/host/libpixyusb2_examples/chirp_command_cpp_demo
make
mv ./chirp_command_cpp_demo ../../../../build/chirp_command_cpp_demo

if [ -f ../../../../build/chirp_command_cpp_demo/chirp_command_cpp_demo ]; then
  GREEN_TEXT
  printf "SUCCESS "
else
  RED_TEXT
  printf "FAILURE "
fi
echo ""
