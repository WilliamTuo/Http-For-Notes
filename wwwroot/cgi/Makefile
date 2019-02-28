#INCLUDE=-I./mysql/include/
bin=user_login
LIB=-L./mysql/lib
LIB_NAME=-lmysqlclient
cc=gcc

.PHONY:all
all: user_register user_login user_index_init password_retrieval

user_register: user_register.c
	gcc -o $@ $^ $(LIB) $(LIB_NAME)

$(bin): user_login.c
	gcc -o $@ $^ $(LIB) $(LIB_NAME)

user_index_init: user_index_init.c
	gcc -o $@ $^ 

password_retrieval: password_retrieval.c
	gcc -o $@ $^ $(LIB) $(LIB_NAME)

.PHONY: clean
clean:
	rm -rf user_register user_login user_index_init password_retrieval

