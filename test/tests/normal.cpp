

#define BIGINT_C_CPP_HIDE_C_LIB_FNS_AND_TYPES_IN_CPP
#include <bigint_c.h>

#include <gtest/gtest.h>

#include "../helper/helper.hpp"
#include "../helper/matcher.hpp"
#include "../helper/printer.hpp"

TEST(BigInt, ParseError1) {
	std::expected<BigInt, bigint::ParseError> maybe_big_int = BigInt::get_from_string("error");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(), (bigint::ParseError{ "invalid character", 0, 'e' }));
}

TEST(BigInt, ParseError2) {
	std::expected<BigInt, bigint::ParseError> maybe_big_int = BigInt::get_from_string("-");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(), (bigint::ParseError{ "'-' alone is not valid", 0 }));
}

TEST(BigInt, ParseError3) {
	std::expected<BigInt, bigint::ParseError> maybe_big_int = BigInt::get_from_string("+");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(), (bigint::ParseError{ "'+' alone is not valid", 0 }));
}

TEST(BigInt, ParseError4) {
	std::expected<BigInt, bigint::ParseError> maybe_big_int = BigInt::get_from_string("");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(), (bigint::ParseError{ "empty string is not valid", 0 }));
}

TEST(BigInt, ParseError5) {
	std::expected<BigInt, bigint::ParseError> maybe_big_int = BigInt::get_from_string("_0");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(),
	          (bigint::ParseError{ "separator not allowed at the start", 0, '_' }));
}

TEST(BigInt, ParseError6) {
	std::expected<BigInt, bigint::ParseError> maybe_big_int = BigInt::get_from_string("!0");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(), (bigint::ParseError{ "invalid character", 0, '!' }));
}

TEST(BigInt, ParseError7) {
	std::expected<BigInt, bigint::ParseError> maybe_big_int = BigInt::get_from_string("-0");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(), (bigint::ParseError{ "-0 is not allowed", 2 }));
}

TEST(BigInt, ParseError8) {

	try {
		BigInt big_int = BigInt{ "01ffgh" };
		FAIL() << "Expected bigint::ParseError to be thrown";
	} catch(const bigint::ParseError& err) {
		EXPECT_EQ(err, (bigint::ParseError{ "invalid character", 2, 'f' }));
	} catch(...) {
		FAIL() << "Expected bigint::ParseError to be thrown, but got a different exception";
	}
}

TEST(BigInt, ParseError9) {
	std::expected<BigInt, bigint::ParseError> maybe_big_int =
	    BigInt::get_from_string("004394235723!?");

	ASSERT_THAT(maybe_big_int, ExpectedHasError());

	EXPECT_EQ(maybe_big_int.error(), (bigint::ParseError{ "invalid character", 12, '!' }));
}

TEST(BigInt, ParseSuccess1) {
	std::expected<BigInt, bigint::ParseError> maybe_big_int = BigInt::get_from_string("+0");

	ASSERT_THAT(maybe_big_int, ExpectedHasValue());

	BigInt big_int = std::move(maybe_big_int.value());

	BigIntTest result = BigIntTest(true, { 0ULL });

	EXPECT_EQ(big_int, result);
}

TEST(BigInt, ParseSuccess2) {
	std::expected<BigInt, bigint::ParseError> maybe_big_int = BigInt::get_from_string("-1");

	ASSERT_THAT(maybe_big_int, ExpectedHasValue());

	BigInt big_int = std::move(maybe_big_int.value());

	BigIntTest result = BigIntTest(false, { 1ULL });

	EXPECT_EQ(big_int, result);
}

TEST(BigInt, ParseSuccess3) {
	std::expected<BigInt, bigint::ParseError> maybe_big_int = BigInt::get_from_string("-1_0");

	ASSERT_THAT(maybe_big_int, ExpectedHasValue());

	BigInt big_int = std::move(maybe_big_int.value());

	BigIntTest result = BigIntTest(false, { 10ULL });

	EXPECT_EQ(big_int, result);
}

TEST(BigInt, ParseSuccess4) {
	std::expected<BigInt, bigint::ParseError> maybe_big_int = BigInt::get_from_string("+0021");

	ASSERT_THAT(maybe_big_int, ExpectedHasValue());

	BigInt big_int = std::move(maybe_big_int.value());

	BigIntTest result = BigIntTest(true, { 21ULL });

	EXPECT_EQ(big_int, result);
}

TEST(BigInt, ParseSuccess5) {
	std::expected<BigInt, bigint::ParseError> maybe_big_int =
	    BigInt::get_from_string("-10_00'00.000,00");

	ASSERT_THAT(maybe_big_int, ExpectedHasValue());

	BigInt big_int = std::move(maybe_big_int.value());

	BigIntTest result = BigIntTest(false, { 10000000000ULL });

	EXPECT_EQ(big_int, result);
}

TEST(BigInt, ParseSuccess6) {

	// calls destructor at the end of the scope
	{
		BigInt big_int = BigInt{ "+0" };
	}

	SUCCEED();
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

		std::expected<BigInt, bigint::ParseError> maybe_big_int = BigInt::get_from_string(test);

		ASSERT_THAT(maybe_big_int, ExpectedHasValue());

		BigInt c_result = std::move(maybe_big_int.value());

		BigIntTest cpp_result = BigIntTest(test);

		EXPECT_EQ(c_result, cpp_result) << "Input string: " << test;
	}
}

TEST(BigInt, ParseSuccess0Normalize) {
	std::expected<BigInt, bigint::ParseError> maybe_big_int = BigInt::get_from_string(
	    "+0000000000000000000000000000000000000000000000000000000000000000000000000000000");

	ASSERT_THAT(maybe_big_int, ExpectedHasValue());

	BigInt big_int = std::move(maybe_big_int.value());

	BigIntTest result = BigIntTest(true, { 0ULL });

	EXPECT_EQ(big_int, result);
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

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
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

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
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

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
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

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
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

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
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

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
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

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
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

		EXPECT_EQ(negated, value2)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
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
			    << "Input value: " << BigIntDebug{ big_int }
			    << "options: " << (prefix ? "prefix" : "no-prefix") << " "
			    << (add_gaps ? "add_gaps" : "no-gaps") << " "
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
			    << "Input value: " << BigIntDebug{ big_int }
			    << "options: " << (prefix ? "prefix" : "no-prefix") << " "
			    << (add_gaps ? "add_gaps" : "no-gaps") << " "
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
	    << "Input value: " << BigIntDebug{ test_positive };
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

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
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

	tests.emplace_back(BigInt::get_from_string("+680564733841876926963642703010955526140").value(),
	                   BigInt::get_from_string("+680564733841876926926749214863536422910").value());

	for(const TestType& test : tests) {

		const auto& [value1, value2] = test;

		const BigInt actual_result = value1 - value2;

		const BigIntTest result_expected = BigIntTest(value1) - BigIntTest(value2);

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
	}
}

TEST(BigInt, IntegerMultiplication) {
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

	tests.emplace_back(BigInt::get_from_string("+0").value(),
	                   BigInt::get_from_string("+2131215135135132515135").value());

	tests.emplace_back(BigInt::get_from_string("+2131215135135132515135").value(),
	                   BigInt::get_from_string("+1").value());

	tests.emplace_back(BigInt::get_from_string("+2131215135135132515135").value(),
	                   BigInt::get_from_string("+0").value());

	tests.emplace_back(BigInt::get_from_string("+21312151351351323495781541456378747474735463736465"
	                                           "37364647384747474747474747566383938475727424515135")
	                       .value(),
	                   BigInt::get_from_string("+352785318753").value());

	tests.emplace_back(BigInt::get_from_string("+352785318753").value(),
	                   BigInt::get_from_string("+21312151351351323495781541456378747474735463736465"
	                                           "37364647384747474747474747566383938475727424515135")
	                       .value());

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

	tests.emplace_back(
	    BigInt::get_from_string(
	        "35132632464234632432532749452368534748774747474747574563458934574389573498573458934758"
	        "93475983457938457349857345845475475757777777777777777777777777777777777777777777777777"
	        "77777775457457475477252574365782456785786827346752577328452378563279852978352379532578"
	        "93255789088010871451780521572161276188761076576805218675867125867051287652186702768706"
	        "54076257801256427855521708561270502716512761526781567085102678516780152678152678512671"
	        "35267352167521367257615236715236715286780152367821678125678513267805236781526780152675"
	        "8123671523363633532562340963427646346346363632")
	        .value(),
	    BigInt::get_from_string(
	        "35132632464234632432532749452368534748774747474747574563458934574389573498573458934758"
	        "93475983457938457349857345845475475757777777777777777777777777777777777777777777777777"
	        "77777775457457475477252574365782456785786827346752577328452378563279852978352379532578"
	        "93255789088010871451780521572161276188761076576805218675867125867051287652186702768706"
	        "54076257801256427855521708561270502716512761526781567085102678516780152678152678512671"
	        "35267352167521367257615236715236715286780152367821678125678513267805236781526780152675"
	        "8123671523363633532562340963427646346346363631")
	        .value());

	for(const TestType& test : tests) {

		const auto& [value1, value2] = test;

		const BigInt actual_result = value1 * value2;

		const BigIntTest result_expected = BigIntTest(value1) * BigIntTest(value2);

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
	}
}

TEST(BigInt, IntegerPostIncrement) {

	{ // test 0 always being positive

		BigInt test_value1 = BigInt{ (int64_t)-1LL };
		std::ignore = test_value1++;

		EXPECT_EQ(test_value1, BigInt{ (uint64_t)0 });
	}

	std::vector<BigInt> tests{};

	tests.emplace_back((int64_t)-1LL);
	tests.emplace_back((int64_t)-2LL);
	tests.emplace_back((uint64_t)1ULL);
	tests.emplace_back((uint64_t)0ULL);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value());
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value());

	tests.emplace_back(BigInt::get_from_string("-351326324642346363634634634636363").value());
	tests.emplace_back(
	    BigInt::get_from_string("-351326324642346363633532562340963427646346346363631").value());

	tests.emplace_back(std::numeric_limits<uint64_t>::max());
	tests.emplace_back(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max());

	tests.emplace_back((uint64_t)2ULL, std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max());

	for(const BigInt& orig_value : tests) {

		BigInt value1 = orig_value.copy();

		const BigInt actual_result = value1++;

		const BigIntTest orig_test = BigIntTest(orig_value);

		BigIntTest test1 = orig_test.copy();

		const BigIntTest result_expected = test1++;

		EXPECT_EQ(orig_value, orig_test);

		EXPECT_EQ(actual_result, result_expected);

		EXPECT_EQ(actual_result, orig_value);

		EXPECT_NE(actual_result, value1);

		EXPECT_EQ(result_expected, orig_test);

		EXPECT_NE(result_expected, test1);
	}
}

TEST(BigInt, IntegerPreIncrement) {

	{ // test 0 always being positive

		BigInt test_value1 = BigInt{ (int64_t)-1LL };
		std::ignore = ++test_value1;

		EXPECT_EQ(test_value1, BigInt{ (uint64_t)0 });
	}

	std::vector<BigInt> tests{};

	tests.emplace_back((int64_t)-1LL);
	tests.emplace_back((int64_t)-2LL);
	tests.emplace_back((uint64_t)1ULL);
	tests.emplace_back((uint64_t)0ULL);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value());
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value());

	tests.emplace_back(BigInt::get_from_string("-351326324642346363634634634636363").value());
	tests.emplace_back(
	    BigInt::get_from_string("-351326324642346363633532562340963427646346346363631").value());

	tests.emplace_back(std::numeric_limits<uint64_t>::max());
	tests.emplace_back(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max());

	tests.emplace_back((uint64_t)2ULL, std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max());

	for(const BigInt& orig_value : tests) {

		BigInt value1 = orig_value.copy();

		const BigInt& actual_result = ++value1;

		const BigIntTest orig_test = BigIntTest(orig_value);

		BigIntTest test1 = orig_test.copy();

		const BigIntTest& result_expected = ++test1;

		EXPECT_EQ(orig_value, orig_test);

		EXPECT_EQ(actual_result, result_expected);

		EXPECT_NE(actual_result, orig_value);

		EXPECT_EQ(actual_result, value1);

		EXPECT_NE(result_expected, orig_test);

		EXPECT_EQ(result_expected, test1);
	}
}

TEST(BigInt, IntegerPostDecrement) {

	{ // test 0 always being positive

		BigInt test_value1 = BigInt{ (uint64_t)1LL };
		std::ignore = test_value1--;

		EXPECT_EQ(test_value1, BigInt{ (uint64_t)0 });
	}

	std::vector<BigInt> tests{};

	tests.emplace_back((int64_t)-1LL);
	tests.emplace_back((int64_t)-2LL);
	tests.emplace_back((uint64_t)1ULL);
	tests.emplace_back((uint64_t)0ULL);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value());
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value());

	tests.emplace_back(BigInt::get_from_string("-351326324642346363634634634636363").value());
	tests.emplace_back(
	    BigInt::get_from_string("-351326324642346363633532562340963427646346346363631").value());

	tests.emplace_back(std::numeric_limits<uint64_t>::max());
	tests.emplace_back(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max());

	tests.emplace_back((uint64_t)1, 0, 0, 0, 0, 0, 0, 0);

	tests.emplace_back((uint64_t)1, 0, 0, 2323, 0, 0, 0, 0);

	for(const BigInt& orig_value : tests) {

		BigInt value1 = orig_value.copy();

		const BigInt actual_result = value1--;

		const BigIntTest orig_test = BigIntTest(orig_value);

		BigIntTest test1 = orig_test.copy();

		const BigIntTest result_expected = test1--;

		EXPECT_EQ(orig_value, orig_test);

		EXPECT_EQ(actual_result, result_expected);

		EXPECT_EQ(actual_result, orig_value);

		EXPECT_NE(actual_result, value1);

		EXPECT_EQ(result_expected, orig_test);

		EXPECT_NE(result_expected, test1);
	}
}

TEST(BigInt, IntegerPreDecrement) {

	{ // test 0 always being positive

		BigInt test_value1 = BigInt{ (uint64_t)1LL };
		std::ignore = --test_value1;

		EXPECT_EQ(test_value1, BigInt{ (uint64_t)0 });
	}

	std::vector<BigInt> tests{};

	tests.emplace_back((int64_t)-1LL);
	tests.emplace_back((int64_t)-2LL);
	tests.emplace_back((uint64_t)1ULL);
	tests.emplace_back((uint64_t)0ULL);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value());
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value());

	tests.emplace_back(BigInt::get_from_string("-351326324642346363634634634636363").value());
	tests.emplace_back(
	    BigInt::get_from_string("-351326324642346363633532562340963427646346346363631").value());

	tests.emplace_back(std::numeric_limits<uint64_t>::max());
	tests.emplace_back(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max());

	tests.emplace_back((uint64_t)2ULL, std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	                   std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max());

	tests.emplace_back((uint64_t)1, 0, 0, 0, 0, 0, 0, 0);

	tests.emplace_back((uint64_t)1, 0, 0, 2323, 0, 0, 0, 0);

	for(const BigInt& orig_value : tests) {

		BigInt value1 = orig_value.copy();

		const BigInt& actual_result = --value1;

		const BigIntTest orig_test = BigIntTest(orig_value);

		BigIntTest test1 = orig_test.copy();

		const BigIntTest& result_expected = --test1;

		EXPECT_EQ(orig_value, orig_test);

		EXPECT_EQ(actual_result, result_expected);

		EXPECT_NE(actual_result, orig_value);

		EXPECT_EQ(actual_result, value1);

		EXPECT_NE(result_expected, orig_test);

		EXPECT_EQ(result_expected, test1);
	}
}

TEST(BigInt, IntegerShiftLeft) {
	using TestType = std::tuple<BigInt, uint64_t>;

	std::vector<TestType> tests{};

	tests.emplace_back(BigInt{ (int64_t)-1LL }, 1ULL);
	tests.emplace_back(BigInt{ (uint64_t)1ULL }, 2ULL);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value(), 2ULL);
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    32235ULL);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(), 65);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    127);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    32235ULL);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    32235ULL);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, 32235ULL);

	tests.emplace_back(BigInt::get_from_string("0").value(), 32235ULL);

	tests.emplace_back(BigInt{ std::numeric_limits<uint64_t>::max() }, 2ULL);
	tests.emplace_back(BigInt{ std::numeric_limits<uint64_t>::max() }, 0ULL);
	tests.emplace_back(BigInt{ std::numeric_limits<uint64_t>::max() }, 32235ULL);
	tests.emplace_back(
	    BigInt{ std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	            std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	            std::numeric_limits<uint64_t>::max() },
	    2ULL);

	tests.emplace_back(
	    BigInt{ std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	            std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	            std::numeric_limits<uint64_t>::max() },
	    32235ULL);

	for(const TestType& test : tests) {

		const auto& [value1, value2] = test;

		const BigInt actual_result = value1 << value2;

		const BigIntTest result_expected = BigIntTest(value1) << value2;

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
	}
}

TEST(BigInt, IntegerShiftRight) {
	using TestType = std::tuple<BigInt, uint64_t>;

	std::vector<TestType> tests{};

	tests.emplace_back(BigInt{ (int64_t)-1LL }, 1ULL);
	tests.emplace_back(BigInt{ (uint64_t)1ULL }, 2ULL);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value(), 2ULL);
	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    32235ULL);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(), 65);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    127);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363631").value(),
	    32235ULL);

	tests.emplace_back(
	    BigInt::get_from_string("351326324642346363633532562340963427646346346363632").value(),
	    32235ULL);
	tests.emplace_back(BigInt{ (int64_t)-1LL }, 32235ULL);

	tests.emplace_back(BigInt::get_from_string("0").value(), 32235ULL);

	tests.emplace_back(BigInt{ std::numeric_limits<uint64_t>::max() }, 2ULL);
	tests.emplace_back(BigInt{ std::numeric_limits<uint64_t>::max() }, 0ULL);
	tests.emplace_back(BigInt{ std::numeric_limits<uint64_t>::max() }, 32235ULL);
	tests.emplace_back(
	    BigInt{ std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	            std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	            std::numeric_limits<uint64_t>::max() },
	    2ULL);

	tests.emplace_back(
	    BigInt{ std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	            std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max(),
	            std::numeric_limits<uint64_t>::max() },
	    32235ULL);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value(),
	                   125ULL);
	tests.emplace_back(BigInt::get_from_string("351326324642346363634634634636363").value(), 0LL);
	tests.emplace_back(BigInt::get_from_string("+0").value(), 125ULL);
	tests.emplace_back(
	    BigInt::get_from_string("252579235623235235235235235235235235235235235235256235723652756234"
	                            "732447474747473747234235631965137956139561395635623523756239562395"
	                            "62394238742375237351326324642346363634634634636363")
	        .value(),
	    125ULL);
	tests.emplace_back(BigInt::get_from_string("123241414214214222424").value(), 69ULL);

	for(const TestType& test : tests) {

		const auto& [value1, value2] = test;

		const BigInt actual_result = value1 >> value2;

		const BigIntTest result_expected = BigIntTest(value1) >> value2;

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
	}
}

static int64_t i64_mod_trunc(int64_t a, int64_t b) {
	return a % b;
}

// see: https://en.wikipedia.org/wiki/Modulo#Implementing_other_modulo_definitions_using_truncation
namespace {
/* Euclidean and Floored divmod, in the style of C's ldiv() */
typedef struct {
	/* This structure is part of the C stdlib.h, but is reproduced here for clarity */
	long int quot;
	long int rem;
} ldiv_t;

/* Euclidean division */
inline ldiv_t ldivE(long numer, long denom) {
	/* The C99 and C++11 languages define both of these as truncating. */
	long q = numer / denom;
	long r = numer % denom;
	if(r < 0) {
		if(denom > 0) {
			q = q - 1;
			r = r + denom;
		} else {
			q = q + 1;
			r = r - denom;
		}
	}
	return ldiv_t{ .quot = q, .rem = r };
}

/* Floored division */
inline ldiv_t ldivF(long numer, long denom) {
	long q = numer / denom;
	long r = numer % denom;
	if((r > 0 && denom < 0) || (r < 0 && denom > 0)) {
		q = q - 1;
		r = r + denom;
	}
	return ldiv_t{ .quot = q, .rem = r };
}
} // namespace

static int64_t i64_mod_floor(int64_t a, int64_t b) {
	return ldivF(a, b).rem;
}

static int64_t i64_mod_euclid(int64_t a, int64_t b) {
	return ldivE(a, b).rem;
}

TEST(BigInt, IntegerModTruncated) {
	using TestType = std::tuple<BigInt, BigInt, BigInt>;

	std::vector<TestType> tests{};

	{ // small tests
		// + % + => +
		tests.emplace_back(BigInt{ (uint64_t)200ULL }, BigInt{ (uint64_t)115ULL },
		                   BigInt{ (uint64_t)85ULL });
		// - % + => -
		tests.emplace_back(BigInt{ (int64_t)-200LL }, BigInt{ (uint64_t)115ULL },
		                   BigInt{ (int64_t)-85LL });
		// - % - => -
		tests.emplace_back(BigInt{ (int64_t)-200LL }, BigInt{ (int64_t)-115LL },
		                   BigInt{ (int64_t)-85LL });
		// + % - => +
		tests.emplace_back(BigInt{ (uint64_t)200ULL }, BigInt{ (int64_t)-115LL },
		                   BigInt{ (uint64_t)85ULL });
	}

	{
		BigInt first_part = "34145781491353196313134131241515731231314217452"_n;

		BigInt divisor = "21413498615801641394132131313"_n;

		BigInt remainder = "23141513513531414124124214"_n;

		EXPECT_TRUE(first_part.is_positive());
		EXPECT_TRUE(divisor.is_positive());
		EXPECT_TRUE(remainder.is_positive());

		EXPECT_LT(remainder, divisor);
		EXPECT_LT(divisor, first_part);

		BigInt actual_value = (first_part * divisor) + remainder;

		{ // big tests
			// + % + => +
			tests.emplace_back(actual_value.copy(), divisor.copy(), remainder.copy());
			// - % + => -
			tests.emplace_back(std::move(-(actual_value.copy())), divisor.copy(),
			                   std::move(-(remainder.copy())));
			// - % - => -
			tests.emplace_back(std::move(-(actual_value.copy())), std::move(-(divisor.copy())),
			                   std::move(-(remainder.copy())));
			// + % - => +
			tests.emplace_back(actual_value.copy(), std::move(-(divisor.copy())), remainder.copy());
		}
	}

	const ModuloRounding rounding = ModuloRoundingTruncated;

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const BigInt actual_result = value1.mod(value2, rounding);

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };

		const BigIntTest result_test = BigIntTest(value1).mod(BigIntTest(value2), rounding);

		EXPECT_EQ(result_test, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
	}
}

TEST(BigInt, IntegerModTruncatedCImpl) {
	using TestType = std::tuple<int64_t, int64_t, int64_t>;

	std::vector<TestType> tests{};

	{
		// + % + => +
		tests.emplace_back(200LL, 115LL, 85LL);
		// - % + => -
		tests.emplace_back(-200LL, 115LL, -85LL);
		// - % - => -
		tests.emplace_back(-200LL, -115LL, -85LL);
		// + % - => +
		tests.emplace_back(200LL, -115LL, 85LL);
	}

	const ModuloRounding rounding = ModuloRoundingTruncated;

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const auto value1_b = BigInt{ value1 };
		const auto value2_b = BigInt{ value2 };
		const auto result_expected_b = BigInt{ result_expected };

		const BigInt actual_result = value1_b.mod(value2_b, rounding);

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1_b } << ", " << BigIntDebug{ value2_b };

		const BigIntTest result_test = BigIntTest(value1).mod(BigIntTest(value2), rounding);

		EXPECT_EQ(result_test, result_expected_b)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };

		const uint64_t result_u64 = i64_mod_trunc(value1, value2);

		EXPECT_EQ(result_u64, result_expected) << "Input values: " << value1 << ", " << value2;
	}
}

TEST(BigInt, IntegerModFloored) {
	using TestType = std::tuple<BigInt, BigInt, BigInt>;

	std::vector<TestType> tests{};

	{ // small tests
		// + % + => +
		tests.emplace_back(BigInt{ (uint64_t)200ULL }, BigInt{ (uint64_t)115ULL },
		                   BigInt{ (uint64_t)85ULL });
		// - % + => +
		tests.emplace_back(BigInt{ (int64_t)-200LL }, BigInt{ (uint64_t)115ULL },
		                   BigInt{ (uint64_t)30ULL });
		// - % - => -
		tests.emplace_back(BigInt{ (int64_t)-200LL }, BigInt{ (int64_t)-115LL },
		                   BigInt{ (int64_t)-85LL });
		// + % - => -
		tests.emplace_back(BigInt{ (uint64_t)200ULL }, BigInt{ (int64_t)-115LL },
		                   BigInt{ (int64_t)-30LL });
	}

	{
		BigInt first_part = "34145781491353196313134131241515731231314217452"_n;

		BigInt divisor = "21413498615801641394132131313"_n;

		BigInt remainder = "23141513513531414124124214"_n;

		EXPECT_TRUE(first_part.is_positive());
		EXPECT_TRUE(divisor.is_positive());
		EXPECT_TRUE(remainder.is_positive());

		EXPECT_LT(remainder, divisor);
		EXPECT_LT(divisor, first_part);

		BigInt remainder_inverted = divisor - remainder;

		EXPECT_TRUE(remainder_inverted.is_positive());
		EXPECT_LT(remainder_inverted, divisor);

		BigInt actual_value = (first_part * divisor) + remainder;

		{ // big tests
			// + % + => +
			tests.emplace_back(actual_value.copy(), divisor.copy(), remainder.copy());
			// - % + => +
			tests.emplace_back(std::move(-(actual_value.copy())), divisor.copy(),
			                   remainder_inverted.copy());
			// - % - => -
			tests.emplace_back(std::move(-(actual_value.copy())), std::move(-(divisor.copy())),
			                   std::move(-(remainder.copy())));
			// + % - => -
			tests.emplace_back(actual_value.copy(), std::move(-(divisor.copy())),
			                   std::move(-(remainder_inverted.copy())));
		}
	}

	const ModuloRounding rounding = ModuloRoundingFloored;

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const BigInt actual_result = value1.mod(value2, rounding);

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };

		const BigIntTest result_test = BigIntTest(value1).mod(BigIntTest(value2), rounding);

		EXPECT_EQ(result_test, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
	}
}

TEST(BigInt, IntegerModFlooredCImpl) {
	using TestType = std::tuple<int64_t, int64_t, int64_t>;

	std::vector<TestType> tests{};

	{
		// + % + => +
		tests.emplace_back(200LL, 115LL, 85LL);
		// - % + => -
		tests.emplace_back(-200LL, 115LL, 30LL);
		// - % - => -
		tests.emplace_back(-200LL, -115LL, -85LL);
		// + % - => +
		tests.emplace_back(200LL, -115LL, -30LL);
	}

	const ModuloRounding rounding = ModuloRoundingFloored;

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const auto value1_b = BigInt{ value1 };
		const auto value2_b = BigInt{ value2 };
		const auto result_expected_b = BigInt{ result_expected };

		const BigInt actual_result = value1_b.mod(value2_b, rounding);

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1_b } << ", " << BigIntDebug{ value2_b };

		const BigIntTest result_test = BigIntTest(value1).mod(BigIntTest(value2), rounding);

		EXPECT_EQ(result_test, result_expected_b)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };

		const uint64_t result_u64 = i64_mod_floor(value1, value2);

		EXPECT_EQ(result_u64, result_expected) << "Input values: " << value1 << ", " << value2;
	}
}

TEST(BigInt, IntegerModCeiled) {
	using TestType = std::tuple<BigInt, BigInt, BigInt>;

	std::vector<TestType> tests{};

	{ // small tests
		// + % + => -
		tests.emplace_back(BigInt{ (uint64_t)200ULL }, BigInt{ (uint64_t)115ULL },
		                   BigInt{ (int64_t)-30LL });
		// - % + => -
		tests.emplace_back(BigInt{ (int64_t)-200LL }, BigInt{ (uint64_t)115ULL },
		                   BigInt{ (int64_t)-85LL });
		// - % - => +
		tests.emplace_back(BigInt{ (int64_t)-200LL }, BigInt{ (int64_t)-115LL },
		                   BigInt{ (uint64_t)30ULL });
		// + % - => +
		tests.emplace_back(BigInt{ (uint64_t)200ULL }, BigInt{ (int64_t)-115LL },
		                   BigInt{ (uint64_t)85ULL });
	}

	{
		BigInt first_part = "34145781491353196313134131241515731231314217452"_n;

		BigInt divisor = "21413498615801641394132131313"_n;

		BigInt remainder = "23141513513531414124124214"_n;

		EXPECT_TRUE(first_part.is_positive());
		EXPECT_TRUE(divisor.is_positive());
		EXPECT_TRUE(remainder.is_positive());

		EXPECT_LT(remainder, divisor);
		EXPECT_LT(divisor, first_part);

		BigInt remainder_inverted = divisor - remainder;

		EXPECT_TRUE(remainder_inverted.is_positive());
		EXPECT_LT(remainder_inverted, divisor);

		BigInt actual_value = (first_part * divisor) + remainder;

		{ // big tests
			// + % + => -
			tests.emplace_back(actual_value.copy(), divisor.copy(),
			                   std::move(-(remainder_inverted.copy())));
			// - % + => -
			tests.emplace_back(std::move(-(actual_value.copy())), divisor.copy(),
			                   std::move(-(remainder.copy())));
			// - % - => +
			tests.emplace_back(std::move(-(actual_value.copy())), std::move(-(divisor.copy())),
			                   remainder_inverted.copy());
			// + % - => +
			tests.emplace_back(actual_value.copy(), std::move(-(divisor.copy())), remainder.copy());
		}
	}

	const ModuloRounding rounding = ModuloRoundingCeiled;

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const BigInt actual_result = value1.mod(value2, rounding);

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };

		const BigIntTest result_test = BigIntTest(value1).mod(BigIntTest(value2), rounding);

		EXPECT_EQ(result_test, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
	}
}

TEST(BigInt, IntegerModEuclidean) {
	using TestType = std::tuple<BigInt, BigInt, BigInt>;

	std::vector<TestType> tests{};

	{ // small tests
		// + % + => +
		tests.emplace_back(BigInt{ (uint64_t)200ULL }, BigInt{ (uint64_t)115ULL },
		                   BigInt{ (uint64_t)85ULL });
		// - % + => +
		tests.emplace_back(BigInt{ (int64_t)-200LL }, BigInt{ (uint64_t)115ULL },
		                   BigInt{ (uint64_t)30ULL });
		// - % - => +
		tests.emplace_back(BigInt{ (int64_t)-200LL }, BigInt{ (int64_t)-115LL },
		                   BigInt{ (uint64_t)30ULL });
		// + % - => +
		tests.emplace_back(BigInt{ (uint64_t)200ULL }, BigInt{ (int64_t)-115LL },
		                   BigInt{ (uint64_t)85ULL });
	}

	{
		BigInt first_part = "34145781491353196313134131241515731231314217452"_n;

		BigInt divisor = "21413498615801641394132131313"_n;

		BigInt remainder = "23141513513531414124124214"_n;

		EXPECT_TRUE(first_part.is_positive());
		EXPECT_TRUE(divisor.is_positive());
		EXPECT_TRUE(remainder.is_positive());

		EXPECT_LT(remainder, divisor);
		EXPECT_LT(divisor, first_part);

		BigInt remainder_inverted = divisor - remainder;

		EXPECT_TRUE(remainder_inverted.is_positive());
		EXPECT_LT(remainder_inverted, divisor);

		BigInt actual_value = (first_part * divisor) + remainder;

		{ // big tests
			// + % + => +
			tests.emplace_back(actual_value.copy(), divisor.copy(), remainder.copy());
			// - % + => +
			tests.emplace_back(std::move(-(actual_value.copy())), divisor.copy(),
			                   remainder_inverted.copy());
			// - % - => +
			tests.emplace_back(std::move(-(actual_value.copy())), std::move(-(divisor.copy())),
			                   remainder_inverted.copy());
			// + % - => +
			tests.emplace_back(actual_value.copy(), std::move(-(divisor.copy())), remainder.copy());
		}
	}

	const ModuloRounding rounding = ModuloRoundingEuclidean;

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const BigInt actual_result = value1.mod(value2, rounding);

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };

		const BigIntTest result_test = BigIntTest(value1).mod(BigIntTest(value2), rounding);

		EXPECT_EQ(result_test, result_expected)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
	}
}

TEST(BigInt, IntegerModEuclideanCImpl) {
	using TestType = std::tuple<int64_t, int64_t, int64_t>;

	std::vector<TestType> tests{};

	{
		// + % + => +
		tests.emplace_back(200LL, 115LL, 85LL);
		// - % + => +
		tests.emplace_back(-200LL, 115LL, 30LL);
		// - % - => +
		tests.emplace_back(-200LL, -115LL, 30LL);
		// + % - => +
		tests.emplace_back(200LL, -115LL, 85LL);
	}

	const ModuloRounding rounding = ModuloRoundingEuclidean;

	for(const TestType& test : tests) {

		const auto& [value1, value2, result_expected] = test;

		const auto value1_b = BigInt{ value1 };
		const auto value2_b = BigInt{ value2 };
		const auto result_expected_b = BigInt{ result_expected };

		const BigInt actual_result = value1_b.mod(value2_b, rounding);

		EXPECT_EQ(actual_result, result_expected)
		    << "Input values: " << BigIntDebug{ value1_b } << ", " << BigIntDebug{ value2_b };

		const BigIntTest result_test = BigIntTest(value1).mod(BigIntTest(value2), rounding);

		EXPECT_EQ(result_test, result_expected_b)
		    << "Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };

		const uint64_t result_u64 = i64_mod_euclid(value1, value2);

		EXPECT_EQ(result_u64, result_expected) << "Input values: " << value1 << ", " << value2;
	}
}

/*
static std::string mod_rounding_to_str(ModuloRounding rounding) {
    switch(rounding) {
        case ModuloRoundingTruncated: {
            return "Truncated";
        }
        case ModuloRoundingFloored: {
            return "Floored";
        }
        case ModuloRoundingCeiled: {
            return "Ceiled";
        }
        case ModuloRoundingEuclidean: {
            return "Euclidean";
        }
        default: {
            return "<unknown>";
        }
    }
}

TEST(BigInt, IntegerModGeneric) {
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

    tests.emplace_back(BigInt::get_from_string("+1").value(),
                       BigInt::get_from_string("+2131215135135132515135").value());

    tests.emplace_back(BigInt::get_from_string("+1").value(),
                       BigInt::get_from_string("+2131215135135132515135").value());

    tests.emplace_back(BigInt::get_from_string("+0").value(),
                       BigInt::get_from_string("+2131215135135132515135").value());

    tests.emplace_back(BigInt::get_from_string("+2131215135135132515135").value(),
                       BigInt::get_from_string("+1").value());

    tests.emplace_back(BigInt::get_from_string("+21312151351351323495781541456378747474735463736465"
                                               "37364647384747474747474747566383938475727424515135")
                           .value(),
                       BigInt::get_from_string("+352785318753").value());

    tests.emplace_back(BigInt::get_from_string("+352785318753").value(),
                       BigInt::get_from_string("+21312151351351323495781541456378747474735463736465"
                                               "37364647384747474747474747566383938475727424515135")
                           .value());

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

    tests.emplace_back(
        BigInt::get_from_string(
            "35132632464234632432532749452368534748774747474747574563458934574389573498573458934758"
            "93475983457938457349857345845475475757777777777777777777777777777777777777777777777777"
            "77777775457457475477252574365782456785786827346752577328452378563279852978352379532578"
            "93255789088010871451780521572161276188761076576805218675867125867051287652186702768706"
            "54076257801256427855521708561270502716512761526781567085102678516780152678152678512671"
            "35267352167521367257615236715236715286780152367821678125678513267805236781526780152675"
            "8123671523363633532562340963427646346346363632")
            .value(),
        BigInt::get_from_string(
            "35132632464234632432532749452368534748774747474747574563458934574389573498573458934758"
            "93475983457938457349857345845475475757777777777777777777777777777777777777777777777777"
            "77777775457457475477252574365782456785786827346752577328452378563279852978352379532578"
            "93255789088010871451780521572161276188761076576805218675867125867051287652186702768706"
            "54076257801256427855521708561270502716512761526781567085102678516780152678152678512671"
            "35267352167521367257615236715236715286780152367821678125678513267805236781526780152675"
            "8123671523363633532562340963427646346346363631")
            .value());

    tests.emplace_back(BigInt::get_from_string("+3527313141353535152542385318753").value(),
                       BigInt::get_from_string("+2323").value());

    tests.emplace_back(BigInt::get_from_string("-3527313141353535152542385318753").value(),
                       BigInt::get_from_string("+2323").value());

    tests.emplace_back(BigInt::get_from_string("+3527313141353535152542385318753").value(),
                       BigInt::get_from_string("-2323").value());

    tests.emplace_back(BigInt::get_from_string("-3527313141353535152542385318753").value(),
                       BigInt::get_from_string("-2323").value());

    for(const TestType& test : tests) {

        const auto& [value1, value2] = test;

        const BigInt actual_result = value1 % value2;

        const BigIntTest result_expected = BigIntTest(value1) % BigIntTest(value2);

        EXPECT_EQ(actual_result, result_expected)
            << "(Default rounding) Input values: " << BigIntDebug{ value1 } << ", "
            << BigIntDebug{ value2 };

        auto roundings = { ModuloRoundingTruncated, ModuloRoundingFloored, ModuloRoundingCeiled,
                           ModuloRoundingEuclidean };

        for(ModuloRounding rounding : roundings) {

            const BigInt actual_result = value1.mod(value2, rounding);

            const BigIntTest result_expected = BigIntTest(value1).mod(BigIntTest(value2), rounding);

            EXPECT_EQ(actual_result, result_expected)
                << "Rounding mode: " << mod_rounding_to_str(rounding)
                << ", Input values: " << BigIntDebug{ value1 } << ", " << BigIntDebug{ value2 };
        }
    }
}
    */
