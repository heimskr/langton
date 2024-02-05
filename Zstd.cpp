#include "Zstd.h"

#include <bit>
#include <cstring>
#include <iostream>
#include <memory>

#include <zstd.h>

namespace Zstd {
	std::vector<uint8_t> decompress(std::span<const uint8_t> span) {
		const size_t out_size = ZSTD_DStreamOutSize();
		std::vector<uint8_t> out_buffer(out_size);
		std::vector<uint8_t> out;

		auto context = std::unique_ptr<ZSTD_DCtx, size_t(*)(ZSTD_DCtx *)>(ZSTD_createDCtx(), ZSTD_freeDCtx);
		auto stream  = std::unique_ptr<ZSTD_DStream, size_t(*)(ZSTD_DStream *)>(ZSTD_createDStream(), ZSTD_freeDStream);

		ZSTD_inBuffer input {span.data(), span.size_bytes(), 0};

		size_t last_result = 0;

		while (input.pos < input.size) {
			ZSTD_outBuffer output {&out_buffer[0], out_size, 0};
			const size_t result = ZSTD_decompressStream(stream.get(), &output, &input);
			if (ZSTD_isError(result)) {
				std::cerr << "Couldn't decompress\n";
				std::terminate();
			}
			last_result = result;

			const uint8_t *raw_bytes = out_buffer.data();
			out.insert(out.end(), raw_bytes, raw_bytes + output.pos / sizeof(uint8_t));
		}

		if (last_result != 0) {
			std::cerr << "Reached end of tile input without finishing decompression\n";
			std::terminate();
		}

		return out;
	}

	std::vector<uint8_t> compress(std::span<const uint8_t> span) {
		const auto buffer_size = ZSTD_compressBound(span.size_bytes());
		auto buffer = std::vector<uint8_t>(buffer_size);
		auto result = ZSTD_compress(&buffer[0], buffer_size, span.data(), span.size_bytes(), ZSTD_defaultCLevel());
		if (ZSTD_isError(result)) {
			std::cerr << "Couldn't compress data\n";
			std::terminate();
		}
		buffer.resize(result);
		return buffer;
	}
}
