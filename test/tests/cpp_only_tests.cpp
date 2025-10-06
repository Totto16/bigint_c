

#define BIGINT_C_CPP_HIDE_C_LIB_FNS_AND_TYPES_IN_CPP
#define BIGINT_C_CPP_ACCESS_TO_UNDERLYING_C_DATA
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

TEST(BigInt, BigIntUnderlying) {
	std::string test = "-13250891325632415132851327653205672349642764295634279051326750329653285642"
	                   "516950265784258";

	const BigInt big_int = BigInt::get_from_string(test).value();

	const auto& value1 = big_int.underlying();

	const auto& value2 = static_cast<decltype(value1)>(big_int);

	EXPECT_EQ(&value1, &value2) << "Input string: " << test;
}

TEST(BigInt, IntegerToStringConversionOperator) {

	std::string test = "-13250891325632415132851327653205672349642764295634279051326750329653285642"
	                   "516950265784258";

	BigInt big_int = BigInt::get_from_string(test).value();

	BigIntTest cpp_result = BigIntTest(test);

	std::string bigint_c_str = static_cast<std::string>(big_int);

	EXPECT_EQ(test, bigint_c_str) << "Input string: " << test;
}

TEST(BigInt, IntegertoStringOsStream) {
	struct Option {
		bool prefix;
		bool add_gaps;
		bool trim_first_number;
		bool uppercase;
	};

	std::vector<BigInt> tests{};

	tests.emplace_back((int64_t)-1LL);
	tests.emplace_back((int64_t)-2LL);
	tests.emplace_back((uint64_t)2ULL);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value());
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value());

	tests.emplace_back(BigInt::get_from_string("0").value());

	tests.emplace_back(BigInt::get_from_string("+2131215135135132515135").value());

	tests.emplace_back(BigInt::get_from_string("-2131215135135").value());

	tests.emplace_back(std::numeric_limits<uint64_t>::max());

	tests.emplace_back(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max());

	std::vector<Option> test_options{
		{
		    Option{
		        .prefix = true, .add_gaps = true, .trim_first_number = true, .uppercase = true },
		},
		{
		    Option{
		        .prefix = true, .add_gaps = true, .trim_first_number = false, .uppercase = true },

		},
		{
		    Option{
		        .prefix = true, .add_gaps = false, .trim_first_number = true, .uppercase = true },
		},
		{
		    Option{
		        .prefix = true, .add_gaps = false, .trim_first_number = false, .uppercase = true },

		},
		{
		    Option{
		        .prefix = false, .add_gaps = true, .trim_first_number = true, .uppercase = true },
		},
		{
		    Option{
		        .prefix = false, .add_gaps = true, .trim_first_number = false, .uppercase = true },

		},
		{
		    Option{
		        .prefix = false, .add_gaps = false, .trim_first_number = true, .uppercase = true },
		},
		{
		    Option{
		        .prefix = false, .add_gaps = false, .trim_first_number = false, .uppercase = true },
		},
		{
		    Option{
		        .prefix = true, .add_gaps = true, .trim_first_number = true, .uppercase = false },
		},
		{
		    Option{
		        .prefix = true, .add_gaps = true, .trim_first_number = false, .uppercase = false },

		},
		{
		    Option{
		        .prefix = true, .add_gaps = false, .trim_first_number = true, .uppercase = false },
		},
		{
		    Option{
		        .prefix = true, .add_gaps = false, .trim_first_number = false, .uppercase = false },

		},
		{
		    Option{
		        .prefix = false, .add_gaps = true, .trim_first_number = true, .uppercase = false },
		},
		{
		    Option{
		        .prefix = false, .add_gaps = true, .trim_first_number = false, .uppercase = false },

		},
		{
		    Option{
		        .prefix = false, .add_gaps = false, .trim_first_number = true, .uppercase = false },
		},
		{
		    Option{ .prefix = false,
		            .add_gaps = false,
		            .trim_first_number = false,
		            .uppercase = false },

		},
	};

	for(const BigInt& bigint : tests) {

		for(const auto& option : test_options) {

			const auto& [prefix, add_gaps, trim_first_number, uppercase] = option;

			for(size_t i = 0; i < 3; ++i) {

				std::string normal_result;
				std::stringstream str_stream = {};

				switch(i) {
					case 0: {
						normal_result = bigint.to_string_bin(prefix, add_gaps, trim_first_number);
						str_stream << bigint::ios::bin;
						break;
					}
					case 1: {
						normal_result = bigint.to_string();
						str_stream << std::ios_base::dec;
						break;
					}
					case 2:
					default: {
						normal_result =
						    bigint.to_string_hex(prefix, add_gaps, trim_first_number, uppercase);
						str_stream << std::ios_base::hex;
						break;
					}
				}

				{ // apply options

					if(prefix) {
						str_stream << std::ios_base::showbase;
					} else {
						str_stream << std::noshowbase;
					}

					if(add_gaps) {
						str_stream << bigint::ios::add_gaps;
					} else {
						str_stream << bigint::ios::no_add_gaps;
					}

					if(trim_first_number) {
						str_stream << bigint::ios::trim_first_number;
					} else {
						str_stream << bigint::ios::no_trim_first_number;
					}

					if(uppercase) {
						str_stream << std::ios_base::uppercase;
					} else {
						str_stream << std::nouppercase;
					}
				}

				str_stream << static_cast<const BigInt&>(bigint);

				std::string stream_result = str_stream.str();

				EXPECT_EQ(stream_result, normal_result)
				    << "Input value: " << bigint << "options: " << (prefix ? "prefix" : "no-prefix")
				    << " " << (add_gaps ? "add_gaps" : "no-gaps") << " "
				    << (trim_first_number ? "trim_first_number" : "no-trim") << " "
				    << (uppercase ? "uppercase" : " lowercase");
				;
			}
		}
	}
}

// TODO: test other cpp only features
