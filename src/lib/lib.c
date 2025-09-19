

#include "./lib.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

// functions on maybe bigint

NODISCARD bool maybe_bigint_is_error(MaybeBigIntC maybe_big_int) {
	return maybe_big_int.error;
}

NODISCARD BigInt maybe_bigint_get_value(MaybeBigIntC maybe_big_int) {
	ASSERT(!maybe_bigint_is_error(maybe_big_int), "MaybeBigIntC has no value");

	return maybe_big_int.data.result;
}

NODISCARD MaybeBigIntError maybe_bigint_get_error(MaybeBigIntC maybe_big_int) {
	ASSERT(maybe_bigint_is_error(maybe_big_int), "MaybeBigIntC has no error");

	return maybe_big_int.data.error;
}

// normal bigint functions

#define U64(n) (uint64_t)(n##ULL)

static void bigint_helper_realloc_to_new_size(BigInt* big_int) {

	uint64_t* new_numbers = realloc(big_int->numbers, sizeof(uint64_t) * big_int->number_count);

	if(new_numbers == NULL) {
		UNREACHABLE_WITH_MSG("realloc failed, no error handling implemented here");
	}

	big_int->numbers = new_numbers;
}

static BigInt bigint_helper_zero(void) {

	BigInt result = { .positive = true, .numbers = NULL, .number_count = 1 };

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
		size_t new_size = digits->capacity == 0 ? BCD_DIGITS_START_CAPACITY : digits->capacity * 2;

		BCDDigit* new_bcd_digits = realloc(digits->bcd_digits, sizeof(BCDDigit) * new_size);

		if(new_bcd_digits == NULL) {
			UNREACHABLE_WITH_MSG("realloc failed, no error handling implemented here");
		}

		digits->capacity = new_size;
		digits->bcd_digits = new_bcd_digits;
	}

	digits->bcd_digits[digits->count] = digit;

	++(digits->count);
}

#define BIGINT_BIT_COUNT_FOR_BCD_ALG 64
#define BCD_DIGIT_BIT_COUNT_FOR_BCD_ALG 4

static void bigint_helper_bcd_digits_to_bigint(BigInt* big_int, BCDDigits bcd_digits) {
	// using reverse double dabble, see
	// https://en.wikipedia.org/wiki/Double_dabble#Reverse_double_dabble

	if(bcd_digits.count == 0) {
		UNREACHABLE_WITH_MSG("not initialized bcd_digits correctly");
	}

	// this acts as a helper type, where we shift bits into, it is stored in reverse order than
	// normal bigints
	BigInt temp = { .positive = true, .numbers = NULL, .number_count = 0 };

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
					uint8_t last_bit = ((temp.numbers[i - 1]) & 0x01);

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
				BCDDigit last_value = bcd_digits.bcd_digits[bcd_digits.count - 1];
				if((last_value & 0x01) != 0) {
					temp.numbers[0] =
					    (U64(1) << (BIGINT_BIT_COUNT_FOR_BCD_ALG - 1)) + temp.numbers[0];
				}
			}

			{ // 1.2. shift every bcd_input along (only those who are not empty already)

				// 1.2.1. shift the last bit of every number into the next one
				for(size_t i = bcd_digits.count; i > bcd_processed_fully_amount; --i) {
					uint8_t last_bit = ((bcd_digits.bcd_digits[i - 1]) & 0x01);

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

				BCDDigit value = bcd_digits.bcd_digits[i - 1];

				if(value >= 8) {
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

			uint8_t alignment = pushed_bits % BIGINT_BIT_COUNT_FOR_BCD_ALG;

			uint8_t to_shift = BIGINT_BIT_COUNT_FOR_BCD_ALG - alignment;

			if(alignment != 0) {

				// 3.1.1. shift the last to_shift bytes of every number into the next one
				for(size_t i = temp.number_count; i != 0; --i) {
					uint64_t last_bytes = (temp.numbers[i - 1]) & ((U64(1) << to_shift) - U64(1));

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

static void bigint_helper_remove_leading_zeroes(BigInt* big_int) {
	if(big_int->number_count == 0) {
		UNREACHABLE_WITH_MSG("big_int has to have at least one number!");
	}

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

	ASSERT(value <= 9, "value is not a valid digit");

	return (StrType)((StrType)value + '0');
}

NODISCARD static inline bool helper_is_separator(StrType value) {
	// valid separators are /[_',.]/
	return value == '_' || value == '\'' || value == ',' || value == '.';
}

NODISCARD MaybeBigIntC maybe_bigint_from_string(ConstStr str) {

	BigInt result = bigint_helper_zero();

	size_t str_len = strlen(str);

	// bigint regex: /^[+-]?[0-9][0-9_',.]*$/

	if(str_len == 0) {
		free_bigint(&result);
		return (MaybeBigIntC){
			.error = true, .data = { .error = (MaybeBigIntError) "empty string is not valid" }
		};
	}

	size_t i = 0;

	if(str[0] == '-') {
		result.positive = false;
		++i;

		if(str_len == 1) {
			free_bigint(&result);
			return (MaybeBigIntC){
				.error = true, .data = { .error = (MaybeBigIntError) "'-' alone is not valid" }
			};
		}

	} else if(str[0] == '+') {
		result.positive = true;
		++i;

		if(str_len == 1) {
			free_bigint(&result);
			return (MaybeBigIntC){
				.error = true, .data = { .error = (MaybeBigIntError) "'+' alone is not valid" }
			};
		}
	} else {
		result.positive = true;
	}

	bool start = true;

	BCDDigits bcd_digits = { .bcd_digits = NULL, .count = 0, .capacity = 0 };

	for(; i < str_len; ++i) {
		StrType value = str[i];

		if(helper_is_digit(value)) {
			helper_add_value_to_bcd_digits(&bcd_digits, helper_char_to_digit(value));
		} else if(helper_is_separator(value)) {
			if(start) {
				// not allowed
				free_bigint(&result);
				free_bcd_digits(bcd_digits);
				// TODO:report position and character
				return (MaybeBigIntC){
					.error = true,
					.data = { .error = (MaybeBigIntError) "separator not allowed at the start" }
				};
			}
			// skip this separator
			continue;
		} else {
			free_bigint(&result);
			free_bcd_digits(bcd_digits);
			// TODO:report position and character
			return (MaybeBigIntC){ .error = true,
				                   .data = { .error = (MaybeBigIntError) "invalid character" } };
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
				return (MaybeBigIntC){
					.error = true, .data = { .error = (MaybeBigIntError) "-0 is not allowed" }
				};
			}
		}
	}

	bigint_helper_remove_leading_zeroes(&result);

	return (MaybeBigIntC){ .error = false, .data = { .result = result } };
}

NODISCARD BigInt bigint_from_unsigned_number(uint64_t number) {
	BigInt result = bigint_helper_zero();
	result.positive = true;
	result.numbers[0] = number;

	return result;
}

NODISCARD BigInt bigint_from_signed_number(int64_t number) {
	BigInt result = bigint_helper_zero();

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

void free_bigint(BigInt* big_int) {
	if(big_int == NULL) {
		return;
	}

	if(big_int->numbers != NULL) {
		free(big_int->numbers);
		big_int->numbers = NULL;
	}
}

void free_bigint_without_reset(BigIntC big_int) {
	if(big_int.numbers != NULL) {
		free(big_int.numbers);
	}
}

NODISCARD static BigIntC bigint_helper_get_full_copy(BigIntC big_int) {

	BigIntC result = { .positive = big_int.positive,
		               .numbers = NULL,
		               .number_count = big_int.number_count };

	bigint_helper_realloc_to_new_size(&result);

	memcpy(result.numbers, big_int.numbers, sizeof(uint64_t) * big_int.number_count);

	return result;
}

NODISCARD BigIntC bigint_copy(BigIntC big_int) {
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

	if(source.number_count == 0) {
		UNREACHABLE_WITH_MSG("not initialized BigInt correctly");
	}

	// reverse the source so that the bits are aligned
	{
		for(size_t i = 0; i < source.number_count / 2; ++i) {
			uint64_t temp = source.numbers[i];
			source.numbers[i] = source.numbers[source.number_count - 1 - i];

			source.numbers[source.number_count - 1 - i] = temp;
		}
	}

	const size_t last_number_bit_amount = bigint_helper_bits_of_number_used(
	    source.numbers[0]); // range 0 -64 0 should never be here, as then i should have remove it
	                        // earlier (remove leading zeroes!)

	// but it is here when the value is just 0 ("0")
	// TODO: handle that case
	ASSERT(last_number_bit_amount != 0, "first number has to have at least one bit of information");

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

				BCDDigit value = bcd_digits.bcd_digits[i];

				if(value >= 5) {
					bcd_digits.bcd_digits[i] = bcd_digits.bcd_digits[i] + 3;
				}
			}
		}

		{ // 2. shift left by one

			{ // 2.1. shift every bcd_output to the left
				// ( not in 8 but in 4 bit shifts), for
				// convience we shift to the right in the array elements as the order is [0., 1. ]
				// and we need to shift from the 0. left to the 1.

				{ // 2.1.1. if we need a new bcd_digit, allocate it and set it to 0

					bool needs_new_digit = false;

					{ // 2.1.1.1. determine if we need a new bcd_digit

						// 2.1.1.2. if no digits i present, we need a new one
						if(bcd_digits.count == 0) {
							needs_new_digit = true;
						} else {
							// 2.1.1.3. we need a new one, if the next shift would overflow!
							BCDDigit value = bcd_digits.bcd_digits[bcd_digits.count - 1];
							uint8_t first_bit =
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

						BCDDigit value = bcd_digits.bcd_digits[i - 1];

						uint8_t first_bit = (value >> (BCD_DIGIT_BIT_COUNT_FOR_BCD_ALG - 1)) & 0x01;

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

				size_t input_index = current_bit / BIGINT_BIT_COUNT_FOR_BCD_ALG;

				size_t input_u64_index =
				    BIGINT_BIT_COUNT_FOR_BCD_ALG - (current_bit % BIGINT_BIT_COUNT_FOR_BCD_ALG) - 1;

				ASSERT(input_index < source.number_count, "input index would index out of bounds");

				uint8_t first_bit = (source.numbers[input_index] >> input_u64_index) & 0x01;

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
NODISCARD Str bigint_to_string(BigInt big_int) {

	BigInt copy = bigint_helper_get_full_copy(big_int);

	BCDDigits bcd_digits = bigint_helper_get_bcd_digits_from_bigint(copy);

	free_bigint(&copy);

	// format bcd_digits into a string, note that the bcd_digits are stored reversed

	size_t sign_amount = big_int.positive ? 0 : 1;

	Str str = malloc(sizeof(StrType) * (bcd_digits.count + 1 + sign_amount));

	if(str == NULL) {
		free_bcd_digits(bcd_digits);
		return NULL;
	}

	str[bcd_digits.count + sign_amount] = '\0';

	if(!big_int.positive) {
		str[0] = '-';
	}

	for(size_t i = 0; i < bcd_digits.count; ++i) {

		str[sign_amount + i] =
		    helper_digit_to_char_checked(bcd_digits.bcd_digits[bcd_digits.count - i - 1]);
	}

	return str;
}

NODISCARD static size_t helper_max(size_t a, size_t b) {
	if(a > b) {
		return a;
	}
	return b;
}

#if defined(__SIZEOF_INT128__)

typedef __uint128_t uint128_t;
typedef __int128_t int128_t;

NODISCARD static BigInt bigint_add_bigint_both_positive_using_128_bit_numbers(BigInt big_int1,
                                                                              BigInt big_int2) {

	size_t max_count = helper_max(big_int1.number_count, big_int2.number_count) + 1;

	BigInt result = { .positive = true, .number_count = max_count, .numbers = NULL };

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

			carry = (uint64_t)(sum >> 64);
		}

		ASSERT(carry == 0,
		       "The carry at the end has to be zero, otherwise we would have an overflow");
	}

	bigint_helper_remove_leading_zeroes(&result);

	return result;
}

NODISCARD static BigInt bigint_sub_bigint_both_positive_using_128_bit_numbers(BigInt big_int1,
                                                                              BigInt big_int2) {

	// NOTE: here it is assumed, that  a > b

	size_t max_count = helper_max(big_int1.number_count, big_int2.number_count) + 1;

	BigInt result = { .positive = true, .number_count = max_count, .numbers = NULL };

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
				temp = ((int128_t)1 << 64) + temp;
				borrow = (int64_t)0LL;
			}

			result.numbers[i] = (uint64_t)temp;
		}

		ASSERT(borrow == 0,
		       "The borrow at the end has to be zero, otherwise we would have an overflow");
	}

	bigint_helper_remove_leading_zeroes(&result);

	return result;
}

#endif

NODISCARD static BigInt bigint_add_bigint_both_positive(BigInt big_int1, BigInt big_int2) {

#if defined(__SIZEOF_INT128__)
	return bigint_add_bigint_both_positive_using_128_bit_numbers(big_int1, big_int2);
#else
#error "TODO"
// TODO: use asm if on x86_64 or arm64 / or standard c way!
#endif
}

NODISCARD BigInt bigint_add_bigint(BigInt big_int1, BigInt big_int2) {

	if(big_int1.positive && big_int2.positive) {
		return bigint_add_bigint_both_positive(big_int1, big_int2);
	}

	if(big_int1.positive && !big_int2.positive) {
		// +a + -b = +a - +b
		big_int2.positive = true;
		return bigint_sub_bigint(big_int1, big_int2);
	}

	if(!big_int1.positive && big_int2.positive) {
		// -a + +b = +b - +a
		big_int1.positive = true;
		return bigint_sub_bigint(big_int2, big_int1);
	}

	// both are negative

	// -a + -b = - ( +a + +b )

	big_int1.positive = true;
	big_int2.positive = true;

	BigInt result = bigint_add_bigint_both_positive(big_int1, big_int2);

	result.positive = false;

	return result;
}

NODISCARD static BigInt bigint_sub_bigint_both_positive_impl(BigInt big_int1, BigInt big_int2) {

#if defined(__SIZEOF_INT128__)
	return bigint_sub_bigint_both_positive_using_128_bit_numbers(big_int1, big_int2);
#else
#error "TODO"
#endif
}

NODISCARD static BigInt bigint_sub_bigint_both_positive(BigInt big_int1, BigInt big_int2) {

	// check in which direction we need to perform the subtraction
	int8_t compared = bigint_compare_bigint(big_int1, big_int2);

	if(compared == 0) {
		return bigint_helper_zero();
	}

	if(compared > 0) {
		return bigint_sub_bigint_both_positive_impl(big_int1, big_int2);
	}

	// +a - +b where b > a = - ( +b - +a)
	BigIntC result = bigint_sub_bigint_both_positive_impl(big_int2, big_int1);

	result.positive = false;
	return result;
}

NODISCARD BigInt bigint_sub_bigint(BigInt big_int1, BigInt big_int2) {

	if(big_int1.positive && big_int2.positive) {
		return bigint_sub_bigint_both_positive(big_int1, big_int2);
	}

	if(big_int1.positive && !big_int2.positive) {
		// +a - -b = +a + +b
		big_int2.positive = true;
		return bigint_add_bigint(big_int1, big_int2);
	}

	if(!big_int1.positive && big_int2.positive) {
		// -a - +b = -a + -b
		big_int2.positive = false;
		return bigint_add_bigint(big_int1, big_int2);
	}

	// both are negative

	// -a - -b = -a + +b = +b - +a

	big_int1.positive = true;
	big_int2.positive = true;

	BigInt result = bigint_sub_bigint_both_positive(big_int2, big_int1);

	return result;
}

NODISCARD bool bigint_eq_bigint(BigIntC big_int1, BigIntC big_int2) {
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

NODISCARD int8_t bigint_compare_bigint(BigIntC big_int1, BigIntC big_int2) {

	if(!big_int1.positive && big_int2.positive) {

		//- 0 is equals to + 0
		if(big_int1.number_count == 1 && big_int2.number_count == 1) {
			if(big_int1.numbers[0] == 0 && big_int2.numbers[0] == 0) {
				return CMP_ARE_EQUAL;
			}
		}

		return CMP_FIRST_ONE_IS_LESS;
	}

	if(big_int1.positive && !big_int2.positive) {

		//- 0 is equals to + 0
		if(big_int1.number_count == 1 && big_int2.number_count == 1) {
			if(big_int1.numbers[0] == 0 && big_int2.numbers[0] == 0) {
				return CMP_ARE_EQUAL;
			}
		}

		return CMP_FIRST_ONE_IS_GREATER;
	}

	if(!big_int1.positive && !big_int2.positive) {

		//-x <=> -y ==  cmp_reverse (x <=> y)

		big_int1.positive = true;
		big_int2.positive = true;
		return cmp_reverse(bigint_compare_bigint(big_int1, big_int2));
	}

	if(big_int1.number_count < big_int2.number_count) {
		return CMP_FIRST_ONE_IS_LESS;
	}

	if(big_int1.number_count > big_int2.number_count) {
		return CMP_FIRST_ONE_IS_GREATER;
	}

	for(size_t i = big_int1.number_count; i != 0; --i) {
		uint64_t num1 = big_int1.numbers[i - 1];
		uint64_t num2 = big_int2.numbers[i - 1];

		if(num1 < num2) {
			return CMP_FIRST_ONE_IS_LESS;
		}

		if(num1 > num2) {
			return CMP_FIRST_ONE_IS_GREATER;
		}
	}

	return CMP_ARE_EQUAL;
}

void bigint_negate(BigIntC* big_int) {

	if(big_int->number_count == 1) {
		if(big_int->numbers[0] == 0) {
			return;
		}
	}

	big_int->positive = !big_int->positive;
}
