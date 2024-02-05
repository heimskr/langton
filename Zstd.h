#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace Zstd {
	std::vector<uint8_t> decompress(std::span<const uint8_t>);
	std::vector<uint8_t> compress(std::span<const uint8_t>);
}
