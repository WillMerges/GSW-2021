# builds XBee library

TARGET = libxbee.so

CXX = g++
CC = g++

OPTIONS +=

# we don't use the -Wextra flag so we don't get warnings for switch case fall through
CFLAGS = -I$(GSW_HOME)/include -Wall -Wpedantic -fpic
CPPFLAGS = -I$(GSW_HOME)/include -Wall -Wpedantic -fpic -ggdb
LDFLAGS = -shared

LIBS =

CPP_FILES := $(wildcard src/*.cpp)
C_FILES := $(wildcard src/*.c)

OBJS := $(CPP_FILES:.cpp=.o) $(C_FILES:.c=.o)

.PHONY: all clean

all: $(TARGET)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm src/*.o $(TARGET)
