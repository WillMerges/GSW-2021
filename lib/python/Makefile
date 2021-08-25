# builds bindings object module

CXX = g++
CC = g++

OPTIONS +=

CFLAGS = -I$(GSW_HOME)/include -Wall -Wextra -Wpedantic -fpic
CPPFLAGS = -I$(GSW_HOME)/include -Wall -Wextra -Wpedantic -fpic -ggdb

.PHONY: all clean

all: $(TARGET)
	$(CXX) $(CPPFLAGS) -c *.cpp
	chmod +x *.py

clean:
	rm *.o
	chmod -x *.py
