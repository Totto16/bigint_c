#pragma once

#ifndef __cplusplus
#error "Only allowed in c++ mode"
#endif

#include "../lib.h"

#include "./literal.hpp"

#include <compare>
#include <expected>
#include <ios>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace std {
template <> struct hash<BigIntC> {
	std::size_t operator()(const BigIntC& value) const noexcept {
		std::size_t hash = std::hash<bool>()(value.positive);

		hash = hash ^ value.number_count;

		// see: https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector
		for(size_t i = 0; i < value.number_count; ++i) {
			hash =
			    hash ^
			    (std::hash<uint64_t>()(
			         value.numbers[i]) + // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			     0x9e3779b9 + // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
			     (hash
			      << 6) + // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
			     (hash >> 2));
		}

		return hash;
	}
};
} // namespace std

namespace bigint {

class ParseError final : public std::runtime_error {
  private:
	MaybeBigIntError m_c_error;

  public:
	explicit ParseError(MaybeBigIntError error);

	ParseError(ConstStr message, size_t index);

	ParseError(ConstStr message, size_t index, StrType symbol);

	[[nodiscard]] const char* message() const noexcept;

	[[nodiscard]] size_t index() const noexcept;

	[[nodiscard]] std::optional<char> symbol() const noexcept;
};

namespace ios {
// custom io manipulators for formatting BigInts

// oct is not supported by this lib, so we can use that base for bin,
// showpoint is not set by default
constexpr const inline auto& bin = // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    std::oct;
// default for base is dec, so this is not set

constexpr const inline auto&
    add_gaps = // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    std::showpoint;

constexpr const inline auto&
    no_add_gaps = // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    std::noshowpoint;

constexpr const inline auto&
    trim_first_number = // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    std::skipws;        // skipws is set by default

constexpr const inline auto&
    no_trim_first_number = // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    std::noskipws;

} // namespace ios
} // namespace bigint

namespace { // NOLINT(cert-dcl59-cpp,google-build-namespaces)
namespace bigint_ios {
constexpr const auto bin_flag = std::ios_base::oct;
constexpr const auto add_gaps_flag = std::ios_base::showpoint;
constexpr const auto trim_first_number_flag = std::ios_base::skipws;
} // namespace bigint_ios
} // namespace

struct BigInt {
  private:
	BigIntC m_c_value;

  public:
	BigInt(BigIntC&& c_value) noexcept; // NOLINT(google-explicit-constructor)

	BigInt(uint64_t value) noexcept; // NOLINT(google-explicit-constructor)

	BigInt(int64_t value) noexcept; // NOLINT(google-explicit-constructor)

	template <typename... Args>
	    requires(sizeof...(Args) >= 2) &&
	            (std::conjunction_v<std::is_convertible<Args, uint64_t>...>)
	BigInt(Args... args) noexcept : m_c_value{} { // NOLINT(google-explicit-constructor)

		std::vector<uint64_t> values = { static_cast<uint64_t>( // GCOVR_EXCL_BR_LINE (c++ template)
			args)... };                                         // GCOVR_EXCL_BR_LINE (c++ template)
		m_c_value = bigint_from_list_of_numbers(values.data(),  // GCOVR_EXCL_BR_LINE (c++ template)
		                                        values.size()); // GCOVR_EXCL_BR_LINE (c++ template)
	}

	[[nodiscard]] static std::expected<BigInt, bigint::ParseError>
	get_from_string(const std::string& str) noexcept;

	/**
	 * @brief Construct a new Big Int object
	 * @throws bigint::ParseError - when the string is invalid
	 * @see @link{BigInt::get_from_string} for a non throwing API
	 * @param str
	 */
	explicit BigInt(const std::string& str);

	~BigInt() noexcept;

	BigInt(const BigInt&) = delete;
	BigInt& operator=(const BigInt&) = delete;

	BigInt(BigInt&& big_int) noexcept;

	BigInt& operator=(BigInt&& big_int) noexcept;

	template <size_t N>
	BigInt(const BigIntConstExpr<N>& big_int) : m_c_value{} { // NOLINT(google-explicit-constructor)

		BigIntC c_value = { .positive = big_int.positive,
			                .numbers = nullptr,
			                .number_count = big_int.numbers.size() };

		auto* const new_numbers =
		    static_cast<uint64_t*>(realloc( // NOLINT(cppcoreguidelines-no-malloc)
		        c_value.numbers, sizeof(uint64_t) * c_value.number_count));

		if(new_numbers == NULL) {                                      // GCOVR_EXCL_BR_LINE (OOM)
			throw std::runtime_error(                                  // GCOVR_EXCL_LINE (OOM
			                                                           // content)
			    "realloc failed, no error handling implemented here"); // GCOVR_EXCL_LINE (OOM
			                                                           // content)
		} // GCOVR_EXCL_LINE (OOM content)

		c_value.numbers = new_numbers;

		for(size_t i = 0; i < c_value.number_count; ++i) {
			c_value.numbers[i] = // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			    big_int.numbers[i];
		}

		this->m_c_value = c_value;
	}

#ifdef BIGINT_C_CPP_ACCESS_TO_UNDERLYING_C_DATA
	[[nodiscard]] explicit operator const BigIntC&() const;

	[[nodiscard]] const BigIntC& underlying() const;
#endif

	[[nodiscard]] std::strong_ordering operator<=>(const BigInt& value2) const;

	[[nodiscard]] bool operator==(const BigInt& value2) const;

	[[nodiscard]] bool operator!=(const BigInt& value2) const;

	[[nodiscard]] bool operator>=(const BigInt& value2) const;

	[[nodiscard]] bool operator>(const BigInt& value2) const;

	[[nodiscard]] bool operator<=(const BigInt& value2) const;

	[[nodiscard]] bool operator<(const BigInt& value2) const;

	[[nodiscard]] BigInt operator+(const BigInt& value2) const;

	[[nodiscard]] BigInt operator-(const BigInt& value2) const;

	[[nodiscard]] bool operator*(const BigInt& value2) const;

	[[nodiscard]] bool operator/(const BigInt& value2) const;

	[[nodiscard]] bool operator%(const BigInt& value2) const;

	[[nodiscard]] bool operator^(const BigInt& value2) const;

	[[nodiscard]] BigInt& operator-();

	[[nodiscard]] BigInt& operator+=(const BigInt& value2);

	[[nodiscard]] BigInt& operator-=(const BigInt& value2);

	[[nodiscard]] BigInt& operator*=(const BigInt& value2);

	[[nodiscard]] BigInt& operator/=(const BigInt& value2);

	[[nodiscard]] BigInt& operator%=(const BigInt& value2) const;

	[[nodiscard]] BigInt& operator^=(const BigInt& value2) const;

	[[nodiscard]] BigInt operator<<(const BigInt& value2) const;

	[[nodiscard]] BigInt operator>>(const BigInt& value2) const;

	[[nodiscard]] BigInt& operator>>=(const BigInt& value2);

	[[nodiscard]] BigInt& operator<<=(const BigInt& value2);

	[[nodiscard]] BigInt& operator++();

	[[nodiscard]] BigInt& operator--();

	[[nodiscard]] BigInt operator++(int);

	[[nodiscard]] BigInt operator--(int);

	[[nodiscard]] std::string to_string() const;

	[[nodiscard]] std::string to_string_hex(bool prefix = true, bool add_gaps = false,
	                                        bool trim_first_number = true,
	                                        bool uppercase = true) const;

	[[nodiscard]] std::string to_string_bin(bool prefix = true, bool add_gaps = false,
	                                        bool trim_first_number = true) const;

	[[nodiscard]] explicit operator std::string();

	[[nodiscard]] std::size_t hash() const;

	[[nodiscard]] BigInt copy() const;
};

std::ostream& operator<<(std::ostream& out_stream, const BigInt& value);

std::istream& operator>>(std::istream& in_stream, const BigInt& value);

namespace std {

template <> struct hash<BigInt> {
	std::size_t operator()(const BigInt& value) const noexcept {
		return value.hash(); // GCOVR_EXCL_BR_LINE (c++ destructor or constructor branches)
	}
};

std::string to_string(const BigInt& value); // NOLINT(cert-dcl58-cpp)

} // namespace std

#ifdef BIGINT_C_CPP_IMPLEMENTATION

static std::string get_parse_error_message(MaybeBigIntError error) {
	std::string result = // GCOVR_EXCL_BR_LINE (c++ string initialization, makes allocation)
	    error.message;   // GCOVR_EXCL_BR_LINE (c++ string initialization, makes allocation)

	result += ": index ";     // GCOVR_EXCL_BR_LINE (c++ string concatenation, makes allocation)
	result += std::to_string( // GCOVR_EXCL_BR_LINE (c++ string concatenation, makes allocation)
	    error.index);

	if(error.symbol == NO_SYMBOL) {
		result += " symbol '";  // GCOVR_EXCL_BR_LINE (c++ string concatenation, makes allocation)
		result += error.symbol; // GCOVR_EXCL_BR_LINE (c++ string concatenation, makes allocation)
		result += "'";          // GCOVR_EXCL_BR_LINE (c++ string concatenation, makes allocation)
	}

	return result;
} // GCOVR_EXCL_BR_LINE (c++ destructor branches)

bigint::ParseError::ParseError(MaybeBigIntError error)
    : std::runtime_error{ get_parse_error_message(error) }, m_c_error{ error } {}

bigint::ParseError::ParseError(ConstStr message, size_t index)
    : ParseError{ MaybeBigIntError{ .message = message, .index = index, .symbol = NO_SYMBOL } } {}

bigint::ParseError::ParseError(ConstStr message, size_t index, StrType symbol)
    : ParseError{ MaybeBigIntError{ .message = message, .index = index, .symbol = symbol } } {}

[[nodiscard]] const char* bigint::ParseError::message() const noexcept {
	return m_c_error.message;
}

[[nodiscard]] size_t bigint::ParseError::index() const noexcept {
	return m_c_error.index;
}

[[nodiscard]] std::optional<char> bigint::ParseError::symbol() const noexcept {
	if(m_c_error.symbol == NO_SYMBOL) {
		return std::nullopt;
	}

	return m_c_error.symbol;
}

BigInt::BigInt(BigIntC&& c_value) noexcept : m_c_value{ c_value } {

	c_value.numbers = nullptr;
	c_value.number_count = 0;
}

BigInt::BigInt(uint64_t value) noexcept {
	m_c_value = bigint_from_unsigned_number(value); // GCOVR_EXCL_BR_LINE (c++ assignment branches)
}

BigInt::BigInt(int64_t value) noexcept {
	m_c_value = bigint_from_signed_number(value); // GCOVR_EXCL_BR_LINE (c++ assignment branches)
}

[[nodiscard]] std::expected<BigInt, bigint::ParseError>
BigInt::get_from_string(const std::string& str) noexcept {
	MaybeBigIntC result =                      // GCOVR_EXCL_BR_LINE (c++ assignment branches)
	    maybe_bigint_from_string(str.c_str()); // GCOVR_EXCL_BR_LINE (c++ assignment branches)

	if(maybe_bigint_is_error(result)) {
		return std::unexpected<bigint::ParseError>{ bigint::ParseError{
			// GCOVR_EXCL_BR_LINE (c++ assignment branches)
			maybe_bigint_get_error(result) } }; // GCOVR_EXCL_BR_LINE (c++ assignment branches)
	}

	return std::expected<BigInt, bigint::ParseError>{
		maybe_bigint_get_value(result) // GCOVR_EXCL_BR_LINE (c++ RVO branches)
	}; // GCOVR_EXCL_BR_LINE (c++ RVO branches)
}

BigInt::BigInt(const std::string& str) {
	std::expected<BigInt, bigint::ParseError> result = BigInt::get_from_string(str);

	if(!result.has_value()) {
		throw result.error();
	}

	*this = std::move(result.value());
}

BigInt::~BigInt() noexcept {
	if(m_c_value.numbers != nullptr) {
		free_bigint(&m_c_value);
	}
}

BigInt::BigInt(BigInt&& big_int) noexcept : m_c_value{ big_int.m_c_value } {

	big_int.m_c_value.number_count = 0;
	big_int.m_c_value.numbers = nullptr;
}

BigInt& BigInt::operator=(BigInt&& big_int) noexcept {

	if(this != &big_int) {
		this->m_c_value.positive = big_int.m_c_value.positive;
		this->m_c_value.number_count = big_int.m_c_value.number_count;
		this->m_c_value.numbers = big_int.m_c_value.numbers;

		big_int.m_c_value.number_count = 0;
		big_int.m_c_value.numbers = nullptr;
	}
	return *this;
}

#ifdef BIGINT_C_CPP_ACCESS_TO_UNDERLYING_C_DATA
[[nodiscard]] BigInt::operator const BigIntC&() const {
	return m_c_value;
}

[[nodiscard]] const BigIntC& BigInt::underlying() const {
	return m_c_value;
}
#endif

[[nodiscard]] std::strong_ordering BigInt::operator<=>(const BigInt& value2) const {
	int8_t value = bigint_compare_bigint(this->m_c_value, value2.m_c_value);

	if(value == 0) {
		return std::strong_ordering::equal;
	}

	return value > 0 ? std::strong_ordering::greater : std::strong_ordering::less;
}

[[nodiscard]] bool BigInt::operator==(const BigInt& value2) const {
	return bigint_eq_bigint(this->m_c_value, value2.m_c_value);
}

[[nodiscard]] bool BigInt::operator!=(const BigInt& value2) const {
	return !(*this == value2);
}

[[nodiscard]] bool BigInt::operator>=(const BigInt& value2) const {
	return (*this <=> value2) >= 0;
}

[[nodiscard]] bool BigInt::operator>(const BigInt& value2) const {
	return (*this <=> value2) > 0;
}

[[nodiscard]] bool BigInt::operator<=(const BigInt& value2) const {
	return (*this <=> value2) <= 0;
}

[[nodiscard]] bool BigInt::operator<(const BigInt& value2) const {
	return (*this <=> value2) < 0;
}

[[nodiscard]] BigInt BigInt::operator+(const BigInt& value2) const {

	BigIntC result = bigint_add_bigint(this->m_c_value, value2.m_c_value);

	return BigInt{ std::move(result) };
}

[[nodiscard]] BigInt BigInt::operator-(const BigInt& value2) const {
	BigIntC result = bigint_sub_bigint(this->m_c_value, value2.m_c_value);

	return BigInt{ std::move(result) };
}

[[nodiscard]] bool BigInt::operator*(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	throw std::runtime_error("TODO");
}

[[nodiscard]] bool BigInt::operator/(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	throw std::runtime_error("TODO");
}

[[nodiscard]] bool BigInt::operator%(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	throw std::runtime_error("TODO");
}

[[nodiscard]] bool BigInt::operator^(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	throw std::runtime_error("TODO");
}

[[nodiscard]] BigInt& BigInt::operator-() {
	bigint_negate(&(this->m_c_value));
	return *this;
}

[[nodiscard]] BigInt& BigInt::operator+=(const BigInt& value2) {
	// TODO
	UNUSED(value2);
	throw std::runtime_error("TODO");
}

[[nodiscard]] BigInt& BigInt::operator-=(const BigInt& value2) {
	// TODO
	UNUSED(value2);
	throw std::runtime_error("TODO");
}

[[nodiscard]] BigInt& BigInt::operator*=(const BigInt& value2) {
	// TODO
	UNUSED(value2);
	throw std::runtime_error("TODO");
}

[[nodiscard]] BigInt& BigInt::operator/=(const BigInt& value2) {
	// TODO
	UNUSED(value2);
	throw std::runtime_error("TODO");
}

[[nodiscard]] BigInt& BigInt::operator%=(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	throw std::runtime_error("TODO");
}

[[nodiscard]] BigInt& BigInt::operator^=(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	throw std::runtime_error("TODO");
}

std::ostream& operator<<(std::ostream& out_stream, const BigInt& value) {
	std::ios_base::fmtflags flags = out_stream.flags();

	bool prefix = (flags & std::ios_base::showbase) != 0;

	bool trim_first_number = (flags & bigint_ios::trim_first_number_flag) != 0;

	bool add_gaps = (flags & bigint_ios::add_gaps_flag) != 0;

	if((flags & std::ios_base::basefield) == std::ios_base::hex) {

		bool uppercase = (flags & std::ios_base::uppercase) != 0;

		std::string temp = value.to_string_hex(prefix, add_gaps, trim_first_number, uppercase);
		out_stream << temp;

	} else if((flags & std::ios_base::basefield) == bigint_ios::bin_flag) {
		std::string temp = value.to_string_bin(prefix, add_gaps, trim_first_number);
		out_stream << temp;
	} else {
		std::string temp = value.to_string();
		out_stream << temp;
	}

	return out_stream;
}

std::istream& operator>>(std::istream& in_stream, const BigInt& value) {
	// TODO
	UNUSED(in_stream);
	UNUSED(value);
	throw std::runtime_error("TODO");
}

[[nodiscard]] BigInt BigInt::operator<<(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	throw std::runtime_error("TODO");
}

[[nodiscard]] BigInt BigInt::operator>>(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	throw std::runtime_error("TODO");
}

[[nodiscard]] BigInt& BigInt::operator>>=(const BigInt& value2) {
	// TODO
	UNUSED(value2);
	throw std::runtime_error("TODO");
}

[[nodiscard]] BigInt& BigInt::operator<<=(const BigInt& value2) {
	// TODO
	UNUSED(value2);
	throw std::runtime_error("TODO");
}

[[nodiscard]] BigInt& BigInt::operator++() {
	// TODO
	throw std::runtime_error("TODO");
}

[[nodiscard]] BigInt& BigInt::operator--() {
	// TODO
	throw std::runtime_error("TODO");
}

[[nodiscard]] BigInt BigInt::operator++(int) {
	// TODO
	throw std::runtime_error("TODO");
}

[[nodiscard]] BigInt BigInt::operator--(int) {
	// TODO
	throw std::runtime_error("TODO");
}

[[nodiscard]] std::string BigInt::to_string() const {
	char* temp = bigint_to_string(m_c_value);
	std::string result{ temp };
	free(temp);
	return result;
}

[[nodiscard]] std::string BigInt::to_string_hex(bool prefix, bool add_gaps, bool trim_first_number,
                                                bool uppercase) const {
	char* temp = bigint_to_string_hex(m_c_value, prefix, add_gaps, trim_first_number, uppercase);
	std::string result{ temp };
	free(temp);
	return result;
}

[[nodiscard]] std::string BigInt::to_string_bin(bool prefix, bool add_gaps,
                                                bool trim_first_number) const {
	char* temp = bigint_to_string_bin(m_c_value, prefix, add_gaps, trim_first_number);
	std::string result{ temp };
	free(temp);
	return result;
}

[[nodiscard]] BigInt::operator std::string() {
	return this->to_string();
}

[[nodiscard]] std::size_t BigInt::hash() const {
	return std::hash<BigIntC>()(m_c_value);
}

[[nodiscard]] BigInt BigInt::copy() const {

	BigIntC copy = bigint_copy(this->m_c_value);

	return BigInt(std::move(copy));
}

std::string std::to_string(const BigInt& value) {
	return value.to_string();
}

#endif

#ifdef BIGINT_C_CPP_HIDE_C_LIB_FNS_AND_TYPES_IN_CPP

#define UNDEF #error "UNDEFINED"

#define BigIntC UNDEF
#define MaybeBigIntC UNDEF
#define MaybeBigIntError UNDEF
#undef NO_SYMBOL
#define maybe_bigint_is_error UNDEF
#define maybe_bigint_get_value UNDEF
#define maybe_bigint_get_error UNDEF
#define maybe_bigint_from_string UNDEF
#define bigint_from_unsigned_number UNDEF
#define bigint_from_signed_number UNDEF
#define free_bigint UNDEF
#define free_bigint_without_reset UNDEF
#define bigint_to_string UNDEF
#define bigint_add_bigint UNDEF
#define bigint_sub_bigint UNDEF
#define bigint_eq_bigint UNDEF
#define bigint_compare_bigint UNDEF
#define bigint_copy UNDEF
#define bigint_from_list_of_numbers UNDEF
#define bigint_to_string_hex UNDEF
#define bigint_to_string_bin UNDEF

#endif
