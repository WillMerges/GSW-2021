# data logging process

TARGET = dlp

CXX = g++
CC = g++

OPTIONS += 

CFLAGS = -I./include -Wall -Wextra -Wpedantic
CPPFLAGS = -I./include -Wall -Wextra -Wpedantic
LDFLAGS =

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
