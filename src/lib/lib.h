#pragma once

// NOLINTBEGIN(modernize-use-using)

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "../utils/utils.h"

typedef struct {
	bool positive;
	uint64_t* numbers;
	size_t number_count;
} BigIntC;

#ifndef __cplusplus
typedef BigIntC BigInt;

#endif

typedef char StrType;

typedef const StrType* ConstStr;

typedef StrType* Str;

typedef struct {
	ConstStr message;
	size_t index;
	StrType symbol;
} MaybeBigIntError;

#define NO_SYMBOL '\0'

typedef struct {
	bool error;
	union {
		BigIntC result;
		MaybeBigIntError error;
	} data;
} MaybeBigIntC;

// NOLINTEND(modernize-use-using)

// functions on maybe bigint

NODISCARD BIGINT_C_LIB_EXPORTED bool maybe_bigint_is_error(MaybeBigIntC maybe_big_int);

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC maybe_bigint_get_value(MaybeBigIntC maybe_big_int);

NODISCARD BIGINT_C_LIB_EXPORTED MaybeBigIntError maybe_bigint_get_error(MaybeBigIntC maybe_big_int);

// normal bigint functions

/**
 * @brief Parses a string and returns a MaybeBigInt, use that to check if it was successfull or if
 * it failed
 *
 * @details The allowed format is /^[+-]?[0-9][0-9_',.]*$/
 *          In words: Optional "-" or "+" at the start, then followed by a digit, afterwards you can
 *          use digits and optional separators "_", "'", ",", "."
 *          note, that "." and "," are no decimal separators like found in doubles (depending on
 *          where in the world it is "," or ".")
 * @param str - the input string
 * @return MaybeBigInt - the result
 */
NODISCARD BIGINT_C_LIB_EXPORTED MaybeBigIntC maybe_bigint_from_string(ConstStr str);

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_from_unsigned_number(uint64_t number);

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_from_signed_number(int64_t number);

/**
 * @brief Constructs a bigint from the logical array of numbers
 *        note, that the order is like in string form most significant digits first, not like in the
 * stored order, which is implementation defined (it is least significant first as of the time of
 * writing)
 *
 * @param numbers
 * @param size
 * @return BigIntC - the result
 */
NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_from_list_of_numbers(uint64_t* numbers, size_t size);

BIGINT_C_LIB_EXPORTED void free_bigint(BigIntC* big_int);

BIGINT_C_LIB_EXPORTED void free_bigint_without_reset(BigIntC big_int);

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_copy(BigIntC big_int);

NODISCARD BIGINT_C_LIB_EXPORTED Str bigint_to_string(BigIntC big_int);

NODISCARD BIGINT_C_LIB_EXPORTED Str bigint_to_string_hex(BigIntC big_int, bool prefix,
                                                         bool add_gaps, bool trim_first_number,
                                                         bool uppercase);

NODISCARD BIGINT_C_LIB_EXPORTED Str bigint_to_string_bin(BigIntC big_int, bool prefix,
                                                         bool add_gaps, bool trim_first_number);

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_add_bigint(BigIntC big_int1, BigIntC big_int2);

NODISCARD BIGINT_C_LIB_EXPORTED BigIntC bigint_sub_bigint(BigIntC big_int1, BigIntC big_int2);

/**
 * @brief This compares two bigints for equality, this is faster than comparing them, as this may
 * return earlier in some cases and perform less checks
 *
 * @param big_int1
 * @param big_int2
 * @return bool
 */
NODISCARD BIGINT_C_LIB_EXPORTED bool bigint_eq_bigint(BigIntC big_int1, BigIntC big_int2);

/**
 * @brief This compares two bigints, return 0 if they are equal, -1 if the first one is less then
 * the second one, 1 if the first one is greater then the second one
 *
 * @param big_int1
 * @param big_int2
 * @return 0, -1 or 1
 */
NODISCARD BIGINT_C_LIB_EXPORTED int8_t bigint_compare_bigint(BigIntC big_int1, BigIntC big_int2);

BIGINT_C_LIB_EXPORTED void bigint_negate(BigIntC* big_int);
