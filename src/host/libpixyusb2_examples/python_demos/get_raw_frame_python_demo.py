from PIL import Image
import numpy as np

import pixy
from pixy import *

pixy.init()
pixy.change_prog('color_connected_components')

frame_width = 316
frame_height = 208

raw_frame = pixy.video_get_raw_frame(frame_width*frame_height)
array = raw_frame.astype(np.uint8)
img = Image.fromarray(array)
