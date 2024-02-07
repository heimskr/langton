#include "lodepng.h"
#include "lz.h"
#include "Grid.h"
#include "Util.h"
#include "Zstd.h"

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

using Coord = int32_t;

template <typename O, typename I>
O safeCast(I input) {
	if (static_cast<I>(std::numeric_limits<O>::max()) < input) {
		std::cerr << std::format("Input number too high: {} > {}\n", input, static_cast<I>(std::numeric_limits<O>::max()));
		std::terminate();
	}

	if constexpr (std::is_signed_v<I> && std::is_signed_v<O>) {
		if (static_cast<I>(std::numeric_limits<O>::min()) > input) {
			std::cerr << "Input number too low\n";
			std::terminate();
		}
	}

	return static_cast<O>(input);
}

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

inline void applyOffset(uint8_t direction, Coord &x, Coord &y) {
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

std::vector<uint8_t> save(const Grid<uint8_t, Coord> &grid, int32_t x, int32_t y, uint8_t direction, size_t steps) {
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
	return Zstd::compress(raw);
}

size_t load(std::span<const uint8_t> compressed, Grid<uint8_t, Coord> &grid, int32_t &x, int32_t &y, uint8_t &direction) {
	std::vector<uint8_t> raw = Zstd::decompress(compressed);

	size_t grid_length{};
	size_t offset = 0;
	size_t steps{};

	auto move = [&offset, &raw](auto &item) {
		std::memmove(&item, raw.data() + offset, sizeof(item));
		offset += sizeof(item);
	};

	move(x);
	move(y);
	move(grid_length);
	move(direction);
	move(steps);

	grid = Grid<uint8_t, Coord>(grid_length);
	std::memmove(grid.getData().data(), raw.data() + offset, grid_length * grid_length);

	return steps;
}

std::unique_ptr<uint8_t[]> makeImage(const Grid<uint8_t, Coord> &grid) {
	auto pixels = std::make_unique<uint8_t[]>(grid.getSize() * 4);

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

	return pixels;
}

int main(int argc, char **argv) {
	size_t steps = 1'000;
	std::filesystem::path checkpoint_path;

	if (2 <= argc)
		steps = parseNumber<size_t>(argv[1]);

	if (3 <= argc)
		checkpoint_path = argv[2];

	Grid<uint8_t, Coord> grid(1);
	Coord x = 0;
	Coord y = 0;
	uint8_t direction = 0;
	size_t previous_steps = 0;

	if (3 <= argc) {
		if (std::filesystem::exists(argv[2])) {
			std::cerr << std::format("Loading steps from {}.\n", argv[2]);
			std::string compressed = readFile(argv[2]);
			std::span span(reinterpret_cast<const uint8_t *>(compressed.data()), compressed.size());
			previous_steps = load(span, grid, x, y, direction);
			std::cerr << std::format("Loaded {} step{}. Grid length is {}.\n", previous_steps, previous_steps == 1? "" : "s", grid.getLength());
		} else {
			std::cerr << std::format("Couldn't find checkpoint {}.\n", argv[2]);
		}
	}

	std::cerr << std::format("Processing {} step{}.\n", steps, steps == 1? "" : "s");

	auto saveAndWrite = [&](const std::string &message = "Compressing checkpoint.") {
		if (argc < 3)
			return;

		std::cerr << message << '\n';
		std::vector<uint8_t> compressed = save(grid, x, y, direction, steps + previous_steps);
		std::ofstream ofs(checkpoint_path);
		std::cerr << "Saving checkpoint.\n";
		ofs.write(reinterpret_cast<const char *>(compressed.data()), compressed.size());

		if (ofs) {
			std::cerr << std::format("Saved checkpoint to {}\n", checkpoint_path.string());
		} else {
			std::cerr << std::format("Failed to save checkpoint to {}\n", checkpoint_path.string());
		}
	};

	if (steps < 1'000'000'000) {
		for (size_t i = 0; i < steps; ++i) {
			auto &color = grid(x, y);
			direction = (direction + 1 + ((color == 1) << 1)) & 3;
			color = color + 1 - (color == 3) * 3;
			applyOffset(direction, x, y);
		}

		saveAndWrite();
	} else {
		constexpr static size_t CHUNKS = 10;
		for (size_t chunk = 0; chunk < CHUNKS; ++chunk) {
			for (size_t i = 0; i < steps / CHUNKS; ++i) {
				auto &color = grid(x, y);
				direction = (direction + 1 + ((color == 1) << 1)) & 3;
				color = color + 1 - (color == 3) * 3;
				applyOffset(direction, x, y);
			}

			saveAndWrite(std::format("Compressing checkpoint at {:.2f}%.", 100.0 * chunk / CHUNKS));
		}
	}

	const auto length = grid.getLength();
	std::cerr << std::format("Producing raw image from {}x{} grid.\n", length, length);
	auto pixels = makeImage(grid);

	std::filesystem::path path{"langton.png"};

	std::cerr << std::format("Writing {}x{} image ({:.2f} MiB) to {}\n", length, length, length * length * 4 / (1024. * 1024.), path.string());

	std::vector<unsigned char> png;
	std::cerr << "Compressing PNG.\n";
	unsigned error = lodepng::encode(png, pixels.get(), length, length);

	if (error) {
		std::cerr << std::format("Failed to compress and write to {}: {}\n", path.string(), error);
		return 1;
	}

	std::cerr << "Writing PNG.\n";
	error = lodepng::save_file(png, path);
	if (error) {
		std::cerr << std::format("Failed to write to {}: {}\n", path.string(), error);
		return 1;
	}

	std::cerr << std::format("Successfully wrote to {}\n", path.string());
}
