#include "lodepng.h"
#include "lz.h"
#include "Grid.h"
#include "Util.h"

#include <charconv>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <format>
#include <iostream>
#include <memory>
#include <span>
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

inline void applyOffset(uint8_t direction, int32_t &x, int32_t &y) {
	switch (direction) {
		case 0: --y; return;
		case 1: ++x; return;
		case 2: ++y; return;
		case 3: --x; return;
		default:
			std::cerr << std::format("Invalid direction: {}\n", int(direction));
			std::terminate();
	}
}

inline size_t getIndex(size_t size, std::pair<int32_t, int32_t> position) {
	return size_t(position.second * size + position.first);
}

std::vector<uint8_t> save(const Grid<uint8_t> &grid, int32_t x, int32_t y, uint8_t direction, size_t steps) {
	const size_t grid_length = grid.getLength();

	std::vector<uint8_t> raw(grid.getSize() + 2 * sizeof(int32_t) + sizeof(grid_length) + sizeof(direction) + sizeof(steps));

	size_t offset = 0;

	auto move = [&offset, &raw](const auto &item) {
		std::memmove(raw.data() + offset, &item, sizeof(item));
		offset += sizeof(item);
	};

	move(x);
	move(y);
	move(grid_length);
	move(direction);
	move(steps);
	std::memmove(raw.data() + offset, grid.getData().data(), grid.getSize());
	return LZ4::compress(raw);
}

size_t load(std::span<const uint8_t> compressed, Grid<uint8_t> &grid, int32_t &x, int32_t &y, uint8_t &direction) {
	std::vector<uint8_t> raw = LZ4::decompress(compressed);

	size_t grid_length{};
	size_t offset = 0;
	size_t steps;

	auto move = [&offset, &raw](auto &item) {
		std::memmove(&item, raw.data() + offset, sizeof(item));
		offset += sizeof(item);
	};

	move(x);
	move(y);
	move(grid_length);
	move(direction);
	move(steps);

	grid = Grid<uint8_t>(grid_length);
	std::memmove(grid.getData().data(), raw.data() + offset, grid_length * grid_length);

	return steps;
}

int main(int argc, char **argv) {
	size_t steps = 1'000;

	if (2 <= argc)
		steps = parseNumber<size_t>(argv[1]);

	Grid<uint8_t> grid(1);
	int32_t x = 0;
	int32_t y = 0;
	uint8_t direction = 0;
	size_t previous_steps = 0;

	if (3 <= argc) {
		if (std::filesystem::exists(argv[2])) {
			std::string compressed = readFile(argv[2]);
			std::span span(reinterpret_cast<const uint8_t *>(compressed.data()), compressed.size());
			previous_steps = load(span, grid, x, y, direction);
			std::cerr << std::format("Loaded {} step{} from {}\n", previous_steps, previous_steps == 1? "" : "s", argv[2]);
		} else {
			std::cerr << std::format("Couldn't find checkpoint {}\n", argv[2]);
		}
	}

	for (size_t i = 0; i < steps; ++i) {
		auto &color = grid(x, y);
		direction = (direction + 1 + ((color == 1) << 1)) & 3;
		color = color + 1 - (color == 3) * 3;
		applyOffset(direction, x, y);
	}

	const auto size = grid.getSize();
	const auto length = grid.getLength();

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

	if (3 <= argc) {
		std::vector<uint8_t> compressed = save(grid, x, y, direction, steps + previous_steps);
		std::ofstream ofs(argv[2]);
		std::cerr << "Saving checkpoint...\n";
		ofs.write(reinterpret_cast<const char *>(compressed.data()), compressed.size());

		if (ofs) {
			std::cerr << std::format("Saved checkpoint to {}\n", argv[2]);
		} else {
			std::cerr << std::format("Failed to save checkpoint to {}\n", argv[2]);
		}
	}

	std::filesystem::path path{"langton.png"};

	std::cerr << std::format("Writing {}x{} image ({:.2f} MiB) to {}\n", length, length, length * length * 4 / (1024. * 1024.), path.string());

	std::vector<unsigned char> png;
	std::cerr << "Compressing...\n";
	unsigned error = lodepng::encode(png, pixels.get(), length, length);

	if (error) {
		std::cerr << std::format("Failed to compress and write to {}: {}\n", path.string(), error);
		return 1;
	}

	std::cerr << "Writing PNG...\n";
	error = lodepng::save_file(png, path);
	if (error) {
		std::cerr << std::format("Failed to write to {}: {}\n", path.string(), error);
		return 1;
	}

	std::cerr << std::format("Successfully wrote to {}\n", path.string());
}
