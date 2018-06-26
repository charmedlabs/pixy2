CC = g++
OUT_FILE_NAME = libpixy2.a

CFLAGS= -g -D__LINUX__

INC = -I/usr/include/libusb-1.0 -I../include -I../../../common/inc -I../inc -I../../arduino/libraries/Pixy2

OBJ_DIR=./obj

OUT_DIR=./lib

# Enumerating of every *.cpp as *.o and using that as dependency.	
# filter list of .c files in a directory.
# FILES =dump_l.c \
#	kter.c \
#
# $(OUT_FILE_NAME): $(patsubst %.c,$(OBJ_DIR)/%.o,$(wildcard $(FILES))) 


# Enumerating of every *.cpp as *.o and using that as dependency
$(OUT_FILE_NAME): $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(wildcard *.cpp)) $(OBJ_DIR)/chirp.o
	ar -r -o $(OUT_DIR)/$@ $^

$(OBJ_DIR)/chirp.o: ../../../common/src/chirp.cpp
	$(CC) -c $(INC) $(CFLAGS) -o $(OBJ_DIR)/chirp.o ../../../common/src/chirp.cpp  

#Compiling every *.cpp to *.o
$(OBJ_DIR)/%.o: %.cpp dirmake
	$(CC) -c $(INC) $(CFLAGS) -o $@  $<

dirmake:
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OBJ_DIR)

clean:
	rm -f $(OBJ_DIR)/*.o $(OUT_DIR)/$(OUT_FILE_NAME) Makefile.bak

rebuild: clean build
