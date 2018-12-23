import pixy
from ctypes import *
from pixy import *

# Pixy2 Python SWIG get Red/Green/Blue example #

print ("Pixy2 Python SWIG Example -- Get Red/Green/Blue")

pixy.init ()
pixy.change_prog ("video");

X = 158
Y = 104
Frame = 1

pixy.set_lamp (1, 0); ### doesn't work
# pixy.set_lamp (30, 40); ### doesn't work
