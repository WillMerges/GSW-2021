# shared memory test

CXX = g++
CC = g++

OPTIONS +=

CFLAGS = -I$(GSW_HOME)/include -Wall -Wextra -Wpedantic
CPPFLAGS = -I$(GSW_HOME)/include -Wall -Wextra -Wpedantic -ggdb
LDFLAGS = -L$(GSW_HOME)/lib/bin/ -Wl,-rpath=$(GSW_HOME)/lib/bin/

LIBS = -lvcm -ltelemetry -lshm -ldls

CPP_FILES := $(wildcard src/*.cpp)
C_FILES := $(wildcard src/*.c)

.PHONY: all clean

all: shmread shmwrite script

shmread: src/shmread.o
	$(CXX) $(LDFLAGS) -o read_test src/shmread.o $(LIBS)

shmwrite: src/shmwrite.o
	$(CXX) $(LDFLAGS) -o write_test src/shmwrite.o $(LIBS)

script:
	chmod +x overload.py

#src/*.o:
#	$(CXX) $(CPPFLAGS) $(CPP_FILES)#
#	$(CC) $(CFLAGS) $(C_FILES)

clean:
	-rm src/*.o write_test read_test
	-chmod -x overload.py
