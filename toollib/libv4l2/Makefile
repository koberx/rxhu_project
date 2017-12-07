CFLAGS = -W -Wall -pthread -g -pipe $(CFLAGS_EXTRA)
CFLAGS += -I inc
RM = rm -rf
#CROSS = arm-linux-gnueabihf- 
CC = arm-linux-gnueabihf-gcc
#AR = $(CROSS)ar
AR = arm-linux-gnueabihf-ar
PREFIX?=/usr

ifneq ($(wildcard $(SYSROOT)$(PREFIX)/include/log4cpp/Category.hh),)
CFLAGS += -DHAVE_LOG4CPP -I $(SYSROOT)$(PREFIX)/include
endif
#arm-linux-gnueabihf-g++ -o test_libv4l2 test_libv4l2.cpp -I ./inc/ -L ./ -lv4l2wrapper

V4L2WRAPPER_CPP:=$(wildcard src/*.cpp)
V4L2WRAPPER_OBJ:=$(V4L2WRAPPER_CPP:%.cpp=%.o)

.DEFAULT_GOAL := all

all: libv4l2wrapper.a

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

libv4l2wrapper.a: $(V4L2WRAPPER_OBJ)
	$(AR) rcs $@ $^


clean:
	-@$(RM) *.a $(V4L2WRAPPER_OBJ)
