CC := arm-linux-gnueabihf-g++
AR := arm-linux-gnueabihf-ar
C_COMPLIE_CC := arm-linux-gnueabihf-gcc
CFLAGS  := -Wall -O3
Libs = 
COMINC := /home/user/rxhu/code/libproject/inc/
COMBIN := /home/user/rxhu/code/libproject/bin
COMLIB := /home/user/rxhu/code/libproject/lib
COMTRDINC := /home/user/rxhu/code/libproject/third_inc/
COMTRDLIB := /home/user/rxhu/code/libproject/third_lib/

#C++/C
Src   	 := $(wildcard *.cpp *.c)
Obj      := $(patsubst %.c,%.o, $(patsubst %.cpp,%.o,$(Src)))

Modules  += src/libjpegdecode src/libv4l2 src/libtsprotocol test/libv4l2 test/libtsprotocol test/animation_jpeg 
AllObjs  := 

#lib section
Target1  :=	libbmp.a  
Target2  := libv4l2wrapper.a 
Target3  := libtsevent.so 


# lib test section
TESV4L2 := test_libv4l2 
TESTS   := tstest 
ANIMATION_JPEG := animation_jpeg
