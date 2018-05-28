CXX=g++
CPPFLAGS=-g -fpermissive -Iinclude -I/usr/include/libusb-1.0
LDLIBS=src/lib/libpixy2.a -lusb-1.0

SRCS=test.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: test

test: $(OBJS)
	$(CXX) $(LDFLAGS) -o test $(OBJS) $(LDLIBS) 

