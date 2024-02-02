#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

template <std::integral I>
I parseNumber(std::string_view view, int base = 10) {
	I out{};
	auto result = std::from_chars(view.begin(), view.end(), out, base);
	if (result.ec == std::errc::invalid_argument) {
		std::cerr << std::format("Not an integer: \"{}\"\n", view);
		std::terminate();
	}
	return out;
}

std::pair<int32_t, int32_t> getOffset(uint8_t direction) {
	switch (direction) {
		case 0: return { 0, -1};
		case 1: return { 1,  0};
		case 2: return { 0,  1};
		case 3: return {-1,  0};
		default:
			std::cerr << std::format("Invalid direction: {}\n", int(direction));
			std::terminate();
	}
}

class Grid {
	private:
		std::vector<uint8_t> data;
		size_t length;

	public:
		Grid(size_t length_):
			data(length_ * length_),
			length(length_) {}

		void expand(int32_t &x, int32_t &y) {
			std::vector<uint8_t> new_data(length * length * 4);
			size_t offset = length / 2;

			for (size_t row = 0; row < length; ++row) {
				for (size_t col = 0; col < length; ++col) {
					new_data[(row + offset) * (length * 2) + (col + offset)] = data[row * length + col];
				}
			}

			data = std::move(new_data);

			x += int32_t(offset);
			y += int32_t(offset);

			length *= 2;
		}

		uint8_t & operator()(int32_t &x, int32_t &y) {
			if (x < 0 || y < 0)
				expand(x, y);

			while (length <= size_t(x) || length <= size_t(y))
				expand(x, y);

			return data[size_t(y) * length + x];
		}

		inline uint8_t & operator()(std::pair<int32_t, int32_t> pair) {
			return (*this)(pair.first, pair.second);
		}

		inline auto getLength() const { return length; }
		inline auto getSize() const { return data.size(); }
		inline const auto & getData() const { return data; }
		inline auto & getData() { return data; }
		inline auto begin() { return data.begin(); }
		inline auto end() { return data.end(); }
		inline auto begin() const { return data.begin(); }
		inline auto end() const { return data.end(); }
};

inline size_t getIndex(size_t size, std::pair<int32_t, int32_t> position) {
	return size_t(position.second * size + position.first);
}

uint8_t rotateRight(uint8_t direction) {
	return (direction + 1) % 4;
}

uint8_t rotateLeft(uint8_t direction) {
	if (direction-- == 0)
		return 3;
	return direction;
}

int main(int argc, char **argv) {
	size_t steps = 1'000;
	size_t length = 1;

	if (2 <= argc)
		steps = parseNumber<size_t>(argv[1]);

	if (3 <= argc)
		length = parseNumber<size_t>(argv[2]);

	Grid grid(length);

	std::pair<int32_t, int32_t> position{length / 2, length / 2};
	uint8_t direction = 0;

	for (size_t i = 0; i < steps; ++i) {
		auto &[x, y] = position;
		auto &color = grid(x, y);

		if (color == 0 || color == 3) {
			color = 1;
			direction = rotateRight(direction);
		} else if (color == 1) {
			color = 2;
			direction = rotateLeft(direction);
		} else if (color == 2) {
			color = 3;
			direction = rotateRight(direction);
		} else {
			std::cerr << std::format("Invalid color: {}\n", int(color));
			std::terminate();
		}

		auto [right, down] = getOffset(direction);
		x += right;
		y += down;
	}

	const auto size = grid.getSize();
	length = grid.getLength();

	auto pixels = std::make_unique<uint8_t[]>(size * 4);

	size_t i = 0;

	auto set_pixel = [&i, &pixels](uint32_t color) {
		pixels[i++] = color >> 24;
		pixels[i++] = (color >> 16) & 0xff;
		pixels[i++] = (color >> 8) & 0xff;
		pixels[i++] = color & 0xff;
	};

	for (const auto cell: grid) {
		if (cell == 0) {
			set_pixel(0xffffffff);
		} else if (cell == 1) {
			set_pixel(0xff0000ff);
		} else if (cell == 2) {
			set_pixel(0x00ff00ff);
		} else if (cell == 3) {
			set_pixel(0x0000ffff);
		} else {
			set_pixel(0x000000ff);
		}
	}

	std::filesystem::path path{"langton.png"};

	const bool success = stbi_write_png(path.c_str(), length, length, 4, pixels.get(), length * 4);

	if (success) {
		std::cerr << std::format("Successfully wrote to {}\n", path.string());
	} else {
		std::cerr << std::format("Failed to write to {}\n", path.string());
	}
}
