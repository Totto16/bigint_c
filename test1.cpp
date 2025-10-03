#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

namespace impl {
// Does nothing, but causes an error if called from a `consteval` function.
inline void ExpectedNullTerminatedArray() {}
} // namespace impl

// A string that can be used as a template parameter.
template <std::size_t N> struct ConstString {
	char str[N]{};

	static constexpr std::size_t size = N - 1;

	[[nodiscard]] constexpr std::string_view view() const { return { str, str + size }; }

	consteval ConstString() {}
	consteval ConstString(const char (&new_str)[N]) {
		if(new_str[N - 1] != '\0') impl::ExpectedNullTerminatedArray();
		std::copy_n(new_str, size, str);
	}
};

#include <iostream>

template <size_t N> struct BigIntConstExpr {
  public:
	bool positive;
	// a constexpr constructible type with N uint64_t values, but only number_count are actually
	// valid, as we have to statically allocate more when creating this type, so that it may have
	// less values (e.g. when separators are in the number, as we estimate the value based on the
	// inital string length)
	std::array<uint64_t, N> numbers;
	size_t number_count;

	// TODO: make constexpr friendly

	template <size_t N2>
	[[nodiscard]] constexpr bool operator==(const BigIntConstExpr<N2>& value2) const {

		if(this->number_count != value2.number_count) {
			return false;
		}

		// TODO
		return false;
	}
};

template <ConstString S> consteval auto operator""_print() {

	constexpr auto C = S.size * 3;

	std::array<uint64_t, C> Z{};

	for(size_t i = 0; i < C; ++i) {
		Z[i] = i;
	}

	return BigIntConstExpr<C>{ .positive = true, .numbers = Z, .number_count = C / 2 };
}

int main() {

	constexpr auto a = "foo"_print;

	for(size_t i = 0; i < a.number_count; ++i) {
		std::cout << a.numbers.at(i);
	}

	std::cout << "\n";
}
