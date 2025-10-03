#include <array>
#include <cstddef>
#include <vector>

/* template <size_t N> consteval auto operator""_m1(const char (&str)[N]) {
    return std::array<char, N>{};
}

constexpr auto arr1 = "hello"_m1; */

// User-defined literal taking a string literal as NTTP
template <typename T, T... Cs> consteval auto operator""_m() {
	return std::array{ Cs... }; // Deduces size and content
}

constexpr auto arr = "hello"_m;

consteval auto operator""_m2(const char* input, std::size_t size) {
	auto res = std::vector<char>{};

	res.push_back('a');
	return res;
}

constexpr auto arr2 = "hello"_m2;

int main() {
	return 1;
}
