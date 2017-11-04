CC = arm-linux-gnueabihf-gcc
CFLAGS = -Wall -g -O0 -I /home/user/rxhu/draw_in_v4l2/install/include -L /home/user/rxhu/draw_in_v4l2/install/lib -lts 

ts_test_tool : test.c v4l2_ops.c show_ops.c 
	    $(CC) $(CFLAGS) -o $@ $^

clean:
	    $(RM) ts_test_tool .*.sw?

