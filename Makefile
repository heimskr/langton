SOURCES := $(shell find . -name '*.cpp')
OBJECTS := $(SOURCES:.cpp=.o)

%.o: %.cpp
	g++ -flto -Ofast -march=native -fno-exceptions -std=c++23 -Wall -Wextra -c $< -o $@

langton: $(OBJECTS)
	g++ -flto $^ -o $@

clean:
	rm -f langton

test: langton
	./$<
