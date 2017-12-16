CC := arm-linux-gnueabihf-g++
AR := arm-linux-gnueabihf-ar
CFLAGS  := -Wall -O3
Libs = 
COMINC := /home/user/rxhu/code/libproject/inc/
COMBIN := /home/user/rxhu/code/libproject/bin
COMLIB := /home/user/rxhu/code/libproject/lib


Src   	 := $(wildcard *.cpp)

Obj     := $(patsubst %.cpp,%.o,$(Src))

Modules  += src/libjpegdecode src/libv4l2 test/libv4l2 
AllObjs  := 

#lib section
Target1  :=	libbmp.a  
Target2  := libv4l2wrapper.a 


# lib test section
TESV4L2 := test_libv4l2 
