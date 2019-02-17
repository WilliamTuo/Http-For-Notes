bin=HttpServer
cc=g++ -std=c++11
LDFLAGS=-lpthread


$(bin): HttpServer.cpp
	$(cc) -o $@ $^ $(LDFLAGS) #-D_DEBUG_

.PHONY: clean
clean:
	rm -rf $(bin)
