SOURCES      := $(shell find . -name '*.cpp')
OBJECTS      := $(SOURCES:.cpp=.o)
CXX          ?= g++
PKG_INCLUDES := $(shell pkg-config --cflags libzstd)

%.o: %.cpp
	$(CXX) $(strip -flto -g -Ofast -march=native -fno-exceptions -std=c++20 -Wall -Wextra $(PKG_INCLUDES)) -c $< -o $@

langton: $(OBJECTS)
	$(CXX) -flto $^ -o $@ $(shell pkg-config --libs libzstd)

clean:
	rm -f langton $(OBJECTS)

test: langton
	./$<
