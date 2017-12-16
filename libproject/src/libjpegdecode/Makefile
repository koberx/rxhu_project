include ../../scripts/Makefile

all : $(Target1)

$(Target1) :
	$(CC) -Wall -g -O0  -fpic -c bmp.cpp -o bmp.o
	$(CC) -Wall -g -O0  -fpic -c jpegdecode.cpp -o jpegdecode.o -std=c++11
	$(AR)  rcs $(Target1) bmp.o jpegdecode.o
	cp $(Target1) $(COMLIB)

clean:
		rm -rf $(Obj1) $(Target1) *.o
