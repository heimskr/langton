langton: langton.cpp
	g++ -Ofast -march=native -fno-exceptions -std=c++23 -Wall -Wextra $< -o $@

clean:
	rm -f langton

test: langton
	./$<
