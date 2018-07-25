CXX=g++
CPPFLAGS=-g -fpermissive -I/usr/include/libusb-1.0 -I../../libpixyusb2/include -I../../arduino/libraries/Pixy2
LDLIBS=../../../../build/libpixyusb2/libpixy2.a -lusb-1.0

SRCS=pan_tilt_demo.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: pan_tilt

clean:
	rm -f *.o pan_tilt_demo

pan_tilt: $(OBJS)
	$(CXX) $(LDFLAGS) -o pan_tilt_demo $(OBJS) $(LDLIBS)
