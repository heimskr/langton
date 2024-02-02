SOURCES := $(shell find . -name '*.cpp')
OBJECTS := $(SOURCES:.cpp=.o)

%.o: %.cpp
	g++ -flto -g -Ofast -march=native -fno-exceptions -std=c++23 -Wall -Wextra -c $< -o $@

langton: $(OBJECTS)
	g++ -flto $^ -o $@ -llz4

clean:
	rm -f langton $(OBJECTS)

test: langton
	./$<
