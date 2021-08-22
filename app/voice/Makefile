# telemtry memory view

TARGET = voice_report

CXX = g++
CC = g++

OPTIONS +=

CFLAGS = -I$(GSW_HOME)/include -Wall -Wextra -Wpedantic
CPPFLAGS = -I$(GSW_HOME)/include -Wall -Wextra -Wpedantic -ggdb
LDFLAGS = -L$(GSW_HOME)/lib/bin/ -Wl,-rpath=$(GSW_HOME)/lib/bin/

LIBS = -ldls -lvcm -ltelemetry -lshm -lconvert

CPP_FILES := $(wildcard src/*.cpp)
C_FILES := $(wildcard src/*.c)

OBJS := $(CPP_FILES:.cpp=.o) $(C_FILES:.c=.o)

.PHONY: all clean

all: $(TARGET)
	-chmod +x speak.sh

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

clean:
	-chmod -x speak.sh
	-rm src/*.o $(TARGET)
