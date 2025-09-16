
#include <bigint_c.h>

#include <gtest/gtest.h>

TEST(BigInt, ParseError) {
	MaybeBigInt maybe_big_int = bigint_from_string("error");

	EXPECT_TRUE(maybe_bigint_is_error(maybe_big_int));

	std::string error = maybe_bigint_get_error(maybe_big_int);

	EXPECT_EQ(error, "test");
}
