include ../../scripts/Makefile
LOCALINC := libjpegdecode

all : $(Target1)

$(Target1) :
	$(CC) -Wall -g -O0  -fpic -c bmp.cpp -o bmp.o -I $(COMINC)$(LOCALINC)
	$(CC) -Wall -g -O0  -fpic -c jpegdecode.cpp -o jpegdecode.o -std=c++11 -I $(COMINC)$(LOCALINC)
	$(AR)  rcs $(Target1) bmp.o jpegdecode.o 
	cp $(Target1) $(COMLIB)

clean:
		rm -rf $(Obj1) $(Target1) *.o
