CXX = g++
CXXFLAGS = -std=c++20 -pthread -O2

server: main.cpp ThreadPool.h EpollServer.h http_handler.h
	$(CXX) $(CXXFLAGS) -o server main.cpp

clean:
	rm -f server
