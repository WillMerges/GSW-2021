# builds shared memory tools library

TARGET = libldm.so

CXX = g++
CC = g++

OPTIONS +=

CFLAGS = -I../../common -I./include -Wall -Wextra -Wpedantic -fpic
CPPFLAGS = -I../../common -I./include -Wall -Wextra -Wpedantic -fpic
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

#$(OBJS):
#ifneq ($(CPP_FILES), )
#		$(CXX) $(CPPFLAGS) $(OPTIONS) -c $(CPP_FILES)
#endif
#
#ifneq ($(C_FILES), )
#		$(CC) $(CFLAGS) $(OPTIONS) -c $(C_FILES)
#endif

clean:
	rm src/*.o $(TARGET)
	rm $TARGET
