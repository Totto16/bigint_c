

#include "./lib.h"
#include "../utils/assert.h"

#define BIGINT_C_LIB_INTERNAL_USAGE
#include "../utils/features.h"
#undef BIGINT_C_LIB_INTERNAL_USAGE

// NOLINTBEGIN(modernize-deprecated-headers)

#include <fenv.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// NOLINTEND(modernize-deprecated-headers)

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic,misc-use-anonymous-namespace,modernize-use-auto,modernize-use-using,cppcoreguidelines-no-malloc)

// functions on maybe bigint

NODISCARD BIGINT_C_LIB_EXPORTED bool maybe_bigint_is_error(MaybeBigIntC maybe_big_int) {
	return maybe_big_int.error;
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC maybe_bigint_get_value(MaybeBigIntC maybe_big_int) {
	ASSERT(!maybe_bigint_is_error(maybe_big_int), "MaybeBigIntC has no value");

	return maybe_big_int.data.result;
}

NODISCARD BIGINT_C_LIB_EXPORTED MaybeBigIntError
maybe_bigint_get_error(MaybeBigIntC maybe_big_int) {
	ASSERT(maybe_bigint_is_error(maybe_big_int), "MaybeBigIntC has no error");

	return maybe_big_int.data.error;
}

// normal bigint functions

#define U64(n) (uint64_t)(n##ULL)

static void bigint_helper_realloc_to_new_size(BigIntC* big_int) {

	uint64_t* new_numbers =
	    (uint64_t*)realloc(big_int->numbers, sizeof(uint64_t) * big_int->number_count);

	if(new_numbers == NULL) { // GCOVR_EXCL_BR_LINE (OOM)
		UNREACHABLE_WITH_MSG( // GCOVR_EXCL_LINE (OOM content)
		    "realloc failed, no error handling implemented here");
	} // GCOVR_EXCL_LINE (OOM content)

	big_int->numbers = new_numbers;
}

NODISCARD static inline BigIntC bigint_helper_number_impl(uint64_t number, bool positive) {

	BigIntC result = { .positive = positive, .numbers = NULL, .number_count = 1 };

	bigint_helper_realloc_to_new_size(&result);

	result.numbers[0] = number;

	return result;
}

NODISCARD static inline BigIntC bigint_helper_zero(void) {
	return bigint_helper_number_impl(0, true);
}

NODISCARD static inline bool bigint_helper_is_zero(BigIntC big_int) {
	return big_int.number_count == 1 && big_int.numbers[0] == 0;
}

typedef uint8_t BCDDigit;

typedef struct {
	BCDDigit* bcd_digits;
	size_t count;
	size_t capacity;
} BCDDigits;

static void free_bcd_digits(BCDDigits digits) {
	if(digits.bcd_digits != NULL) {
		free(digits.bcd_digits);
	}
}

#define BCD_DIGITS_START_CAPACITY 16

// TODO: as one bcd input only uses 4 bits, 2 of them could be stored in one uint8_t , but that is
// more complicated, when processing it, so this is a optimization for later

static void helper_add_value_to_bcd_digits(BCDDigits* digits, BCDDigit digit) {

	if(digits->count + 1 > digits->capacity) {
		const size_t new_size =
		    digits->capacity == 0 ? BCD_DIGITS_START_CAPACITY : digits->capacity * 2;

		BCDDigit* new_bcd_digits =
		    (BCDDigit*)realloc(digits->bcd_digits, sizeof(BCDDigit) * new_size);

		if(new_bcd_digits == NULL) { // GCOVR_EXCL_BR_LINE (OOM)
			UNREACHABLE_WITH_MSG(    // GCOVR_EXCL_LINE (OOM content)
			    "realloc failed, no error handling implemented here");
		} // GCOVR_EXCL_LINE (OOM content)

		digits->capacity = new_size;
		digits->bcd_digits = new_bcd_digits;
	}

	digits->bcd_digits[digits->count] = digit;

	++(digits->count);
}

#define BIGINT_BIT_COUNT 64

#define BIGINT_BIT_COUNT_FOR_BCD_ALG BIGINT_BIT_COUNT
#define BCD_DIGIT_BIT_COUNT_FOR_BCD_ALG 4

static void bigint_helper_bcd_digits_to_bigint(BigIntC* big_int, BCDDigits bcd_digits) {
	// using reverse double dabble, see
	// https://en.wikipedia.org/wiki/Double_dabble#Reverse_double_dabble

	if(bcd_digits.count == 0) { // GCOVR_EXCL_BR_LINE (every caller assures that, internal function)
		UNREACHABLE_WITH_MSG("not initialized bcd_digits correctly"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)

	// this acts as a helper type, where we shift bits into, it is stored in reverse order than
	// normal bigints
	BigIntC temp = { .positive = true, .numbers = NULL, .number_count = 0 };

	size_t pushed_bits = 0;

	size_t bcd_processed_fully_amount = 0;

	// first, process bcd_digits and populate temp in reverse order, we use this and pushed_bits to
	// make the final result!

	while(bcd_processed_fully_amount < bcd_digits.count) {

		{ // 1. shift right by one

			{ // 1.1. shift last bit into the result

				// 1.1.1. if we need a new uint64_t, allocate it and set it to 0
				if((pushed_bits % BIGINT_BIT_COUNT_FOR_BCD_ALG) == 0) {
					++(temp.number_count);
					bigint_helper_realloc_to_new_size(&temp);
					temp.numbers[temp.number_count - 1] = U64(0);
				}

				// 1.1.2. shift the last bit of every number into the next one
				for(size_t i = temp.number_count; i != 0; --i) {
					const uint8_t last_bit = ((temp.numbers[i - 1]) & 0x01);

					if(i == temp.number_count) {
						ASSERT((last_bit == 0), "no additional uint64_t was allocated in time (we "
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
				const BCDDigit last_value = bcd_digits.bcd_digits[bcd_digits.count - 1];
				if((last_value & 0x01) != 0) {
					temp.numbers[0] =
					    (U64(1) << (BIGINT_BIT_COUNT_FOR_BCD_ALG - 1)) + temp.numbers[0];
				}
			}

			{ // 1.2. shift every bcd_input along (only those who are not empty already)

				// 1.2.1. shift the last bit of every number into the next one
				for(size_t i = bcd_digits.count; i > bcd_processed_fully_amount; --i) {
					const uint8_t last_bit = ((bcd_digits.bcd_digits[i - 1]) & 0x01);

					if(i == bcd_digits.count) {
						// we already processed that earlier, ignore the last bit, it is shifted
						// away later in this for loop
					} else {
						if(last_bit != 0) {
							bcd_digits.bcd_digits[i] =
							    ((BCDDigit)1 << (BCD_DIGIT_BIT_COUNT_FOR_BCD_ALG - 1)) +
							    bcd_digits.bcd_digits[i];
						}
					}

					bcd_digits.bcd_digits[i - 1] = bcd_digits.bcd_digits[i - 1] >> 1;
				}
			}
		}

		{ // 2. For each bcd_digit

			for(size_t i = bcd_digits.count; i > bcd_processed_fully_amount; --i) {

				// 2.1 If value >= 8 then subtract 3 from value

				const BCDDigit value = bcd_digits.bcd_digits[i - 1];

				if(value >=
				   8) { // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
					bcd_digits.bcd_digits[i - 1] = bcd_digits.bcd_digits[i - 1] - 3;
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

		big_int->number_count = temp.number_count;
		bigint_helper_realloc_to_new_size(big_int);

		// align the resulting uint64_t's e.g. if we would align to 6 bytes:
		// [100101,01xxxx] -> [yyyy10,010101], where x may be any bit, (but is 0 in practice), y is
		// always 0

		{ // 3.1 align the temp values

			const uint8_t alignment = pushed_bits % BIGINT_BIT_COUNT_FOR_BCD_ALG;

			const uint8_t to_shift = BIGINT_BIT_COUNT_FOR_BCD_ALG - alignment;

			if(alignment != 0) {

				// 3.1.1. shift the last to_shift bytes of every number into the next one
				for(size_t i = temp.number_count; i != 0; --i) {
					const uint64_t last_bytes =
					    (temp.numbers[i - 1]) & ((U64(1) << to_shift) - U64(1));

					if(i == temp.number_count) {
						// those x values from above are not 0
						ASSERT((last_bytes == 0), "alignment and to_shift incorrectly calculated");
					} else {
						temp.numbers[i] = (last_bytes << alignment) + temp.numbers[i];
					}

					temp.numbers[i - 1] = temp.numbers[i - 1] >> to_shift;
				}
			}
		}

		{ // 3.2. reverse the numbers and put them into the result
			for(size_t i = temp.number_count; i != 0; --i) {
				big_int->numbers[temp.number_count - i] = temp.numbers[i - 1];
			}
		}
	}

	free_bigint(&temp);
}

static void bigint_helper_normalize(BigIntC* big_int) {
	if(big_int->number_count == 0) { // GCOVR_EXCL_BR_LINE (every caller assures that)
		UNREACHABLE_WITH_MSG(        // GCOVR_EXCL_LINE (see above)
		    "big_int has to have at least one number!"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)

	if(big_int->number_count == 1) {
		if(bigint_helper_is_zero(*big_int)) {
			big_int->positive = true;
		}

		return;
	}

	for(size_t i = big_int->number_count; i > 1; --i) {
		if(big_int->numbers[i - 1] == 0) {
			--(big_int->number_count);
		} else {
			break;
		}
	}

	if(bigint_helper_is_zero(*big_int)) {
		big_int->positive = true;
	}

	bigint_helper_realloc_to_new_size(big_int);
}

static void bigint_helper_remove_leading_zeroes_but_not_normalize(BigIntC* big_int) {
	if(big_int->number_count == 0) { // GCOVR_EXCL_BR_LINE (every caller assures that)
		UNREACHABLE_WITH_MSG(        // GCOVR_EXCL_LINE (see above)
		    "big_int has to have at least one number!"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)

	if(big_int->number_count == 1) {
		if(bigint_helper_is_zero(*big_int)) {
			ASSERT(big_int->positive,
			       "0 can't be negative, call 'bigint_helper_normalize' instead");
		}

		return;
	}

	for(size_t i = big_int->number_count; i > 1; --i) {
		if(big_int->numbers[i - 1] == 0) {
			--(big_int->number_count);
		} else {
			break;
		}
	}

	if(bigint_helper_is_zero(*big_int)) {
		ASSERT(big_int->positive, "0 can't be negative, call 'bigint_helper_normalize' instead");
	}

	bigint_helper_realloc_to_new_size(big_int);
}

NODISCARD static inline bool helper_is_digit(StrType value) {
	return value >= '0' && value <= '9';
}

NODISCARD static uint8_t helper_char_to_digit(StrType value) {
	return value - '0';
}

NODISCARD static StrType helper_digit_to_char_checked(uint8_t value) {

	ASSERT(value < 10, "value is not a valid digit");

	return (StrType)((StrType)value + '0');
}

NODISCARD static StrType helper_digit_to_hex_char_checked(uint8_t value, bool uppercase) {

	ASSERT(value < 0x10, "value is not a valid hex digit");

	if(value < 10) { // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
		return (StrType)((StrType)value + '0');
	}

	if(uppercase) {
		return (
		    StrType)((StrType)(value -
		                       (uint8_t)10) + // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
		             'A');
	}

	return (
	    StrType)((StrType)(value -
	                       (uint8_t)10) + // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
	             'a');
}

NODISCARD static inline bool helper_is_separator(StrType value) {
	// valid separators are /[_',.]/
	return value == '_' || value == '\'' || value == ',' || value == '.';
}

// TODO: add separate functions for parsing from bin and hex, and also one, that detects it based on
// prefix  (none means dec)
NODISCARD BIGINT_C_LIB_EXPORTED MaybeBigIntC maybe_bigint_from_string(ConstStr str) {

	BigIntC result = bigint_helper_zero();

	const size_t str_len = strlen(str);

	// bigint regex: /^[+-]?[0-9][0-9_',.]*$/

	if(str_len == 0) {
		free_bigint(&result);
		return (MaybeBigIntC){ .error = true,
			                   .data = { .error = (MaybeBigIntError){
			                                 .message = "empty string is not valid",
			                                 .index = 0,
			                                 .symbol = NO_SYMBOL,
			                             } } };
	}

	size_t index = 0;

	if(str[0] == '-') {
		result.positive = false;
		++index;

		if(str_len == 1) {
			free_bigint(&result);
			return (MaybeBigIntC){ .error = true,
				                   .data = { .error = (MaybeBigIntError){
				                                 .message = "'-' alone is not valid",
				                                 .index = 0,
				                                 .symbol = NO_SYMBOL,
				                             } } };
		}

	} else if(str[0] == '+') {
		result.positive = true;
		++index;

		if(str_len == 1) {
			free_bigint(&result);
			return (MaybeBigIntC){ .error = true,
				                   .data = { .error = (MaybeBigIntError){
				                                 .message = "'+' alone is not valid",
				                                 .index = 0,
				                                 .symbol = NO_SYMBOL,
				                             } } };
		}
	} else {
		result.positive = true;
	}

	bool start = true;

	BCDDigits bcd_digits = { .bcd_digits = NULL, .count = 0, .capacity = 0 };

	for(; index < str_len; ++index) {
		const StrType value = str[index];

		if(helper_is_digit(value)) {
			helper_add_value_to_bcd_digits(&bcd_digits, helper_char_to_digit(value));
		} else if(helper_is_separator(value)) {
			if(start) {
				// not allowed
				free_bigint(&result);
				free_bcd_digits(bcd_digits);
				return (
				    MaybeBigIntC){ .error = true,
					               .data = { .error = (MaybeBigIntError){
					                             .message = "separator not allowed at the start",
					                             .index = index,
					                             .symbol = value,
					                         } } };
			}
			// skip this separator
			continue;
		} else {
			free_bigint(&result);
			free_bcd_digits(bcd_digits);
			return (MaybeBigIntC){ .error = true,
				                   .data = { .error = (MaybeBigIntError){
				                                 .message = "invalid character",
				                                 .index = index,
				                                 .symbol = value,
				                             } } };
		}

		if(start) {
			start = false;
		}
	}

	bigint_helper_bcd_digits_to_bigint(&result, bcd_digits);

	free_bcd_digits(bcd_digits);

	if(bigint_helper_is_zero(result)) {
		if(!result.positive) {
			free_bigint(&result);
			return (MaybeBigIntC){ .error = true,
				                   .data = { .error = (MaybeBigIntError){
				                                 .message = "-0 is not allowed",
				                                 .index = index,
				                                 .symbol = NO_SYMBOL,
				                             } } };
		}
	}

	bigint_helper_normalize(&result);

	return (MaybeBigIntC){ .error = false, .data = { .result = result } };
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_from_unsigned_number(uint64_t number) {
	BigIntC result = bigint_helper_number_impl(number, true);

	return result;
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_from_signed_number(int64_t number) {
	BigIntC result = bigint_helper_zero();

	if(number < 0LL) {
		result.positive = false;
		// overflow, when using - on int64_t
		if(number < -INT64_MAX) {
			result.numbers[0] = (uint64_t)(-(number + 1LL)) + 1ULL;
		} else {
			result.numbers[0] = (uint64_t)(-number);
		}
	} else {
		result.positive = true;
		result.numbers[0] = number;
	}

	return result;
}

NODISCARD static BigIntC bigint_helper_get_full_copy(BigIntC big_int) {

	BigIntC result = { .positive = big_int.positive,
		               .numbers = NULL,
		               .number_count = big_int.number_count };

	bigint_helper_realloc_to_new_size(&result);

	memcpy(result.numbers, big_int.numbers, // NOLINT(clang-analyzer-core.NonNullParamChecker)
	       sizeof(uint64_t) * big_int.number_count);

	return result;
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_from_list_of_numbers(const uint64_t* const numbers,
                                                                    size_t size, bool positive) {

	BigIntC result = { .positive = positive, .numbers = NULL, .number_count = size };

	bigint_helper_realloc_to_new_size(&result);

	for(size_t i = 0; i < size; ++i) {
		result.numbers[size - i - 1] = numbers[i];
	}

	bigint_helper_normalize(&result);

	return result;
}

BIGINT_C_LIB_EXPORTED void free_bigint(BigIntC* big_int) {
	if(big_int == NULL) {
		return;
	}

	if(big_int->numbers != NULL) {
		free(big_int->numbers);
		big_int->numbers = NULL;
	}
}

BIGINT_C_LIB_EXPORTED void free_bigint_without_reset(BigIntC big_int) {
	if(big_int.numbers != NULL) {
		free(big_int.numbers);
	}
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_copy(BigIntC big_int) {
	return bigint_helper_get_full_copy(big_int);
}

/**
 * @brief gets the amount of bits used, in the range 0 - 64
 *
 * @param number
 * @return used bits
 */
NODISCARD static size_t bigint_helper_bits_of_number_used(uint64_t number) {

	uint64_t temp = number;
	size_t result = 0;

	while(temp != U64(0)) {
		temp = temp >> 1;
		++result;
	}

	return result;
}

NODISCARD static BCDDigits bigint_helper_get_bcd_digits_from_bigint(BigIntC source) {

	// using double dabble, see
	// https://en.wikipedia.org/wiki/Double_dabble

	if(source.number_count == 0) { // GCOVR_EXCL_BR_LINE (every caller assures that)
		UNREACHABLE_WITH_MSG("not initialized BigIntC correctly"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)

	// reverse the source so that the bits are aligned
	{
		for(size_t i = 0; i < source.number_count / 2; ++i) {
			const uint64_t temp = source.numbers[i];
			source.numbers[i] = source.numbers[source.number_count - 1 - i];

			source.numbers[source.number_count - 1 - i] = temp;
		}
	}

	size_t last_number_bit_amount = bigint_helper_bits_of_number_used(
	    source.numbers[0]); // range 0 -64 0 should never be here, as then i should have remove
	                        // it earlier (remove leading zeroes!)

	// but it is here when the value is just 0 ("0")
	if(last_number_bit_amount == 0) {
		last_number_bit_amount = 1; // print one 0, even if it's a not a 1
	}

	// calculate the amount of input bits
	const size_t total_input_bits = source.number_count * BIGINT_BIT_COUNT_FOR_BCD_ALG;

	// setup working variables
	BCDDigits bcd_digits = { .bcd_digits = NULL, .count = 0, .capacity = 0 };

	size_t current_bit =
	    BIGINT_BIT_COUNT_FOR_BCD_ALG -
	    last_number_bit_amount; // range 0 - 63 at start, later 0 -> total_input_bits -1

	while(current_bit < total_input_bits) {

		{ // 1. For each bcd_digit

			for(size_t i = 0; i < bcd_digits.count; ++i) {

				// 2.1. If value >= 5 then add 3 to value

				const BCDDigit value = bcd_digits.bcd_digits[i];

				if(value >=
				   5) { // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
					bcd_digits.bcd_digits[i] = bcd_digits.bcd_digits[i] + 3;
				}
			}
		}

		{ // 2. shift left by one

			{ // 2.1. shift every bcd_output to the left
				// ( not in 8 but in 4 bit shifts), for
				// convience we shift to the right in the array elements as the order is [0., 1.
				// ] and we need to shift from the 0. left to the 1.

				{ // 2.1.1. if we need a new bcd_digit, allocate it and set it to 0

					bool needs_new_digit = false;

					{ // 2.1.1.1. determine if we need a new bcd_digit

						// 2.1.1.2. if no digits i present, we need a new one
						if(bcd_digits.count == 0) {
							needs_new_digit = true;
						} else {
							// 2.1.1.3. we need a new one, if the next shift would overflow!
							const BCDDigit value = bcd_digits.bcd_digits[bcd_digits.count - 1];
							const uint8_t first_bit =
							    (value >> (BCD_DIGIT_BIT_COUNT_FOR_BCD_ALG - 1)) & 0x01;
							if(first_bit != 0) {
								needs_new_digit = true;
							}
						}
					}

					if(needs_new_digit) {
						helper_add_value_to_bcd_digits(&bcd_digits, 0);
					}
				}

				{ // 2.1.2 shift the first bit (4. bit) of every number into the next one

					for(size_t i = bcd_digits.count; i != 0; --i) {

						const BCDDigit value = bcd_digits.bcd_digits[i - 1];

						const uint8_t first_bit =
						    (value >> (BCD_DIGIT_BIT_COUNT_FOR_BCD_ALG - 1)) & 0x01;

						if(i == bcd_digits.count) {
							ASSERT((first_bit == 0), "the first bit of the first bcd_digit has to "
							                         "be 0, as this was just created empty or "
							                         "should only get the value after this shift");
						} else {
							if(first_bit != 0) {
								bcd_digits.bcd_digits[i] = bcd_digits.bcd_digits[i] | first_bit;
							}
						}

						// shift to the left, but only keep 4 bits
						bcd_digits.bcd_digits[i - 1] =
						    (bcd_digits.bcd_digits[i - 1] << 1) &
						    (((BCDDigit)1 << BCD_DIGIT_BIT_COUNT_FOR_BCD_ALG) - (BCDDigit)1);
					}
				}
			}

			{ // 2.2 shift first input bit into the bcd_result

				const size_t input_index = current_bit / BIGINT_BIT_COUNT_FOR_BCD_ALG;

				const size_t input_u64_index =
				    BIGINT_BIT_COUNT_FOR_BCD_ALG - (current_bit % BIGINT_BIT_COUNT_FOR_BCD_ALG) - 1;

				ASSERT(input_index < source.number_count, "input index would index out of bounds");

				const uint8_t first_bit = (source.numbers[input_index] >> input_u64_index) & 0x01;

				ASSERT(bcd_digits.count > 0, "bcd_digits has to be initialized");
				if(first_bit != 0) {
					bcd_digits.bcd_digits[0] = bcd_digits.bcd_digits[0] | first_bit;
				}
			}
		}

		// increment current_bit
		++current_bit;
	}

	return bcd_digits;
}

// TODO: support also some options, as for to_string_hex and to_string_bin
NODISCARD BIGINT_C_LIB_EXPORTED Str bigint_to_string(BigIntC big_int) {

	if(big_int.number_count == 0) {
		return NULL;
	}

	BigIntC copy = bigint_helper_get_full_copy(big_int);

	const BCDDigits bcd_digits = bigint_helper_get_bcd_digits_from_bigint(copy);

	free_bigint(&copy);

	// format bcd_digits into a string, note that the bcd_digits are stored reversed

	size_t string_size = bcd_digits.count;

	if(!big_int.positive) {
		string_size = string_size + 1;
	}

	Str str = (Str)malloc(sizeof(StrType) * (string_size + 1));

	if(str == NULL) {                // GCOVR_EXCL_BR_LINE (OOM)
		free_bcd_digits(bcd_digits); // GCOVR_EXCL_LINE (OOM content)
		return NULL;                 // GCOVR_EXCL_LINE (OOM content)
	}

	str[string_size] = '\0';

	size_t index = 0;

	if(!big_int.positive) {
		str[index] = '-';
		++index;
	}

	const size_t offset = string_size - bcd_digits.count;

	for(; index < string_size; ++index) {

		const size_t digits_index = offset + bcd_digits.count - index - 1;
		ASSERT(digits_index < bcd_digits.count, "string conversion overflowed bcd_digits");

		str[index] = helper_digit_to_char_checked(bcd_digits.bcd_digits[digits_index]);
	}

	free_bcd_digits(bcd_digits);

	return str;
}

#define SIZEOF_HEX_PREFIX 2UL
#define HEX_PREFIX "0x"
#define SIZEOF_BYTE_AS_HEX_STR 2UL
#define SIZEOF_VALUE_AS_HEX_STR (SIZEOF_BYTE_AS_HEX_STR * 8UL)

// TODO: add option to show + when it is positive!	add ability to choose gap character, use
// struct to not pass a million booleans around!
NODISCARD BIGINT_C_LIB_EXPORTED Str bigint_to_string_hex(BigIntC big_int, bool prefix,
                                                         bool add_gaps, bool trim_first_number,
                                                         bool uppercase) {

	if(big_int.number_count == 0) {
		return NULL;
	}

	size_t string_size = big_int.number_count * SIZEOF_VALUE_AS_HEX_STR;

	if(!big_int.positive) {
		string_size = string_size + 1;
	}

	if(prefix) {
		string_size = string_size + SIZEOF_HEX_PREFIX;
	}

	if(add_gaps) {
		string_size = string_size + (big_int.number_count - 1);
	}

	Str str = (Str)malloc(sizeof(StrType) * (string_size + 1));

	if(str == NULL) { // GCOVR_EXCL_BR_LINE (OOM)
		return NULL;  // GCOVR_EXCL_LINE (OOM content)
	}

	str[string_size] = '\0';

	size_t index = 0;

	if(!big_int.positive) {
		str[index] = '-';
		++index;
	}

	if(prefix) {
		for(size_t j = 0; j < SIZEOF_HEX_PREFIX; ++j) {
			str[index] = HEX_PREFIX[j];
			++index;
		}
	}

	size_t current_number = big_int.number_count;

	for(; index < string_size && current_number != 0; --current_number) {

		const uint64_t number = big_int.numbers[current_number - 1];

		size_t start_point = 0;

		if(trim_first_number) {
			if(current_number == big_int.number_count) {
				const size_t bits_used = bigint_helper_bits_of_number_used(number);
				start_point = (BIGINT_BIT_COUNT - bits_used) / 4;
				if(start_point == SIZEOF_VALUE_AS_HEX_STR) {
					start_point =
					    SIZEOF_VALUE_AS_HEX_STR - 1; // print one 0, even if it's a not a 1
				}
				ASSERT(start_point < SIZEOF_VALUE_AS_HEX_STR, "start_point was too high");
			}
		}

		for(size_t j = start_point; j < SIZEOF_VALUE_AS_HEX_STR; ++index, ++j) {
			const uint8_t digit = (number >> ((BIGINT_BIT_COUNT - ((j + 1) * 4)))) & 0x0F;
			str[index] = helper_digit_to_hex_char_checked(digit, uppercase);
		}

		if(add_gaps && current_number != 1) {
			str[index] = ' ';
			++index;
		}
	}

	ASSERT(current_number == 0, "for loop exited too early");
	ASSERT(index <= string_size, "string size was not enough or for loop implementation error");

	// if we trim the first number, the end of the string is sooner, so set the 0 byte there
	str[index] = '\0';

	return str;
}

#define SIZEOF_BIN_PREFIX 2UL
#define BIN_PREFIX "0b"
#define SIZEOF_BYTE_AS_BIN_STR 8UL
#define SIZEOF_VALUE_AS_BIN_STR (SIZEOF_BYTE_AS_BIN_STR * 8UL)

NODISCARD BIGINT_C_LIB_EXPORTED Str bigint_to_string_bin(BigIntC big_int, bool prefix,
                                                         bool add_gaps, bool trim_first_number) {
	if(big_int.number_count == 0) {
		return NULL;
	}

	size_t string_size = big_int.number_count * SIZEOF_VALUE_AS_BIN_STR;

	if(!big_int.positive) {
		string_size = string_size + 1;
	}

	if(prefix) {
		string_size = string_size + SIZEOF_BIN_PREFIX;
	}

	if(add_gaps) {
		string_size = string_size + (big_int.number_count - 1);
	}

	Str str = (Str)malloc(sizeof(StrType) * (string_size + 1));

	if(str == NULL) { // GCOVR_EXCL_BR_LINE (OOM)
		return NULL;  // GCOVR_EXCL_LINE (OOM content)
	}

	str[string_size] = '\0';

	size_t index = 0;

	if(!big_int.positive) {
		str[index] = '-';
		++index;
	}

	if(prefix) {
		for(size_t j = 0; j < SIZEOF_BIN_PREFIX; ++j) {
			str[index] = BIN_PREFIX[j];
			++index;
		}
	}

	size_t current_number = big_int.number_count;

	for(; index < string_size && current_number != 0; --current_number) {

		const uint64_t number = big_int.numbers[current_number - 1];

		size_t start_point = 0;

		if(trim_first_number) {
			if(current_number == big_int.number_count) {
				const size_t bits_used = bigint_helper_bits_of_number_used(number);
				start_point = BIGINT_BIT_COUNT - bits_used;
				if(start_point == SIZEOF_VALUE_AS_BIN_STR) {
					start_point =
					    SIZEOF_VALUE_AS_BIN_STR - 1; // print one 0, even if it's a not a 1
				}
				ASSERT(start_point < SIZEOF_VALUE_AS_BIN_STR, "start_point was too high");
			}
		}

		for(size_t j = start_point; j < SIZEOF_VALUE_AS_BIN_STR; ++index, ++j) {
			const uint8_t digit = (number >> ((BIGINT_BIT_COUNT - ((j + 1))))) & 0x01;
			str[index] = digit == 0 ? '0' : '1';
		}

		if(add_gaps && current_number != 1) {
			str[index] = ' ';
			++index;
		}
	}

	ASSERT(current_number == 0, "for loop exited too early");
	ASSERT(index <= string_size, "string size was not enough or for loop implementation error");

	// if we trim the first number, the end of the string is sooner, so set the 0 byte there
	str[index] = '\0';

	return str;
}

NODISCARD static size_t helper_max(size_t num1, size_t num2) {
	if(num1 > num2) {
		return num1;
	}
	return num2;
}

#if !defined(BIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION)
#error "DEFINE BIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION"
#elif BIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION == 0
typedef __uint128_t uint128_t; // NOLINT(readability-identifier-naming)
typedef __int128_t int128_t;   // NOLINT(readability-identifier-naming)
#elif BIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION == 1

#else
#error "unknown BIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION"
#endif

#if BIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION == 0

NODISCARD static BigIntC bigint_add_bigint_both_positive_using_128_bit_numbers(BigIntC big_int1,
                                                                               BigIntC big_int2) {

	const size_t max_count = helper_max(big_int1.number_count, big_int2.number_count) + 1;

	BigIntC result = { .positive = true, .numbers = NULL, .number_count = max_count };

	bigint_helper_realloc_to_new_size(&result);

	{ // 1. perform the actual addition

		uint64_t carry = U64(0);

		for(size_t i = 0; i < result.number_count; ++i) {

			uint128_t sum = (uint128_t)carry;

			if(i < big_int1.number_count) {
				sum = sum + (uint128_t)big_int1.numbers[i];
			}

			if(i < big_int2.number_count) {
				sum = sum + (uint128_t)big_int2.numbers[i];
			}

			result.numbers[i] = (uint64_t)sum;

			carry =
			    (uint64_t)(sum >>
			               64); // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
		}

		ASSERT(carry == 0,
		       "The carry at the end has to be zero, otherwise we would have an overflow");
	}

	bigint_helper_remove_leading_zeroes_but_not_normalize(&result);

	return result;
}

NODISCARD static BigIntC bigint_sub_bigint_both_positive_using_128_bit_numbers(BigIntC big_int1,
                                                                               BigIntC big_int2) {

	// NOTE: here it is assumed, that  a > b

	const size_t max_count = helper_max(big_int1.number_count, big_int2.number_count) + 1;

	BigIntC result = { .positive = true, .numbers = NULL, .number_count = max_count };

	bigint_helper_realloc_to_new_size(&result);

	{ // 1. perform the actual subtraction

		int64_t borrow = (int64_t)0LL;

		for(size_t i = 0; i < result.number_count; ++i) {

			int128_t temp = (int128_t)0LL;

			if(i < big_int1.number_count) {
				temp = (int128_t)big_int1.numbers[i];
			}

			if(i < big_int2.number_count) {
				temp = temp - (int128_t)big_int2.numbers[i];
			}

			if(borrow != 0) {
				temp = temp - borrow;
			}

			// check if we need to adjust temp and set the borrow
			if(temp >= 0) {
				borrow = (int64_t)0LL;
			} else {
				temp =
				    ((int128_t)1
				     << 64) + // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
				    temp;
				borrow = (int64_t)1LL;
			}

			result.numbers[i] = (uint64_t)temp;
		}

		ASSERT(borrow == 0,
		       "The borrow at the end has to be zero, otherwise we would have an overflow");
	}

	bigint_helper_remove_leading_zeroes_but_not_normalize(&result);

	return result;
}
#elif BIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION == 1

NODISCARD static uint8_t bigint_helper_add_uint64_with_carry(uint8_t carry_in, uint64_t value1,
                                                             uint64_t value2, uint64_t* result_out);

NODISCARD static uint8_t bigint_helper_sub_uint64_with_borrow(uint8_t borrow_in, uint64_t value1,
                                                              uint64_t value2,
                                                              uint64_t* result_out);

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)

// use fast intrinsic (in ASM ADC) on x86_64

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

NODISCARD static inline uint8_t bigint_helper_add_uint64_with_carry(uint8_t carry_in,
                                                                    uint64_t value1,
                                                                    uint64_t value2,
                                                                    uint64_t* result_out) {

#if defined(_MSC_VER)
	// see:
	// https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#text=_addcarry_u64&ig_expand=175
	return _addcarry_u64(carry_in, value1, value2, result_out);
#else
	STATIC_ASSERT(sizeof(unsigned long long) == sizeof(uint64_t),
	              "we must use the same type for ass intrinsics");
	unsigned long long result = 0;
	uint8_t res = _addcarry_u64(carry_in, value1, value2, &result);

	*result_out = result;

	return res;
#endif
}

NODISCARD static inline uint8_t bigint_helper_sub_uint64_with_borrow(uint8_t borrow_in,
                                                                     uint64_t value1,
                                                                     uint64_t value2,
                                                                     uint64_t* result_out) {

#if defined(_MSC_VER)
	// see:
	// https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#text=_subborrow_u64&ig_expand=6666
	return _subborrow_u64(borrow_in, value1, value2, result_out);
#else
	STATIC_ASSERT(sizeof(unsigned long long) == sizeof(uint64_t),
	              "we must use the same type for sub intrinsics");
	unsigned long long result = 0;
	uint8_t res = _subborrow_u64(borrow_in, value1, value2, &result);

	*result_out = result;

	return res;

#endif
}

#elif defined(__GNUC__)

NODISCARD static inline uint8_t bigint_helper_add_uint64_with_carry(uint8_t carry_in,
                                                                    uint64_t value1,
                                                                    uint64_t value2,
                                                                    uint64_t* result_out) {

	uint64_t value1_r = 0;
	bool carry1 = __builtin_add_overflow(value1, (uint64_t)carry_in, &value1_r);

	bool carry2 = __builtin_add_overflow(value1_r, value2, result_out);

	return carry1 || carry2 ? 1 : 0;
}

NODISCARD static inline uint8_t bigint_helper_sub_uint64_with_borrow(uint8_t borrow_in,
                                                                     uint64_t value1,
                                                                     uint64_t value2,
                                                                     uint64_t* result_out) {

	uint64_t value2_r = 0;
	bool borrow1 = __builtin_add_overflow(value2, (uint64_t)borrow_in, &value2_r);

	bool borrow2 = __builtin_sub_overflow(value1, value2_r, result_out);

	return borrow1 || borrow2 ? 1 : 0;
}

#else
NODISCARD static uint8_t bigint_helper_add_uint64_with_carry(uint8_t carry_in, uint64_t value1,
                                                             uint64_t value2,
                                                             uint64_t* result_out) {
	const uint64_t sum = value1 + value2;
	*result_out = sum + carry_in;

	bool carry1 = sum < value1;
	bool carry2 = *result_out < sum;

	uint8_t carry = carry1 || carry2 ? 1 : 0;
	return carry;
}

NODISCARD static inline uint8_t bigint_helper_sub_uint64_with_borrow(uint8_t borrow_in,
                                                                     uint64_t value1,
                                                                     uint64_t value2,
                                                                     uint64_t* result_out) {

	uint64_t value2_r = value2;

	bool local_borrow = false;

	if(borrow_in != 0) {

		value2_r = value2 + borrow_in;

		local_borrow = (value2_r < value2);
	}

	// check if the next subtraction would underflow
	local_borrow = local_borrow || value1 < value2_r;

	uint64_t temp = value1 - value2_r;

	*result_out = temp;

	return local_borrow ? 1 : 0;
}

#endif

NODISCARD static BigIntC bigint_add_bigint_both_positive_normal(BigIntC big_int1,
                                                                BigIntC big_int2) {

	size_t max_count = helper_max(big_int1.number_count, big_int2.number_count) + 1;

	BigIntC result = { .positive = true, .number_count = max_count, .numbers = NULL };

	bigint_helper_realloc_to_new_size(&result);

	{ // 1. perform the actual addition

		uint8_t carry = U64(0);

		for(size_t i = 0; i < result.number_count; ++i) {

			uint64_t value1 = U64(0);
			uint64_t value2 = U64(0);

			if(i < big_int1.number_count) {
				value1 = big_int1.numbers[i];
			}

			if(i < big_int2.number_count) {
				value2 = big_int2.numbers[i];
			}

			carry =
			    bigint_helper_add_uint64_with_carry(carry, value1, value2, &(result.numbers[i]));
		}

		ASSERT(carry == 0,
		       "The carry at the end has to be zero, otherwise we would have an overflow");
	}

	bigint_helper_remove_leading_zeroes_but_not_normalize(&result);

	return result;
}

NODISCARD static BigIntC bigint_sub_bigint_both_positive_normal(BigIntC big_int1,
                                                                BigIntC big_int2) {

	// NOTE: here it is assumed, that  a > b

	size_t max_count = helper_max(big_int1.number_count, big_int2.number_count) + 1;

	BigIntC result = { .positive = true, .number_count = max_count, .numbers = NULL };

	bigint_helper_realloc_to_new_size(&result);

	{ // 1. perform the actual subtraction

		unsigned char borrow = 0;

		for(size_t i = 0; i < result.number_count; ++i) {

			uint64_t value1 = U64(0);
			uint64_t value2 = U64(0);

			if(i < big_int1.number_count) {
				value1 = big_int1.numbers[i];
			}

			if(i < big_int2.number_count) {
				value2 = big_int2.numbers[i];
			}

			borrow =
			    bigint_helper_sub_uint64_with_borrow(borrow, value1, value2, &(result.numbers[i]));
		}

		ASSERT(borrow == 0,
		       "The borrow at the end has to be zero, otherwise we would have an overflow");
	}

	bigint_helper_remove_leading_zeroes_but_not_normalize(&result);

	return result;
}
#else
#error "unknown BIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION"
#endif

NODISCARD static BigIntC bigint_add_bigint_both_positive(BigIntC big_int1, BigIntC big_int2) {

#if BIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION == 0
	return bigint_add_bigint_both_positive_using_128_bit_numbers(big_int1, big_int2);
#else
	return bigint_add_bigint_both_positive_normal(big_int1, big_int2);
#endif
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC
bigint_add_bigint(BigIntC big_int1, BigIntC big_int2) { // NOLINT(misc-no-recursion)

	if(big_int1.positive) {

		if(big_int2.positive) {
			// +a + +b
			return bigint_add_bigint_both_positive(big_int1, big_int2);
		}

		// +a + -b = +a - +b
		big_int2.positive = true;
		return bigint_sub_bigint(big_int1, big_int2);
	}

	if(big_int2.positive) {
		// -a + +b = +b - +a
		big_int1.positive = true;
		return bigint_sub_bigint(big_int2, // NOLINT(readability-suspicious-call-argument)
		                         big_int1);
	}

	// both are negative

	// -a + -b = - ( +a + +b )

	big_int1.positive = true;
	big_int2.positive = true;

	BigIntC result = bigint_add_bigint_both_positive(big_int1, big_int2);

	result.positive = false;

	return result;
}

NODISCARD static BigIntC bigint_sub_bigint_both_positive_impl(BigIntC big_int1, BigIntC big_int2) {

#if BIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION == 0
	return bigint_sub_bigint_both_positive_using_128_bit_numbers(big_int1, big_int2);
#else
	return bigint_sub_bigint_both_positive_normal(big_int1, big_int2);
#endif
}

NODISCARD static BigIntC bigint_sub_bigint_both_positive(BigIntC big_int1, BigIntC big_int2) {

	// check in which direction we need to perform the subtraction
	const int8_t compared = bigint_compare_bigint(big_int1, big_int2);

	if(compared == 0) {
		return bigint_helper_zero();
	}

	if(compared > 0) {
		return bigint_sub_bigint_both_positive_impl(big_int1, big_int2);
	}

	// +a - +b where b > a = - ( +b - +a)
	BigIntC result =
	    bigint_sub_bigint_both_positive_impl( // NOLINT(readability-suspicious-call-argument)
	        big_int2, big_int1);

	result.positive = false;
	return result;
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC
bigint_sub_bigint(BigIntC big_int1, BigIntC big_int2) { // NOLINT(misc-no-recursion)

	if(big_int1.positive) {
		if(big_int2.positive) {
			//+a - +b
			return bigint_sub_bigint_both_positive(big_int1, big_int2);
		}

		// +a - -b = +a + +b
		big_int2.positive = true;
		return bigint_add_bigint(big_int1, big_int2);
	}

	if(big_int2.positive) {
		// -a - +b = -a + -b
		big_int2.positive = false;
		return bigint_add_bigint(big_int1, big_int2);
	}

	// both are negative

	// -a - -b = -a + +b = +b - +a

	big_int1.positive = true;
	big_int2.positive = true;

	BigIntC result =
	    bigint_sub_bigint_both_positive( // NOLINT(readability-suspicious-call-argument)
	        big_int2, big_int1);

	return result;
}

static void bigint_increment_bigint_positive_or_zero_impl(BigIntC* big_int1) {
	// no need for a fast track, as the fast track is the first loop iteration

	// also 0 is handled correctly in here, as it is just assumed to be positive and 0 ++ = 1

	// increment the first uint64_t, that isn't maxed out, so that it adds one, if it is max, set
	// it to 0, as it carries to the next one, we may never exit, if all numbers are max, than we
	// need another digit!
	for(size_t i = 0; i < big_int1->number_count; ++i) {

		uint64_t* number = &(big_int1->numbers[i]);

		if(*number != UINT64_MAX) {
			++(*number);
			return;
		} else {
			*number = 0;
		}
	}

	++(big_int1->number_count);
	bigint_helper_realloc_to_new_size(big_int1);

	big_int1->numbers[big_int1->number_count - 1] = 1;
	return;
}

static void bigint_decrement_bigint_positive_not_zero_impl(BigIntC* big_int1) {
	// no need for a fast track, as the fast track is the first loop iteration

	// also 0 is not handled correctly in here, so never pass 0!

	// decrement the first uint64_t, that isn't 0, so that it removes one, if it is 0, set
	// it to UINT64_MAX, as it borrows to the next one, we HAVE TO EXIT, except if all numbers are
	// 0, which should never happen, but it is asserted here too!
	for(size_t i = 0; i < big_int1->number_count; // GCOVR_EXCL_BR_LINE (gcovr can't detect asserts,
	                                              // the end condition is an assert)

	    ++i) {
		uint64_t* number = &(big_int1->numbers[i]);

		if(*number != 0) {
			--(*number);

			// we may have created some zeroes!
			bigint_helper_remove_leading_zeroes_but_not_normalize(big_int1);

			return;
		} else {
			if(big_int1->number_count == 1) { // GCOVR_EXCL_BR_LINE (no caller uses the 0 here)
				UNREACHABLE_WITH_MSG(         // GCOVR_EXCL_LINE (see above)
				    "not supporting 0 in this function");
			} // GCOVR_EXCL_LINE (see above)

			*number = UINT64_MAX;
		}
	}

	UNREACHABLE_WITH_MSG("leading zeros detected"); // GCOVR_EXCL_LINE (gcovr can't detect asserts)
}

BIGINT_C_LIB_EXPORTED void bigint_increment_bigint(BigIntC* big_int1) {

	if(big_int1 == NULL) { // GCOVR_EXCL_BR_LINE (gcovr can't detect asserts)
		UNREACHABLE_WITH_MSG("passed in NULL pointer"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)

	if(big_int1->number_count == 0) { // GCOVR_EXCL_BR_LINE (gcovr can't detect asserts)
		UNREACHABLE_WITH_MSG("invalid bigint passed"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)

	// treat 0 as special case, as it NEVER should be -, but if it would be, the code afterwards
	// would break
	if(bigint_helper_is_zero(*big_int1)) {
		big_int1->numbers[0] = 1;
		big_int1->positive = true;
		return;
	}

	if(big_int1->positive) {
		bigint_increment_bigint_positive_or_zero_impl(big_int1);
		return;
	}

	// -a ++ = -a + +1 =  (-1 * +a) + (-1 * -1) = -1 * ( +a + -1) = - ( +a - +1) = - (+a --)

	big_int1->positive = true;
	bigint_decrement_bigint_positive_not_zero_impl(big_int1);

	if(bigint_helper_is_zero(*big_int1)) {
		big_int1->positive = true;
		return;
	}

	big_int1->positive = false;

	return;
}

BIGINT_C_LIB_EXPORTED void bigint_decrement_bigint(BigIntC* big_int1) {

	if(big_int1 == NULL) { // GCOVR_EXCL_BR_LINE (gcovr can't detect asserts)
		UNREACHABLE_WITH_MSG("passed in NULL pointer"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)

	if(big_int1->number_count == 0) { // GCOVR_EXCL_BR_LINE (gcovr can't detect asserts)
		UNREACHABLE_WITH_MSG("invalid bigint passed"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)

	// treat 0 as special case, as it NEVER should be -, but if it would be, the code afterwards
	// would break
	if(bigint_helper_is_zero(*big_int1)) {
		big_int1->numbers[0] = 1;
		big_int1->positive = false;
		return;
	}

	if(!big_int1->positive) {

		// -a -- = -a - +1 =  -a + -1 = - (+a + +1) = - (+a ++)

		big_int1->positive = true;
		bigint_increment_bigint_positive_or_zero_impl(big_int1);
		big_int1->positive = false;

		return;
	}

	bigint_decrement_bigint_positive_not_zero_impl(big_int1);

	return;
}

NODISCARD BIGINT_C_LIB_EXPORTED bool bigint_eq_bigint(BigIntC big_int1, BigIntC big_int2) {
	if(big_int1.positive != big_int2.positive) {
		return false;
	}

	if(big_int1.number_count != big_int2.number_count) {
		return false;
	}

	for(size_t i = 0; i < big_int1.number_count; ++i) {
		if(big_int1.numbers[i] != big_int2.numbers[i]) {
			return false;
		}
	}

	return true;
}

NODISCARD static int8_t cmp_reverse(int8_t value) {
	if(value == 0) {
		return 0;
	}

	if(value > 0) {
		return -1;
	}

	return 1;
}

#define CMP_FIRST_ONE_IS_LESS ((int8_t)-1)
#define CMP_FIRST_ONE_IS_GREATER ((int8_t)1)
#define CMP_ARE_EQUAL ((int8_t)0)

NODISCARD BIGINT_C_LIB_EXPORTED int8_t
bigint_compare_bigint(BigIntC big_int1, BigIntC big_int2) { // NOLINT(misc-no-recursion)

	if(!big_int1.positive) {
		if(big_int2.positive) {
			// -a < +b
			return CMP_FIRST_ONE_IS_LESS;
		}

		//-a <=> -b ==  cmp_reverse (+a <=> +b)

		big_int1.positive = true;
		big_int2.positive = true;
		return cmp_reverse(bigint_compare_bigint(big_int1, big_int2));
	}

	if(!big_int2.positive) {
		// +a > -b
		return CMP_FIRST_ONE_IS_GREATER;
	}

	// +x <=> +b, needs to be calculated

	if(big_int1.number_count < big_int2.number_count) {
		// only valid, if normalized (no leading zeros)
		return CMP_FIRST_ONE_IS_LESS;
	}

	if(big_int1.number_count > big_int2.number_count) {
		// only valid, if normalized (no leading zeros)
		return CMP_FIRST_ONE_IS_GREATER;
	}

	for(size_t i = big_int1.number_count; i != 0; --i) {
		const uint64_t num1 = big_int1.numbers[i - 1];
		const uint64_t num2 = big_int2.numbers[i - 1];

		if(num1 < num2) {
			return CMP_FIRST_ONE_IS_LESS;
		}

		if(num1 > num2) {
			return CMP_FIRST_ONE_IS_GREATER;
		}
	}

	return CMP_ARE_EQUAL;
}

BIGINT_C_LIB_EXPORTED void bigint_negate(BigIntC* big_int) {

	if(big_int == NULL) { // GCOVR_EXCL_BR_LINE (gcovr can't detect asserts)
		UNREACHABLE_WITH_MSG("passed in NULL pointer"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)

	if(bigint_helper_is_zero(*big_int)) {
		return;
	}

	big_int->positive = !big_int->positive;
}

typedef struct {
	const uint64_t* numbers;
	size_t number_count;
} BigIntSlice;

typedef struct {
	const uint64_t* numbers;
	size_t number_count;
} BigIntNullableSlice;

#define NULL_SLICE ((BigIntNullableSlice){ .numbers = NULL, .number_count = 0 })

NODISCARD static inline BigIntSlice bigint_slice_from_nullable(BigIntNullableSlice big_int) {

	union {
		BigIntSlice normal;
		BigIntNullableSlice nullable;
	} value = { .nullable = big_int };

	return value.normal;
}

NODISCARD static inline BigIntSlice bigint_slice_from_bigint(BigInt big_int) {
	return (BigIntSlice){ .numbers = big_int.numbers, .number_count = big_int.number_count };
}

static void bigint_mul_two_numbers_impl(uint64_t big_int1, uint64_t big_int2, uint64_t* low,
                                        uint64_t* high);

#if BIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION == 0

static void
bigint_mul_two_numbers_impl(uint64_t big_int1, uint64_t big_int2,
                            uint64_t* low, // NOLINT(bugprone-easily-swappable-parameters)
                            uint64_t* high) {

	uint128_t result = (uint128_t)big_int1 * (uint128_t)big_int2;

	*low = (uint64_t)result;
	*high =
	    (uint64_t)(result >>
	               64); // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
}

#else

#if defined(_MSC_VER) && (defined(_M_X64) || defined(__x86_64__) || defined(__amd64__))

// use fast intrinsic on x86_64 (_umul128 is only supported in msvc, as gcc / clang and linux
// support all operations on 128 bits numbers, but that is not enabled with
// BIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION == 1)

#include <intrin.h>

static void bigint_mul_two_numbers_impl(uint64_t big_int1, uint64_t big_int2, uint64_t* low,
                                        uint64_t* high) {

	// see https://learn.microsoft.com/en-us/cpp/intrinsics/umul128?view=msvc-170
	*low = _umul128(big_int1, big_int2, high);
}

#elif defined(_MSC_VER) && (defined(__aarch64__))

// use fast intrinsic on aarch64 (__umulh is only supported in msvc, as gcc / clang and linux
// support all operations on 128 bits numbers, but that is not enabled with
// BIGINT_C_UNDERLYING_COMPUTATION_IMPLEMENTATION == 1)

#include <intrin.h>

static void bigint_mul_two_numbers_impl(uint64_t big_int1, uint64_t big_int2, uint64_t* low,
                                        uint64_t* high) {

	*low = (uint64_t)(big_int1 * big_int2);

	*high = __umulh(big_int1, big_int2);
}

#else

static void bigint_mul_two_numbers_impl(uint64_t big_int1, uint64_t big_int2, uint64_t* low,
                                        uint64_t* high) {

	uint64_t b1_low = (uint32_t)(big_int1);
	uint64_t b1_high = big_int1 >> 32;
	uint64_t b2_low = (uint32_t)(big_int2);
	uint64_t b2_high = big_int2 >> 32;

	uint64_t res_ll = b1_low * b2_low;
	uint64_t res_lh = b1_low * b2_high;
	uint64_t res_hl = b1_high * b2_low;
	uint64_t res_hh = b1_high * b2_high;

	uint64_t carry = ((res_ll >> 32) + (res_lh & 0xFFFFFFFF) + (res_hl & 0xFFFFFFFF)) >> 32;

	*low = res_ll + (res_lh << 32) + (res_hl << 32);
	*high = res_hh + (res_lh >> 32) + (res_hl >> 32) + carry;
}
#endif
#endif

NODISCARD static BigInt bigint_mul_two_numbers_normal(uint64_t big_int1, uint64_t big_int2) {

	uint64_t low = U64(0);
	uint64_t high = U64(0);

	bigint_mul_two_numbers_impl(big_int1, big_int2, &low, &high);

	bool need_two_numbers = high != 0;

	BigInt result_big_int = { .positive = true,
		                      .numbers = NULL,
		                      .number_count = need_two_numbers ? 2 : 1 };

	bigint_helper_realloc_to_new_size(&result_big_int);

	result_big_int.numbers[0] = low;

	if(need_two_numbers) {
		result_big_int.numbers[1] = high;
	}

	return result_big_int;
}

NODISCARD static inline BigInt bigint_mul_bigint_karatsuba_base(uint64_t big_int1,
                                                                uint64_t big_int2) {
	return bigint_mul_two_numbers_normal(big_int1, big_int2);
}

typedef struct {
	uint64_t div;
	uint64_t mod;
} DivModU64;

NODISCARD static DivModU64 helper_div_mod_u64_impl(uint64_t dividend, uint64_t divisor);

#if defined(__GNUC__)

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)

NODISCARD static DivModU64 helper_div_mod_u64_impl(uint64_t dividend, uint64_t divisor) {
	DivModU64 res = {};

	// On x86-64: DIV r/m64 divides RDX:RAX by the operand
	//   Quotient  -> RAX
	//   Remainder -> RDX
	__asm__("xor %%rdx, %%rdx\n\t"              // Clear RDX for 128-bit dividend (RDX:RAX)
	        "divq %[dvs]\n\t"                   // Divide RDX:RAX by divisor (unsigned)
	        : "=a"(res.div), "=d"(res.mod)      // Outputs: RAX -> res.div, RDX -> res.mod
	        : "a"(dividend), [dvs] "r"(divisor) // Inputs: RAX=dividend, operand=divisor
	        : "cc"                              // Clobbers: condition codes
	);

	return res;
}

#else
NODISCARD static DivModU64 helper_div_mod_u64_impl(uint64_t dividend, uint64_t divisor) {
	DivModU64 res = {};
	res.div = dividend / divisor;
	res.mod = dividend % divisor;

	return res;
}

#endif

#else
NODISCARD static DivModU64 helper_div_mod_u64_impl(uint64_t dividend, uint64_t divisor) {

	DivModU64 res = {};

	res.div = dividend / divisor;
	res.mod = dividend % divisor;

	return res;
}
#endif

NODISCARD static inline uint64_t helper_ceil_div(uint64_t dividend, uint64_t divisor) {

	DivModU64 res = helper_div_mod_u64_impl(dividend, divisor);

	return res.div + ((res.mod != 0) ? 1 : 0);
}

// NOTES: about "fake" 0 bigints:
//  to make less memory allocations, we use bigint slices, that have NULL as numbers and count as 0,
//  this represent 0, normally we would represent 0 by the numbers_count 0 and the first number
//  being 0 to make this work, we need special checks in some places, where this could be used!
// this are called "<x>_internal"

NODISCARD static inline bool bigint_mul_karatsuba_is_zero_slice(BigIntNullableSlice slice) {
	return slice.number_count == 0 || slice.numbers == NULL;
}

NODISCARD static inline BigInt bigint_helper_copy_of_slice(BigIntSlice big_int_slice,
                                                           bool positive) {

	BigInt big_int = { .positive = positive,
		               .numbers = (uint64_t*)big_int_slice.numbers,
		               .number_count = big_int_slice.number_count };

	return bigint_helper_get_full_copy(big_int);
}

NODISCARD static inline BigInt bigint_helper_as_ref_bigint(BigIntSlice big_int_slice,
                                                           bool positive) {

	BigInt big_int = { .positive = positive,
		               .numbers = (uint64_t*)big_int_slice.numbers,
		               .number_count = big_int_slice.number_count };

	return big_int;
}

NODISCARD static BigInt bigint_mul_bigint_karatsuba(BigIntSlice big_int1, BigIntSlice big_int2);

NODISCARD static inline BigInt
bigint_mul_bigint_karatsuba_internal(BigIntNullableSlice big_int1, // NOLINT(misc-no-recursion)
                                     BigIntNullableSlice big_int2) {

	if(bigint_mul_karatsuba_is_zero_slice(big_int1)) {
		return bigint_helper_zero();
	}

	if(bigint_mul_karatsuba_is_zero_slice(big_int2)) {
		return bigint_helper_zero();
	}

	return bigint_mul_bigint_karatsuba(bigint_slice_from_nullable(big_int1),
	                                   bigint_slice_from_nullable(big_int2));
}

NODISCARD static inline BigInt
bigint_mul_bigint_karatsuba_add_internal(BigIntNullableSlice big_int1, BigIntSlice big_int2)

{

	if(bigint_mul_karatsuba_is_zero_slice(big_int1)) {

		// 0 + +b = +b
		return bigint_helper_copy_of_slice(big_int2, true);
	}

	// +a + +b

	BigInt number_a = bigint_helper_copy_of_slice(bigint_slice_from_nullable(big_int1), true);
	BigInt number_b = bigint_helper_copy_of_slice(big_int2, true);

	BigInt result = bigint_add_bigint_both_positive(number_a, number_b);

	free_bigint_without_reset(number_a);
	free_bigint_without_reset(number_b);

	return result;
}

// this adds <amount> 0 numbers to the end of the number, also known as <big_int> * (2^64)^<amount>
static void bigint_mul_bigint_karatsuba_shift_bigint_numbers_internally_by(BigInt* big_int,
                                                                           size_t amount) {

	if(amount == 0) { // GCOVR_EXCL_BR_LINE (no caller uses the 0 here)
		return;       // GCOVR_EXCL_LINE (see above)
	}

	size_t old_size = big_int->number_count;

	big_int->number_count = old_size + amount;
	bigint_helper_realloc_to_new_size(big_int);

	// move the old numbers to the right (inverse as in normal numbers), starting from the right, so
	// this can be done in one swoop, all leftover numbers are set to 0
	for(size_t i = big_int->number_count; i != 0; --i) {

		if(i > amount) {
			uint64_t number_to_move = big_int->numbers[i - amount - 1];
			big_int->numbers[i - 1] = number_to_move;
		} else {
			big_int->numbers[i - 1] = U64(0);
		}
	}
}

NODISCARD static inline bool bigint_helper_is_power_of_2_unsigned(uint64_t value) {
	return value != 0 && (value & (value - 1)) == 0;
}

NODISCARD static bool bigint_helper_is_power_of_2(BigIntSlice big_int) {

	for(size_t i = big_int.number_count; i != 0; --i) {
		if(i == big_int.number_count) {
			if(!bigint_helper_is_power_of_2_unsigned(big_int.numbers[i - 1])) {
				return false;
			}
		} else {
			if(big_int.numbers[i - 1] != 0) {
				return false;
			}
		}
	}
	return true;
}

/**
 * @brief assumes that bigint_helper_is_power_of_2 returned true
 *
 * @param big_int
 * @return
 */
NODISCARD static uint64_t bigint_helper_get_power_of_2(BigIntSlice big_int) {

	size_t start_amount = (big_int.number_count - 1);

	size_t head_amount =
	    bigint_helper_bits_of_number_used(big_int.numbers[big_int.number_count - 1]);

	ASSERT(head_amount != 0, "precondition of bigint_helper_is_power_of_2 not met!");

	{ // fast addition of BigInt and and amount, that is likely never bigger than uint64_t::max
		if(helper_ceil_div(UINT64_MAX, BIGINT_BIT_COUNT) > start_amount) {
			size_t amount = start_amount * BIGINT_BIT_COUNT;

			if(UINT64_MAX - amount >= head_amount) {
				amount = amount + (head_amount - 1);
				return amount;
			}
		}
	}

	UNREACHABLE_WITH_MSG("Can't bit shift larger amounts, so this fast path fails here, it should "
	                     "never come to that, as TB of RAM is needed for that!");
}

NODISCARD static inline BigInt bigint_mul_bigint_both_positive(BigInt big_int1, BigInt big_int2);

NODISCARD static BigInt
bigint_mul_bigint_karatsuba(BigIntSlice big_int1, // NOLINT(misc-no-recursion)
                            BigIntSlice big_int2) {

	{ // check for simple bases cases e.g. * 0 or * 1

		if(big_int1.number_count == 1) {

			uint64_t number = big_int1.numbers[0];

			if(number == 0) {
				return bigint_helper_zero();
			}

			if(number == 1) {
				return bigint_helper_copy_of_slice(big_int2, true);
			}
		}

		if(big_int2.number_count == 1) {

			uint64_t number = big_int2.numbers[0];

			if(number == 0) {
				return bigint_helper_zero();
			}

			if(number == 1) {
				return bigint_helper_copy_of_slice(big_int1, true);
			}
		}
	}

	// basic algorihtm

	// this is a divide and conquer algorithm based on
	// https://en.wikipedia.org/wiki/Karatsuba_algorithm

	// base case
	if(big_int1.number_count == 1 && big_int2.number_count == 1) {
		return bigint_mul_bigint_karatsuba_base(big_int1.numbers[0], big_int2.numbers[0]);
	}

	{ // check for another simple base case  * 2**x, do that after the base case detection, as that
	  // is a faster path

		if(bigint_helper_is_power_of_2(big_int1)) {

			BigIntC copy_of_big_int2 = bigint_helper_copy_of_slice(big_int2, true);

			uint64_t amount = bigint_helper_get_power_of_2(big_int1);

			bigint_shift_left(&copy_of_big_int2, amount);
			return copy_of_big_int2;
		}

		if(bigint_helper_is_power_of_2(big_int2)) {

			BigIntC copy_of_big_int1 = bigint_helper_copy_of_slice(big_int1, true);

			uint64_t amount = bigint_helper_get_power_of_2(big_int2);

			bigint_shift_left(&copy_of_big_int1, amount);
			return copy_of_big_int1;
		}
	}

	// recursive case

	{
		size_t max_count = helper_max(big_int1.number_count, big_int2.number_count);

		size_t divide_at = helper_ceil_div(max_count, 2);

		// get the 4 parts of the numbers, the first part can be NULL, as e.g. one can be smaller as
		// the divide_at
		// NOTE: PAY ATTENTION to the order, as the uint64_t values are stored in reverse order! but
		// a1 is msb and a2 lsb

		BigIntNullableSlice num_a1 = NULL_SLICE;
		BigIntSlice num_a2 = { .numbers = big_int1.numbers, .number_count = 0 };

		if(big_int1.number_count > divide_at) {
			num_a1 = (BigIntNullableSlice){ .numbers = big_int1.numbers + divide_at,
				                            .number_count = big_int1.number_count - divide_at };
			num_a2.number_count = divide_at;

		} else {
			num_a2.number_count = big_int1.number_count;
		}

		BigIntNullableSlice num_b1 = NULL_SLICE;
		BigIntSlice num_b2 = { .numbers = big_int2.numbers, .number_count = 0 };

		if(big_int2.number_count > divide_at) {
			num_b1 = (BigIntNullableSlice){ .numbers = big_int2.numbers + divide_at,
				                            .number_count = big_int2.number_count - divide_at };
			num_b2.number_count = divide_at;

		} else {
			num_b2.number_count = big_int2.number_count;
		}

		// do the necessary steps, use internal algorithm, where we could pass "fake" 0 bigint
		// slices, see above what "fake" means

		BigInt z_2 = bigint_mul_bigint_karatsuba_internal(num_a1, num_b1);

		const BigInt z_0 = bigint_mul_bigint_karatsuba(num_a2, num_b2);

		const BigInt z_1_temp1 = bigint_mul_bigint_karatsuba_add_internal(num_a1, num_a2);

		const BigInt z_1_temp2 = bigint_mul_bigint_karatsuba_add_internal(num_b1, num_b2);

		const BigInt z_1_mul_temp = bigint_mul_bigint_both_positive(z_1_temp1, z_1_temp2);

		free_bigint_without_reset(z_1_temp1);
		free_bigint_without_reset(z_1_temp2);

		const BigInt z_1_sub_temp = bigint_sub_bigint_both_positive(z_1_mul_temp, z_2);
		ASSERT(z_1_sub_temp.positive, "result of this subtraction should always be positive!");

		BigInt z_1 = bigint_sub_bigint_both_positive(z_1_sub_temp, z_0);
		ASSERT(z_1.positive, "result of this subtraction should always be positive!");

		// these two asserts should always hold since:
		// (a1 + a2) * (b1 + b2) > (a1 * b1) + (a2 * b2)
		// (a1 * b1) + (a1 * b2) + (a2 * b1) + (a2 * b2) > (a1 * b1) + (a2 * b2)
		// (a1 * b2) + (a2 * b1) > 0

		free_bigint_without_reset(z_1_mul_temp);
		free_bigint_without_reset(z_1_sub_temp);

		// make the final number

		bigint_mul_bigint_karatsuba_shift_bigint_numbers_internally_by(&z_2, divide_at * 2);

		bigint_mul_bigint_karatsuba_shift_bigint_numbers_internally_by(&z_1, divide_at);

		const BigInt result_add_temp = bigint_add_bigint_both_positive(z_2, z_1);

		free_bigint_without_reset(z_2);
		free_bigint_without_reset(z_1);

		BigInt result = bigint_add_bigint_both_positive(result_add_temp, z_0);

		free_bigint_without_reset(result_add_temp);
		free_bigint_without_reset(z_0);

		bigint_helper_remove_leading_zeroes_but_not_normalize(&result);

		return result;
	}
}

NODISCARD static inline BigInt
bigint_mul_bigint_both_positive(BigInt big_int1, BigInt big_int2) { // NOLINT(misc-no-recursion)

	return bigint_mul_bigint_karatsuba(bigint_slice_from_bigint(big_int1),
	                                   bigint_slice_from_bigint(big_int2));
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_mul_bigint(BigIntC big_int1, BigIntC big_int2) {

	if(big_int1.positive) {
		if(big_int2.positive) {
			// +a * +b
			return bigint_mul_bigint_both_positive(big_int1, big_int2);
		}

		// +a * -b = - (+a * +b)
		big_int2.positive = true;
		BigInt result = bigint_mul_bigint_both_positive(big_int1, big_int2);

		result.positive = false;

		// - 0 becomes +0
		if(bigint_helper_is_zero(result)) {
			result.positive = true;
		}

		return result;
	}

	if(big_int2.positive) {
		// -a * +b = - (+b * +a)
		big_int1.positive = true;
		BigInt result = bigint_mul_bigint_both_positive(big_int1, big_int2);

		result.positive = false;

		// - 0 becomes +0
		if(bigint_helper_is_zero(result)) {
			result.positive = true;
		}

		return result;
	}

	// both are negative

	// -a * -b = + ( +a * +b )

	big_int1.positive = true;
	big_int2.positive = true;

	return bigint_mul_bigint_both_positive(big_int1, big_int2);
}

static void bigint_helper_shift_right_impl(BigIntC* big_int, uint64_t amount) {

	if(amount == 0) {
		return;
	}

	if(amount >= BIGINT_BIT_COUNT) {
		size_t removed_parts_count = amount / BIGINT_BIT_COUNT;
		amount = amount % BIGINT_BIT_COUNT;

		// make a +0, when the shift is large enough!
		if(removed_parts_count >= big_int->number_count) {
			big_int->number_count = 1;
			big_int->positive = true;
			bigint_helper_realloc_to_new_size(big_int);
			big_int->numbers[0] = U64(0);
			return;
		}

		// move the numbers and remove the rest later
		for(size_t i = 0; i < big_int->number_count - removed_parts_count; ++i) {
			big_int->numbers[i] = big_int->numbers[removed_parts_count + i];
		}
		big_int->number_count = big_int->number_count - removed_parts_count;
		bigint_helper_realloc_to_new_size(big_int);
	}

	ASSERT(amount < BIGINT_BIT_COUNT, "implementation error");

	// shift each limb separate, pay attention to the order of the limbs (LSB)
	for(size_t i = 0; i < big_int->number_count; ++i) {

		uint64_t* restrict number = &(big_int->numbers[i]);
		// first shift the current limb by amount
		*number = *number >> amount;

		// than get the bits of the last number and add it to the number
		if(i + 1 < big_int->number_count) {
			const uint64_t value = big_int->numbers[i + 1];
			const uint64_t last_bits = value & ((U64(1) << amount) - 1);
			if(last_bits != 0) {
				*number = *number | (last_bits << (BIGINT_BIT_COUNT - amount));
			}
		}
	}

	bigint_helper_normalize(big_int);
}

BIGINT_C_LIB_EXPORTED void bigint_shift_right(BigIntC* big_int, uint64_t amount) {
	if(big_int == NULL) { // GCOVR_EXCL_BR_LINE (gcovr can't detect asserts)
		UNREACHABLE_WITH_MSG("passed in NULL pointer"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)

	if(bigint_helper_is_zero(*big_int)) {
		return;
	}

	bigint_helper_shift_right_impl(big_int, amount);
}

static void bigint_helper_shift_left_impl(BigIntC* big_int, uint64_t amount) {

	if(amount == 0) {
		return;
	}

	if(amount >= BIGINT_BIT_COUNT) {
		const size_t newly_needed_parts = helper_ceil_div(amount, BIGINT_BIT_COUNT);
		amount = amount % BIGINT_BIT_COUNT;

		big_int->number_count = big_int->number_count + newly_needed_parts;
		bigint_helper_realloc_to_new_size(big_int);

		ASSERT(newly_needed_parts != 0,
		       "unreachable, as by impl of ceil div and condition amount >= 64");
		const size_t move_by_amount = newly_needed_parts - 1;

		// move the numbers and fill the rest with 0s
		for(size_t i = big_int->number_count; i != 0; --i) {

			if(i <= move_by_amount) {
				big_int->numbers[i - 1] = U64(0);
			} else if(i == big_int->number_count) {
				big_int->numbers[i - 1] = U64(0);
			} else {
				big_int->numbers[i - 1] = big_int->numbers[i - 1 - move_by_amount];
			}
		}
	}

	ASSERT(amount < BIGINT_BIT_COUNT, "implementation error");

	// Note: this is needed, as when the condition of amount >= 64 fails, we could need an over
	// allocation, otherwise the last number is always 0, so this does no harm either
	bool needs_new_digit =
	    bigint_helper_bits_of_number_used(big_int->numbers[big_int->number_count - 1]) >=
	    (BIGINT_BIT_COUNT + 1 - amount);

	if(needs_new_digit) {
		big_int->number_count++;
		bigint_helper_realloc_to_new_size(big_int);
		big_int->numbers[big_int->number_count - 1] = U64(0);
	}

	// shift each limb separate, pay attention to the order of the limbs (LSB)
	for(size_t i = big_int->number_count; i != 0; --i) {

		uint64_t* restrict number = &(big_int->numbers[i - 1]);

		// first shift the current limb by amount
		*number = *number << amount;

		// than get the bits of the last number and add it to the number
		if(i > 1) {
			const uint64_t value = big_int->numbers[i - 2];
			const uint64_t first_bits =
			    (value >> (BIGINT_BIT_COUNT - amount)) & ((U64(1) << amount) - 1);
			if(first_bits != 0) {
				*number = *number | first_bits;
			}
		}
	}

	bigint_helper_remove_leading_zeroes_but_not_normalize(big_int);
}

BIGINT_C_LIB_EXPORTED void bigint_shift_left(BigIntC* big_int, uint64_t amount) {
	if(big_int == NULL) { // GCOVR_EXCL_BR_LINE (gcovr can't detect asserts)
		UNREACHABLE_WITH_MSG("passed in NULL pointer"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)

	if(bigint_helper_is_zero(*big_int)) {
		return;
	}

	bigint_helper_shift_left_impl(big_int, amount);
}

#define DEFAULT_DIV_ROUNDING DivisionRoundingTowardsZero

#define DEFAULT_MOD_ROUNDING ModuloRoundingTruncated

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_div_bigint(BigIntC dividend, BigIntC divisor) {
	return bigint_div_bigint_advanced(dividend, divisor, DEFAULT_DIV_ROUNDING);
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_div_bigint_advanced(BigIntC dividend,
                                                                   BigIntC divisor,
                                                                   DivisionRounding rounding) {

	BigIntC out_div = {};

	bigint_div_mod_bigint_advanced(dividend, divisor, &out_div, NULL, rounding,
	                               DEFAULT_MOD_ROUNDING);

	return out_div;
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_mod_bigint(BigIntC dividend, BigIntC divisor) {
	return bigint_mod_bigint_advanced(dividend, divisor, DEFAULT_MOD_ROUNDING);
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_mod_bigint_advanced(BigIntC dividend,
                                                                   BigIntC divisor,
                                                                   ModuloRounding rounding) {

	BigIntC out_mod = {};

	bigint_div_mod_bigint_advanced(dividend, divisor, NULL, &out_mod, DEFAULT_DIV_ROUNDING,
	                               rounding);

	return out_mod;
}

BIGINT_C_LIB_EXPORTED void bigint_div_mod_bigint(BigIntC dividend, BigIntC divisor,
                                                 BigIntC* out_div, BigIntC* out_mod) {
	bigint_div_mod_bigint_advanced(dividend, divisor, out_div, out_mod, DEFAULT_DIV_ROUNDING,
	                               DEFAULT_MOD_ROUNDING);
}

static NO_RETURN void helper_raise_floating_point_exception(int exceptions) {
	if(feraiseexcept(exceptions) != 0) {
		// an error occurred while raising
		abort();
	}

	abort();
}

NODISCARD static BigInt bigint_helper_only_mod_positive_impl(const BigIntSlice dividend,
                                                             const BigIntSlice divisor) {

	{ // check for simple base case % 1

		if(divisor.number_count == 1) {

			uint64_t number = divisor.numbers[0];

			if(number == 1) {
				return bigint_helper_zero();
			}
		}
	}

	{ // check for other simple cases, e.g.  a == b or a < b

		const int8_t compared = bigint_compare_bigint(bigint_helper_as_ref_bigint(dividend, true),
		                                              bigint_helper_as_ref_bigint(divisor, true));

		if(compared == 0) {
			// a == b => result is zero
			return bigint_helper_zero();
		}

		if(compared < 0) {
			// if a < b => result is a

			return bigint_helper_copy_of_slice(dividend, true);
		}
	}

	// TODO: power of 2 optimization!

	{ // actual algorithm, using a bitshift  + subtraction algorithm

		BigInt shifted_divisor = bigint_helper_copy_of_slice(divisor, true);

		BigInt dividend_result = bigint_helper_copy_of_slice(dividend, true);

		// shift until shifted_divisor is > dividend_result
		while(true) {

			bigint_helper_shift_left_impl(&shifted_divisor, 1);

			const int8_t compared = bigint_compare_bigint(shifted_divisor, dividend_result);

			if(compared > 0) {
				break;
			}
		}

		// unshift one bit
		bigint_helper_shift_right_impl(&shifted_divisor, 1);

		// subtract shifted values from  dividend_result until it is <= shifted_divisor
		while(true) {

			{ // "inline" subtraction
				BigIntC new_dividend_result = bigint_sub_bigint(dividend_result, shifted_divisor);

				ASSERT(new_dividend_result.positive,
				       "implementation error: new_dividend_result should always be positive");
				free_bigint_without_reset(dividend_result);
				dividend_result = new_dividend_result;
			}

			{ // check if we got the result

				const int8_t compared = bigint_compare_bigint(
				    dividend_result, bigint_helper_as_ref_bigint(divisor, true));

				if(compared == 0) {
					free_bigint_without_reset(dividend_result);
					free_bigint_without_reset(shifted_divisor);

					// a == b => result is zero
					return bigint_helper_zero();
				}

				if(compared < 0) {
					free_bigint_without_reset(shifted_divisor);

					// if a < b => result is a

					return dividend_result;
				}
			}

			// make shifted_divisor <=  dividend_result
			while(true) {

				bigint_helper_shift_right_impl(&shifted_divisor, 1);

				const int8_t compared = bigint_compare_bigint(shifted_divisor, dividend_result);

				if(compared <= 0) {
					break;
				}
			}
		}
	}
}

NODISCARD static BigIntC bigint_helper_only_mod_impl(BigIntC dividend, BigIntC divisor,
                                                     ModuloRounding mod_rounding) {

	bool final_is_positive = true;
	bool result_needs_to_be_inverted = false;

	// see e.g. https://en.wikipedia.org/wiki/Modulo#Variants_of_the_definition
	switch(mod_rounding) {
		case ModuloRoundingTruncated: {
			// the defintions says, it is always the sign as the dividend
			final_is_positive = dividend.positive;
			break;
		}
		case ModuloRoundingFloored: {
			// the defintions says, it is always the sign as the divisor
			final_is_positive = divisor.positive;

			// if the signs are different, the result needs to be "inverted"
			if(dividend.positive != divisor.positive) {
				result_needs_to_be_inverted = true;
			}
			break;
		}
		case ModuloRoundingCeiled: {
			// the defintions says, it is always the opposite sign as the divisor
			final_is_positive = !divisor.positive;

			// if the signs are the same, the result needs to be "inverted"
			if(dividend.positive == divisor.positive) {
				result_needs_to_be_inverted = true;
			}
			break;
		}
		case ModuloRoundingEuclidean: {
			// the defintions says, it is always positive
			final_is_positive = true;
			// if the dividend is negative, the result needs to be "inverted"
			if(!dividend.positive) {
				result_needs_to_be_inverted = true;
			}
			break;
		}
		default: {
			helper_raise_floating_point_exception(FE_INVALID);
		}
	}

	// perform the operation, the final sign is
	// set by the switch case, we use the BigIntSlice, that has no sign to signify that both would
	// be positive

	{

		BigInt result = bigint_helper_only_mod_positive_impl(bigint_slice_from_bigint(dividend),
		                                                     bigint_slice_from_bigint(divisor));

		if(bigint_helper_is_zero(result)) {
			result.positive = true;
		} else {

			if(result_needs_to_be_inverted) {
				// the result gets "inverted", where inverted means the inversion in the mod class
				// respective to the + operation (e.g. 200 % 115 => 85 => inverted -> 30 = (115 -
				// 80) mod 115)

				// result has range (0, divisor), (both exclusive)
				// so result_inverted has_range [1,divisor-2], both (inclusive) or (0, divisor-1)
				// both exclusive), which is a valid range in the mod

				// make divisor positive, otherwise this makes no sense
				divisor.positive = true;

				BigInt result_inverted = bigint_sub_bigint(divisor, result);

				free_bigint_without_reset(result);
				result = result_inverted;
			}

			result.positive = final_is_positive;
		}

		return result;
	}
}

static void bigint_helper_only_div_impl(BigIntC dividend, BigIntC divisor, BigIntC* out_div,
                                        DivisionRounding div_rounding) {

	// TODO
	UNUSED(dividend);
	UNUSED(divisor);
	UNUSED(out_div);

	switch(div_rounding) {
		case DivisionRoundingFloor: {
		}
		case DivisionRoundingCeil: {
		}
		case DivisionRoundingTowardsZero: {
		}
		default: {
			helper_raise_floating_point_exception(FE_INVALID);
			return;
		}
	}
}

static void bigint_helper_div_mod_impl(BigIntC dividend, BigIntC divisor, BigIntC* out_div,
                                       BigIntC* out_mod, DivisionRounding div_rounding,
                                       ModuloRounding mod_rounding) {
	// TODO
	UNUSED(dividend);
	UNUSED(divisor);
	UNUSED(out_div);
	UNUSED(out_mod);
	UNUSED(div_rounding);
	UNUSED(mod_rounding);
}

BIGINT_C_LIB_EXPORTED
void bigint_div_mod_bigint_advanced(BigIntC dividend, BigIntC divisor, BigIntC* out_div,
                                    BigIntC* out_mod, DivisionRounding div_rounding,
                                    ModuloRounding mod_rounding) {

	if(bigint_helper_is_zero(divisor)) {
		helper_raise_floating_point_exception(FE_DIVBYZERO);
		return;
	}

	if(out_div == NULL) {
		if(out_mod == NULL) {
			// no need to compute anything
			return;
		}

		// only compute mod
		*out_mod = bigint_helper_only_mod_impl(dividend, divisor, mod_rounding);
		return;
	}

	if(out_mod == NULL) {
		// only compute div
		bigint_helper_only_div_impl(dividend, divisor, out_div, div_rounding);
		return;
	}

	bigint_helper_div_mod_impl(dividend, divisor, out_div, out_mod, div_rounding, mod_rounding);
	return;
}

// bitwise implementations

typedef enum {
	BitWiseOperationXOR,
	BitWiseOperationOR,
	BitWiseOperationAND,
} BitWiseOperation;

static void helper_bigint_bitwise_xor_generic_impl(size_t array_size,
                                                   const uint64_t* restrict const array1,
                                                   const uint64_t* restrict const array2,
                                                   uint64_t* restrict result_array) {

	for(size_t i = 0; i < array_size; ++i) {
		result_array[i] = array1[i] ^ array2[i];
	}
}

static void helper_bigint_bitwise_xor_same_generic_impl(size_t array_size,
                                                        const uint64_t* restrict const array1,
                                                        uint64_t* restrict result_array) {

	UNUSED(array1);
	memset(result_array, 0, array_size * sizeof(uint64_t));
}

static void helper_bigint_bitwise_or_generic_impl(size_t array_size,
                                                  const uint64_t* restrict const array1,
                                                  const uint64_t* restrict const array2,
                                                  uint64_t* restrict result_array) {

	for(size_t i = 0; i < array_size; ++i) {
		result_array[i] = array1[i] | array2[i];
	}
}

static void helper_bigint_bitwise_or_same_generic_impl(size_t array_size,
                                                       const uint64_t* restrict const array1,
                                                       uint64_t* restrict result_array) {

	memcpy(result_array, array1, array_size * sizeof(uint64_t));
}

static void helper_bigint_bitwise_and_generic_impl(size_t array_size,
                                                   const uint64_t* restrict const array1,
                                                   const uint64_t* restrict const array2,
                                                   uint64_t* restrict result_array) {

	for(size_t i = 0; i < array_size; ++i) {
		result_array[i] = array1[i] & array2[i];
	}
}

static void helper_bigint_bitwise_and_same_generic_impl(size_t array_size,
                                                        const uint64_t* restrict const array1,
                                                        uint64_t* restrict result_array) {

	memcpy(result_array, array1, array_size * sizeof(uint64_t));
}

#define COPY_BIGINT_TO_BIGGER_ARRAY(array, bigint, new_size, fill_with) \
	do { \
		memcpy(array, (bigint).numbers, (bigint).number_count * sizeof(uint64_t)); \
		memset((uint64_t*)(array) + (bigint).number_count, fill_with, \
		       ((new_size) - (bigint).number_count) * sizeof(uint64_t)); \
	} while(false)

#define MALLOC_UINT64_T_ARRAY_AND_FILL_REST_WITH_X(array, new_size, bigint, fill_with) \
	do { \
		uint64_t* new_array = (uint64_t*)malloc(sizeof(uint64_t) * (new_size)); \
		if(new_array == NULL) { \
			UNREACHABLE_WITH_MSG("malloc failed, no error handling implemented here"); \
		} \
		COPY_BIGINT_TO_BIGGER_ARRAY(new_array, bigint, new_size, fill_with); \
		(array) = new_array; \
	} while(false)

NODISCARD static BigIntC process_bitwise_operation_generic(BigIntC big_int1, BigIntC big_int2,
                                                           BitWiseOperation op, size_t max_size) {

	BigIntC result = { .positive = big_int1.positive, .numbers = NULL, .number_count = max_size };

	bigint_helper_realloc_to_new_size(&result);
	memset((void*)result.numbers, 0, max_size * sizeof(uint64_t));

	uint64_t* array1 = big_int1.numbers;

	if(big_int1.number_count != max_size) {
		MALLOC_UINT64_T_ARRAY_AND_FILL_REST_WITH_X(array1, max_size, big_int1, 0);
	}

	uint64_t* array2 = big_int2.numbers;

	if(big_int2.number_count != max_size) {
		MALLOC_UINT64_T_ARRAY_AND_FILL_REST_WITH_X(array2, max_size, big_int2, 0);
	}

	switch(op) {
		case BitWiseOperationXOR: {
			helper_bigint_bitwise_xor_generic_impl(max_size, array1, array2, result.numbers);
			break;
		}
		case BitWiseOperationOR: {
			helper_bigint_bitwise_or_generic_impl(max_size, array1, array2, result.numbers);
			break;
		}
		case BitWiseOperationAND: {
			helper_bigint_bitwise_and_generic_impl(max_size, array1, array2, result.numbers);
			break;
		}
		default: {
			UNREACHABLE_WITH_MSG("invalid bitwise operation");
		}
	}

	if(big_int1.number_count != max_size) {
		free(array1);
	}

	if(big_int2.number_count != max_size) {
		free(array2);
	}

	return result;
}

NODISCARD static BigIntC process_bitwise_operation_same_generic(BigIntC big_int,
                                                                BitWiseOperation op) {

	BigIntC result = { .positive = big_int.positive,
		               .numbers = NULL,
		               .number_count = big_int.number_count };

	bigint_helper_realloc_to_new_size(&result);
	memset((void*)result.numbers, 0, big_int.number_count * sizeof(uint64_t));

	uint64_t* array1 = big_int.numbers;

	switch(op) {
		case BitWiseOperationXOR: {
			helper_bigint_bitwise_xor_same_generic_impl(big_int.number_count, array1,
			                                            result.numbers);
			break;
		}
		case BitWiseOperationOR: {
			helper_bigint_bitwise_or_same_generic_impl(big_int.number_count, array1,
			                                           result.numbers);
			break;
		}
		case BitWiseOperationAND: {
			helper_bigint_bitwise_and_same_generic_impl(big_int.number_count, array1,
			                                            result.numbers);
			break;
		}
		default: {
			UNREACHABLE_WITH_MSG("invalid bitwise operation");
		}
	}

	return result;
}

// hardware accelerated code

#define MIN_HW_ACCEL_SIZE_MULT 2UL

#define SIZE_OF_UINT64_IN_BITS 64UL
#define BITS_BYTES_MULTIPLIER 8UL

#define UINT64_BYTE_AMOUNT (SIZE_OF_UINT64_IN_BITS / BITS_BYTES_MULTIPLIER)

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)

// sse2 defines
#define ALIGN_BYTES_OF_SSE2 16UL // 128 bits
#define BITS_AT_ONCE_SSE2 128UL

#define UINT64_AMOUNT_AT_ONCE_SSE2 (BITS_AT_ONCE_SSE2 / SIZE_OF_UINT64_IN_BITS)

// at least 2 uint64_t are needed for a sse2 usage, and if we nee to align it, it becomes the double
// (alias +1 at the start and +1 at the end)
#define MIN_SIZE_FOR_SSE2 ((UINT64_AMOUNT_AT_ONCE_SSE2) * MIN_HW_ACCEL_SIZE_MULT)

// avx2 defines
#define ALIGN_BYTES_OF_AVX2 32UL // 256 bits
#define BITS_AT_ONCE_AVX2 256UL

#define UINT64_AMOUNT_AT_ONCE_AVX2 (BITS_AT_ONCE_AVX2 / SIZE_OF_UINT64_IN_BITS)

#define MIN_SIZE_FOR_AVX2 ((UINT64_AMOUNT_AT_ONCE_AVX2) * MIN_HW_ACCEL_SIZE_MULT)

// avx512 (AVX512F) defines
#define ALIGN_BYTES_OF_AVX512 64UL // 512 bits
#define BITS_AT_ONCE_AVX512 512UL

#define UINT64_AMOUNT_AT_ONCE_AVX512 (BITS_AT_ONCE_AVX512 / SIZE_OF_UINT64_IN_BITS)

#define MIN_SIZE_FOR_AVX512 ((UINT64_AMOUNT_AT_ONCE_AVX512) * MIN_HW_ACCEL_SIZE_MULT)

// general defines
#define MIN_SIZE_FOR_HARDWARE_ACCEL MIN_SIZE_FOR_SSE2
#elif defined(__aarch64__)

// NEON defines
#define ALIGN_BYTES_OF_NEON 16UL // 128 bits
#define BITS_AT_ONCE_NEON 128UL

#define UINT64_AMOUNT_AT_ONCE_NEON (BITS_AT_ONCE_NEON / SIZE_OF_UINT64_IN_BITS)

#define MIN_SIZE_FOR_NEON ((UINT64_AMOUNT_AT_ONCE_NEON) * MIN_HW_ACCEL_SIZE_MULT)

// general defines
#define MIN_SIZE_FOR_HARDWARE_ACCEL MIN_SIZE_FOR_NEON

#elif defined(__riscv) && __riscv_xlen == 64
// RVV defines
#define BITS_AT_ONCE_RVV_MINIMAL 128UL // 128 - 1024 bits

#define UINT64_AMOUNT_AT_ONCE_RVV_MINIMAL (BITS_AT_ONCE_RVV_MINIMAL / SIZE_OF_UINT64_IN_BITS)

#define MIN_SIZE_FOR_RVV_SCALABLE ((UINT64_AMOUNT_AT_ONCE_RVV_MINIMAL) * MIN_HW_ACCEL_SIZE_MULT)

// general defines
#define MIN_SIZE_FOR_HARDWARE_ACCEL MIN_SIZE_FOR_RVV_SCALABLE
#endif

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)

#include <emmintrin.h> // SSE2 intrinsics
#include <immintrin.h> // many intrincs, also avx2 and avx512

#define USE_HARDWARE_ACCEL 1

#elif defined(__aarch64__)

#include <arm_neon.h> // NEON intrinsics
#include <arm_sve.h>  // SVE intrinsics

#define USE_HARDWARE_ACCEL 1

#elif defined(__riscv) && __riscv_xlen == 64

#include <riscv_vector.h> // RVV intrinsics

#define USE_HARDWARE_ACCEL 1

#endif

#if defined(USE_HARDWARE_ACCEL)

typedef enum {
	AlignedTheSameNone = 0x00,
	AlignedTheSameFirst = 0x01,
	AlignedTheSameSecond = 0x02,
	AlignedTheSameBoth = AlignedTheSameFirst | AlignedTheSameSecond
} AlignedTheSame;

NODISCARD static size_t helper_get_alignment_bytes_of(const void* const ptr,
                                                      size_t aligned_to_bytes) {

	return ((uintptr_t)ptr) % aligned_to_bytes;
}

static void helper_get_config_for_aligned_arrays(BigIntC big_int1, BigIntC big_int2,
                                                 size_t max_size, size_t aligned_to_bytes,
                                                 PARAMS_OUT AlignedTheSame* aligned_info,
                                                 PARAMS_OUT size_t* offset_bytes) {

	// note: this function returns, which alignment it used, and which of the two it used as a
	// reference, so that the other one is correctly aligned later on, this also takes into account
	// sizes, if it isn't aligned, it may also be aligned, but not big enough, but reallocs don't
	// assure alignment, if not used properly

	AlignedTheSame alignment_start = AlignedTheSameNone;

	{ // covers 4 cases
		if(big_int1.number_count != max_size) {
			if(big_int2.number_count != max_size) {
				UNREACHABLE_WITH_MSG("One of the has to have the max_size!");
			} else {
				// use alignment of second
				alignment_start = AlignedTheSameSecond;
			}

		} else {
			if(big_int2.number_count != max_size) {
				// use alignment of first
				alignment_start = AlignedTheSameFirst;
			} else {
				// use alignment of both, if both have the same one (e.g. NOT mod align = 0, vs 8)
				alignment_start = AlignedTheSameBoth;
			}
		}
	}

	switch(alignment_start) {
		case AlignedTheSameFirst: {

			const size_t aligned_bytes =
			    helper_get_alignment_bytes_of(big_int1.numbers, aligned_to_bytes);

			if((aligned_bytes % UINT64_BYTE_AMOUNT) == 0) {
				// clean alloc with 8 bytes (sizeof uint64_t) aligned, may not be aligned_to_bytes
				// bytes aligned
				*aligned_info = AlignedTheSameFirst;
				*offset_bytes = aligned_bytes;

			} else {
				// not clean alloc need to reallocate both values
				*aligned_info = AlignedTheSameNone;
				*offset_bytes = 0;
			}

			break;
		}
		case AlignedTheSameSecond: {
			const size_t aligned_bytes =
			    helper_get_alignment_bytes_of(big_int2.numbers, aligned_to_bytes);

			if((aligned_bytes % UINT64_BYTE_AMOUNT) == 0) {
				// clean alloc with 8 bytes (sizeof uint64_t) aligned, may not be aligned_to_bytes
				// bytes aligned
				*aligned_info = AlignedTheSameSecond;
				*offset_bytes = aligned_bytes;

			} else {
				// not clean alloc need to reallocate both values
				*aligned_info = AlignedTheSameNone;
				*offset_bytes = 0;
			}

			break;
		}
		case AlignedTheSameBoth: {
			const size_t aligned_bytes1 =
			    helper_get_alignment_bytes_of(big_int2.numbers, aligned_to_bytes);

			const size_t aligned_bytes2 =
			    helper_get_alignment_bytes_of(big_int2.numbers, aligned_to_bytes);

			if(aligned_bytes1 == aligned_bytes2) {
				// both are aligned the same, now just check, if that is a multiple of 8
				if((aligned_bytes1 % UINT64_BYTE_AMOUNT) == 0) {
					// clean alloc with 8 bytes (sizeof uint64_t) aligned, may not be
					// aligned_to_bytes bytes aligned
					*aligned_info = AlignedTheSameBoth;
					*offset_bytes = aligned_bytes1;

				} else {
					// not clean alloc need to reallocate both values
					*aligned_info = AlignedTheSameNone;
					*offset_bytes = 0;
				}
			} else {
				// use the one, that is "better aligned"
				if(aligned_bytes1 == 0) {
					// first is better as it is directly aligned
					*aligned_info = AlignedTheSameFirst;
					*offset_bytes = aligned_bytes1;

				} else if(aligned_bytes2 == 0) {
					// second is better as it is directly aligned
					*aligned_info = AlignedTheSameSecond;
					*offset_bytes = aligned_bytes1;

				} else if((aligned_bytes1 % UINT64_BYTE_AMOUNT) == 0) {
					// first is better as it is aligned at least to some bytes
					*aligned_info = AlignedTheSameFirst;
					*offset_bytes = aligned_bytes1;

				} else if((aligned_bytes2 % UINT64_BYTE_AMOUNT) == 0) {
					// second is better as it is aligned at least to some bytes
					*aligned_info = AlignedTheSameSecond;
					*offset_bytes = aligned_bytes1;

				} else {
					// not clean alloc need to reallocate both values
					*aligned_info = AlignedTheSameNone;
					*offset_bytes = 0;
				}
			}

			break;
		}
		case AlignedTheSameNone:
		default: {

			UNREACHABLE_WITH_MSG("logically not reachable, implementation error");
		}
	}
}

NODISCARD static void* helper_alloc_aligned_with_offset(void** result, size_t size,
                                                        size_t align_bytes, size_t offset_bytes) {

	if(offset_bytes == 0) {

		void* aligned_ptr = aligned_alloc(align_bytes, size);

		if(aligned_ptr == NULL) { // GCOVR_EXCL_BR_LINE (OOM)
			UNREACHABLE_WITH_MSG( // GCOVR_EXCL_LINE (OOM content)
			    "aligned_alloc failed, no error handling implemented here");
		} // GCOVR_EXCL_LINE (OOM content)

		*result = aligned_ptr;
		return aligned_ptr;
	}

	// allocate and return an offset pointer

	void* aligned_ptr = aligned_alloc(align_bytes, size + offset_bytes);

	if(aligned_ptr == NULL) { // GCOVR_EXCL_BR_LINE (OOM)
		UNREACHABLE_WITH_MSG( // GCOVR_EXCL_LINE (OOM content)
		    "aligned_alloc failed, no error handling implemented here");
	} // GCOVR_EXCL_LINE (OOM content)

	*result = (void*)(((uint8_t*)aligned_ptr) + offset_bytes);
	return aligned_ptr;
}

#endif // defined(USE_HARDWARE_ACCEL)

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)

// See: https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html
//  for intel intrinsics with sse2, avx2 and avx512F

CPU_TARGET(sse2)
static void helper_bigint_bitwise_xor_hardware_accelerated_amd64_sse2_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes) {

	size_t i = 0;
	size_t simd_width = UINT64_AMOUNT_AT_ONCE_SSE2;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] ^ array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		__m128i array1_sse2 = _mm_load_si128((const __m128i*)&(array1[i]));
		__m128i array2_sse2 = _mm_load_si128((const __m128i*)&(array2[i]));
		__m128i result_sse2 = _mm_xor_si128(array1_sse2, array2_sse2);
		_mm_store_si128((__m128i*)&(result_array[i]), result_sse2);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] ^ array2[i];
	}
}

CPU_TARGET(sse2)
static void helper_bigint_bitwise_or_hardware_accelerated_amd64_sse2_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes) {

	size_t i = 0;
	size_t simd_width = UINT64_AMOUNT_AT_ONCE_SSE2;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] | array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		__m128i array1_sse2 = _mm_load_si128((const __m128i*)&(array1[i]));
		__m128i array2_sse2 = _mm_load_si128((const __m128i*)&(array2[i]));
		__m128i result_sse2 = _mm_or_si128(array1_sse2, array2_sse2);
		_mm_store_si128((__m128i*)&(result_array[i]), result_sse2);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] | array2[i];
	}
}

CPU_TARGET(sse2)
static void helper_bigint_bitwise_and_hardware_accelerated_amd64_sse2_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes) {
	size_t i = 0;
	size_t simd_width = UINT64_AMOUNT_AT_ONCE_SSE2;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] & array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		__m128i array1_sse2 = _mm_load_si128((const __m128i*)&(array1[i]));
		__m128i array2_sse2 = _mm_load_si128((const __m128i*)&(array2[i]));
		__m128i result_sse2 = _mm_and_si128(array1_sse2, array2_sse2);
		_mm_store_si128((__m128i*)&(result_array[i]), result_sse2);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] & array2[i];
	}
}

NODISCARD static BigIntC CPU_TARGET(sse2)
    process_bitwise_operation_hardware_accelerated_amd64_sse2(BigIntC big_int1, BigIntC big_int2,
                                                              BitWiseOperation op,
                                                              size_t max_size) {

	AlignedTheSame aligned_info = AlignedTheSameNone;
	size_t offset_bytes = 0;
	helper_get_config_for_aligned_arrays(big_int1, big_int2, max_size, ALIGN_BYTES_OF_SSE2,
	                                     &aligned_info, &offset_bytes);

	uint64_t* array1 = big_int1.numbers;
	uint64_t* array2 = big_int2.numbers;

	void* array1_real_alloc_ptr = NULL;
	void* array2_real_alloc_ptr = NULL;

	switch(aligned_info) {
		case AlignedTheSameNone: {
			array1_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array1, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_SSE2, offset_bytes);
			array2_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array2, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_SSE2, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array1, big_int1, max_size, 0);
			COPY_BIGINT_TO_BIGGER_ARRAY(array2, big_int2, max_size, 0);

			break;
		}
		case AlignedTheSameFirst: {
			array2_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array2, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_SSE2, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array2, big_int2, max_size, 0);

			break;
		}
		case AlignedTheSameSecond: {
			array1_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array1, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_SSE2, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array1, big_int1, max_size, 0);
			break;
		}
		case AlignedTheSameBoth: {
			// do nothing
			break;
		}
		default: {
			UNREACHABLE_WITH_MSG("invalid bitwise operation");
		}
	}

	BigIntC result = { .positive = big_int1.positive, .numbers = NULL, .number_count = max_size };

	void* real_result_allocation_ptr = helper_alloc_aligned_with_offset(
	    (void**)(&(result.numbers)), result.number_count * sizeof(uint64_t), ALIGN_BYTES_OF_SSE2,
	    offset_bytes);
	memset((void*)result.numbers, 0, max_size * sizeof(uint64_t));

	switch(op) {
		case BitWiseOperationXOR: {
			helper_bigint_bitwise_xor_hardware_accelerated_amd64_sse2_impl(
			    max_size, array1, array2, result.numbers, offset_bytes);
			break;
		}
		case BitWiseOperationOR: {
			helper_bigint_bitwise_or_hardware_accelerated_amd64_sse2_impl(
			    max_size, array1, array2, result.numbers, offset_bytes);
			break;
		}
		case BitWiseOperationAND: {
			helper_bigint_bitwise_and_hardware_accelerated_amd64_sse2_impl(
			    max_size, array1, array2, result.numbers, offset_bytes);
			break;
		}
		default: {
			UNREACHABLE_WITH_MSG("invalid bitwise operation");
		}
	}

	if(array1_real_alloc_ptr != NULL) {
		free(array1_real_alloc_ptr);
	}

	if(array2_real_alloc_ptr != NULL) {
		free(array2_real_alloc_ptr);
	}

	// reallocate the result numbers ptr, if it is not the same as the ptr, that can be freed, this
	// is needed, to keep the same alignment as the two input values
	if(real_result_allocation_ptr != result.numbers) {

		BigIntC new_result = bigint_helper_get_full_copy(result);

		free(real_result_allocation_ptr);

		result = new_result;
	}

	return result;
}

CPU_TARGET(avx2)
static void helper_bigint_bitwise_xor_hardware_accelerated_amd64_avx2_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes) {

	size_t i = 0;
	size_t simd_width = UINT64_AMOUNT_AT_ONCE_AVX2;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] ^ array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		__m256i array1_avx2 = _mm256_load_si256((const __m256i*)&(array1[i]));
		__m256i array2_avx2 = _mm256_load_si256((const __m256i*)&(array2[i]));
		__m256i result_avx2 = _mm256_xor_si256(array1_avx2, array2_avx2);
		_mm256_store_si256((__m256i*)&(result_array[i]), result_avx2);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] ^ array2[i];
	}
}

CPU_TARGET(avx2)
static void helper_bigint_bitwise_or_hardware_accelerated_amd64_avx2_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes) {

	size_t i = 0;
	size_t simd_width = UINT64_AMOUNT_AT_ONCE_AVX2;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] | array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		__m256i array1_avx2 = _mm256_load_si256((const __m256i*)&(array1[i]));
		__m256i array2_avx2 = _mm256_load_si256((const __m256i*)&(array2[i]));
		__m256i result_avx2 = _mm256_or_si256(array1_avx2, array2_avx2);
		_mm256_store_si256((__m256i*)&(result_array[i]), result_avx2);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] | array2[i];
	}
}

CPU_TARGET(avx2)
static void helper_bigint_bitwise_and_hardware_accelerated_amd64_avx2_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes) {
	size_t i = 0;
	size_t simd_width = UINT64_AMOUNT_AT_ONCE_AVX2;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] & array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		__m256i array1_avx2 = _mm256_load_si256((const __m256i*)&(array1[i]));
		__m256i array2_avx2 = _mm256_load_si256((const __m256i*)&(array2[i]));
		__m256i result_avx2 = _mm256_and_si256(array1_avx2, array2_avx2);
		_mm256_store_si256((__m256i*)&(result_array[i]), result_avx2);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] & array2[i];
	}
}

NODISCARD static BigIntC CPU_TARGET(avx2)
    process_bitwise_operation_hardware_accelerated_amd64_avx2(BigIntC big_int1, BigIntC big_int2,
                                                              BitWiseOperation op,
                                                              size_t max_size) {
	AlignedTheSame aligned_info = AlignedTheSameNone;
	size_t offset_bytes = 0;
	helper_get_config_for_aligned_arrays(big_int1, big_int2, max_size, ALIGN_BYTES_OF_AVX2,
	                                     &aligned_info, &offset_bytes);

	uint64_t* array1 = big_int1.numbers;
	uint64_t* array2 = big_int2.numbers;

	void* array1_real_alloc_ptr = NULL;
	void* array2_real_alloc_ptr = NULL;

	switch(aligned_info) {
		case AlignedTheSameNone: {
			array1_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array1, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_AVX2, offset_bytes);
			array2_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array2, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_AVX2, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array1, big_int1, max_size, 0);
			COPY_BIGINT_TO_BIGGER_ARRAY(array2, big_int2, max_size, 0);

			break;
		}
		case AlignedTheSameFirst: {
			array2_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array2, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_AVX2, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array2, big_int2, max_size, 0);

			break;
		}
		case AlignedTheSameSecond: {
			array1_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array1, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_AVX2, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array1, big_int1, max_size, 0);
			break;
		}
		case AlignedTheSameBoth: {
			// do nothing
			break;
		}
		default: {
			UNREACHABLE_WITH_MSG("invalid bitwise operation");
		}
	}

	BigIntC result = { .positive = big_int1.positive, .numbers = NULL, .number_count = max_size };

	void* real_result_allocation_ptr = helper_alloc_aligned_with_offset(
	    (void**)(&(result.numbers)), result.number_count * sizeof(uint64_t), ALIGN_BYTES_OF_AVX2,
	    offset_bytes);
	memset((void*)result.numbers, 0, max_size * sizeof(uint64_t));

	switch(op) {
		case BitWiseOperationXOR: {
			helper_bigint_bitwise_xor_hardware_accelerated_amd64_avx2_impl(
			    max_size, array1, array2, result.numbers, offset_bytes);
			break;
		}
		case BitWiseOperationOR: {
			helper_bigint_bitwise_or_hardware_accelerated_amd64_avx2_impl(
			    max_size, array1, array2, result.numbers, offset_bytes);
			break;
		}
		case BitWiseOperationAND: {
			helper_bigint_bitwise_and_hardware_accelerated_amd64_avx2_impl(
			    max_size, array1, array2, result.numbers, offset_bytes);
			break;
		}
		default: {
			UNREACHABLE_WITH_MSG("invalid bitwise operation");
		}
	}

	if(array1_real_alloc_ptr != NULL) {
		free(array1_real_alloc_ptr);
	}

	if(array2_real_alloc_ptr != NULL) {
		free(array2_real_alloc_ptr);
	}

	// reallocate the result numbers ptr, if it is not the same as the ptr, that can be freed, this
	// is needed, to keep the same alignment as the two input values
	if(real_result_allocation_ptr != result.numbers) {

		BigIntC new_result = bigint_helper_get_full_copy(result);

		free(real_result_allocation_ptr);

		result = new_result;
	}

	return result;
}

CPU_TARGET(avx512f)
static void helper_bigint_bitwise_xor_hardware_accelerated_amd64_avx512_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes) {

	size_t i = 0;
	size_t simd_width = UINT64_AMOUNT_AT_ONCE_AVX512;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] ^ array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		__m512i array1_avx512 = _mm512_load_si512((const __m512i*)&(array1[i]));
		__m512i array2_avx512 = _mm512_load_si512((const __m512i*)&(array2[i]));
		__m512i result_avx512 = _mm512_xor_si512(array1_avx512, array2_avx512);
		_mm512_store_si512((__m512i*)&(result_array[i]), result_avx512);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] ^ array2[i];
	}
}

CPU_TARGET(avx512f)
static void helper_bigint_bitwise_or_hardware_accelerated_amd64_avx512_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes) {

	size_t i = 0;
	size_t simd_width = UINT64_AMOUNT_AT_ONCE_AVX512;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] | array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		__m512i array1_avx512 = _mm512_load_si512((const __m512i*)&(array1[i]));
		__m512i array2_avx512 = _mm512_load_si512((const __m512i*)&(array2[i]));
		__m512i result_avx512 = _mm512_or_si512(array1_avx512, array2_avx512);
		_mm512_store_si512((__m512i*)&(result_array[i]), result_avx512);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] | array2[i];
	}
}

CPU_TARGET(avx512f)
static void helper_bigint_bitwise_and_hardware_accelerated_amd64_avx512_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes) {
	size_t i = 0;
	size_t simd_width = UINT64_AMOUNT_AT_ONCE_AVX512;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] & array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		__m512i array1_avx512 = _mm512_load_si512((const __m512i*)&(array1[i]));
		__m512i array2_avx512 = _mm512_load_si512((const __m512i*)&(array2[i]));
		__m512i result_avx512 = _mm512_and_si512(array1_avx512, array2_avx512);
		_mm512_store_si512((__m512i*)&(result_array[i]), result_avx512);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] & array2[i];
	}
}

NODISCARD static BigIntC CPU_TARGET(avx512f)
    process_bitwise_operation_hardware_accelerated_amd64_avx512(BigIntC big_int1, BigIntC big_int2,
                                                                BitWiseOperation op,
                                                                size_t max_size) {
	AlignedTheSame aligned_info = AlignedTheSameNone;
	size_t offset_bytes = 0;
	helper_get_config_for_aligned_arrays(big_int1, big_int2, max_size, ALIGN_BYTES_OF_AVX512,
	                                     &aligned_info, &offset_bytes);

	uint64_t* array1 = big_int1.numbers;
	uint64_t* array2 = big_int2.numbers;

	void* array1_real_alloc_ptr = NULL;
	void* array2_real_alloc_ptr = NULL;

	switch(aligned_info) {
		case AlignedTheSameNone: {
			array1_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array1, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_AVX512, offset_bytes);
			array2_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array2, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_AVX512, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array1, big_int1, max_size, 0);
			COPY_BIGINT_TO_BIGGER_ARRAY(array2, big_int2, max_size, 0);

			break;
		}
		case AlignedTheSameFirst: {
			array2_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array2, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_AVX512, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array2, big_int2, max_size, 0);

			break;
		}
		case AlignedTheSameSecond: {
			array1_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array1, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_AVX512, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array1, big_int1, max_size, 0);
			break;
		}
		case AlignedTheSameBoth: {
			// do nothing
			break;
		}
		default: {
			UNREACHABLE_WITH_MSG("invalid bitwise operation");
		}
	}

	BigIntC result = { .positive = big_int1.positive, .numbers = NULL, .number_count = max_size };

	void* real_result_allocation_ptr = helper_alloc_aligned_with_offset(
	    (void**)(&(result.numbers)), result.number_count * sizeof(uint64_t), ALIGN_BYTES_OF_AVX512,
	    offset_bytes);
	memset((void*)result.numbers, 0, max_size * sizeof(uint64_t));

	switch(op) {
		case BitWiseOperationXOR: {
			helper_bigint_bitwise_xor_hardware_accelerated_amd64_avx512_impl(
			    max_size, array1, array2, result.numbers, offset_bytes);
			break;
		}
		case BitWiseOperationOR: {
			helper_bigint_bitwise_or_hardware_accelerated_amd64_avx512_impl(
			    max_size, array1, array2, result.numbers, offset_bytes);
			break;
		}
		case BitWiseOperationAND: {
			helper_bigint_bitwise_and_hardware_accelerated_amd64_avx512_impl(
			    max_size, array1, array2, result.numbers, offset_bytes);
			break;
		}
		default: {
			UNREACHABLE_WITH_MSG("invalid bitwise operation");
		}
	}

	if(array1_real_alloc_ptr != NULL) {
		free(array1_real_alloc_ptr);
	}

	if(array2_real_alloc_ptr != NULL) {
		free(array2_real_alloc_ptr);
	}

	// reallocate the result numbers ptr, if it is not the same as the ptr, that can be freed, this
	// is needed, to keep the same alignment as the two input values
	if(real_result_allocation_ptr != result.numbers) {

		BigIntC new_result = bigint_helper_get_full_copy(result);

		free(real_result_allocation_ptr);

		result = new_result;
	}

	return result;
}
#elif defined(__aarch64__)

#if defined(__GNUC__) && !defined(__clang__)

// use first possible neon compatible target, only using "+neon" could be unsafe, as the base arch
// could not support the neon extension, using "neon" doesn't work with gcc, also "+simd" is an
// alias for neon, and sometimes "+neon" doesn't work
#define CPU_TARGET_NEON __attribute__((target("arch=armv8-a+simd")))
#define CPU_TARGET_SVE __attribute__((target("arch=armv8-a+sve")))
// CPU_TARGET(+nothing+simd)
#else
#define CPU_TARGET_NEON CPU_TARGET(neon)
#define CPU_TARGET_SVE CPU_TARGET(sve)
#endif

// TODO. don't annotate funcions, which call these functions!
//  TODO. check, if the arm sve and neon instructions need aligned data or not.

// for neon intrinsics, see https://developer.arm.com/documentation/den0018/a/NEON-Intrinsics

CPU_TARGET_NEON
static void helper_bigint_bitwise_xor_hardware_accelerated_arm64_neon_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes) {

	size_t i = 0;
	size_t simd_width = UINT64_AMOUNT_AT_ONCE_NEON;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] ^ array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		uint64x2_t array1_neon = vld1q_u64(&(array1[i]));
		uint64x2_t array2_neon = vld1q_u64(&(array2[i]));
		uint64x2_t result_neon = veorq_u64(array1_neon, array2_neon);
		vst1q_u64(&(result_array[i]), result_neon);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] ^ array2[i];
	}
}

CPU_TARGET_NEON
static void helper_bigint_bitwise_or_hardware_accelerated_arm64_neon_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes) {

	size_t i = 0;
	size_t simd_width = UINT64_AMOUNT_AT_ONCE_NEON;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] | array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		uint64x2_t array1_neon = vld1q_u64(&(array1[i]));
		uint64x2_t array2_neon = vld1q_u64(&(array2[i]));
		uint64x2_t result_neon = vorrq_u64(array1_neon, array2_neon);
		vst1q_u64(&(result_array[i]), result_neon);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] | array2[i];
	}
}

CPU_TARGET_NEON
static void helper_bigint_bitwise_and_hardware_accelerated_arm64_neon_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes) {
	size_t i = 0;
	size_t simd_width = UINT64_AMOUNT_AT_ONCE_NEON;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] & array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		uint64x2_t array1_neon = vld1q_u64(&(array1[i]));
		uint64x2_t array2_neon = vld1q_u64(&(array2[i]));
		uint64x2_t result_neon = vandq_u64(array1_neon, array2_neon);
		vst1q_u64(&(result_array[i]), result_neon);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] & array2[i];
	}
}

CPU_TARGET_NEON
NODISCARD static BigIntC
process_bitwise_operation_hardware_accelerated_arm64_neon(BigIntC big_int1, BigIntC big_int2,
                                                          BitWiseOperation op, size_t max_size) {

	AlignedTheSame aligned_info = AlignedTheSameNone;
	size_t offset_bytes = 0;
	helper_get_config_for_aligned_arrays(big_int1, big_int2, max_size, ALIGN_BYTES_OF_NEON,
	                                     &aligned_info, &offset_bytes);

	uint64_t* array1 = big_int1.numbers;
	uint64_t* array2 = big_int2.numbers;

	void* array1_real_alloc_ptr = NULL;
	void* array2_real_alloc_ptr = NULL;

	switch(aligned_info) {
		case AlignedTheSameNone: {
			array1_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array1, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_NEON, offset_bytes);
			array2_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array2, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_NEON, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array1, big_int1, max_size, 0);
			COPY_BIGINT_TO_BIGGER_ARRAY(array2, big_int2, max_size, 0);

			break;
		}
		case AlignedTheSameFirst: {
			array2_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array2, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_NEON, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array2, big_int2, max_size, 0);

			break;
		}
		case AlignedTheSameSecond: {
			array1_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array1, max_size * sizeof(uint64_t), ALIGN_BYTES_OF_NEON, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array1, big_int1, max_size, 0);
			break;
		}
		case AlignedTheSameBoth: {
			// do nothing
			break;
		}
		default: {
			UNREACHABLE_WITH_MSG("invalid bitwise operation");
		}
	}

	BigIntC result = { .positive = big_int1.positive, .numbers = NULL, .number_count = max_size };

	void* real_result_allocation_ptr = helper_alloc_aligned_with_offset(
	    (void**)(&(result.numbers)), result.number_count * sizeof(uint64_t), ALIGN_BYTES_OF_NEON,
	    offset_bytes);
	memset((void*)result.numbers, 0, max_size * sizeof(uint64_t));

	switch(op) {
		case BitWiseOperationXOR: {
			helper_bigint_bitwise_xor_hardware_accelerated_arm64_neon_impl(
			    max_size, array1, array2, result.numbers, offset_bytes);
			break;
		}
		case BitWiseOperationOR: {
			helper_bigint_bitwise_or_hardware_accelerated_arm64_neon_impl(
			    max_size, array1, array2, result.numbers, offset_bytes);
			break;
		}
		case BitWiseOperationAND: {
			helper_bigint_bitwise_and_hardware_accelerated_arm64_neon_impl(
			    max_size, array1, array2, result.numbers, offset_bytes);
			break;
		}
		default: {
			UNREACHABLE_WITH_MSG("invalid bitwise operation");
		}
	}

	if(array1_real_alloc_ptr != NULL) {
		free(array1_real_alloc_ptr);
	}

	if(array2_real_alloc_ptr != NULL) {
		free(array2_real_alloc_ptr);
	}

	// reallocate the result numbers ptr, if it is not the same as the ptr, that can be freed, this
	// is needed, to keep the same alignment as the two input values
	if(real_result_allocation_ptr != result.numbers) {

		BigIntC new_result = bigint_helper_get_full_copy(result);

		free(real_result_allocation_ptr);

		result = new_result;
	}

	return result;
}

// note, sve is size independent, that means, it can have vector sizes of 128 up to 2048, neon uses
// 128 fixed (like sse2 on x86_64) and therefore we can't guarantee, that sve is 256 bits large, and
// maybe could use larger values, that is rather complicated, also the notion of sizeless types, VLA
// and lanes (which is the activation of "lanes" aka if there are 4 lanes, we have a vector size of
// 4*64)

// note: svbool_t is a sizeless bool array, that has a 1 (true) for each activated lane, so it e.g.
// has "1111" for 4 64 bit lanes active, this is the runtime way of using only available lanes

// for sve intrinsics, see https://developer.arm.com/Architectures/Scalable%20Vector%20Extensions

CPU_TARGET_SVE
static void helper_bigint_bitwise_xor_hardware_accelerated_arm64_sve_sizeless_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes, size_t sve_vector_length_in_u64,
    svbool_t predicate) {

	size_t i = 0;
	size_t simd_width = sve_vector_length_in_u64;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] ^ array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		svuint64_t array1_sve = svld1_u64(predicate, &(array1[i]));
		svuint64_t array2_sve = svld1_u64(predicate, &(array2[i]));
		svuint64_t result_sve = sveor_u64_x(predicate, array1_sve, array2_sve);
		svst1_u64(predicate, &(result_array[i]), result_sve);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] ^ array2[i];
	}
}

CPU_TARGET_SVE
static void helper_bigint_bitwise_or_hardware_accelerated_arm64_sve_sizeless_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes, size_t sve_vector_length_in_u64,
    svbool_t predicate) {

	size_t i = 0;
	size_t simd_width = sve_vector_length_in_u64;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] | array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		svuint64_t array1_sve = svld1_u64(predicate, &(array1[i]));
		svuint64_t array2_sve = svld1_u64(predicate, &(array2[i]));
		svuint64_t result_sve = svorr_u64_x(predicate, array1_sve, array2_sve);
		svst1_u64(predicate, &(result_array[i]), result_sve);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] | array2[i];
	}
}

CPU_TARGET_SVE
static void helper_bigint_bitwise_and_hardware_accelerated_arm64_sve_sizeless_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes, size_t sve_vector_length_in_u64,
    svbool_t predicate) {

	size_t i = 0;
	size_t simd_width = sve_vector_length_in_u64;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] & array2[i];
	}

	// main loop
	for(; i + simd_width <= array_size; i += simd_width) {
		svuint64_t array1_sve = svld1_u64(predicate, &(array1[i]));
		svuint64_t array2_sve = svld1_u64(predicate, &(array2[i]));
		svuint64_t result_sve = svand_u64_x(predicate, array1_sve, array2_sve);
		svst1_u64(predicate, &(result_array[i]), result_sve);
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] & array2[i];
	}
}

CPU_TARGET_SVE
static void sve_get_current_config_impl(uint64_t* sve_vector_length_in_u64, svbool_t* predicate) {

	const uint64_t sve_vector_length = svcntd();

	if(sve_vector_length_in_u64 != NULL) {
		*sve_vector_length_in_u64 = sve_vector_length;
	}

	if(predicate != NULL) {
		// TODO: check if this is correct
		*predicate = svwhilelt_b64_u64(0, sve_vector_length);
	}
}

CPU_TARGET_SVE
NODISCARD static uint64_t sve_get_current_vector_length(void) {

	uint64_t sve_vector_length = 0;
	sve_get_current_config_impl(&sve_vector_length, NULL);
	return sve_vector_length;
}

CPU_TARGET_SVE
static svbool_t sve_get_current_config_predicate(void) {

	svbool_t predicate;
	sve_get_current_config_impl(NULL, &predicate);
	return predicate;
}

CPU_TARGET_SVE NODISCARD static BigIntC
process_bitwise_operation_hardware_accelerated_arm64_sve_sizeless(BigIntC big_int1,
                                                                  BigIntC big_int2,
                                                                  BitWiseOperation op,
                                                                  size_t max_size,
                                                                  size_t sve_vector_length_in_u64) {

	size_t align_bytes_of_sve =
	    sve_vector_length_in_u64 * (SIZE_OF_UINT64_IN_BITS / BITS_BYTES_MULTIPLIER);

	AlignedTheSame aligned_info = AlignedTheSameNone;
	size_t offset_bytes = 0;
	helper_get_config_for_aligned_arrays(big_int1, big_int2, max_size, align_bytes_of_sve,
	                                     &aligned_info, &offset_bytes);

	uint64_t* array1 = big_int1.numbers;
	uint64_t* array2 = big_int2.numbers;

	void* array1_real_alloc_ptr = NULL;
	void* array2_real_alloc_ptr = NULL;

	switch(aligned_info) {
		case AlignedTheSameNone: {
			array1_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array1, max_size * sizeof(uint64_t), align_bytes_of_sve, offset_bytes);
			array2_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array2, max_size * sizeof(uint64_t), align_bytes_of_sve, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array1, big_int1, max_size, 0);
			COPY_BIGINT_TO_BIGGER_ARRAY(array2, big_int2, max_size, 0);

			break;
		}
		case AlignedTheSameFirst: {
			array2_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array2, max_size * sizeof(uint64_t), align_bytes_of_sve, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array2, big_int2, max_size, 0);

			break;
		}
		case AlignedTheSameSecond: {
			array1_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array1, max_size * sizeof(uint64_t), align_bytes_of_sve, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array1, big_int1, max_size, 0);
			break;
		}
		case AlignedTheSameBoth: {
			// do nothing
			break;
		}
		default: {
			UNREACHABLE_WITH_MSG("invalid bitwise operation");
		}
	}

	BigIntC result = { .positive = big_int1.positive, .numbers = NULL, .number_count = max_size };

	void* real_result_allocation_ptr = helper_alloc_aligned_with_offset(
	    (void**)(&(result.numbers)), result.number_count * sizeof(uint64_t), align_bytes_of_sve,
	    offset_bytes);
	memset((void*)result.numbers, 0, max_size * sizeof(uint64_t));

	svbool_t predicate = sve_get_current_config_predicate();

	switch(op) {
		case BitWiseOperationXOR: {
			helper_bigint_bitwise_xor_hardware_accelerated_arm64_sve_sizeless_impl(
			    max_size, array1, array2, result.numbers, offset_bytes, sve_vector_length_in_u64,
			    predicate);
			break;
		}
		case BitWiseOperationOR: {
			helper_bigint_bitwise_or_hardware_accelerated_arm64_sve_sizeless_impl(
			    max_size, array1, array2, result.numbers, offset_bytes, sve_vector_length_in_u64,
			    predicate);
			break;
		}
		case BitWiseOperationAND: {
			helper_bigint_bitwise_and_hardware_accelerated_arm64_sve_sizeless_impl(
			    max_size, array1, array2, result.numbers, offset_bytes, sve_vector_length_in_u64,
			    predicate);
			break;
		}
		default: {
			UNREACHABLE_WITH_MSG("invalid bitwise operation");
		}
	}

	if(array1_real_alloc_ptr != NULL) {
		free(array1_real_alloc_ptr);
	}

	if(array2_real_alloc_ptr != NULL) {
		free(array2_real_alloc_ptr);
	}

	// reallocate the result numbers ptr, if it is not the same as the ptr, that can be freed, this
	// is needed, to keep the same alignment as the two input values
	if(real_result_allocation_ptr != result.numbers) {

		BigIntC new_result = bigint_helper_get_full_copy(result);

		free(real_result_allocation_ptr);

		result = new_result;
	}

	return result;
}

#elif defined(__riscv) && __riscv_xlen == 64

#if __riscv_v_elen < 64
// NOTE: elen 64 support is checked at runtime!
// #error  "ELEN is smaller than 64 bits, so uint64_t arrays can't be processed, this should never
// occur, if it does, hardware acceleation should be disabled!"
#endif

#if defined(__GNUC__)

// need extensions v + zve64x
#define CPU_TARGET_RVV __attribute__((target("arch=+zve64x")))

#if defined(__clang__)

// clang doesn't define these wrappers, if we don't have compile time __riscv_v_elen >= 64 support,
// but we just needs those for the runtime
#if __riscv_v_elen < 64

#define __riscv_vsetvlmax_e64m1() __builtin_rvv_vsetvlimax(3, 0)
#define __riscv_vsetvlmax_e64m2() __builtin_rvv_vsetvlimax(3, 1)
#define __riscv_vsetvlmax_e64m4() __builtin_rvv_vsetvlimax(3, 2)
#define __riscv_vsetvlmax_e64m8() __builtin_rvv_vsetvlimax(3, 3)
#endif

#define UNREACHABLE_LMUL() UNREACHABLE_WITH_MSG("LMUL (enum) too big, has to be in range 0-3");

#else
// gcc

#define UNREACHABLE_LMUL() __builtin_unreachable()

#endif
#else
#error "Not supported"
#endif

typedef struct {
	size_t vl;        // length of uint64s in a vector
	uint8_t lmul_pow; // 0,1,2 or 3, maps to 2^<lmul> so means 1,2,4 or 8
} RVVSetting;

// according to spec, see https://github.com/riscvarchive/riscv-v-spec/releases/tag/v1.0

// TODO: check if this is the same on gcc!

// clang has a enum for SEW and says e64 is 3
#define E64 3 //  SEW=64b

#define M1 0 // LMUL=1
#define M2 1 // LMUL=2
#define M4 2 // LMUL=4
#define M8 3 // LMUL=8

CPU_TARGET_RVV
NODISCARD static uint64_t rvv_get_and_set_final_vl_for_lmul_impl(uint8_t lmul_pow) {

	// dynamic wrapper for  __riscv_vsetvlmax_e64m<lmul>

	switch(lmul_pow) {
		case M1: return __riscv_vsetvlmax_e64m1();
		case M2: return __riscv_vsetvlmax_e64m2();
		case M4: return __riscv_vsetvlmax_e64m4();
		case M8: return __riscv_vsetvlmax_e64m8();
		default: {
			UNREACHABLE_LMUL();
		}
	}
}

CPU_TARGET_RVV NODISCARD static bool rvv_support_elen_64_impl(void) {

	// note the spec for 1.0 says:
	// If the vtype setting is not supported by the implementation, then the vill bit is set in
	// vtype, the remaining bits in vtype are set to zero, and the vl register is also set to zero.
	return __riscv_vsetvlmax_e64m1() != 0;
}

#define LMUL_FROM_POW(lmul_pow) (((uint8_t)2UL) << (lmul_pow))

CPU_TARGET_RVV NODISCARD static uint64_t rvv_get_max_u64_per_iteration_impl(size_t vl,
                                                                            uint8_t lmul_pow) {
	// return VLEN * LMUL, lmul is encoded in lmul_pow
	return vl * LMUL_FROM_POW(lmul_pow);
}

CPU_TARGET_RVV NODISCARD static uint64_t rvv_get_max_u64_per_iteration(RVVSetting setting) {
	return rvv_get_max_u64_per_iteration_impl(setting.vl, setting.lmul_pow);
}

CPU_TARGET_RVV
NODISCARD static bool rvv_is_invalid_rvv_setting(RVVSetting setting) {
	return setting.lmul_pow > 3;
}

#define INVALID_RVV_SETTING ((RVVSetting){ .vl = 0, .lmul_pow = 10 })

// NOte: this function does two things, it sets up the maximum available vl for the best LMUL value,
// and it checks, that it doesn't overshoot, as e.g. actual_size = 8 doesn't need 16 values
// processed at once
CPU_TARGET_RVV
NODISCARD static RVVSetting rvv_get_and_set_maximum_viable_setting(size_t actual_size) {

	if(!rvv_support_elen_64_impl()) {
		return INVALID_RVV_SETTING;
	}

#define LMUL_VARAINTS_SIZE 4

	uint8_t lmul_pow_array[LMUL_VARAINTS_SIZE] = { M1, M2, M4, M8 };

	uint64_t max_thoughput = 0;
	size_t max_t_i = 0;

	for(size_t i = 0; i < LMUL_VARAINTS_SIZE; ++i) {

		const uint8_t lmul_pow = lmul_pow_array[i];

		const uint64_t vl = rvv_get_and_set_final_vl_for_lmul_impl(lmul_pow);

		const uint64_t amount_to_process = rvv_get_max_u64_per_iteration_impl(vl, lmul_pow);

		if(amount_to_process > max_thoughput) {
			max_t_i = i;
			max_thoughput = amount_to_process;
		}

		if(actual_size <= (amount_to_process * MIN_HW_ACCEL_SIZE_MULT)) {
			// we overshot, reset and return the earlier setting if possible. otherwise return
			// INVALID_RVV_SETTING
			if(i > 0) {
				// reset the config to the best one so far (or one before, if that is the current
				// one), and return that
				size_t lmul_idx = max_t_i == i ? i - 1 : max_t_i;
				const uint8_t lmul_pow = lmul_pow_array[lmul_idx];

				const uint64_t vl = rvv_get_and_set_final_vl_for_lmul_impl(lmul_pow);

				return ((RVVSetting){ .vl = vl, .lmul_pow = lmul_pow });

			} else {
				return INVALID_RVV_SETTING;
			}
		}
	}

	// return the best result
	const uint8_t lmul_pow = lmul_pow_array[max_t_i];

	const uint64_t vl = rvv_get_and_set_final_vl_for_lmul_impl(lmul_pow);

	return ((RVVSetting){ .vl = vl, .lmul_pow = lmul_pow });
}

CPU_TARGET_RVV
static void helper_bigint_bitwise_xor_hardware_accelerated_riscv64_rvv_sizeless_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes, size_t rvv_vector_length_in_u64,
    RVVSetting rvv_setting) {

	size_t i = 0;
	size_t simd_width = rvv_vector_length_in_u64;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] ^ array2[i];
	}

	size_t vl = rvv_setting.vl;

	switch(rvv_setting.lmul_pow) {
		case M1: {
			// main loop
			for(; i + simd_width <= array_size; i += simd_width) {
				vuint64m1_t array1_rvv_m1 = __riscv_vle64_v_u64m1(&(array1[i]), vl);
				vuint64m1_t array2_rvv_m1 = __riscv_vle64_v_u64m1(&(array2[i]), vl);
				vuint64m1_t result_rvv_m1 = __riscv_vxor_vv_u64m1(array1_rvv_m1, array2_rvv_m1, vl);
				__riscv_vse64_v_u64m1(&(result_array[i]), result_rvv_m1, vl);
			}
			break;
		}
		case M2: {
			for(; i + simd_width <= array_size; i += simd_width) {
				vuint64m2_t array1_rvv_m2 = __riscv_vle64_v_u64m2(&(array1[i]), vl);
				vuint64m2_t array2_rvv_m2 = __riscv_vle64_v_u64m2(&(array2[i]), vl);
				vuint64m2_t result_rvv_m2 = __riscv_vxor_vv_u64m2(array1_rvv_m2, array2_rvv_m2, vl);
				__riscv_vse64_v_u64m2(&(result_array[i]), result_rvv_m2, vl);
			}
			break;
		}
		case M4: {
			for(; i + simd_width <= array_size; i += simd_width) {
				vuint64m4_t array1_rvv_m4 = __riscv_vle64_v_u64m4(&(array1[i]), vl);
				vuint64m4_t array2_rvv_m4 = __riscv_vle64_v_u64m4(&(array2[i]), vl);
				vuint64m4_t result_rvv_m4 = __riscv_vxor_vv_u64m4(array1_rvv_m4, array2_rvv_m4, vl);
				__riscv_vse64_v_u64m4(&(result_array[i]), result_rvv_m4, vl);
			}
			break;
		}
		case M8: {
			for(; i + simd_width <= array_size; i += simd_width) {
				vuint64m8_t array1_rvv_m8 = __riscv_vle64_v_u64m8(&(array1[i]), vl);
				vuint64m8_t array2_rvv_m8 = __riscv_vle64_v_u64m8(&(array2[i]), vl);
				vuint64m8_t result_rvv_m8 = __riscv_vxor_vv_u64m8(array1_rvv_m8, array2_rvv_m8, vl);
				__riscv_vse64_v_u64m8(&(result_array[i]), result_rvv_m8, vl);
			}
			break;
		}
		default: {
			UNREACHABLE_LMUL();
		}
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] ^ array2[i];
	}
}

CPU_TARGET_RVV
static void helper_bigint_bitwise_or_hardware_accelerated_riscv64_rvv_sizeless_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes, size_t rvv_vector_length_in_u64,
    RVVSetting rvv_setting) {

	size_t i = 0;
	size_t simd_width = rvv_vector_length_in_u64;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] | array2[i];
	}

	size_t vl = rvv_setting.vl;

	switch(rvv_setting.lmul_pow) {
		case M1: {
			// main loop
			for(; i + simd_width <= array_size; i += simd_width) {
				vuint64m1_t array1_rvv_m1 = __riscv_vle64_v_u64m1(&(array1[i]), vl);
				vuint64m1_t array2_rvv_m1 = __riscv_vle64_v_u64m1(&(array2[i]), vl);
				vuint64m1_t result_rvv_m1 = __riscv_vor_vv_u64m1(array1_rvv_m1, array2_rvv_m1, vl);
				__riscv_vse64_v_u64m1(&(result_array[i]), result_rvv_m1, vl);
			}
			break;
		}
		case M2: {
			for(; i + simd_width <= array_size; i += simd_width) {
				vuint64m2_t array1_rvv_m2 = __riscv_vle64_v_u64m2(&(array1[i]), vl);
				vuint64m2_t array2_rvv_m2 = __riscv_vle64_v_u64m2(&(array2[i]), vl);
				vuint64m2_t result_rvv_m2 = __riscv_vor_vv_u64m2(array1_rvv_m2, array2_rvv_m2, vl);
				__riscv_vse64_v_u64m2(&(result_array[i]), result_rvv_m2, vl);
			}
			break;
		}
		case M4: {
			for(; i + simd_width <= array_size; i += simd_width) {
				vuint64m4_t array1_rvv_m4 = __riscv_vle64_v_u64m4(&(array1[i]), vl);
				vuint64m4_t array2_rvv_m4 = __riscv_vle64_v_u64m4(&(array2[i]), vl);
				vuint64m4_t result_rvv_m4 = __riscv_vor_vv_u64m4(array1_rvv_m4, array2_rvv_m4, vl);
				__riscv_vse64_v_u64m4(&(result_array[i]), result_rvv_m4, vl);
			}
			break;
		}
		case M8: {
			for(; i + simd_width <= array_size; i += simd_width) {
				vuint64m8_t array1_rvv_m8 = __riscv_vle64_v_u64m8(&(array1[i]), vl);
				vuint64m8_t array2_rvv_m8 = __riscv_vle64_v_u64m8(&(array2[i]), vl);
				vuint64m8_t result_rvv_m8 = __riscv_vor_vv_u64m8(array1_rvv_m8, array2_rvv_m8, vl);
				__riscv_vse64_v_u64m8(&(result_array[i]), result_rvv_m8, vl);
			}
			break;
		}
		default: {
			UNREACHABLE_LMUL();
		}
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] | array2[i];
	}
}

CPU_TARGET_RVV
static void helper_bigint_bitwise_and_hardware_accelerated_riscv64_rvv_sizeless_impl(
    size_t array_size, const uint64_t* restrict const array1, const uint64_t* restrict const array2,
    uint64_t* restrict result_array, size_t aligned_bytes, size_t rvv_vector_length_in_u64,
    RVVSetting rvv_setting) {
	size_t i = 0;
	size_t simd_width = rvv_vector_length_in_u64;

	// normal unaligned process, as the head is not aligned by aligned_bytes, doing this spares one
	// reallocation, as we use the alignment of one bigint, and "align" the second one to that
	for(; i < aligned_bytes; ++i) {
		result_array[i] = array1[i] & array2[i];
	}

	size_t vl = rvv_setting.vl;

	switch(rvv_setting.lmul_pow) {
		case M1: {
			// main loop
			for(; i + simd_width <= array_size; i += simd_width) {
				vuint64m1_t array1_rvv_m1 = __riscv_vle64_v_u64m1(&(array1[i]), vl);
				vuint64m1_t array2_rvv_m1 = __riscv_vle64_v_u64m1(&(array2[i]), vl);
				vuint64m1_t result_rvv_m1 = __riscv_vand_vv_u64m1(array1_rvv_m1, array2_rvv_m1, vl);
				__riscv_vse64_v_u64m1(&(result_array[i]), result_rvv_m1, vl);
			}
			break;
		}
		case M2: {
			for(; i + simd_width <= array_size; i += simd_width) {
				vuint64m2_t array1_rvv_m2 = __riscv_vle64_v_u64m2(&(array1[i]), vl);
				vuint64m2_t array2_rvv_m2 = __riscv_vle64_v_u64m2(&(array2[i]), vl);
				vuint64m2_t result_rvv_m2 = __riscv_vand_vv_u64m2(array1_rvv_m2, array2_rvv_m2, vl);
				__riscv_vse64_v_u64m2(&(result_array[i]), result_rvv_m2, vl);
			}
			break;
		}
		case M4: {
			for(; i + simd_width <= array_size; i += simd_width) {
				vuint64m4_t array1_rvv_m4 = __riscv_vle64_v_u64m4(&(array1[i]), vl);
				vuint64m4_t array2_rvv_m4 = __riscv_vle64_v_u64m4(&(array2[i]), vl);
				vuint64m4_t result_rvv_m4 = __riscv_vand_vv_u64m4(array1_rvv_m4, array2_rvv_m4, vl);
				__riscv_vse64_v_u64m4(&(result_array[i]), result_rvv_m4, vl);
			}
			break;
		}
		case M8: {
			for(; i + simd_width <= array_size; i += simd_width) {
				vuint64m8_t array1_rvv_m8 = __riscv_vle64_v_u64m8(&(array1[i]), vl);
				vuint64m8_t array2_rvv_m8 = __riscv_vle64_v_u64m8(&(array2[i]), vl);
				vuint64m8_t result_rvv_m8 = __riscv_vand_vv_u64m8(array1_rvv_m8, array2_rvv_m8, vl);
				__riscv_vse64_v_u64m8(&(result_array[i]), result_rvv_m8, vl);
			}
			break;
		}
		default: {
			UNREACHABLE_LMUL();
		}
	}

	// unaligned tail
	for(; i < array_size; ++i) {
		result_array[i] = array1[i] & array2[i];
	}
}

NODISCARD static BigIntC process_bitwise_operation_hardware_accelerated_riscv64_rvv_sizeless(
    BigIntC big_int1, BigIntC big_int2, BitWiseOperation op, size_t max_size,
    RVVSetting rvv_setting, size_t rvv_vector_length_in_u64) {

	size_t align_bytes_of_rvv =
	    rvv_vector_length_in_u64 * (SIZE_OF_UINT64_IN_BITS / BITS_BYTES_MULTIPLIER);

	AlignedTheSame aligned_info = AlignedTheSameNone;
	size_t offset_bytes = 0;
	helper_get_config_for_aligned_arrays(big_int1, big_int2, max_size, align_bytes_of_rvv,
	                                     &aligned_info, &offset_bytes);

	uint64_t* array1 = big_int1.numbers;
	uint64_t* array2 = big_int2.numbers;

	void* array1_real_alloc_ptr = NULL;
	void* array2_real_alloc_ptr = NULL;

	switch(aligned_info) {
		case AlignedTheSameNone: {
			array1_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array1, max_size * sizeof(uint64_t), align_bytes_of_rvv, offset_bytes);
			array2_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array2, max_size * sizeof(uint64_t), align_bytes_of_rvv, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array1, big_int1, max_size, 0);
			COPY_BIGINT_TO_BIGGER_ARRAY(array2, big_int2, max_size, 0);

			break;
		}
		case AlignedTheSameFirst: {
			array2_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array2, max_size * sizeof(uint64_t), align_bytes_of_rvv, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array2, big_int2, max_size, 0);

			break;
		}
		case AlignedTheSameSecond: {
			array1_real_alloc_ptr = helper_alloc_aligned_with_offset(
			    (void**)&array1, max_size * sizeof(uint64_t), align_bytes_of_rvv, offset_bytes);

			COPY_BIGINT_TO_BIGGER_ARRAY(array1, big_int1, max_size, 0);
			break;
		}
		case AlignedTheSameBoth: {
			// do nothing
			break;
		}
		default: {
			UNREACHABLE_WITH_MSG("invalid bitwise operation");
		}
	}

	BigIntC result = { .positive = big_int1.positive, .numbers = NULL, .number_count = max_size };

	void* real_result_allocation_ptr = helper_alloc_aligned_with_offset(
	    (void**)(&(result.numbers)), result.number_count * sizeof(uint64_t), align_bytes_of_rvv,
	    offset_bytes);
	memset((void*)result.numbers, 0, max_size * sizeof(uint64_t));

	switch(op) {
		case BitWiseOperationXOR: {
			helper_bigint_bitwise_xor_hardware_accelerated_riscv64_rvv_sizeless_impl(
			    max_size, array1, array2, result.numbers, offset_bytes, rvv_vector_length_in_u64,
			    rvv_setting);
			break;
		}
		case BitWiseOperationOR: {
			helper_bigint_bitwise_or_hardware_accelerated_riscv64_rvv_sizeless_impl(
			    max_size, array1, array2, result.numbers, offset_bytes, rvv_vector_length_in_u64,
			    rvv_setting);
			break;
		}
		case BitWiseOperationAND: {
			helper_bigint_bitwise_and_hardware_accelerated_riscv64_rvv_sizeless_impl(
			    max_size, array1, array2, result.numbers, offset_bytes, rvv_vector_length_in_u64,
			    rvv_setting);
			break;
		}
		default: {
			UNREACHABLE_WITH_MSG("invalid bitwise operation");
		}
	}

	if(array1_real_alloc_ptr != NULL) {
		free(array1_real_alloc_ptr);
	}

	if(array2_real_alloc_ptr != NULL) {
		free(array2_real_alloc_ptr);
	}

	// reallocate the result numbers ptr, if it is not the same as the ptr, that can be freed, this
	// is needed, to keep the same alignment as the two input values
	if(real_result_allocation_ptr != result.numbers) {

		BigIntC new_result = bigint_helper_get_full_copy(result);

		free(real_result_allocation_ptr);

		result = new_result;
	}

	return result;
}

#endif

#if defined(USE_HARDWARE_ACCEL)

NODISCARD static BigIntC
process_bitwise_operation_generic_hardware_accelerated(BigIntC big_int1, BigIntC big_int2,
                                                       BitWiseOperation op, size_t max_size,
                                                       OptimizationLevel opt_level) {

	switch(opt_level) {
#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
			// 86_64
		case OptimizationLevel_AMD64_SSE2: {
		use_sse2:

			if(max_size <= MIN_SIZE_FOR_SSE2) {
				goto use_generic;
			}

			return process_bitwise_operation_hardware_accelerated_amd64_sse2(big_int1, big_int2, op,
			                                                                 max_size);
		}
		case OptimizationLevel_AMD64_AVX2: {
		use_avx2:

			if(max_size <= MIN_SIZE_FOR_AVX2) {
				goto use_sse2;
			}

			return process_bitwise_operation_hardware_accelerated_amd64_avx2(big_int1, big_int2, op,
			                                                                 max_size);
		}
		case OptimizationLevel_AMD64_AVX512: {

			if(max_size <= MIN_SIZE_FOR_AVX512) {
				goto use_avx2;
			}

			return process_bitwise_operation_hardware_accelerated_amd64_avx512(big_int1, big_int2,
			                                                                   op, max_size);
		}
#elif defined(__aarch64__)
			// aarch64
		case OptimizationLevel_ARM64_NEON: {
		use_neon:

			if(max_size <= MIN_SIZE_FOR_NEON) {
				goto use_generic;
			}

			return process_bitwise_operation_hardware_accelerated_arm64_neon(big_int1, big_int2, op,
			                                                                 max_size);
		}
		case OptimizationLevel_ARM64_SVE: {

			size_t sve_vector_length_in_u64 = sve_get_current_vector_length();

			if(max_size <= (sve_vector_length_in_u64 * MIN_HW_ACCEL_SIZE_MULT)) {
				goto use_neon;
			}

			return process_bitwise_operation_hardware_accelerated_arm64_sve_sizeless(
			    big_int1, big_int2, op, max_size, sve_vector_length_in_u64);
		}
#elif defined(__riscv) && __riscv_xlen == 64
			// riscv64
		case OptimizationLevel_RISCV64_RVV: {

			RVVSetting rvv_setting = rvv_get_and_set_maximum_viable_setting(max_size);

			if(rvv_is_invalid_rvv_setting(rvv_setting)) {
				goto use_generic;
			}

			uint64_t rvv_vector_length_in_u64 = rvv_get_max_u64_per_iteration(rvv_setting);

			if(max_size <= (rvv_vector_length_in_u64 * MIN_HW_ACCEL_SIZE_MULT)) {
				goto use_generic;
			}

			return process_bitwise_operation_hardware_accelerated_riscv64_rvv_sizeless(
			    big_int1, big_int2, op, max_size, rvv_setting, rvv_vector_length_in_u64);
		}
#endif
		case OptimizationLevelNone:
		default: {
		use_generic:
			return process_bitwise_operation_generic(big_int1, big_int2, op, max_size);
		}
	}
}

#endif // defined(USE_HARDWARE_ACCEL)

NODISCARD static BigIntC process_bitwise_operation(BigIntC big_int1, BigIntC big_int2,
                                                   BitWiseOperation op) {

	// if the arrays are the same, we passed the same bigint as a and b, as we use restrict for that
	// arrays, that could lead to problems, so we just use a fast approach for getting the result of
	// a <op> a
	if(big_int1.numbers == big_int2.numbers) {

		if(big_int1.number_count != big_int2.number_count ||
		   big_int1.positive != big_int2.positive) {
			PANIC(
			    "a pointer to data is used for different sized or signed bigints, that means, the "
			    "user did some illegal modifications to one of the bigints!");
		}

		// this function is not hardware acceleated, as it just uses memset and mecpy, nothing
		// fancy, so it's not really needed
		return process_bitwise_operation_same_generic(big_int1, op);
	}

	size_t max_size = helper_max(big_int1.number_count, big_int2.number_count);

#if defined(USE_HARDWARE_ACCEL)
	if(max_size <= MIN_SIZE_FOR_HARDWARE_ACCEL) {
		return process_bitwise_operation_generic(big_int1, big_int2, op, max_size);
	}

	OptimizationLevel best_optimization_level = get_best_optimization_level();

	return process_bitwise_operation_generic_hardware_accelerated(big_int1, big_int2, op, max_size,
	                                                              best_optimization_level);
#else
	return process_bitwise_operation_generic(big_int1, big_int2, op, max_size);
#endif
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_bitwise_xor(BigIntC big_int1, BigIntC big_int2) {
	return process_bitwise_operation(big_int1, big_int2, BitWiseOperationXOR);
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_bitwise_or(BigIntC big_int1, BigIntC big_int2) {
	return process_bitwise_operation(big_int1, big_int2, BitWiseOperationOR);
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_bitwise_and(BigIntC big_int1, BigIntC big_int2) {
	return process_bitwise_operation(big_int1, big_int2, BitWiseOperationAND);
}

BIGINT_C_LIB_EXPORTED void bigint_bitwise_complement(BigIntC* big_int) {

	if(big_int == NULL) { // GCOVR_EXCL_BR_LINE (gcovr can't detect asserts)
		UNREACHABLE_WITH_MSG("passed in NULL pointer"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)
	//
}

// TODO: use also hardware accelaration for bit shifts, and even adds, if it is faster, than normal
// things, dependending on the situation

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic,misc-use-anonymous-namespace,modernize-use-auto,modernize-use-using,cppcoreguidelines-no-malloc)
