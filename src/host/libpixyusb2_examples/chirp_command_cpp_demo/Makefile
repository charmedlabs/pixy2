CXX=g++
CPPFLAGS=-g -fpermissive -I/usr/include/libusb-1.0 -I../../libpixyusb2/include -I../../arduino/libraries/Pixy2
LDLIBS=../../../../build/libpixyusb2/libpixy2.a -lusb-1.0

SRCS=chirp_command_cpp_demo.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: chirp_command_demo

clean:
	rm -f *.o chirp_command_cpp_demo

chirp_command_demo: $(OBJS)
	$(CXX) $(LDFLAGS) -o chirp_command_cpp_demo $(OBJS) $(LDLIBS)
