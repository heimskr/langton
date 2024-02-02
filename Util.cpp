#include "Util.h"

#include <fstream>
#include <iostream>

std::string readFile(const std::filesystem::path &path) {
	if (std::filesystem::is_directory(path)) {
		std::cerr << "Can't read directory\n";
		std::terminate();
	}
	std::ifstream stream;
	stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	stream.open(path);
	stream.exceptions(std::ifstream::goodbit);
	if (!stream.is_open()) {
		std::cerr << "Couldn't open file for reading\n";
		std::terminate();
	}
	stream.seekg(0, std::ios::end);
	std::string out;
	out.reserve(stream.tellg());
	stream.seekg(0, std::ios::beg);
	out.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
	stream.close();
	return out;
}
