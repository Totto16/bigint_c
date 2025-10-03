

#define BIGINT_C_CPP_HIDE_C_LIB_FNS_AND_TYPES_IN_CPP
#include <bigint_c.h>

#include <gtest/gtest.h>

#include "../helper/helper.hpp"
#include "../helper/matcher.hpp"
#include "../helper/printer.hpp"

#include <unordered_set>

TEST(BigInt, IntegerToStringStd) {

	std::string test = "-13250891325632415132851327653205672349642764295634279051326750329653285642"
	                   "516950265784258";

	BigInt big_int = BigInt::get_from_string(test).value();

	BigIntTest cpp_result = BigIntTest(test);

	std::string bigint_c_str = std::to_string(big_int);

	EXPECT_EQ(test, bigint_c_str) << "Input string: " << test;
}

// TODO: tests hash

TEST(BigInt, IntegerHash) {
	BigInt a1{ (int64_t)-1LL };
	BigInt a2{ (int64_t)-1LL };
	BigInt b{ (uint64_t)1ULL };
	BigInt c{ (uint64_t)3255325ULL, (uint64_t)3255325ULL, (uint64_t)3255325ULL };

	std::hash<BigInt> hasher;

	EXPECT_EQ(a1, a2);
	EXPECT_EQ(hasher(a1), hasher(a2));

	EXPECT_NE(a1, b);
	EXPECT_NE(a1, c);
	EXPECT_NE(b, c);

	// Test: Usable in unordered_set
	std::unordered_set<BigInt> ints;
	ints.insert(a1.copy());
	ints.insert(c.copy());
	ASSERT_NE(ints.find(a1), ints.end());
	ASSERT_NE(ints.find(c), ints.end());
	ASSERT_EQ(ints.find(b), ints.end());
}

TEST(BigInt, BigIntLiteral1) {

	std::string test = "-13250891325632415132851327653205672349642764295634279051326750329653285642"
	                   "516950265784258";

	BigInt big_int = BigInt::get_from_string(test).value();

	constexpr const BigIntConstExpr cpp_literal =
	    "-13250891325632415132851327653205672349642764295634279051326750329653285642"
	    "516950265784258"_n;

	// as BigInt always needs allocations, the final value is constructed at runtime, but only a
	// malloc and memcpy needs to be done, so it's fatser than parsing, as that is done at
	// compile time.
	const BigInt runtime_converted_literal = cpp_literal;

	EXPECT_EQ(big_int, runtime_converted_literal) << "Input string: " << test;
}

TEST(BigInt, BigIntLiteral2) {
	std::string test = "-13250891325632415132851327653205672349642764295634279051326750329653285642"
	                   "516950265784258"
	                   "12235335";

	BigInt big_int = BigInt::get_from_string(test).value();

	constexpr const BigIntConstExpr cpp_literal =
	    "-13250891325632415132851327653205672349642764295634279051326750329653285642516950265784258"
	    "12235335"_n;

	// as BigInt always needs allocations, the final value is constructed at runtime, but only a
	// malloc and memcpy needs to be done, so it's fatser than parsing, as that is done at
	// compile time.
	const BigInt runtime_converted_literal = cpp_literal;

	EXPECT_EQ(big_int, runtime_converted_literal) << "Input string: " << test;
}

// TODO: test other cpp only features
