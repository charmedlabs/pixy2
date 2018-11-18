import pixy
from pixy import *

pixy.init()

frame = Uint32Array(316*208)

pixy.video_get_rgb_frame(frame)

print(frame)