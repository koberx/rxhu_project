include ../../scripts/Makefile

LOCALINC := libv4l2
TRDINC	 := libjpeg
TRDLIB   := libjpeg -ljpeg
COMLIB   += -lv4l2wrapper

$(ANIMATION_JPEG) : $(Obj)
	$(CC) -o $@ $< -I $(COMINC)$(LOCALINC) -I$(COMTRDINC)$(TRDINC) -L $(COMLIB) -L$(COMTRDLIB)$(TRDLIB)
	cp $(ANIMATION_JPEG) $(COMBIN)
%.o : %.cpp
	#arm-linux-gnueabihf-g++ -o $(TESV4L2) $(Src) -I $(COMINC)$(LOCALINC) -L $(COMLIB)
	$(CC) -c -o $@ $< -Wall -g -O0 -I $(COMINC)$(LOCALINC) -I$(COMTRDINC)$(TRDINC)

clean :
	$(RM) $(ANIMATION_JPEG) *.o
