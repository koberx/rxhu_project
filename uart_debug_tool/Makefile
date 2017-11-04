CC = arm-linux-gnueabihf-gcc
#CFLAGS = -Wall -g -O0 -I /home/user/rxhu/draw_in_v4l2/install/include -L /home/user/rxhu/draw_in_v4l2/install/lib -lts 
CFLAGS = -Wall -g -O0  

uart_test_tool : uart_init.c uart_test.c 
	    $(CC) $(CFLAGS) -o $@ $^

clean:
	    $(RM) uart_test_tool .*.sw?

