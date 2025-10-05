

#include <bigint_c.h>

#include <gtest/gtest.h>

#include "../helper/helper.hpp"
#include "../helper/matcher.hpp"
#include "../helper/printer.hpp"

TEST(BigIntCFuncs, FreeAllowsNull) {

	free_bigint(nullptr);
	SUCCEED();
}

TEST(BigIntCFuncs, InvalidNumbersFree) {

	BigIntC big_int_c = { .positive = true, .numbers = nullptr, .number_count = 0 };
	free_bigint_without_reset(big_int_c);
	free_bigint(&big_int_c);

	EXPECT_EQ(big_int_c.numbers, nullptr);
}

TEST(BigIntCFuncs, FreeBehaviour1) {

	BigIntC big_int_c = bigint_from_unsigned_number(1ULL);
	free_bigint(&big_int_c);

	EXPECT_EQ(big_int_c.numbers, nullptr);
}

TEST(BigIntCFuncs, FreeBehaviour2) {

	BigIntC big_int_c = bigint_from_unsigned_number(1ULL);
	free_bigint_without_reset(big_int_c);

	EXPECT_NE(big_int_c.numbers, nullptr);
}

TEST(BigIntCFuncs, StrReturnsNullOnInvalidInput) {

	BigIntC big_int_c = { .positive = true, .numbers = nullptr, .number_count = 0 };
	char* str = bigint_to_string(big_int_c);

	EXPECT_EQ(str, nullptr);
}

TEST(BigIntCFuncs, HexStrReturnsNullOnInvalidInput) {

	BigIntC big_int_c = { .positive = true, .numbers = nullptr, .number_count = 0 };
	char* str = bigint_to_string_hex(big_int_c, true, true, true, true);

	EXPECT_EQ(str, nullptr);
}

TEST(BigIntCFuncs, BinStrReturnsNullOnInvalidInput) {

	BigIntC big_int_c = { .positive = true, .numbers = nullptr, .number_count = 0 };
	char* str = bigint_to_string_bin(big_int_c, true, true, true);

	EXPECT_EQ(str, nullptr);
}

// TODO: input invalid BigInts into all public functions an see how the behave, make the behavior
// expected, e.g. that negate doesn't care about the amount or numbers being NULL, or that it does
// care
