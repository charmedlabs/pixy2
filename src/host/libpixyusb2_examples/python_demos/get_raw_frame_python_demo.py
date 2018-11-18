from PIL import Image
import numpy as np

import pixy
from pixy import *

pixy.init()
pixy.change_prog('color_connected_components')

frame_width = 316
frame_height = 208

for i in range(5):
    raw_frame = pixy.video_get_raw_frame(frame_width*frame_height)
    img = Image.fromarray(raw_frame.reshape(frame_height, frame_width), 'I')
    img.save('pixy' + str(i) + '.bmp')
