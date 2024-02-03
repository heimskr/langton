SOURCES := $(shell find . -name '*.cpp')
OBJECTS := $(SOURCES:.cpp=.o)
CXX ?= g++

%.o: %.cpp
	$(CXX) -flto -g -Ofast -march=native -fno-exceptions -std=c++20 -Wall -Wextra -c $< -o $@

langton: $(OBJECTS)
	$(CXX) -flto $^ -o $@ -llz4

clean:
	rm -f langton $(OBJECTS)

test: langton
	./$<
