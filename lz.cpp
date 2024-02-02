#include "lz.h"

#include <format>
#include <iostream>
#include <limits>
#include <memory>
#include <string>

#include <lz4.h>

namespace LZ4 {
	namespace {
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
	}

	std::vector<uint8_t> compress(std::span<const uint8_t> input) {
		if (static_cast<size_t>(std::numeric_limits<int>::max()) < input.size_bytes()) {
			std::cerr << std::format("Can't compress: input size {} is too large\n", input.size_bytes());
		}

		std::vector<uint8_t> output;
		output.resize(LZ4_compressBound(static_cast<int>(input.size())));

		const int result = LZ4_compress_default(reinterpret_cast<const char *>(input.data()), reinterpret_cast<char *>(output.data()), safeCast<int>(input.size_bytes()), safeCast<int>(output.size()));
		if (result == 0) {
			std::cerr << "Failed to compress\n";
			std::terminate();
		}

		output.resize(result);
		return output;
	}

	std::vector<uint8_t> decompress(std::span<const uint8_t> input) {
		if (static_cast<size_t>(std::numeric_limits<int>::max()) < input.size()) {
			std::cerr << "Can't decompress: input size is too large\n";
			std::terminate();
		}

		std::vector<uint8_t> output;
		output.resize(input.size() * 4);

		const char *input_data = reinterpret_cast<const char *>(input.data());
		const int input_size = safeCast<int>(input.size_bytes());

		for (;;) {
			const int result = LZ4_decompress_safe(input_data, reinterpret_cast<char *>(output.data()), input_size, safeCast<int>(output.size()));

			if (result == -1) {
				std::cerr << "Can't decompress: input is malformed\n";
				std::terminate();
			}

			if (result < -1) {
				output.resize(output.size() * 4);
				continue;
			}

			output.resize(result);
			return output;
		}
	}
}
