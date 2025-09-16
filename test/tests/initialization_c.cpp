
#include <bigint_c.h>

#include "../helper/helper.hpp"
#include "../helper/printer.hpp"

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

	EXPECT_EQ(error, "separator not allowed at the start");
}

TEST(BigInt, ParseError6) {
	MaybeBigInt maybe_big_int = bigint_from_string("!0");

	EXPECT_TRUE(maybe_bigint_is_error(maybe_big_int));

	std::string error = maybe_bigint_get_error(maybe_big_int);

	EXPECT_EQ(error, "invalid character");
}

TEST(BigInt, ParseSuccess1) {
	MaybeBigInt maybe_big_int = bigint_from_string("+0");

	EXPECT_FALSE(maybe_bigint_is_error(maybe_big_int));

	BigInt big_int = maybe_bigint_get_value(maybe_big_int);

	BigIntCPP result = BigIntCPP(true, { 0ULL });

	EXPECT_EQ(big_int, result);

	free_bigint(big_int);
}

TEST(BigInt, ParseSuccess2) {
	MaybeBigInt maybe_big_int = bigint_from_string("-1");

	EXPECT_FALSE(maybe_bigint_is_error(maybe_big_int));

	BigInt big_int = maybe_bigint_get_value(maybe_big_int);

	BigIntCPP result = BigIntCPP(false, { 1ULL });

	EXPECT_EQ(big_int, result);

	free_bigint(big_int);
}

TEST(BigInt, ParseSuccess3) {
	MaybeBigInt maybe_big_int = bigint_from_string("-1_0");

	EXPECT_FALSE(maybe_bigint_is_error(maybe_big_int));

	BigInt big_int = maybe_bigint_get_value(maybe_big_int);

	BigIntCPP result = BigIntCPP(false, { 10ULL });

	EXPECT_EQ(big_int, result);

	free_bigint(big_int);
}

TEST(BigInt, ParseSuccess4) {
	MaybeBigInt maybe_big_int = bigint_from_string("+0021");

	EXPECT_FALSE(maybe_bigint_is_error(maybe_big_int));

	BigInt big_int = maybe_bigint_get_value(maybe_big_int);

	BigIntCPP result = BigIntCPP(true, { 21ULL });

	EXPECT_EQ(big_int, result);

	free_bigint(big_int);
}

TEST(BigInt, ParseSuccess5) {
	MaybeBigInt maybe_big_int = bigint_from_string("-10_00'00.000,00");

	EXPECT_FALSE(maybe_bigint_is_error(maybe_big_int));

	BigInt big_int = maybe_bigint_get_value(maybe_big_int);

	BigIntCPP result = BigIntCPP(false, { 10000000000ULL });

	EXPECT_EQ(big_int, result);

	free_bigint(big_int);
}

TEST(BigInt, ParseSuccessLargeNumbers) {

	std::vector<std::string> tests{
		"1234567890123456789012345678901234567890123456789012345678901234567890",
		"-11234567890123456789012345678901234567890123456789012345678901234567890",
		"11234567890123456789012345678901234567890123456789012345678901234567890",
		"24324532532563264267342634556842562345613297843267583265789324659324673284732894563981"
		"4561"
		"39785613294561329856328951326589326593285619745274152487134561328456132789453267845132"
		"4671"
		"245476124714912461294128412412412041204020202020202022020202",
		"21412513613241245132512512",
		"-12",
		"-13250891325632415132851327653205672349642764295634279051326750329653285642516950265784258"
		"342756342875346857346583456342875634256257263526347891326841364578916478134613784612784612"
		"47812647812641278461278461247812648126478124612461274612841241",
		std::to_string(std::numeric_limits<uint64_t>::max())
	};

	for(const std::string& test : tests) {

		MaybeBigInt maybe_big_int = bigint_from_string(test.c_str());

		EXPECT_FALSE(maybe_bigint_is_error(maybe_big_int));

		BigInt c_result = maybe_bigint_get_value(maybe_big_int);

		BigIntCPP cpp_result = BigIntCPP(test);

		EXPECT_EQ(c_result, cpp_result) << "Input string: " << test;

		free_bigint(c_result);
	}
}
