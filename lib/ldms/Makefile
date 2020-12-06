# builds Launch Data Measurement System library

TARGET = libldms.so

CXX = g++
CC = g++

OPTIONS +=

CFLAGS = -I$(GSW_HOME)/include -Wall -Wextra -Wpedantic -fpic
CPPFLAGS = -I$(GSW_HOME)/include -Wall -Wextra -Wpedantic -fpic
LDFLAGS = -shared

LIBS = -lshm -ldls

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
