include ../../scripts/Makefile
LOCALINC := libv4l2

all : $(Target2)

%.o : %.cpp
	$(CC) -c -o $@ $< -Wall -g -O0  -fpic  -I $(COMINC)$(LOCALINC)

$(Target2) : $(Obj)
	$(AR) rcs $@ $^
	cp $(Target2) $(COMLIB) 

clean:
	-$(RM) *.a $(Obj)
