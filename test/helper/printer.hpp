#pragma once

#include "./helper.hpp"

#include <expected>
#include <gtest/gtest.h>

void PrintTo(const BigIntTest& value, std::ostream* os) {

	if(value.positive()) {
		*os << "+ ";
	} else {
		*os << "- ";
	}

	for(size_t i = 0; i < value.values().size(); ++i) {
		if(i != 0) {
			*os << ", ";
		}
		*os << value.values().at(value.values().size() - i - 1);
	}
	*os << "\n";
}

void PrintTo(const BigInt& value, std::ostream* os) {

	BigIntTest value_cpp = BigIntTest(value);

	PrintTo(value_cpp, os);
}

std::ostream& operator<<(std::ostream& os, const BigInt& value) {
	PrintTo(value, &os);
	return os;
}

std::ostream& operator<<(std::ostream& os, const BigIntTest& value) {
	PrintTo(value, &os);
	return os;
}

namespace std {

// make std::expected and std::optional printable
template <typename T, typename S> void PrintTo(const expected<T, S>& value, std::ostream* os) {
	if(value.has_value()) {
		*os << "<Expected.Value>: " << ::testing::PrintToString<T>(value.value());
	} else {
		*os << "<Expected.Error>: " << ::testing::PrintToString<S>(value.error());
	}
}

template <typename T, typename S>
std::ostream& operator<<(std::ostream& os, const expected<T, S>& value) {
	PrintTo<T, S>(value, &os);
	return os;
}

template <typename T> void PrintTo(const optional<T>& value, std::ostream* os) {
	if(value.has_value()) {
		*os << "<Optional.Value>: " << ::testing::PrintToString<T>(value.value());
	} else {
		*os << "<Optional.Empty>";
	}
}

template <typename T> std::ostream& operator<<(std::ostream& os, const optional<T>& value) {
	PrintTo<T>(value, &os);
	return os;
}

} // namespace std

namespace bigint {

void PrintTo(const ParseError& value, std::ostream* os) {

	*os << "ParseError: " << value.message() << ": " << value.index() << " '" << value.symbol()
	    << "'";
}
} // namespace bigint
