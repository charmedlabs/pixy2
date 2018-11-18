from ctypes import Structure, c_uint

import pixy
from pixy import *

pixy.init()
pixy.change_prog('color_connected_components')

frame_width = 316
frame_height = 208
raw_frame = Uint32Array(frame_width * frame_height)


class Blocks (Structure):
    _fields_ = [("m_signature", c_uint),
                ("m_x", c_uint),
                ("m_y", c_uint),
                ("m_width", c_uint),
                ("m_height", c_uint),
                ("m_angle", c_uint),
                ("m_index", c_uint),
                ("m_age", c_uint)]


blocks = BlockArray(100)
frame = 0

while 1:
    count = pixy.ccc_get_blocks(100, blocks)

    if count > 0:
        print('frame %3d:' % (frame))
        frame = frame + 1
        for index in range(0, count):
            print('[BLOCK: SIG=%d X=%3d Y=%3d WIDTH=%3d HEIGHT=%3d]' % (blocks[index].m_signature, blocks[index].m_x, blocks[index].m_y, blocks[index].m_width, blocks[index].m_height))

    pixy.video_get_raw_frame(raw_frame)

    for x in range(0, frame_width):
        for y in range(0, frame_width):
            print(raw_frame[x], raw_frame[y], end='')
        print(flush=True)
