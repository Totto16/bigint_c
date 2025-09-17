

#pragma once

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

typedef ConstStr MaybeBigIntError;

typedef struct {
	bool error;
	union {
		BigIntC result;
		MaybeBigIntError error;
	} data;
} MaybeBigIntC;

// functions on maybe bigint

NODISCARD bool maybe_bigint_is_error(MaybeBigIntC maybe_big_int);

NODISCARD BigIntC maybe_bigint_get_value(MaybeBigIntC maybe_big_int);

NODISCARD MaybeBigIntError maybe_bigint_get_error(MaybeBigIntC maybe_big_int);

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
NODISCARD MaybeBigIntC maybe_bigint_from_string(ConstStr str);

NODISCARD BigIntC bigint_from_unsigned_number(uint64_t number);

NODISCARD BigIntC bigint_from_signed_number(int64_t number);

void free_bigint(BigIntC* big_int);

void free_bigint_without_reset(BigIntC big_int);

NODISCARD Str bigint_to_string(BigIntC big_int);

NODISCARD BigIntC bigint_add_bigint(BigIntC big_int1, BigIntC big_int2);

NODISCARD BigIntC bigint_sub_bigint(BigIntC big_int1, BigIntC big_int2);

/**
 * @brief This compares two bigints for equality, this is faster than comparing them, as this may
 * return earlier in some cases and perform less checks
 *
 * @param big_int1
 * @param big_int2
 * @return bool
 */
NODISCARD bool bigint_eq_bigint(BigIntC big_int1, BigIntC big_int2);

/**
 * @brief This compares two bigints, return 0 if they are equal, -1 if the first one is less then
 * the second one, 1 if the first one is greater then the second one
 *
 * @param big_int1
 * @param big_int2
 * @return 0, -1 or 1
 */
NODISCARD int8_t bigint_compare_bigint(BigIntC big_int1, BigIntC big_int2);
