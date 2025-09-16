

#include "./lib.h"

#include <stdio.h>
#include <string.h>

// functions on maybe bigint

NODISCARD bool maybe_bigint_is_error(MaybeBigInt maybe_big_int) {
	return maybe_big_int.error;
}

NODISCARD BigInt maybe_bigint_get_value(MaybeBigInt maybe_big_int) {
	ASSERT(!maybe_bigint_is_error(maybe_big_int), "MaybeBigInt has no value");

	return maybe_big_int.data.result;
}

NODISCARD MaybeBigIntError maybe_bigint_get_error(MaybeBigInt maybe_big_int) {
	ASSERT(maybe_bigint_is_error(maybe_big_int), "MaybeBigInt has no error");

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

static BigInt bigint_helper_positive_zero(void) {

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
		ASSERT(big_int->number_count == 0, "not initialized BigInt correctly");
		big_int->numbers[0] = U64(0);
		return;
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

				// 2.1 If value >= 8 subtract 3 from value

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

				uint64_t previous_last_bytes = U64(0);

				// 3.1.1. shift the last to_shift bytes of every number into the next one
				for(size_t i = temp.number_count; i != 0; --i) {
					uint64_t last_bytes = (temp.numbers[i - 1]) & ((U64(1) << to_shift) - U64(1));

					if(i == temp.number_count) {
						// those x values from above are not 0
						ASSERT((last_bytes == 0), "alignment and to_shift incorrectly calculated");
					} else {
						temp.numbers[i] = (previous_last_bytes << alignment) + temp.numbers[i];
					}

					previous_last_bytes = last_bytes;

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

	free_bigint(temp);
}

static void bigint_helper_remove_leading_zeroes(BigInt* big_int) {
	if(big_int->number_count == 0) {
		UNREACHABLE_WITH_MSG("big_int has to have at least one number!");
	}

	if(big_int->number_count == 1) {
		return;
	}

	for(size_t i = big_int->number_count; i != 0; --i) {
		if(big_int->numbers[i - 1] == 0) {
			--(big_int->number_count);
		} else {
			break;
		}
	}

	bigint_helper_realloc_to_new_size(big_int);
}

NODISCARD MaybeBigInt bigint_from_string(ConstStr str) {

	BigInt result = bigint_helper_positive_zero();

	size_t str_len = strlen(str);

	// bigint regex: /^[+-]?[0-9][0-9_]*$/

	if(str_len == 0) {
		free_bigint(result);
		return (MaybeBigInt){ .error = true,
			                  .data = { .error = (MaybeBigIntError) "empty string is not valid" } };
	}

	size_t i = 0;

	if(str[0] == '-') {
		result.positive = false;
		++i;

		if(str_len == 1) {
			free_bigint(result);
			return (MaybeBigInt){
				.error = true, .data = { .error = (MaybeBigIntError) "'-' alone is not valid" }
			};
		}

	} else if(str[0] == '+') {
		result.positive = true;
		++i;

		if(str_len == 1) {
			free_bigint(result);
			return (MaybeBigInt){
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

		if(value >= '0' && value <= '9') {
			helper_add_value_to_bcd_digits(&bcd_digits, value - '0');
		} else if(value == '_') {
			if(start) {
				// not allowed
				free_bigint(result);
				free_bcd_digits(bcd_digits);
				// TODO:report position and character
				return (MaybeBigInt){
					.error = true,
					.data = { .error = (MaybeBigIntError) "'_' not allowed at the start" }
				};
			}
			// skip this separator
			continue;
		} else {
			free_bigint(result);
			free_bcd_digits(bcd_digits);
			// TODO:report position and character
			return (MaybeBigInt){ .error = true,
				                  .data = { .error = (MaybeBigIntError) "invalid character" } };
		}

		if(start) {
			start = false;
		}
	}

	bigint_helper_bcd_digits_to_bigint(&result, bcd_digits);

	free_bcd_digits(bcd_digits);

	bigint_helper_remove_leading_zeroes(&result);

	return (MaybeBigInt){ .error = false, .data = { .result = result } };
}

void free_bigint(BigInt big_int) {
	if(big_int.numbers != NULL) {
		free(big_int.numbers);
	}
}

NODISCARD Str bigint_to_string(BigInt big_int) {
	UNUSED(big_int);
	UNREACHABLE();
}
