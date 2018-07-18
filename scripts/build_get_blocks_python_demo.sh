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
echo "# Building Get Blocks Python (SWIG) Demo...                                            #"
echo "########################################################################################"
NORMAL_TEXT

uname -a

TARGET_BUILD_FOLDER=../build

mkdir $TARGET_BUILD_FOLDER
mkdir $TARGET_BUILD_FOLDER/get_blocks_python_demo

rm $TARGET_BUILD_FOLDER/get_blocks_python_demo/get_blocks_cpp_demo
cd ../src/host/examples/get_blocks_python_demo

swig -c++ -python pixy.i
python setup.py build_ext --inplace -D__LINUX__

if [ -f ../../../../build/get_blocks_python_demo/_pixy.so ]; then
  rm ../../../../build/get_blocks_python_demo/_pixy.so
fi

cp * ../../../../build/get_blocks_python_demo

if [ -f ../../../../build/get_blocks_python_demo/_pixy.so ]; then
  GREEN_TEXT
  printf "SUCCESS "
else
  RED_TEXT
  printf "FAILURE "
fi
echo ""
