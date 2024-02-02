#pragma once

#include <cstdint>
#include <vector>

template <typename T>
class Grid {
	private:
		std::vector<T> data;
		size_t length;

	public:
		Grid(size_t length_):
			data(length_ * length_),
			length(length_) {}

		void expand(int32_t &x, int32_t &y) {
			std::vector<T> new_data(length * length * 4);
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

		auto & operator()(int32_t &x, int32_t &y) {
			if (x < 0 || y < 0)
				expand(x, y);

			while (length <= size_t(x) || length <= size_t(y))
				expand(x, y);

			return data[size_t(y) * length + x];
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
