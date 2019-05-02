from __future__ import print_function
import pixy
from ctypes import *
from pixy import *

# Pixy2 Python SWIG pan/tilt demo #

# Constants #
PID_MAXIMUM_INTEGRAL      =  2000
PID_MINIMUM_INTEGRAL      = -2000
ZUMO_BASE_DEADBAND        =    20
PIXY_RCS_MAXIMUM_POSITION =  1000
PIXY_RCS_MINIMUM_POSITION =     0
PIXY_RCS_CENTER_POSITION  = ((PIXY_RCS_MAXIMUM_POSITION - PIXY_RCS_MINIMUM_POSITION) / 2)
MINIMUM_BLOCK_AGE_TO_LOCK =    30
PAN_GAIN                  =   400
TILT_GAIN                 =   500

def Reset ():
  global Locked_On_Block
  global Locked_Block_Index
  Locked_On_Block    = False
  Locked_Block_Index = 0

def Display_Block (Index, Block):
        print('                   Block[%3d]: I: %3d / S:%2d / X:%3d / Y:%3d / W:%3d / H:%3d / A:%3d' % (Index, Block.m_index, Block.m_signature, Block.m_x, Block.m_y, Block.m_width, Block.m_height, Block.m_age))

class PID_Controller:
  def __init__ (self, Proportion_Gain, Integral_Gain, Derivative_Gain, Servo):
    self.Proportion_Gain = Proportion_Gain
    self.Integral_Gain   = Integral_Gain
    self.Derivative_Gain = Derivative_Gain
    self.Servo           = Servo
    self.Reset ()

  def Reset (self):
    self.Previous_Error  = 0x80000000
    self.Integral_Value  = 0
    if self.Servo:
      self.Command = PIXY_RCS_CENTER_POSITION
    else:
      self.Command = 0

  def Update (self, Error):
    PID = 0

    if self.Previous_Error !=  0x80000000:
      # Update integral component #
      self.Integral_Value = self.Integral_Value + Error

      # Enforce integral boundries #
      if self.Integral_Value > PID_MAXIMUM_INTEGRAL:
        self.Integral_Value = PID_MAXIMUM_INTEGRAL
      if self.Integral_Value < PID_MINIMUM_INTEGRAL:
        self.Integral_Value = PID_MINIMUM_INTEGRAL

      # Calculate Proportion, Integral, Derivative (PID) term #
      PID = int(Error * self.Proportion_Gain + (int(self.Integral_Value * self.Integral_Gain) >> 4) + (Error - self.Previous_Error) * self.Derivative_Gain) >> 10;

      if self.Servo:
        # Integrate the PID term because the servo is a position device #
        self.Command = self.Command + PID

        if self.Command > PIXY_RCS_MAXIMUM_POSITION:
          self.Command = PIXY_RCS_MAXIMUM_POSITION
        if self.Command < PIXY_RCS_MINIMUM_POSITION:
          self.Command = PIXY_RCS_MINIMUM_POSITION

      else:
        # Handle Zumo base deadband #
        if PID > 0:
          PID = PID + ZUMO_BASE_DEADBAND
        if PID < 0:
          PID = PID - ZUMO_BASE_DEADBAND

        # Use the PID term directly because the Zumo base is a velocity device #
        self.Command = PID

    self.Previous_Error = Error

print("Pixy2 Python SWIG Example -- Pan/Tilt Tracking Demo")

# Initialize pan/tilt controllers #
Pan_PID_Controller  = PID_Controller (PAN_GAIN, 0, PAN_GAIN, True)
Tilt_PID_Controller = PID_Controller (TILT_GAIN, 0, TILT_GAIN, True)

pixy.init ()
pixy.change_prog ("color_connected_components");

Reset ()

Blocks = BlockArray(1)
Frame  = 0

while 1:
  Count = pixy.ccc_get_blocks (1, Blocks)

  if Count > 0:
    Frame = Frame + 1

    # Block acquisition logic #
    if Locked_On_Block:
      # Find the block that we are locked to #
      for Index in range (0, Count):
        if Blocks[Index].m_index == Locked_Block_Index:
          print('Frame %3d: Locked' % (Frame))
          Display_Block (Index, Blocks[Index])

          Pan_Offset  = (pixy.get_frame_width () / 2) - Blocks[Index].m_x;
          Tilt_Offset = Blocks[Index].m_y - (pixy.get_frame_height () / 2)

          Pan_PID_Controller.Update (Pan_Offset)
          Tilt_PID_Controller.Update (Tilt_Offset)

          pixy.set_servos (int(Pan_PID_Controller.Command), int(Tilt_PID_Controller.Command))
    else:
      print('Frame %3d:' % (Frame))

      # Display all the blocks in the frame #
      for Index in range (0, Count):
        Display_Block (Index, Blocks[Index])

      # Find an acceptable block to lock on to #
      if Blocks[0].m_age > MINIMUM_BLOCK_AGE_TO_LOCK:
        Locked_Block_Index = Blocks[0].m_index;
        Locked_On_Block    = True
  else:
    Reset ()

