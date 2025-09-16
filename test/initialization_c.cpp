
#include <bigint_c.h>

#include <gtest/gtest.h>

TEST(BigInt, ParseError1) {
	MaybeBigInt maybe_big_int = bigint_from_string("error");

	EXPECT_TRUE(maybe_bigint_is_error(maybe_big_int));

	std::string error = maybe_bigint_get_error(maybe_big_int);

	EXPECT_EQ(error, "invalid character");
}

TEST(BigInt, ParseError2) {
	MaybeBigInt maybe_big_int = bigint_from_string("-");

	EXPECT_TRUE(maybe_bigint_is_error(maybe_big_int));

	std::string error = maybe_bigint_get_error(maybe_big_int);

	EXPECT_EQ(error, "'-' alone is not valid");
}

TEST(BigInt, ParseError3) {
	MaybeBigInt maybe_big_int = bigint_from_string("+");

	EXPECT_TRUE(maybe_bigint_is_error(maybe_big_int));

	std::string error = maybe_bigint_get_error(maybe_big_int);

	EXPECT_EQ(error, "'+' alone is not valid");
}

TEST(BigInt, ParseError4) {
	MaybeBigInt maybe_big_int = bigint_from_string("");

	EXPECT_TRUE(maybe_bigint_is_error(maybe_big_int));

	std::string error = maybe_bigint_get_error(maybe_big_int);

	EXPECT_EQ(error, "empty string is not valid");
}

TEST(BigInt, ParseError5) {
	MaybeBigInt maybe_big_int = bigint_from_string("_0");

	EXPECT_TRUE(maybe_bigint_is_error(maybe_big_int));

	std::string error = maybe_bigint_get_error(maybe_big_int);

	EXPECT_EQ(error, "'_' not allowed at the start");
}
