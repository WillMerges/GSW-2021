# builds Data Logging System library

TARGET = libdls.so

CXX = g++
CC = g++

OPTIONS += 

CFLAGS = -I../../common -I./include -Wall -Wextra -Wpedantic -fpic -lrt
CPPFLAGS = -I../../common -I./include -Wall -Wextra -Wpedantic -fpic -lrt
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
	rm $TARGET
