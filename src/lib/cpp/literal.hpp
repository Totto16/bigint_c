#pragma once

#include "./const_utils.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <string>

template <size_t N> struct BigIntConstExpr {
  public:
	bool positive;
	const_utils::VectorLike<uint64_t, N> numbers;

	template <size_t N2>
	[[nodiscard]] constexpr bool operator==(const BigIntConstExpr<N2>& value2) const {

		if(this->positive != value2.positive) {
			return false;
		}

		if(this->numbers.size() != value2.numbers.size()) {
			return false;
		}

		for(size_t i = 0; i < this->numbers.size(); ++i) {
			if(this->numbers[i] != value2.numbers[i]) {
				return false;
			}
		}

		return true;
	}
};

using BCDDigit = uint8_t;

template <size_t N> using BCDDigitsConstExpr = const_utils::VectorLike<BCDDigit, N>;

namespace { // NOLINT(cert-dcl59-cpp,google-build-namespaces)

namespace const_constants {

// maximal value for uint64_t is 18446744073709551615 so 20 digits, so we say per 19 digits we
// need one number, we need to ceil it
constexpr size_t uint64_t_str_size_max = 20;
// in some cases we need 2 more uint64_t values, that get stripped off by the leadinf zero remover
// at the end, but are needed in the middle of the algorithm, to be safe, we add an additional 1
constexpr size_t padding_for_bcd_algorihtm = 2;

} // namespace const_constants

namespace {
consteval size_t consteval_ceil_div(size_t input, size_t divider) {
	return (input + divider - 1) / divider;
}

consteval size_t get_maximum_uint64_numbers_for_string_length(size_t string_length) {

	// maximal value for uint64_t is 18446744073709551615 so 20 digits, so we say per 19 digits we
	// need one number, we need to ceil it

	// in some cases we need 2 more uint64_t values, that get stripped off by the leadinf zero
	// remover
	// at the end, but are needed in the middle of the algorithm, to be safe, we add an additional 1
	return consteval_ceil_div(string_length, const_constants::uint64_t_str_size_max - 1) +
	       const_constants::padding_for_bcd_algorihtm + 1;
}

consteval bool consteval_is_digit(StrType value) {
	return value >= '0' && value <= '9';
}

consteval uint8_t consteval_char_to_digit(StrType value) {
	return value - '0';
}

consteval bool consteval_is_separator(StrType value) {
	// valid separators are /[_',.]/
	return value == '_' || value == '\'' || value == ',' || value == '.';
}

template <size_t N> consteval void consteval_remove_leading_zeroes(BigIntConstExpr<N>& big_int) {
	if(big_int.numbers.size() ==
	   0) { // GCOVR_EXCL_BR_LINE (every caller assures that, internal function)
		CONSTEVAL_STATIC_ASSERT(
		    false,
		    "big_int has to have at least one number!"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)

	if(big_int.numbers.size() == 1) {
#ifndef NDEBUG
		if(big_int.numbers[0] == 0) {
			CONSTEVAL_STATIC_ASSERT(big_int.positive, "0 can't be negative");
		}
#endif

		return;
	}

	for(size_t i = big_int.numbers.size(); i > 1; --i) {
		if(big_int.numbers[i - 1] == 0) {
			big_int.numbers.pop_back();
		} else {
			break;
		}
	}

#ifndef NDEBUG
	if(big_int.numbers.size() == 1) {
		if(big_int.numbers[0] == 0) {
			CONSTEVAL_STATIC_ASSERT(big_int.positive, "0 can't be negative");
		}
	}
#endif
}

#define BIGINT_BIT_COUNT_FOR_BCD_ALG 64
#define BCD_DIGIT_BIT_COUNT_FOR_BCD_ALG 4

#define U64(n) (uint64_t)(n##ULL)

template <size_t N, size_t S>
consteval void consteval_bcd_digits_to_bigint(BigIntConstExpr<N>& big_int,
                                              BCDDigitsConstExpr<S>& bcd_digits) {
	// using reverse double dabble, see
	// https://en.wikipedia.org/wiki/Double_dabble#Reverse_double_dabble

	if(bcd_digits.size() == 0) { // GCOVR_EXCL_BR_LINE (every caller assures that)
		CONSTEVAL_STATIC_ASSERT(
		    false, "not initialized bcd_digits correctly"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)

	// this acts as a helper type, where we shift bits into, it is stored in reverse order than
	// normal bigints
	BigIntConstExpr<N> temp = { .positive = true, .numbers = {} };

	size_t pushed_bits = 0;

	size_t bcd_processed_fully_amount = 0;

	// first, process bcd_digits and populate temp in reverse order, we use this and pushed_bits to
	// make the final result!

	while(bcd_processed_fully_amount < bcd_digits.size()) {

		{ // 1. shift right by one

			{ // 1.1. shift last bit into the result

				// 1.1.1. if we need a new uint64_t, allocate it and set it to 0
				if((pushed_bits % BIGINT_BIT_COUNT_FOR_BCD_ALG) == 0) {
					temp.numbers.push_back(U64(0));
				}

				// 1.1.2. shift the last bit of every number into the next one
				for(size_t i = temp.numbers.size(); i != 0; --i) {
					uint8_t last_bit = ((temp.numbers[i - 1]) & 0x01);

					if(i == temp.numbers.size()) {
						CONSTEVAL_STATIC_ASSERT((last_bit == 0),
						                        "no additional uint64_t was allocated in time (we "
						                        "would overflow on >>)");
					} else {
						if(last_bit != 0) {
							temp.numbers[i] =
							    (U64(1) << (BIGINT_BIT_COUNT_FOR_BCD_ALG - 1)) + temp.numbers[i];
						}
					}

					temp.numbers[i - 1] = temp.numbers[i - 1] >> 1;
				}

				// 1.1.3. shift the last bit of the last bcd input into the first output
				BCDDigit last_value = bcd_digits[bcd_digits.size() - 1];
				if((last_value & 0x01) != 0) {
					temp.numbers[0] =
					    (U64(1) << (BIGINT_BIT_COUNT_FOR_BCD_ALG - 1)) + temp.numbers[0];
				}
			}

			{ // 1.2. shift every bcd_input along (only those who are not empty already)

				// 1.2.1. shift the last bit of every number into the next one
				for(size_t i = bcd_digits.size(); i > bcd_processed_fully_amount; --i) {
					uint8_t last_bit = ((bcd_digits[i - 1]) & 0x01);

					if(i == bcd_digits.size()) {
						// we already processed that earlier, ignore the last bit, it is shifted
						// away later in this for loop
					} else {
						if(last_bit != 0) {
							bcd_digits[i] = ((BCDDigit)1 << (BCD_DIGIT_BIT_COUNT_FOR_BCD_ALG - 1)) +
							                bcd_digits[i];
						}
					}

					bcd_digits[i - 1] = bcd_digits[i - 1] >> 1;
				}
			}
		}

		{ // 2. For each bcd_digit

			for(size_t i = bcd_digits.size(); i > bcd_processed_fully_amount; --i) {

				// 2.1 If value >= 8 then subtract 3 from value

				BCDDigit value = bcd_digits[i - 1];

				if(value >= 8) {
					bcd_digits[i - 1] = bcd_digits[i - 1] - 3;
				}
			}
		}
		// increment pushed_bits
		++pushed_bits;

		// if we emptied another bcd_digit, increment that counter, so that the end check stops
		// if necessary
		if((pushed_bits % BCD_DIGIT_BIT_COUNT_FOR_BCD_ALG) == 0) {
			++bcd_processed_fully_amount;
		}
	}

	{ // 3. set the final result

		big_int.numbers.resize(temp.numbers.size());

		// align the resulting uint64_t's e.g. if we would align to 6 bytes:
		// [100101,01xxxx] -> [yyyy10,010101], where x may be any bit, (but is 0 in practice), y
		// is always 0

		{ // 3.1 align the temp values

			uint8_t alignment = pushed_bits % BIGINT_BIT_COUNT_FOR_BCD_ALG;

			uint8_t to_shift = BIGINT_BIT_COUNT_FOR_BCD_ALG - alignment;

			if(alignment != 0) {

				// 3.1.1. shift the last to_shift bytes of every number into the next one
				for(size_t i = temp.numbers.size(); i != 0; --i) {
					uint64_t last_bytes = (temp.numbers[i - 1]) & ((U64(1) << to_shift) - U64(1));

					if(i == temp.numbers.size()) {
						// those x values from above are not 0
						CONSTEVAL_STATIC_ASSERT((last_bytes == 0),
						                        "alignment and to_shift incorrectly calculated");
					} else {
						temp.numbers[i] = (last_bytes << alignment) + temp.numbers[i];
					}

					temp.numbers[i - 1] = temp.numbers[i - 1] >> to_shift;
				}
			}
		}

		{ // 3.2. reverse the numbers and put them into the result
			for(size_t i = temp.numbers.size(); i != 0; --i) {
				big_int.numbers[temp.numbers.size() - i] = temp.numbers[i - 1];
			}
		}
	}
}

} // namespace

template <const_utils::ConstString S> [[nodiscard]] consteval auto get_bigint_from_string_impl() {

	constexpr size_t N = get_maximum_uint64_numbers_for_string_length(S.size);

	using SuccessResult = BigIntConstExpr<N>;

	using ResultType = const_utils::Expected<SuccessResult, MaybeBigIntError>;

	// bigint regex: /^[+-]?[0-9][0-9_',.]*$/

	if constexpr(S.size == 0) {
		return ResultType::error_result(MaybeBigIntError{
		    .message = "empty string is not valid",
		    .index = 0,
		    .symbol = NO_SYMBOL,
		});
	}

	// like bigint_helper_zero
	SuccessResult result = {
		.positive = true,
		.numbers = {},
	};
	result.numbers.push_back(0);

	size_t i = 0;

	if(S.str[0] == '-') {
		result.positive = false;
		++i;

		if constexpr(S.size == 1) {
			return ResultType::error_result(MaybeBigIntError{
			    .message = "'-' alone is not valid",
			    .index = 0,
			    .symbol = NO_SYMBOL,
			});
		}
	} else if(S.str[0] == '+') {
		result.positive = true;
		++i;

		if constexpr(S.size == 1) {
			return ResultType::error_result(MaybeBigIntError{
			    .message = "'+' alone is not valid",
			    .index = 0,
			    .symbol = NO_SYMBOL,
			});
		}
	} else {
		result.positive = true;
	}

	bool start = true;

	BCDDigitsConstExpr<S.size> bcd_digits = {};

	for(; i < S.size; ++i) {
		StrType value = S.str[i];

		if(consteval_is_digit(value)) {
			bcd_digits.push_back(consteval_char_to_digit(value));
		} else if(consteval_is_separator(value)) {
			if(start) {
				return ResultType::error_result(MaybeBigIntError{
				    .message = "separator not allowed at the start",
				    .index = i,
				    .symbol = value,
				});
			}
			// skip this separator
			continue;
		} else {

			return ResultType::error_result(MaybeBigIntError{
			    .message = "invalid character",
			    .index = i,
			    .symbol = value,
			});
		}

		if(start) {
			start = false;
		}
	}

	consteval_bcd_digits_to_bigint<N, S.size>(result, bcd_digits);

	if(result.numbers.size() == 1) {
		if(result.numbers[0] == 0) {
			if(!result.positive) {

				return ResultType::error_result(MaybeBigIntError{
				    .message = "-0 is not allowed",
				    .index = i,
				    .symbol = NO_SYMBOL,
				});
			}
		}
	}

	consteval_remove_leading_zeroes<N>(result);

	return ResultType::good_result(result);
}

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
} // namespace

template <const_utils::ConstString S> consteval auto operator""_n() {

	const auto result = get_bigint_from_string_impl<S>();

	CONSTEVAL_STATIC_ASSERT(result.has_value(), "incorrect bigint literal");

	return result.value();
}

// sanity tests at compile time:

static_assert("112412412351515313515"_n == "+112_412_412_351_515_313_515"_n);

static_assert("+112412412351515313515_____2___2___2"_n != "-12"_n);
static_assert("-13425264"_n != "+3523462422372356536723675467456753546735467435674536735467354"_n);
static_assert("-13425264"_n != "+0"_n);
static_assert("-13425264"_n == "-1_3_4_2_5_2_6_4"_n);
