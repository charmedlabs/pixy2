import pixy
from pixy import *

pixy.init()

frame_width = 316
frame_height = 208
frame = Uint32Array(frame_width * frame_height)

pixy.video_get_raw_frame(frame)

for x in range(0, frame_width):
    for y in range(0, frame_width):
        print(x, y, end='')
    print(flush=True)
