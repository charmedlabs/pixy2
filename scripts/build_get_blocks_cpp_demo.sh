#!/bin/bash
echo "Starting build..."

set -x

TARGET_BUILD_FOLDER=../build

mkdir $TARGET_BUILD_FOLDER
mkdir $TARGET_BUILD_FOLDER/get_blocks_cpp_demo

echo "Starting build..."
rm $TARGET_BUILD_FOLDER/get_blocks_cpp_demo/get_blocks_cpp_demo
cd ../src/host/get_blocks_cpp_demo
make
mv ./get_blocks_cpp_demo ../../../build/get_blocks_cpp_demo
