CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = server
OBJS = ./*.cc

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(TARGET)  -pthread

clean:
	rm -rf *.o server

