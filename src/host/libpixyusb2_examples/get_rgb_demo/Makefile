CXX=g++
CPPFLAGS=-g -fpermissive -I/usr/include/libusb-1.0 -I../../libpixyusb2/include -I../../arduino/libraries/Pixy2
LDLIBS=../../../../build/libpixyusb2/libpixy2.a -lusb-1.0

SRCS=get_rgb_demo.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: get_blocks

clean:
	rm -f *.o get_rgb_demo 

get_blocks: $(OBJS)
	$(CXX) $(LDFLAGS) -o get_rgb_demo $(OBJS) $(LDLIBS)
