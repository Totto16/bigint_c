

#include "./lib.h"
#include "../utils/assert.h"

// NOLINTBEGIN(modernize-deprecated-headers)

#include <limits.h>
#include <stdio.h>
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

static BigIntC bigint_helper_zero(void) {

	BigIntC result = { .positive = true, .numbers = NULL, .number_count = 1 };

	bigint_helper_realloc_to_new_size(&result);

	result.numbers[0] = U64(0);

	return result;
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

#define BIGINT_BIT_COUNT_FOR_BCD_ALG 64
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

static void bigint_helper_remove_leading_zeroes(BigIntC* big_int) {
	if(big_int->number_count == 0) { // GCOVR_EXCL_BR_LINE (every caller assures that)
		UNREACHABLE_WITH_MSG(        // GCOVR_EXCL_LINE (see above)
		    "big_int has to have at least one number!"); // GCOVR_EXCL_LINE (see above)
	} // GCOVR_EXCL_LINE (see above)

	if(big_int->number_count == 1) {
#ifndef NDEBUG
		if(big_int->numbers[0] == 0) {
			ASSERT(big_int->positive, "0 can't be negative");
		}
#endif

		return;
	}

	for(size_t i = big_int->number_count; i > 1; --i) {
		if(big_int->numbers[i - 1] == 0) {
			--(big_int->number_count);
		} else {
			break;
		}
	}

#ifndef NDEBUG
	if(big_int->number_count == 1) {
		if(big_int->numbers[0] == 0) {
			ASSERT(big_int->positive, "0 can't be negative");
		}
	}
#endif

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

	if(result.number_count == 1) {
		if(result.numbers[0] == 0) {
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
	}

	bigint_helper_remove_leading_zeroes(&result);

	return (MaybeBigIntC){ .error = false, .data = { .result = result } };
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_from_unsigned_number(uint64_t number) {
	BigIntC result = bigint_helper_zero();
	result.positive = true;
	result.numbers[0] = number;

	return result;
}

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_from_signed_number(int64_t number) {
	BigIntC result = bigint_helper_zero();

	if(number < 0LL) {
		result.positive = false;
		// overflow, when using - on int64_t
		if(number < -LLONG_MAX) {
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
                                                                    size_t size) {

	BigIntC result = { .positive = true, .numbers = NULL, .number_count = size };

	bigint_helper_realloc_to_new_size(&result);

	for(size_t i = 0; i < size; ++i) {
		result.numbers[size - i - 1] = numbers[i];
	}

	bigint_helper_remove_leading_zeroes(&result);

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
				start_point =
				    (64 - // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
				     bits_used) /
				    4;
				if(start_point == SIZEOF_VALUE_AS_HEX_STR) {
					start_point =
					    SIZEOF_VALUE_AS_HEX_STR - 1; // print one 0, even if it's a not a 1
				}
				ASSERT(start_point < SIZEOF_VALUE_AS_HEX_STR, "start_point was too high");
			}
		}

		for(size_t j = start_point; j < SIZEOF_VALUE_AS_HEX_STR; ++index, ++j) {
			const uint8_t digit = (number >> ((64 - ((j + 1) * 4)))) & 0x0F;
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
				start_point =
				    64 - // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
				    bits_used;
				if(start_point == SIZEOF_VALUE_AS_BIN_STR) {
					start_point =
					    SIZEOF_VALUE_AS_BIN_STR - 1; // print one 0, even if it's a not a 1
				}
				ASSERT(start_point < SIZEOF_VALUE_AS_BIN_STR, "start_point was too high");
			}
		}

		for(size_t j = start_point; j < SIZEOF_VALUE_AS_BIN_STR; ++index, ++j) {
			const uint8_t digit = (number >> ((64 - ((j + 1))))) & 0x01;
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

	bigint_helper_remove_leading_zeroes(&result);

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

	bigint_helper_remove_leading_zeroes(&result);

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

	bigint_helper_remove_leading_zeroes(&result);

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

	bigint_helper_remove_leading_zeroes(&result);

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
// TODO: use asm if on x86_64 or arm64 / or standard c way!
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

		if(*number != LLONG_MAX) {
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
	// it to LLONG_MAX, as it borrows to the next one, we HAVE TO EXIT, except if all numbers are 0,
	// which should never happen, but it is asseretd here too!
	for(size_t i = 0; i < big_int1->number_count; ++i) {

		uint64_t* number = &(big_int1->numbers[i]);

		if(*number != 0) {
			--(*number);
			return;
		} else {
			if(big_int1->number_count == 1) {
				UNREACHABLE_WITH_MSG("not supporting 0 in this function");
			}

			// big_int1->number_count is always > 0, asserted by calling functions
			if(i == big_int1->number_count - 1) {
				UNREACHABLE_WITH_MSG("leading zeros detected");
			}

			*number = LLONG_MAX;
		}
	}

	UNREACHABLE_WITH_MSG("leading zeros detected");
}

// TODO: make bigint_helper_is_zero() helper, as it is used often in this code!

BIGINT_C_LIB_EXPORTED void bigint_increment_bigint(BigIntC* big_int1) {

	if(big_int1 == NULL) {
		UNREACHABLE_WITH_MSG("passed in NULL pointer");
	}

	if(big_int1->number_count == 0) {
		UNREACHABLE_WITH_MSG("invalid bigint passed");
	}

	// treat 0 as special case, as it NEVER should be -, but if it would be, the code afterwards
	// would break
	if(big_int1->number_count == 0) {
		if(big_int1->numbers[0] == 0) {
			big_int1->numbers[0] = 1;
			big_int1->positive = true;
			return;
		}
	}

	if(big_int1->positive) {
		bigint_increment_bigint_positive_or_zero_impl(big_int1);
		return;
	}

	// -a ++ = -a + +1 =  (-1 * +a) + (-1 * -1) = -1 * ( +a + -1) = - ( +a - +1) = - (+a --)

	big_int1->positive = true;
	bigint_decrement_bigint_positive_not_zero_impl(big_int1);
	big_int1->positive = false;

	return;
}

BIGINT_C_LIB_EXPORTED void bigint_decrement_bigint(BigIntC* big_int1) {

	if(big_int1 == NULL) {
		UNREACHABLE_WITH_MSG("passed in NULL pointer");
	}

	if(big_int1->number_count == 0) {
		UNREACHABLE_WITH_MSG("invalid bigint passed");
	}

	// treat 0 as special case, as it NEVER should be -, but if it would be, the code afterwards
	// would break
	if(big_int1->number_count == 0) {
		if(big_int1->numbers[0] == 0) {
			big_int1->numbers[0] = 1;
			big_int1->positive = false;
			return;
		}
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

	if(big_int->number_count == 1) {
		if(big_int->numbers[0] == 0) {
			return;
		}
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

NODISCARD static inline size_t helper_ceil_div(size_t input, size_t divider) {
	return (input + divider - 1) / divider;
}

// NOTES: about "fake" 0 bigints:
//  to make less memory allocations, we use bigint slices, that have NULL as numbers and count as 0,
//  this represent 0, normally we would represent 0 by the numbers_count 0 and the first number
//  being 0 to make this work, we need special checks in some places, where this could be used!
// this are called "<x>_internal"

NODISCARD static inline bool bigint_mul_karatsuba_is_zero_slice(BigIntNullableSlice slice) {
	return slice.number_count == 0 || slice.numbers == NULL;
}

NODISCARD static inline BigInt bigint_helper_copy_of_slice(BigIntSlice big_int_slice) {

	BigInt big_int = { .positive = true,
		               .numbers = (uint64_t*)big_int_slice.numbers,
		               .number_count = big_int_slice.number_count };

	return bigint_helper_get_full_copy(big_int);
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
		return bigint_helper_copy_of_slice(big_int2);
	}

	// +a + +b

	BigInt number_a = bigint_helper_copy_of_slice(bigint_slice_from_nullable(big_int1));
	BigInt number_b = bigint_helper_copy_of_slice(big_int2);

	BigInt result = bigint_add_bigint_both_positive(number_a, number_b);

	free_bigint_without_reset(number_a);
	free_bigint_without_reset(number_b);

	return result;
}

// this adds <amount> 0 numbers to the end of the number, also known as <big_int> * (2^64)^<amount>
static void bigint_mul_bigint_karatsuba_shift_bigint_internally_by(BigInt* big_int, size_t amount) {

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
				return bigint_helper_copy_of_slice(big_int2);
			}
		}

		if(big_int2.number_count == 1) {

			uint64_t number = big_int2.numbers[0];

			if(number == 0) {
				return bigint_helper_zero();
			}

			if(number == 1) {
				return bigint_helper_copy_of_slice(big_int1);
			}
		}
	}

	// basic algorihtm

	// this is a divide and conquer algorithm based on en.wikipedia.org/wiki/Karatsuba_algorithm

	// base case
	if(big_int1.number_count == 1 && big_int2.number_count == 1) {
		return bigint_mul_bigint_karatsuba_base(big_int1.numbers[0], big_int2.numbers[0]);
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

		bigint_mul_bigint_karatsuba_shift_bigint_internally_by(&z_2, divide_at * 2);

		bigint_mul_bigint_karatsuba_shift_bigint_internally_by(&z_1, divide_at);

		const BigInt result_add_temp = bigint_add_bigint_both_positive(z_2, z_1);

		free_bigint_without_reset(z_2);
		free_bigint_without_reset(z_1);

		BigInt result = bigint_add_bigint_both_positive(result_add_temp, z_0);

		free_bigint_without_reset(result_add_temp);
		free_bigint_without_reset(z_0);

		bigint_helper_remove_leading_zeroes(&result);

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
		if(result.number_count == 1) {
			if(result.numbers[0] == 0) {
				result.positive = true;
			}
		}

		return result;
	}

	if(big_int2.positive) {
		// -a * +b = - (+b * +a)
		big_int1.positive = true;
		BigInt result = bigint_mul_bigint_both_positive(big_int1, big_int2);

		result.positive = false;

		// - 0 becomes +0
		if(result.number_count == 1) {
			if(result.numbers[0] == 0) {
				result.positive = true;
			}
		}

		return result;
	}

	// both are negative

	// -a * -b = + ( +a * +b )

	big_int1.positive = true;
	big_int2.positive = true;

	return bigint_mul_bigint_both_positive(big_int1, big_int2);
}

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic,misc-use-anonymous-namespace,modernize-use-auto,modernize-use-using,cppcoreguidelines-no-malloc)
