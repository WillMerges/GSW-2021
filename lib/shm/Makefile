# builds shared memory tools library

TARGET = libshm.so

CXX = g++
CC = g++

OPTIONS +=

CFLAGS = I$(GSW_HOME)/include -Wall -Wextra -Wpedantic -fpic -ggdb
CPPFLAGS = -I$(GSW_HOME)/include -Wall -Wextra -Wpedantic -fpic -ggdb
LDFLAGS = -shared

LIBS = -lrt -lpthread

CPP_FILES := $(wildcard src/*.cpp)
C_FILES := $(wildcard src/*.c)

OBJS := $(CPP_FILES:.cpp=.o) $(C_FILES:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm src/*.o $(TARGET)
	rm $TARGET
