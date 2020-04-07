#!/bin/bash

if [ "$EUID" -ne 0 ]; then
    echo "Please run as root"
    exit 1
fi

if [ ! -f "../src/host/libpixyusb2/include/libpixyusb2.h" ]; then
    echo "Must be within the pixy2 repository to run this script"
    exit 1
fi

pixy2_directory=$(realpath ..)

if [ ! -d ${pixy2_directory}/build ]; then
    echo "Run the build_libpixyusb2.sh script in the scripts directory first."
    exit 1
fi

header_directories=("${pixy2_directory}/src/common/inc/"
    "${pixy2_directory}/src/host/libpixyusb2/include"
    "${pixy2_directory}/src/host/arduino/libraries/Pixy2")

header_files=()
for directory in "${header_directories[@]}"; do
    for header_file in $(ls "$directory"); do
        header_files+=("${directory}/${header_file}")
    done
done

include_directory="/usr/local/include/libpixy2"
if [ ! -d ${include_directory} ]; then
    mkdir ${include_directory}
fi

lib_directory="/usr/local/lib/libpixy2"
if [ ! -d ${lib_directory} ]; then
    mkdir ${lib_directory}
fi

for header_file in "${header_files[@]}"; do
    cp "${header_file}" /usr/local/include/libpixy2
done

sed -i 's/#include \"..\/..\/..\/common\/inc\/chirp.hpp\"/#include \"chirp.hpp\"/g' ${include_directory}/libpixyusb2.h

cp ${pixy2_directory}/build/libpixyusb2/libpixy2.a ${lib_directory}

echo "Installed header files in: ${include_directory}"
echo "Installed library file in: ${lib_directory}"
