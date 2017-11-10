CC = arm-linux-gnueabihf-gcc
CFLAGS  := -Wall -g -O0

OBJS = get_cpu_mem.o #.o文件与.c文件同名
LIB = libresource.so # 目标文件名 

all : $(LIB)

# 2. 生成.o文件 
%.o : %.c
	$(CC) $(CFLAGS) -fpic -c $< -o $@ 

# 3. 生成动态库文件
$(LIB) : $(OBJS)
	rm -f $@
	arm-linux-gnueabihf-gcc -shared -o $@ $(OBJS)
	rm -f $(OBJS)

tags :
	ctags -R *

# 4. 删除中间过程生成的文件 
clean:
	rm -f $(OBJS) $(LIB)
