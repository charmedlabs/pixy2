from __future__ import print_function
import pixy
from ctypes import *
from pixy import *

# Pixy2 Python SWIG Set Lamp Example #

print("Pixy2 Python SWIG Example -- Set Lamp")

pixy.init ()
pixy.change_prog ("video");

pixy.set_lamp (1, 0);
