import pixy
from ctypes import *
from pixy import *

# Pixy2 Python SWIG Set Lamp example #

print ("Pixy2 Python SWIG Example -- Set Lamp")

pixy.init ()
pixy.change_prog ("video");

X = 158
Y = 104
Frame = 1

pixy.set_lamp (1, 0);
