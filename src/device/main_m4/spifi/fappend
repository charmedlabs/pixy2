#/bin/bash
# This script adds a version header to a Pixy .hex firmware file.
# ./fappend 3 0 14 0 2 0 3 0 255
# here the 3 0 14 is the firmware version.  
# The 0 2 means anything less than or equal to 2 major version hardware will work
# the 0 3 means anything less than or equal to 3 minor version hardware will work
# The 0 255 means anhything less than or equal to 255 build version hardware will work
sed -b -i 's/^.*:/:/' pixy_firmware.hex
if [[ $# -eq 9 ]]; then
	printf "general,%d,%d,%d,%d,%d,%d,%d,%d,%d" $1 $2 $3 $4 $5 $6 $7 $8 $9 | cat - pixy_firmware.hex > pixy_firmware2.hex
	mv pixy_firmware2.hex pixy2_firmware-$1.$2.$3-general.hex
elif [[ $# -eq 3 ]]; then
	printf "general,%d,%d,%d,0,255,0,255,0,255" $1 $2 $3 | cat - pixy_firmware.hex > pixy_firmware2.hex
	mv pixy_firmware2.hex pixy_firmware-$1.$2.$3-general.hex
else
	printf "general,x,x,x,0,255,0,255,0,255" | cat - pixy_firmware.hex > pixy_firmware2.hex
	mv pixy_firmware2.hex pixy2_firmware-x.x.x-general.hex
fi
