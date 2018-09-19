CXX=g++
CPPFLAGS=-g -fpermissive -I/usr/include/libusb-1.0 -I../../libpixyusb2/include -I../../arduino/libraries/Pixy2
LDLIBS=../../../../build/libpixyusb2/libpixy2.a -lusb-1.0

SRCS=get_lines_cpp_demo.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: get_lines

clean:
	rm -f *.o get_lines_cpp_demo

get_lines: $(OBJS)
	$(CXX) $(LDFLAGS) -o get_lines_cpp_demo $(OBJS) $(LDLIBS)
