

#define BIGINT_C_HIDE_C_LIB_FNS_AND_TYPES_IN_CPP
#include <bigint_c.h>

#include <gtest/gtest.h>

#include "../helper/helper.hpp"
#include "../helper/matcher.hpp"
#include "../helper/printer.hpp"

TEST(BigInt, ParseError1) {
	std::expected<BigInt, std::string> maybe_big_int = BigInt::get_from_string("error");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(), "invalid character");
}

TEST(BigInt, ParseError2) {
	std::expected<BigInt, std::string> maybe_big_int = BigInt::get_from_string("-");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(), "'-' alone is not valid");
}

TEST(BigInt, ParseError3) {
	std::expected<BigInt, std::string> maybe_big_int = BigInt::get_from_string("+");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(), "'+' alone is not valid");
}

TEST(BigInt, ParseError4) {
	std::expected<BigInt, std::string> maybe_big_int = BigInt::get_from_string("");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(), "empty string is not valid");
}

TEST(BigInt, ParseError5) {
	std::expected<BigInt, std::string> maybe_big_int = BigInt::get_from_string("_0");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(), "separator not allowed at the start");
}

TEST(BigInt, ParseError6) {
	std::expected<BigInt, std::string> maybe_big_int = BigInt::get_from_string("!0");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(), "invalid character");
}

TEST(BigInt, ParseSuccess1) {
	std::expected<BigInt, std::string> maybe_big_int = BigInt::get_from_string("+0");

	ASSERT_THAT(maybe_big_int, ExpectedHasValue());

	BigInt big_int = std::move(maybe_big_int.value());

	BigIntTest result = BigIntTest(true, { 0ULL });

	EXPECT_EQ(big_int, result);
}

TEST(BigInt, ParseSuccess2) {
	std::expected<BigInt, std::string> maybe_big_int = BigInt::get_from_string("-1");

	ASSERT_THAT(maybe_big_int, ExpectedHasValue());

	BigInt big_int = std::move(maybe_big_int.value());

	BigIntTest result = BigIntTest(false, { 1ULL });

	EXPECT_EQ(big_int, result);
}

TEST(BigInt, ParseSuccess3) {
	std::expected<BigInt, std::string> maybe_big_int = BigInt::get_from_string("-1_0");

	ASSERT_THAT(maybe_big_int, ExpectedHasValue());

	BigInt big_int = std::move(maybe_big_int.value());

	BigIntTest result = BigIntTest(false, { 10ULL });

	EXPECT_EQ(big_int, result);
}

TEST(BigInt, ParseSuccess4) {
	std::expected<BigInt, std::string> maybe_big_int = BigInt::get_from_string("+0021");

	ASSERT_THAT(maybe_big_int, ExpectedHasValue());

	BigInt big_int = std::move(maybe_big_int.value());

	BigIntTest result = BigIntTest(true, { 21ULL });

	EXPECT_EQ(big_int, result);
}

TEST(BigInt, ParseSuccess5) {
	std::expected<BigInt, std::string> maybe_big_int = BigInt::get_from_string("-10_00'00.000,00");

	ASSERT_THAT(maybe_big_int, ExpectedHasValue());

	BigInt big_int = std::move(maybe_big_int.value());

	BigIntTest result = BigIntTest(false, { 10000000000ULL });

	EXPECT_EQ(big_int, result);
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

		std::expected<BigInt, std::string> maybe_big_int = BigInt::get_from_string(test.c_str());

		ASSERT_THAT(maybe_big_int, ExpectedHasValue());

		BigInt c_result = std::move(maybe_big_int.value());

		BigIntTest cpp_result = BigIntTest(test);

		EXPECT_EQ(c_result, cpp_result) << "Input string: " << test;
	}
}

TEST(BigInt, IntegerToBigIntU) {
	std::vector<uint64_t> tests{ 4351325ULL, 0ULL, 1313131ULL,
		                         std::numeric_limits<uint64_t>::max() };

	for(const uint64_t& test : tests) {

		BigInt c_result = BigInt(test);

		BigIntTest cpp_result = BigIntTest(test);

		EXPECT_EQ(c_result, cpp_result) << "Input number: " << test;
	}
}

TEST(BigInt, IntegerToBigIntI) {
	std::vector<int64_t> tests{ 4351325LL, 0ULL, -1313131LL, std::numeric_limits<int64_t>::max(),
		                        std::numeric_limits<int64_t>::min() };

	for(const int64_t& test : tests) {

		BigInt c_result = BigInt(test);

		BigIntTest cpp_result = BigIntTest(test);

		EXPECT_EQ(c_result, cpp_result) << "Input number: " << test;
	}
}

TEST(BigInt, IntegerEqComparison) {
	using TestType = std::tuple<BigInt, BigInt, bool>;

	std::vector<TestType> tests{};

	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(BigInt{ (int64_t)-1LL },
	                                                         BigInt{ (uint64_t)1ULL }, false));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(BigInt{ (int64_t)-1LL },
	                                                         BigInt{ (int64_t)-2LL }, false));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(BigInt{ (uint64_t)1ULL },
	                                                         BigInt{ (uint64_t)2ULL }, false));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("351326324642346363634634634636363").value(),
	    BigInt{ (uint64_t)2ULL }, false));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    false));

	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    true));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(BigInt{ (int64_t)-1LL },
	                                                         BigInt{ (int64_t)-1LL }, true));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(BigInt{ (uint64_t)2ULL },
	                                                         BigInt{ (uint64_t)2ULL }, true));

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const bool actual_result = value1 == value2;

		EXPECT_EQ(actual_result, result_expected) << "Input values: " << value1 << ", " << value2;
	}
}

TEST(BigInt, IntegerLtComparison) {
	using TestType = std::tuple<BigInt, BigInt, bool>;

	std::vector<TestType> tests{};

	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(BigInt{ (int64_t)-1LL },
	                                                         BigInt{ (uint64_t)1ULL }, true));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(BigInt{ (int64_t)-1LL },
	                                                         BigInt{ (int64_t)-2LL }, false));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(BigInt{ (uint64_t)1ULL },
	                                                         BigInt{ (uint64_t)2ULL }, true));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("351326324642346363634634634636363").value(),
	    BigInt{ (uint64_t)2ULL }, false));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    true));

	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    false));

	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    false));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(BigInt{ (int64_t)-1LL },
	                                                         BigInt{ (int64_t)-1LL }, false));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(BigInt{ (uint64_t)2ULL },
	                                                         BigInt{ (uint64_t)2ULL }, false));

	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("-0").value(), BigInt::get_from_string("+0").value(), false));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("+0").value(), BigInt::get_from_string("-0").value(), false));

	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("+1").value(),
	    BigInt::get_from_string("-2131215135135132515135").value(), false));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("-1").value(),
	    BigInt::get_from_string("+2131215135135132515135").value(), true));

	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("-2131215135135132515135").value(),
	    BigInt::get_from_string("+1").value(), true));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("+2131215135135132515135").value(),
	    BigInt::get_from_string("-1").value(), false));

	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("+0").value(), BigInt::get_from_string("-2131215135135").value(),
	    false));
	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("-0").value(), BigInt::get_from_string("+2131215135135").value(),
	    true));

	tests.emplace_back(
	    std::make_tuple<BigInt, BigInt, bool>(BigInt::get_from_string("-2131215135135").value(),
	                                          BigInt::get_from_string("+0").value(), true));
	tests.emplace_back(
	    std::make_tuple<BigInt, BigInt, bool>(BigInt::get_from_string("+2131215135135").value(),
	                                          BigInt::get_from_string("-0").value(), false));

	tests.emplace_back(std::make_tuple<BigInt, BigInt, bool>(
	    BigInt::get_from_string("+1").value(),
	    BigInt::get_from_string("+2131215135135132515135").value(), true));

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const bool actual_result = value1 < value2;

		EXPECT_EQ(actual_result, result_expected) << "Input values: " << value1 << ", " << value2;
	}
}
