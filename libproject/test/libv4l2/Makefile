include ../../scripts/Makefile

LOCALINC := libv4l2
COMLIB   += -lv4l2wrapper

$(TESV4L2) : $(Obj)
	$(CC) -o $@ $< -I $(COMINC)$(LOCALINC) -L $(COMLIB)
	cp $(TESV4L2) $(COMBIN)
%.o : %.cpp
	#arm-linux-gnueabihf-g++ -o $(TESV4L2) $(Src) -I $(COMINC)$(LOCALINC) -L $(COMLIB)
	$(CC) -c -o $@ $< -Wall -g -O0 -I $(COMINC)$(LOCALINC)

clean :
	$(RM) $(TESV4L2) *.o
