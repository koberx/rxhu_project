include ../../scripts/Makefile
LOCALINC := libtsprotocol

all : $(Target3)

# 2. 生成.o文件 
%.o : %.c
	$(C_COMPLIE_CC) -Wall -g -O0 -fpic -c $< -o $@ -I $(COMINC)$(LOCALINC)

# 3. 生成动态库文件
$(Target3) : $(Obj)
	$(C_COMPLIE_CC) -shared -o $@ $^
	cp $(Target3) $(COMLIB)

tags :
	ctags -R *

# 4. 删除中间过程生成的文件 
clean:
	rm -f $(Obj) $(Target3)
