

#include <bigint_c.h>

#include <gtest/gtest.h>

#include "../helper/helper.hpp"
#include "../helper/matcher.hpp"
#include "../helper/printer.hpp"

TEST(BigIntCFuncs, FreeAllowsNull) {

	// checks that no assertion or similar occurs
	auto temp = []() {
		free_bigint(nullptr);
		std::exit(0);
	};

	EXPECT_EXIT(temp(), testing::ExitedWithCode(0), testing::Eq(""));
}

TEST(BigIntCFuncs, InvalidNumbersFree) {

	// checks that no assertion or similar occurs
	auto temp = []() {
		BigIntC big_int_c = { .positive = true, .numbers = NULL, .number_count = 0 };
		free_bigint(&big_int_c);
		std::exit(0);
	};

	EXPECT_EXIT(temp(), testing::ExitedWithCode(0), testing::Eq(""));
}
