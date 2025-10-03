

#define BIGINT_C_CPP_HIDE_C_LIB_FNS_AND_TYPES_IN_CPP
#include <bigint_cpp.hpp>

#include <gtest/gtest.h>

#include "../helper/helper.hpp"
#include "../helper/matcher.hpp"
#include "../helper/printer.hpp"

static_assert(
    "-13425264"_n !=
    "-3785236794684657956237952359752367925497467945237452345283452384523784235423584235482378523723742342634768234867348762823423467237463674687838346324236785623852365"_n);

TEST(BigInt, BigIntLiteral3) {
	std::string test =
	    "-37852367946846579562379523597523679254974679452374523452834523845237842354235842354823785"
	    "23723742342634768234867348762823423467237463674687838346324236785623852365";

	BigInt big_int = BigInt::get_from_string(test).value();

	constexpr const BigIntConstExpr cpp_literal =
	    "-3785236794684657956237952359752367925497467945237452345283452384523784235423584235482378523723742342634768234867348762823423467237463674687838346324236785623852365"_n;

	// as BigInt always needs allocations, the final value is constructed at runtime, but only a
	// malloc and memcpy needs to be done, so it's fatser than parsing, as that is done at
	// compile time.
	const BigInt runtime_converted_literal = cpp_literal;

	EXPECT_EQ(big_int, runtime_converted_literal) << "Input string: " << test;
}
