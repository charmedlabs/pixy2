#!/usr/bin/env python

from distutils.core import setup, Extension

pixy_module = Extension('_pixy',
  include_dirs = ['/usr/include/libusb-1.0',
  '/usr/local/include/libusb-1.0',
  '../../../common/inc',
  '../../../host/libpixyusb2/include/',
  '../../../host/arduino/libraries/Pixy2'],
  libraries = ['pthread',
  'usb-1.0'],
  sources =   ['pixy_wrap.cxx',
  '../../../common/src/chirp.cpp',
  '../../../host/libpixyusb2_examples/python_demos/pixy_python_interface.cpp',
  '../../../host/libpixyusb2/src/usblink.cpp',
  '../../../host/libpixyusb2/src/util.cpp',
  '../../../host/libpixyusb2/src/libpixyusb2.cpp'])

import os
print "dir = "
print os.path.dirname(os.path.realpath(__file__))

setup (name = 'pixy',
  version = '0.1',
  author = 'Charmed Labs, LLC',
  description = """libpixyusb2 module""",
  ext_modules = [pixy_module],
  py_modules = ["pixy"],
  )
