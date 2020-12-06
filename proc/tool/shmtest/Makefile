# shared memory test

CXX = g++
CC = g++

OPTIONS +=

CFLAGS = -I$(GSW_HOME)/include -Wall -Wextra -Wpedantic
CPPFLAGS = -I$(GSW_HOME)/include -Wall -Wextra -Wpedantic
LDFLAGS = -L$(GSW_HOME)/lib/bin/ -Wl,-rpath=$(GSW_HOME)/lib/bin/

LIBS = -lshm

CPP_FILES := $(wildcard src/*.cpp)
C_FILES := $(wildcard src/*.c)

OBJS := $(CPP_FILES:.cpp=.o) $(C_FILES:.c=.o)

.PHONY: all clean

all:
	$(CXX) $(CPPFLAGS) src/shmwrite.cpp -o write_test $(LDFLAGS) $(LIBS)
	$(CXX) $(CPPFLAGS) src/shmread.cpp -o read_test $(LDFLAGS) $(LIBS)

clean:
	rm *.o write_test read_test
