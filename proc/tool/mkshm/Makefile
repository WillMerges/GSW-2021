# initialize shared memory tool

TARGET = mkshm 

CXX = g++
CC = g++

OPTIONS += 

CFLAGS = -I$(GSW_HOME)/include -Wall -Wextra -Wpedantic
CPPFLAGS = -I$(GSW_HOME)/include -Wall -Wextra -Wpedantic
LDFLAGS =

LIBS = 

CPP_FILES := $(wildcard *.cpp)
C_FILES := $(wildcard *.c)

OBJS := $(CPP_FILES:.cpp=.o) $(C_FILES:.c=.o)

.PHONY: all clean 

all: $(TARGET)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm *.o $(TARGET)
