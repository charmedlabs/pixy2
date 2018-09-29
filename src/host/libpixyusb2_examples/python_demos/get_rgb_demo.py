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

while 1:
  RGB = video_get_RGB (X, Y)
  print 'Frame %d RGB: ' % (Frame)
  print RGB
  Frame = Frame + 1
