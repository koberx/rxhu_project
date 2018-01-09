include ../../scripts/Makefile

LOCALINC := libtsprotocol
COMLIB   += -ltsevent

$(TESTS) : $(Obj)
	$(C_COMPLIE_CC) -o $@ $< -I $(COMINC)$(LOCALINC) -L $(COMLIB)
	cp $(TESTS) $(COMBIN)
%.o : %.c
	$(C_COMPLIE_CC) -c -o $@ $< -Wall -g -O0 -I $(COMINC)$(LOCALINC)

clean :
	$(RM) $(TESTS) *.o
