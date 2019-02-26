#INCLUDE=-I./mysql/include/
bin=user_login
LIB=-L./mysql/lib
LIB_NAME=-lmysqlclient
cc=gcc

.PHONY:all
all: user_login

$(bin): user_login.c
	gcc -g -o $@ $^ $(LIB) $(LIB_NAME)


.PHONY: clean
clean:
	rm -rf user_login

