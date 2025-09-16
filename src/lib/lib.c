

#include "./lib.h"

// function on maybe bigint

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

// TODO: remove
#include <stdio.h>

NODISCARD MaybeBigInt bigint_from_string(const char* str) {
	UNUSED(str);
	UNREACHABLE();
}

void free_bigint(BigInt big_int) {
	UNUSED(big_int);
	UNREACHABLE();
}

NODISCARD char* bigint_to_string(BigInt big_int) {
	UNUSED(big_int);
	UNREACHABLE();
}
