import pixy
from ctypes import *
from pixy import *

# Pixy2 Python SWIG pan title demo #

# Constants #
PID_MAXIMUM_INTEGRAL      =  2000
PID_MINIMUM_INTEGRAL      = -2000
ZUMO_BASE_DEADBAND        =    20
PIXY_RCS_MAXIMUM_POSITION =  1000
PIXY_RCS_MINIMUM_POSITION =     0
PIXY_RCS_CENTER_POSITION  = ((PIXY_RCS_MAXIMUM_POSITION - PIXY_RCS_MINIMUM_POSITION) / 2)
MINIMUM_BLOCK_AGE_TO_LOCK =    30

def Reset ():
  global Locked_On_Block
  global Locked_Block_Index
  Locked_On_Block    = False
  Locked_Block_Index = 0

def Display_Block (Index, Block):
        print '                   Block[%3d]: I: %3d / S:%2d / X:%3d / Y:%3d / W:%3d / H:%3d / A:%3d' % (Index, Block.m_index, Block.m_signature, Block.m_x, Block.m_y, Block.m_width, Block.m_height, Block.m_age)

class PID_Controller:
  Previous_Error  = 0
  Integral_Value  = 0

  def _init_ (self, Proportion_Gain, Integral_Gain, Derivative_Gain, Servo):
    self.Proportion_Gain = Proportion_Gain
    self.Integral_Gain   = Integral_Gain
    self.Derivative_Gain = Derivative_Gain
    self.Servo           = Servo
    Reset ()

  def Reset ():
    Previous_Error  = 0x80000000L
    Integral_Value  = 0
    if Servo:
      PID_Command = PIXY_RCS_CENTER_POSITION
    else:
      PID_Command = 0

  def Update_PID (Error):
    PID = 0

    if Previous_Error !=  0x80000000L:
      # Update integral component #
      Integral_Value = Integral_Value + Error

      # Enforce integral boundries #
      if Integral_Value > PID_MAXIMUM_INTEGRAL:
        Integral_Value = PID_MAXIMUM_INTEGRAL
      if Integral_Value < PID_MINIMUM_INTEGRAL:
        Integral_Value = PID_MINIMUM_INTEGRAL

      # Calculate Proportion, Integral, Derivative (PID) term #
      PID = (Error * Proportion_Gain + ((Integral_Value * Integral_Gain) >> 4) + (Error - Previous_Error) * Derivative_Gain) >> 10;

      if Servo:
        # Integrate the PID term because the servo is a position device #
        PID_Command = PID_Command + PID

        if PID_Command > PIXY_RCS_MAX_POSITION:
          PID_Command = PIXY_RCS_MAX_POSITION
        if PID_Command < PIXY_RCS_MIN_POS:
          PID_Command = PIXY_RCS_MINIMUM_POSITION

      else:
        # Handle Zumo base deadband #
        if PID > 0:
          PID = PID + ZUMO_BASE_DEADBAND
        if PID < 0:
          PID = PID - ZUMO_BASE_DEADBAND

        # Use the PID term directly because the Zumo base is a velocity device #
        PID_Command = PID

    Previous_Error = Error

print ("Pixy2 Python SWIG Example -- Get Blocks")

# TODO Init pan / tilt controllers

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
          print 'Frame %3d: Locked' % (Frame)
          Display_Block (Index, Blocks[Index])

          Pan_Offset  = (pixy.get_frame_width () / 2) - Blocks[Index].m_x;
          Tilt_Offset = Blocks[Index].m_y - (pixy.get_frame_height () / 2)

          # TODO Update Pan / Tilt controllers

    else:
      print 'Frame %3d:' % (Frame)

      # Display all the blocks in the frame #
      for Index in range (0, Count):
        Display_Block (Index, Blocks[Index])

      # Find an acceptable block to lock on to #
      if Blocks[0].m_age > MINIMUM_BLOCK_AGE_TO_LOCK:
        Locked_Block_Index = Blocks[0].m_index;
        Locked_On_Block    = True
  else:
    Reset ()

