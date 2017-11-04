CC = arm-linux-gnueabihf-g++
#CFLAGS = -Wall -g -O0 -I ./usr/include/  -lpthread  -L ./usr/lib/ -lasound -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale -lm -lz  -lSDL -lpthread
CFLAGS =  -I ./usr/include/  -lpthread  -L ./usr/lib/ -lasound -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale -lm -lz  -lSDL -lpthread

player : main.cpp media.cpp video.cpp audio.cpp
	    $(CC) $(CFLAGS) -o $@ $^

clean:
	    $(RM) player .*.sw?
