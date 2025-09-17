

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "../utils/utils.h"

typedef struct {
	bool positive;
	uint64_t* numbers;
	size_t number_count;
} BigIntImpl;

#ifndef __cplusplus
typedef BigIntImpl BigInt;

#endif

typedef char StrType;

typedef const StrType* ConstStr;

typedef StrType* Str;

typedef ConstStr MaybeBigIntError;

typedef struct {
	bool error;
	union {
		BigIntImpl result;
		MaybeBigIntError error;
	} data;
} MaybeBigInt;

// functions on maybe bigint

NODISCARD bool maybe_bigint_is_error(MaybeBigInt maybe_big_int);

NODISCARD BigIntImpl maybe_bigint_get_value(MaybeBigInt maybe_big_int);

NODISCARD MaybeBigIntError maybe_bigint_get_error(MaybeBigInt maybe_big_int);

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
 * @param str
 * @return MaybeBigInt
 */
NODISCARD MaybeBigInt maybe_bigint_from_string(ConstStr str);

NODISCARD BigIntImpl bigint_from_unsigned_number(uint64_t number);

NODISCARD BigIntImpl bigint_from_signed_number(int64_t number);

void free_bigint(BigIntImpl* big_int);

void free_bigint_without_reset(BigIntImpl big_int);

NODISCARD Str bigint_to_string(BigIntImpl big_int);

NODISCARD BigIntImpl bigint_add_bigint(BigIntImpl big_int1, BigIntImpl big_int2);

NODISCARD BigIntImpl bigint_sub_bigint(BigIntImpl big_int1, BigIntImpl big_int2);

NODISCARD uint8_t bigint_compare_bigint(BigIntImpl big_int1, BigIntImpl big_int2);
