

#define BIGINT_C_CPP_HIDE_C_LIB_FNS_AND_TYPES_IN_CPP
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

TEST(BigInt, ParseError7) {
	std::expected<BigInt, std::string> maybe_big_int = BigInt::get_from_string("-0");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(), "-0 is not allowed");
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

		std::expected<BigInt, std::string> maybe_big_int = BigInt::get_from_string(test);

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

	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (uint64_t)1ULL }, false);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-2LL }, false);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-3LL }, false);
	tests.emplace_back(BigInt{ (int64_t)-3LL }, BigInt{ (int64_t)-1LL }, false);
	tests.emplace_back(BigInt{ (uint64_t)1ULL }, BigInt{ (uint64_t)2ULL }, false);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value(),
	                   BigInt{ (uint64_t)2ULL }, false);
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    false);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    true);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-1LL }, true);
	tests.emplace_back(BigInt{ (uint64_t)2ULL }, BigInt{ (uint64_t)2ULL }, true);

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const bool actual_result = value1 == value2;

		EXPECT_EQ(actual_result, result_expected) << "Input values: " << value1 << ", " << value2;
	}
}

TEST(BigInt, IntegerNeqComparison) {
	using TestType = std::tuple<BigInt, BigInt, bool>;

	std::vector<TestType> tests{};

	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (uint64_t)1ULL }, true);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-2LL }, true);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-3LL }, true);
	tests.emplace_back(BigInt{ (int64_t)-3LL }, BigInt{ (int64_t)-1LL }, true);
	tests.emplace_back(BigInt{ (uint64_t)1ULL }, BigInt{ (uint64_t)2ULL }, true);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value(),
	                   BigInt{ (uint64_t)2ULL }, true);
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    true);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    false);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-1LL }, false);
	tests.emplace_back(BigInt{ (uint64_t)2ULL }, BigInt{ (uint64_t)2ULL }, false);

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const bool actual_result = value1 != value2;

		EXPECT_EQ(actual_result, result_expected) << "Input values: " << value1 << ", " << value2;
	}
}

TEST(BigInt, IntegerGeneralComparison) {
	using TestType = std::tuple<BigInt, BigInt, std::strong_ordering>;

	std::vector<TestType> tests{};

	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (uint64_t)1ULL },
	                   std::strong_ordering::less);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-2LL },
	                   std::strong_ordering::greater);
	tests.emplace_back(BigInt{ (uint64_t)1ULL }, BigInt{ (uint64_t)2ULL },
	                   std::strong_ordering::less);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value(),
	                   BigInt{ (uint64_t)2ULL }, std::strong_ordering::greater);
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    std::strong_ordering::less);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    std::strong_ordering::equal);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    std::strong_ordering::greater);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-1LL },
	                   std::strong_ordering::equal);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-3LL },
	                   std::strong_ordering::greater);
	tests.emplace_back(BigInt{ (int64_t)-3LL }, BigInt{ (int64_t)-1LL },
	                   std::strong_ordering::less);
	tests.emplace_back(BigInt{ (uint64_t)2ULL }, BigInt{ (uint64_t)2ULL },
	                   std::strong_ordering::equal);

	tests.emplace_back(BigInt::get_from_string("0").value(), BigInt::get_from_string("+0").value(),
	                   std::strong_ordering::equal);
	tests.emplace_back(BigInt::get_from_string("+0").value(), BigInt::get_from_string("0").value(),
	                   std::strong_ordering::equal);

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("-2131215135135132515135").value(),
	                   std::strong_ordering::greater);
	tests.emplace_back(BigInt::get_from_string("-1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value(),
	                   std::strong_ordering::less);

	tests.emplace_back(BigInt::get_from_string("-2131215135135132515135").value(),
	                   BigInt::get_from_string("+1").value(), std::strong_ordering::less);
	tests.emplace_back(BigInt::get_from_string("+2131215135135132515135").value(),
	                   BigInt::get_from_string("-1").value(), std::strong_ordering::greater);

	tests.emplace_back(BigInt::get_from_string("+0").value(),
	                   BigInt::get_from_string("-2131215135135").value(),
	                   std::strong_ordering::greater);
	tests.emplace_back(BigInt::get_from_string("0").value(),
	                   BigInt::get_from_string("+2131215135135").value(),
	                   std::strong_ordering::less);

	tests.emplace_back(BigInt::get_from_string("-2131215135135").value(),
	                   BigInt::get_from_string("+0").value(), std::strong_ordering::less);
	tests.emplace_back(BigInt::get_from_string("+2131215135135").value(),
	                   BigInt::get_from_string("0").value(), std::strong_ordering::greater);

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value(),
	                   std::strong_ordering::less);

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const std::strong_ordering actual_result = value1 <=> value2;

		EXPECT_EQ(actual_result, result_expected) << "Input values: " << value1 << ", " << value2;
	}
}

TEST(BigInt, IntegerLtComparison) {
	using TestType = std::tuple<BigInt, BigInt, bool>;

	std::vector<TestType> tests{};

	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (uint64_t)1ULL }, true);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-2LL }, false);
	tests.emplace_back(BigInt{ (uint64_t)1ULL }, BigInt{ (uint64_t)2ULL }, true);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value(),
	                   BigInt{ (uint64_t)2ULL }, false);
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    true);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    false);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    false);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-1LL }, false);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-3LL }, false);
	tests.emplace_back(BigInt{ (int64_t)-3LL }, BigInt{ (int64_t)-1LL }, true);
	tests.emplace_back(BigInt{ (uint64_t)2ULL }, BigInt{ (uint64_t)2ULL }, false);

	tests.emplace_back(BigInt::get_from_string("0").value(), BigInt::get_from_string("+0").value(),
	                   false);
	tests.emplace_back(BigInt::get_from_string("+0").value(), BigInt::get_from_string("0").value(),
	                   false);

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("-2131215135135132515135").value(), false);
	tests.emplace_back(BigInt::get_from_string("-1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value(), true);

	tests.emplace_back(BigInt::get_from_string("-2131215135135132515135").value(),
	                   BigInt::get_from_string("+1").value(), true);
	tests.emplace_back(BigInt::get_from_string("+2131215135135132515135").value(),
	                   BigInt::get_from_string("-1").value(), false);

	tests.emplace_back(BigInt::get_from_string("+0").value(),
	                   BigInt::get_from_string("-2131215135135").value(), false);
	tests.emplace_back(BigInt::get_from_string("0").value(),
	                   BigInt::get_from_string("+2131215135135").value(), true);

	tests.emplace_back(BigInt::get_from_string("-2131215135135").value(),
	                   BigInt::get_from_string("+0").value(), true);
	tests.emplace_back(BigInt::get_from_string("+2131215135135").value(),
	                   BigInt::get_from_string("0").value(), false);

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value(), true);

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const bool actual_result = value1 < value2;

		EXPECT_EQ(actual_result, result_expected) << "Input values: " << value1 << ", " << value2;
	}
}

TEST(BigInt, IntegerLteComparison) {
	using TestType = std::tuple<BigInt, BigInt, bool>;

	std::vector<TestType> tests{};

	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (uint64_t)1ULL }, true);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-2LL }, false);
	tests.emplace_back(BigInt{ (uint64_t)1ULL }, BigInt{ (uint64_t)2ULL }, true);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value(),
	                   BigInt{ (uint64_t)2ULL }, false);
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    true);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    true);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    false);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-1LL }, true);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-3LL }, false);
	tests.emplace_back(BigInt{ (int64_t)-3LL }, BigInt{ (int64_t)-1LL }, true);
	tests.emplace_back(BigInt{ (uint64_t)2ULL }, BigInt{ (uint64_t)2ULL }, true);

	tests.emplace_back(BigInt::get_from_string("0").value(), BigInt::get_from_string("+0").value(),
	                   true);
	tests.emplace_back(BigInt::get_from_string("+0").value(), BigInt::get_from_string("0").value(),
	                   true);

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("-2131215135135132515135").value(), false);
	tests.emplace_back(BigInt::get_from_string("-1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value(), true);

	tests.emplace_back(BigInt::get_from_string("-2131215135135132515135").value(),
	                   BigInt::get_from_string("+1").value(), true);
	tests.emplace_back(BigInt::get_from_string("+2131215135135132515135").value(),
	                   BigInt::get_from_string("-1").value(), false);

	tests.emplace_back(BigInt::get_from_string("+0").value(),
	                   BigInt::get_from_string("-2131215135135").value(), false);
	tests.emplace_back(BigInt::get_from_string("0").value(),
	                   BigInt::get_from_string("+2131215135135").value(), true);

	tests.emplace_back(BigInt::get_from_string("-2131215135135").value(),
	                   BigInt::get_from_string("+0").value(), true);
	tests.emplace_back(BigInt::get_from_string("+2131215135135").value(),
	                   BigInt::get_from_string("0").value(), false);

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value(), true);

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const bool actual_result = value1 <= value2;

		EXPECT_EQ(actual_result, result_expected) << "Input values: " << value1 << ", " << value2;
	}
}

TEST(BigInt, IntegerGtComparison) {
	using TestType = std::tuple<BigInt, BigInt, bool>;

	std::vector<TestType> tests{};

	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (uint64_t)1ULL }, false);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-2LL }, true);
	tests.emplace_back(BigInt{ (uint64_t)1ULL }, BigInt{ (uint64_t)2ULL }, false);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value(),
	                   BigInt{ (uint64_t)2ULL }, true);
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    false);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    false);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    true);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-1LL }, false);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-3LL }, true);
	tests.emplace_back(BigInt{ (int64_t)-3LL }, BigInt{ (int64_t)-1LL }, false);
	tests.emplace_back(BigInt{ (uint64_t)2ULL }, BigInt{ (uint64_t)2ULL }, false);

	tests.emplace_back(BigInt::get_from_string("0").value(), BigInt::get_from_string("+0").value(),
	                   false);
	tests.emplace_back(BigInt::get_from_string("+0").value(), BigInt::get_from_string("0").value(),
	                   false);

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("-2131215135135132515135").value(), true);
	tests.emplace_back(BigInt::get_from_string("-1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value(), false);

	tests.emplace_back(BigInt::get_from_string("-2131215135135132515135").value(),
	                   BigInt::get_from_string("+1").value(), false);
	tests.emplace_back(BigInt::get_from_string("+2131215135135132515135").value(),
	                   BigInt::get_from_string("-1").value(), true);

	tests.emplace_back(BigInt::get_from_string("+0").value(),
	                   BigInt::get_from_string("-2131215135135").value(), true);
	tests.emplace_back(BigInt::get_from_string("0").value(),
	                   BigInt::get_from_string("+2131215135135").value(), false);

	tests.emplace_back(BigInt::get_from_string("-2131215135135").value(),
	                   BigInt::get_from_string("+0").value(), false);
	tests.emplace_back(BigInt::get_from_string("+2131215135135").value(),
	                   BigInt::get_from_string("0").value(), true);

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value(), false);

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const bool actual_result = value1 > value2;

		EXPECT_EQ(actual_result, result_expected) << "Input values: " << value1 << ", " << value2;
	}
}

TEST(BigInt, IntegerGteComparison) {
	using TestType = std::tuple<BigInt, BigInt, bool>;

	std::vector<TestType> tests{};

	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (uint64_t)1ULL }, false);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-2LL }, true);
	tests.emplace_back(BigInt{ (uint64_t)1ULL }, BigInt{ (uint64_t)2ULL }, false);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value(),
	                   BigInt{ (uint64_t)2ULL }, true);
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    false);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    true);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    true);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-1LL }, true);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-3LL }, true);
	tests.emplace_back(BigInt{ (int64_t)-3LL }, BigInt{ (int64_t)-1LL }, false);
	tests.emplace_back(BigInt{ (uint64_t)2ULL }, BigInt{ (uint64_t)2ULL }, true);

	tests.emplace_back(BigInt::get_from_string("0").value(), BigInt::get_from_string("+0").value(),
	                   true);
	tests.emplace_back(BigInt::get_from_string("+0").value(), BigInt::get_from_string("0").value(),
	                   true);

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("-2131215135135132515135").value(), true);
	tests.emplace_back(BigInt::get_from_string("-1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value(), false);

	tests.emplace_back(BigInt::get_from_string("-2131215135135132515135").value(),
	                   BigInt::get_from_string("+1").value(), false);
	tests.emplace_back(BigInt::get_from_string("+2131215135135132515135").value(),
	                   BigInt::get_from_string("-1").value(), true);

	tests.emplace_back(BigInt::get_from_string("+0").value(),
	                   BigInt::get_from_string("-2131215135135").value(), true);
	tests.emplace_back(BigInt::get_from_string("0").value(),
	                   BigInt::get_from_string("+2131215135135").value(), false);

	tests.emplace_back(BigInt::get_from_string("-2131215135135").value(),
	                   BigInt::get_from_string("+0").value(), false);
	tests.emplace_back(BigInt::get_from_string("+2131215135135").value(),
	                   BigInt::get_from_string("0").value(), true);

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value(), false);

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const bool actual_result = value1 >= value2;

		EXPECT_EQ(actual_result, result_expected) << "Input values: " << value1 << ", " << value2;
	}
}

TEST(BigInt, IntegerNegate) {
	std::vector<std::pair<BigInt, BigInt>> tests{};

	tests.emplace_back(BigInt::get_from_string("+0").value(), BigInt::get_from_string("0").value());

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("-1").value());
	tests.emplace_back(BigInt::get_from_string("+13532532637355324813252495259").value(),
	                   BigInt::get_from_string("-13532532637355324813252495259").value());
	tests.emplace_back(BigInt::get_from_string("-384324_132132_3123123_3").value(),
	                   BigInt::get_from_string("+384324_132132_3123123_3").value());
	tests.emplace_back(BigInt::get_from_string("-112").value(),
	                   BigInt::get_from_string("+112").value());
	tests.emplace_back(BigInt::get_from_string("-53427592652352534267532769352786325678352768352673"
	                                           "526785267526783526783526735267352673528")
	                       .value(),
	                   BigInt::get_from_string("+53427592652352534267532769352786325678352768352673"
	                                           "526785267526783526783526735267352673528")
	                       .value());

	for(auto& test : tests) {

		auto& [value1, value2] = test;

		const auto& negated = -value1;

		EXPECT_EQ(negated, value2) << "Input values: " << value1 << ", " << value2;
	}
}

static bool has_no_special_chars(const std::string& input) {

	size_t i = 0;

	if(input.at(0) == '-' || input.at(0) == '+') {
		++i;
	}

	for(; i < input.size(); ++i) {
		char value = input.at(i);

		if(value >= '0' && value <= '9') {
			continue;
		} else if(BigIntTest::is_special_separator(value)) {
			return false;
		} else {
			throw std::runtime_error("unexpected value in bigint string");
		}
	}

	return true;
}

TEST(BigInt, IntegerToString) {

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
		std::to_string(std::numeric_limits<uint64_t>::max()),
		"-384324_132132_3123123_3",
		"+384324_132132_3123123_3"
	};

	for(const std::string& test : tests) {

		BigInt big_int = BigInt::get_from_string(test).value();

		BigIntTest cpp_result = BigIntTest(test);

		std::string bigint_c_str = big_int.to_string();

		std::string bigint_cpp_str = cpp_result.to_string();

		EXPECT_EQ(bigint_c_str, bigint_cpp_str) << "Input string: " << test;

		if(has_no_special_chars(test)) {
			EXPECT_EQ(test, bigint_c_str);
		}
	}
}

TEST(BigInt, IntegertoHexString) {
	struct HexOption {
		bool prefix;
		bool add_gaps;
		bool trim_first_number;
		bool uppercase;
	};
	using HexTests = std::pair<HexOption, std::string>;
	using TestType = std::tuple<BigInt, std::vector<HexTests>>;

	std::vector<TestType> tests{};

	{
		std::vector<HexTests> test_one{
			{ HexOption{
			      .prefix = true, .add_gaps = true, .trim_first_number = true, .uppercase = true },
			  "-0x2 155B5C319BAD3101" },
			{ HexOption{
			      .prefix = true, .add_gaps = true, .trim_first_number = true, .uppercase = false },
			  "-0x2 155b5c319bad3101" },
			{ HexOption{
			      .prefix = true, .add_gaps = true, .trim_first_number = false, .uppercase = true },
			  "-0x0000000000000002 155B5C319BAD3101" },
			{ HexOption{ .prefix = true,
			             .add_gaps = true,
			             .trim_first_number = false,
			             .uppercase = false },
			  "-0x0000000000000002 155b5c319bad3101" },
			{ HexOption{
			      .prefix = true, .add_gaps = false, .trim_first_number = true, .uppercase = true },
			  "-0x2155B5C319BAD3101" },
			{ HexOption{ .prefix = true,
			             .add_gaps = false,
			             .trim_first_number = true,
			             .uppercase = false },
			  "-0x2155b5c319bad3101" },
			{ HexOption{ .prefix = true,
			             .add_gaps = false,
			             .trim_first_number = false,
			             .uppercase = true },
			  "-0x0000000000000002155B5C319BAD3101" },
			{ HexOption{ .prefix = true,
			             .add_gaps = false,
			             .trim_first_number = false,
			             .uppercase = false },
			  "-0x0000000000000002155b5c319bad3101" },
			{ HexOption{
			      .prefix = false, .add_gaps = true, .trim_first_number = true, .uppercase = true },
			  "-2 155B5C319BAD3101" },
			{ HexOption{ .prefix = false,
			             .add_gaps = true,
			             .trim_first_number = true,
			             .uppercase = false },
			  "-2 155b5c319bad3101" },
			{ HexOption{ .prefix = false,
			             .add_gaps = true,
			             .trim_first_number = false,
			             .uppercase = true },
			  "-0000000000000002 155B5C319BAD3101" },
			{ HexOption{ .prefix = false,
			             .add_gaps = true,
			             .trim_first_number = false,
			             .uppercase = false },
			  "-0000000000000002 155b5c319bad3101" },
			{ HexOption{ .prefix = false,
			             .add_gaps = false,
			             .trim_first_number = true,
			             .uppercase = true },
			  "-2155B5C319BAD3101" },
			{ HexOption{ .prefix = false,
			             .add_gaps = false,
			             .trim_first_number = true,
			             .uppercase = false },
			  "-2155b5c319bad3101" },
			{ HexOption{ .prefix = false,
			             .add_gaps = false,
			             .trim_first_number = false,
			             .uppercase = true },
			  "-0000000000000002155B5C319BAD3101" },
			{ HexOption{ .prefix = false,
			             .add_gaps = false,
			             .trim_first_number = false,
			             .uppercase = false },
			  "-0000000000000002155b5c319bad3101" },
		};

		tests.emplace_back(BigInt::get_from_string("-384324_132132_3123123_3").value(),
		                   std::move(test_one));

		std::vector<HexTests> test_two{
			{ HexOption{
			      .prefix = true, .add_gaps = true, .trim_first_number = true, .uppercase = true },
			  "0x2 155B5C319BAD3101" },
			{ HexOption{
			      .prefix = true, .add_gaps = true, .trim_first_number = true, .uppercase = false },
			  "0x2 155b5c319bad3101" },
			{ HexOption{
			      .prefix = true, .add_gaps = true, .trim_first_number = false, .uppercase = true },
			  "0x0000000000000002 155B5C319BAD3101" },
			{ HexOption{ .prefix = true,
			             .add_gaps = true,
			             .trim_first_number = false,
			             .uppercase = false },
			  "0x0000000000000002 155b5c319bad3101" },
			{ HexOption{
			      .prefix = true, .add_gaps = false, .trim_first_number = true, .uppercase = true },
			  "0x2155B5C319BAD3101" },
			{ HexOption{ .prefix = true,
			             .add_gaps = false,
			             .trim_first_number = true,
			             .uppercase = false },
			  "0x2155b5c319bad3101" },
			{ HexOption{ .prefix = true,
			             .add_gaps = false,
			             .trim_first_number = false,
			             .uppercase = true },
			  "0x0000000000000002155B5C319BAD3101" },
			{ HexOption{ .prefix = true,
			             .add_gaps = false,
			             .trim_first_number = false,
			             .uppercase = false },
			  "0x0000000000000002155b5c319bad3101" },
			{ HexOption{
			      .prefix = false, .add_gaps = true, .trim_first_number = true, .uppercase = true },
			  "2 155B5C319BAD3101" },
			{ HexOption{ .prefix = false,
			             .add_gaps = true,
			             .trim_first_number = true,
			             .uppercase = false },
			  "2 155b5c319bad3101" },
			{ HexOption{ .prefix = false,
			             .add_gaps = true,
			             .trim_first_number = false,
			             .uppercase = true },
			  "0000000000000002 155B5C319BAD3101" },
			{ HexOption{ .prefix = false,
			             .add_gaps = true,
			             .trim_first_number = false,
			             .uppercase = false },
			  "0000000000000002 155b5c319bad3101" },
			{ HexOption{ .prefix = false,
			             .add_gaps = false,
			             .trim_first_number = true,
			             .uppercase = true },
			  "2155B5C319BAD3101" },
			{ HexOption{ .prefix = false,
			             .add_gaps = false,
			             .trim_first_number = true,
			             .uppercase = false },
			  "2155b5c319bad3101" },
			{ HexOption{ .prefix = false,
			             .add_gaps = false,
			             .trim_first_number = false,
			             .uppercase = true },
			  "0000000000000002155B5C319BAD3101" },
			{ HexOption{ .prefix = false,
			             .add_gaps = false,
			             .trim_first_number = false,
			             .uppercase = false },
			  "0000000000000002155b5c319bad3101" },
		};

		tests.emplace_back(BigInt::get_from_string("+384324_132132_3123123_3").value(),
		                   std::move(test_two));

		std::vector<HexTests> test_three{
			{ HexOption{
			      .prefix = true, .add_gaps = true, .trim_first_number = true, .uppercase = true },
			  "0xDEADBEEF" },
			{ HexOption{
			      .prefix = true, .add_gaps = true, .trim_first_number = true, .uppercase = false },
			  "0xdeadbeef" },
			{ HexOption{
			      .prefix = true, .add_gaps = true, .trim_first_number = false, .uppercase = true },
			  "0x00000000DEADBEEF" },
			{ HexOption{ .prefix = true,
			             .add_gaps = true,
			             .trim_first_number = false,
			             .uppercase = false },
			  "0x00000000deadbeef" },
			{ HexOption{
			      .prefix = true, .add_gaps = false, .trim_first_number = true, .uppercase = true },
			  "0xDEADBEEF" },
			{ HexOption{ .prefix = true,
			             .add_gaps = false,
			             .trim_first_number = true,
			             .uppercase = false },
			  "0xdeadbeef" },
			{ HexOption{ .prefix = true,
			             .add_gaps = false,
			             .trim_first_number = false,
			             .uppercase = true },
			  "0x00000000DEADBEEF" },
			{ HexOption{ .prefix = true,
			             .add_gaps = false,
			             .trim_first_number = false,
			             .uppercase = false },
			  "0x00000000deadbeef" },
			{ HexOption{
			      .prefix = false, .add_gaps = true, .trim_first_number = true, .uppercase = true },
			  "DEADBEEF" },
			{ HexOption{ .prefix = false,
			             .add_gaps = true,
			             .trim_first_number = true,
			             .uppercase = false },
			  "deadbeef" },
			{ HexOption{ .prefix = false,
			             .add_gaps = true,
			             .trim_first_number = false,
			             .uppercase = true },
			  "00000000DEADBEEF" },
			{ HexOption{ .prefix = false,
			             .add_gaps = true,
			             .trim_first_number = false,
			             .uppercase = false },
			  "00000000deadbeef" },
			{ HexOption{ .prefix = false,
			             .add_gaps = false,
			             .trim_first_number = true,
			             .uppercase = true },
			  "DEADBEEF" },
			{ HexOption{ .prefix = false,
			             .add_gaps = false,
			             .trim_first_number = true,
			             .uppercase = false },
			  "deadbeef" },
			{ HexOption{ .prefix = false,
			             .add_gaps = false,
			             .trim_first_number = false,
			             .uppercase = true },
			  "00000000DEADBEEF" },
			{ HexOption{ .prefix = false,
			             .add_gaps = false,
			             .trim_first_number = false,
			             .uppercase = false },
			  "00000000deadbeef" },
		};

		tests.emplace_back(BigInt{ (uint64_t)0xDEADBEEFULL }, std::move(test_three));
	}

	for(const TestType& test : tests) {

		const auto& [big_int, hex_tests] = test;

		for(const auto& hex_test : hex_tests) {

			const auto& [option, expected_result] = hex_test;

			const auto& [prefix, add_gaps, trim_first_number, uppercase] = option;

			std::string actual_result =
			    big_int.to_string_hex(prefix, add_gaps, trim_first_number, uppercase);

			EXPECT_EQ(actual_result, expected_result)
			    << "Input value: " << big_int << "options: " << (prefix ? "prefix" : "no-prefix")
			    << " " << (add_gaps ? "add_gaps" : "no-gaps") << " "
			    << (trim_first_number ? "trim_first_number" : "no-trim") << " "
			    << (uppercase ? "uppercase" : " lowercase");
		}
	}
}

TEST(BigInt, IntegertoBinString) {
	struct BinOption {
		bool prefix;
		bool add_gaps;
		bool trim_first_number;
	};
	using BinTests = std::pair<BinOption, std::string>;
	using TestType = std::tuple<BigInt, std::vector<BinTests>>;

	std::vector<TestType> tests{};

	{
		std::vector<BinTests> test_one{
			{ BinOption{ .prefix = true, .add_gaps = true, .trim_first_number = true },
			  "-0b10 0001010101011011010111000011000110011011101011010011000100000001" },
			{ BinOption{ .prefix = true, .add_gaps = true, .trim_first_number = false },
			  "-0b0000000000000000000000000000000000000000000000000000000000000010 "
			  "0001010101011011010111000011000110011011101011010011000100000001" },
			{ BinOption{ .prefix = true, .add_gaps = false, .trim_first_number = true },
			  "-0b100001010101011011010111000011000110011011101011010011000100000001" },
			{ BinOption{ .prefix = true, .add_gaps = false, .trim_first_number = false },
			  "-0b000000000000000000000000000000000000000000000000000000000000001000010101010110110"
			  "10111000011000110011011101011010011000100000001" },
			{ BinOption{ .prefix = false, .add_gaps = true, .trim_first_number = true },
			  "-10 0001010101011011010111000011000110011011101011010011000100000001" },
			{ BinOption{ .prefix = false, .add_gaps = true, .trim_first_number = false },
			  "-0000000000000000000000000000000000000000000000000000000000000010 "
			  "0001010101011011010111000011000110011011101011010011000100000001" },
			{ BinOption{ .prefix = false, .add_gaps = false, .trim_first_number = true },
			  "-100001010101011011010111000011000110011011101011010011000100000001" },
			{ BinOption{ .prefix = false, .add_gaps = false, .trim_first_number = false },
			  "-00000000000000000000000000000000000000000000000000000000000000100001010101011011010"
			  "111000011000110011011101011010011000100000001" },
		};

		tests.emplace_back(BigInt::get_from_string("-384324_132132_3123123_3").value(),
		                   std::move(test_one));

		std::vector<BinTests> test_two{
			{ BinOption{ .prefix = true, .add_gaps = true, .trim_first_number = true },
			  "0b10 0001010101011011010111000011000110011011101011010011000100000001" },
			{ BinOption{ .prefix = true, .add_gaps = true, .trim_first_number = false },
			  "0b0000000000000000000000000000000000000000000000000000000000000010 "
			  "0001010101011011010111000011000110011011101011010011000100000001" },
			{ BinOption{ .prefix = true, .add_gaps = false, .trim_first_number = true },
			  "0b100001010101011011010111000011000110011011101011010011000100000001" },
			{ BinOption{ .prefix = true, .add_gaps = false, .trim_first_number = false },
			  "0b0000000000000000000000000000000000000000000000000000000000000010000101010101101101"
			  "0111000011000110011011101011010011000100000001" },
			{ BinOption{ .prefix = false, .add_gaps = true, .trim_first_number = true },
			  "10 0001010101011011010111000011000110011011101011010011000100000001" },
			{ BinOption{ .prefix = false, .add_gaps = true, .trim_first_number = false },
			  "0000000000000000000000000000000000000000000000000000000000000010 "
			  "0001010101011011010111000011000110011011101011010011000100000001" },
			{ BinOption{ .prefix = false, .add_gaps = false, .trim_first_number = true },
			  "100001010101011011010111000011000110011011101011010011000100000001" },
			{ BinOption{ .prefix = false, .add_gaps = false, .trim_first_number = false },
			  "000000000000000000000000000000000000000000000000000000000000001000010101010110110101"
			  "11000011000110011011101011010011000100000001" },
		};

		tests.emplace_back(BigInt::get_from_string("+384324_132132_3123123_3").value(),
		                   std::move(test_two));

		std::vector<BinTests> test_three{
			{ BinOption{ .prefix = true, .add_gaps = true, .trim_first_number = true },
			  "0b11011110101011011011111011101111" },
			{ BinOption{ .prefix = true, .add_gaps = true, .trim_first_number = false },
			  "0b0000000000000000000000000000000011011110101011011011111011101111" },
			{ BinOption{ .prefix = true, .add_gaps = false, .trim_first_number = true },
			  "0b11011110101011011011111011101111" },
			{ BinOption{ .prefix = true, .add_gaps = false, .trim_first_number = false },
			  "0b0000000000000000000000000000000011011110101011011011111011101111" },
			{ BinOption{ .prefix = false, .add_gaps = true, .trim_first_number = true },
			  "11011110101011011011111011101111" },
			{ BinOption{ .prefix = false, .add_gaps = true, .trim_first_number = false },
			  "0000000000000000000000000000000011011110101011011011111011101111" },
			{ BinOption{ .prefix = false, .add_gaps = false, .trim_first_number = true },
			  "11011110101011011011111011101111" },
			{ BinOption{ .prefix = false, .add_gaps = false, .trim_first_number = false },
			  "0000000000000000000000000000000011011110101011011011111011101111" },
		};

		tests.emplace_back(BigInt{ (uint64_t)0xDEADBEEFULL }, std::move(test_three));
	}

	for(const TestType& test : tests) {

		const auto& [big_int, bin_tests] = test;

		for(const auto& bin_test : bin_tests) {

			const auto& [option, expected_result] = bin_test;

			const auto& [prefix, add_gaps, trim_first_number] = option;

			std::string actual_result = big_int.to_string_bin(prefix, add_gaps, trim_first_number);

			EXPECT_EQ(actual_result, expected_result)
			    << "Input value: " << big_int << "options: " << (prefix ? "prefix" : "no-prefix")
			    << " " << (add_gaps ? "add_gaps" : "no-gaps") << " "
			    << (trim_first_number ? "trim_first_number" : "no-trim");
		}
	}
}

TEST(BigInt, IntegerBulkInitialization) {

	BigInt test_positive{ (uint64_t)1ULL,
		                  std::numeric_limits<uint64_t>::max(),
		                  std::numeric_limits<uint64_t>::max(),
		                  std::numeric_limits<uint64_t>::max(),
		                  std::numeric_limits<uint64_t>::max(),
		                  std::numeric_limits<uint64_t>::max() };

	std::string bigint_c_str = test_positive.to_string_hex(true, true, true, true);

	EXPECT_EQ(
	    bigint_c_str,
	    "0x1 FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF")
	    << "Input value: " << test_positive;
}

TEST(BigInt, IntegerAddition) {
	using TestType = std::tuple<BigInt, BigInt>;

	std::vector<TestType> tests{};

	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (uint64_t)1ULL });
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-2LL });
	tests.emplace_back(BigInt{ (uint64_t)1ULL }, BigInt{ (uint64_t)2ULL });
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value(),
	                   BigInt{ (uint64_t)2ULL });
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value());

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value());

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value());
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-1LL });
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-3LL });
	tests.emplace_back(BigInt{ (int64_t)-3LL }, BigInt{ (int64_t)-1LL });
	tests.emplace_back(BigInt{ (uint64_t)2ULL }, BigInt{ (uint64_t)2ULL });

	tests.emplace_back(BigInt::get_from_string("0").value(), BigInt::get_from_string("+0").value());
	tests.emplace_back(BigInt::get_from_string("+0").value(), BigInt::get_from_string("0").value());

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("-2131215135135132515135").value());
	tests.emplace_back(BigInt::get_from_string("-1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value());

	tests.emplace_back(BigInt::get_from_string("-2131215135135132515135").value(),
	                   BigInt::get_from_string("+1").value());
	tests.emplace_back(BigInt::get_from_string("+2131215135135132515135").value(),
	                   BigInt::get_from_string("-1").value());

	tests.emplace_back(BigInt::get_from_string("+0").value(),
	                   BigInt::get_from_string("-2131215135135").value());
	tests.emplace_back(BigInt::get_from_string("0").value(),
	                   BigInt::get_from_string("+2131215135135").value());

	tests.emplace_back(BigInt::get_from_string("-2131215135135").value(),
	                   BigInt::get_from_string("+0").value());
	tests.emplace_back(BigInt::get_from_string("+2131215135135").value(),
	                   BigInt::get_from_string("0").value());

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value());

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value());

	tests.emplace_back(BigInt{ std::numeric_limits<uint64_t>::max() }, BigInt{ (uint64_t)2ULL });
	tests.emplace_back(BigInt{ std::numeric_limits<uint64_t>::max() },
	                   BigInt{ std::numeric_limits<uint64_t>::max() });
	tests.emplace_back(
	    BigInt{ std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	            std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	            std::numeric_limits<uint64_t>::max() },
	    BigInt{ (uint64_t)2ULL });

	tests.emplace_back(BigInt{ (uint64_t)2ULL }, BigInt{ std::numeric_limits<uint64_t>::max() });
	tests.emplace_back(BigInt{ std::numeric_limits<uint64_t>::max() },
	                   BigInt{ std::numeric_limits<uint64_t>::max() });
	tests.emplace_back(BigInt{ (uint64_t)2ULL }, BigInt{ std::numeric_limits<uint64_t>::max(),
	                                                     std::numeric_limits<uint64_t>::max(),
	                                                     std::numeric_limits<uint64_t>::max(),
	                                                     std::numeric_limits<uint64_t>::max(),
	                                                     std::numeric_limits<uint64_t>::max() });

	std::vector<BigInt> numbers{};

	numbers.emplace_back((int64_t)10);
	numbers.emplace_back((int64_t)5);
	numbers.emplace_back((int64_t)3);
	numbers.emplace_back((int64_t)2);
	numbers.emplace_back(BigInt::get_from_string("+0").value());
	numbers.emplace_back(BigInt::get_from_string("0").value());
	numbers.emplace_back((int64_t)-2);
	numbers.emplace_back((int64_t)-3);
	numbers.emplace_back((int64_t)-5);
	numbers.emplace_back((int64_t)-10);

	for(size_t i = 0; i < numbers.size(); ++i) {

		for(size_t j = 0; j < numbers.size(); ++j) {

			const BigInt& value1 = numbers.at(i);
			const BigInt& value2 = numbers.at(j);

			tests.emplace_back(value1.copy(), value2.copy());
		}
	}

	for(const TestType& test : tests) {

		const auto& [value1, value2] = test;

		const BigInt actual_result = value1 + value2;

		const BigIntTest result_expected = BigIntTest(value1) + BigIntTest(value2);

		EXPECT_EQ(actual_result, result_expected) << "Input values: " << value1 << ", " << value2;
	}
}

TEST(BigInt, IntegerSubtraction) {
	using TestType = std::tuple<BigInt, BigInt>;

	std::vector<TestType> tests{};

	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (uint64_t)1ULL });
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-2LL });
	tests.emplace_back(BigInt{ (uint64_t)1ULL }, BigInt{ (uint64_t)2ULL });
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value(),
	                   BigInt{ (uint64_t)2ULL });
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value());

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value());

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value());
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-1LL });
	tests.emplace_back(BigInt{ (int64_t)-1LL }, BigInt{ (int64_t)-3LL });
	tests.emplace_back(BigInt{ (int64_t)-3LL }, BigInt{ (int64_t)-1LL });
	tests.emplace_back(BigInt{ (uint64_t)2ULL }, BigInt{ (uint64_t)2ULL });

	tests.emplace_back(BigInt::get_from_string("0").value(), BigInt::get_from_string("+0").value());
	tests.emplace_back(BigInt::get_from_string("+0").value(), BigInt::get_from_string("0").value());

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("-2131215135135132515135").value());
	tests.emplace_back(BigInt::get_from_string("-1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value());

	tests.emplace_back(BigInt::get_from_string("-2131215135135132515135").value(),
	                   BigInt::get_from_string("+1").value());
	tests.emplace_back(BigInt::get_from_string("+2131215135135132515135").value(),
	                   BigInt::get_from_string("-1").value());

	tests.emplace_back(BigInt::get_from_string("+0").value(),
	                   BigInt::get_from_string("-2131215135135").value());
	tests.emplace_back(BigInt::get_from_string("0").value(),
	                   BigInt::get_from_string("+2131215135135").value());

	tests.emplace_back(BigInt::get_from_string("-2131215135135").value(),
	                   BigInt::get_from_string("+0").value());
	tests.emplace_back(BigInt::get_from_string("+2131215135135").value(),
	                   BigInt::get_from_string("0").value());

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value());

	tests.emplace_back(BigInt::get_from_string("+1").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value());

	tests.emplace_back(BigInt{ std::numeric_limits<uint64_t>::max() }, BigInt{ (uint64_t)2ULL });
	tests.emplace_back(BigInt{ std::numeric_limits<uint64_t>::max() },
	                   BigInt{ std::numeric_limits<uint64_t>::max() });
	tests.emplace_back(
	    BigInt{ std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	            std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	            std::numeric_limits<uint64_t>::max() },
	    BigInt{ (uint64_t)2ULL });

	tests.emplace_back(BigInt{ (uint64_t)2ULL }, BigInt{ std::numeric_limits<uint64_t>::max() });
	tests.emplace_back(BigInt{ std::numeric_limits<uint64_t>::max() },
	                   BigInt{ std::numeric_limits<uint64_t>::max() });
	tests.emplace_back(BigInt{ (uint64_t)2ULL }, BigInt{ std::numeric_limits<uint64_t>::max(),
	                                                     std::numeric_limits<uint64_t>::max(),
	                                                     std::numeric_limits<uint64_t>::max(),
	                                                     std::numeric_limits<uint64_t>::max(),
	                                                     std::numeric_limits<uint64_t>::max() });

	tests.emplace_back(BigInt{ (uint64_t)2ULL, (uint64_t)14ULL },
	                   BigInt{ (uint64_t)1ULL, std::numeric_limits<uint64_t>::max() });

	std::vector<BigInt> numbers{};

	numbers.emplace_back((int64_t)10);
	numbers.emplace_back((int64_t)5);
	numbers.emplace_back((int64_t)3);
	numbers.emplace_back((int64_t)2);
	numbers.emplace_back(BigInt::get_from_string("+0").value());
	numbers.emplace_back(BigInt::get_from_string("0").value());
	numbers.emplace_back((int64_t)-2);
	numbers.emplace_back((int64_t)-3);
	numbers.emplace_back((int64_t)-5);
	numbers.emplace_back((int64_t)-10);

	for(size_t i = 0; i < numbers.size(); ++i) {

		for(size_t j = 0; j < numbers.size(); ++j) {

			const BigInt& value1 = numbers.at(i);
			const BigInt& value2 = numbers.at(j);

			tests.emplace_back(value1.copy(), value2.copy());
		}
	}

	for(const TestType& test : tests) {

		const auto& [value1, value2] = test;

		const BigInt actual_result = value1 - value2;

		const BigIntTest result_expected = BigIntTest(value1) - BigIntTest(value2);

		EXPECT_EQ(actual_result, result_expected) << "Input values: " << value1 << ", " << value2;
	}
}
