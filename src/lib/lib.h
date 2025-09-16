

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "../utils/utils.h"

typedef struct {
	bool positive;
	uint64_t* numbers;
	size_t number_count;
} BigInt;

typedef const char* MaybeBigIntError;

typedef struct {
	bool error;
	union {
		BigInt result;
		MaybeBigIntError error;
	} data;
} MaybeBigInt;

// function on maybe bigint

NODISCARD bool maybe_bigint_is_error(MaybeBigInt maybe_big_int);

NODISCARD BigInt maybe_bigint_get_value(MaybeBigInt maybe_big_int);

NODISCARD MaybeBigIntError maybe_bigint_get_error(MaybeBigInt maybe_big_int);

// normal bigint functions

NODISCARD MaybeBigInt bigint_from_string(const char* str);

void free_bigint(BigInt big_int);

NODISCARD char* bigint_to_string(BigInt big_int);
