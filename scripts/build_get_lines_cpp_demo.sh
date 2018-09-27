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
echo "# Building Get Lines CPP Demo...                                                       #"
echo "########################################################################################"
NORMAL_TEXT

uname -a

TARGET_BUILD_FOLDER=../build

mkdir $TARGET_BUILD_FOLDER
mkdir $TARGET_BUILD_FOLDER/get_lines_cpp_demo

rm $TARGET_BUILD_FOLDER/get_lines_cpp_demo/get_lines_cpp_demo
cd ../src/host/libpixyusb2_examples/get_lines_cpp_demo
make
mv ./get_lines_cpp_demo ../../../../build/get_lines_cpp_demo

if [ -f ../../../../build/get_lines_cpp_demo/get_lines_cpp_demo ]; then
  GREEN_TEXT
  printf "SUCCESS "
else
  RED_TEXT
  printf "FAILURE "
fi
echo ""
