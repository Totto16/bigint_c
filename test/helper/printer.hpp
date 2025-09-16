
#pragma once

#include "./helper.hpp"

#include <gtest/gtest.h>

void PrintTo(const BigIntCPP& value, std::ostream* os) {

	if(value.positive()) {
		*os << "+ ";
	} else {
		*os << "- ";
	}

	for(size_t i = 0; i < value.values().size(); ++i) {
		if(i != 0) {
			*os << ", ";
		}
		*os << value.values().at(i);
	}
	*os << "\n";
}

void PrintTo(const BigInt& value, std::ostream* os) {

	BigIntCPP value_cpp = BigIntCPP(value);

	PrintTo(value_cpp, os);
}

std::ostream& operator<<(std::ostream& os, const BigInt& value) {
	PrintTo(value, &os);
	return os;
}

std::ostream& operator<<(std::ostream& os, const BigIntCPP& value) {
	PrintTo(value, &os);
	return os;
}
