
#pragma once

#ifndef __cplusplus
#error "Only allowed in c++ mode"
#endif

#include "./lib.h"

#include <compare>
#include <expected>
#include <ios>
#include <iostream>
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
			hash = hash ^ (std::hash<uint64_t>()(value.numbers[i]) + 0x9e3779b9 + (hash << 6) +
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

} // namespace bigint

struct BigInt {
  private:
	BigIntC m_c_value;

  public:
	BigInt(BigIntC&& c_value) noexcept;

	BigInt(uint64_t value) noexcept;

	BigInt(int64_t value) noexcept;

	template <typename... Args>
	    requires(sizeof...(Args) >= 2) &&
	            (std::conjunction_v<std::is_convertible<Args, uint64_t>...>)
	BigInt(Args... args) noexcept {

		std::vector<uint64_t> values = { static_cast<uint64_t>(args)... };
		// Use values...
		m_c_value = bigint_from_list_of_numbers(values.data(), values.size());
	}

	// TODO: make a constexpres literal in c++

	[[nodiscard]] static std::expected<BigInt, bigint::ParseError>
	get_from_string(const std::string& str) noexcept;

	explicit BigInt(const std::string& str);

	~BigInt() noexcept;

	BigInt(const BigInt&) = delete;
	BigInt& operator=(const BigInt&) = delete;

	BigInt(BigInt&& big_int) noexcept;

	BigInt& operator=(BigInt&& big_int) noexcept;

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

	// custom io manipulators for formatting BigInts
	static const decltype(std::ios_base::oct) bin =
	    std::ios_base::oct; // oct is not supported by this lib, so we can use that base for bin,
	                        // default for base is dec, so this is not set

	static const decltype(std::ios_base::oct) add_gaps =
	    std::ios_base::showpoint; // showpoint is not set by default

	static const decltype(std::ios_base::oct) trim_first_number =
	    std::ios_base::skipws; // skipws is set by default

	[[nodiscard]] std::ostream& operator<<(std::ostream& os) const;

	[[nodiscard]] std::istream& operator>>(std::istream& is) const;

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

namespace std {

template <> struct hash<BigInt> {
	std::size_t operator()(const BigInt& value) const noexcept { return value.hash(); }
};

std::string to_string(const BigInt& value);

} // namespace std

#ifdef BIGINT_C_CPP_IMPLEMENTATION

static std::string get_parse_error_message(MaybeBigIntError error) {
	std::string result = error.message;

	result += ": index ";
	result += std::to_string(error.index);

	if(error.symbol == NO_SYMBOL) {
		result += " symbol '";
		result += error.symbol;
		result += "'";
	}

	return result;
}

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
	m_c_value = bigint_from_unsigned_number(value);
}

BigInt::BigInt(int64_t value) noexcept {
	m_c_value = bigint_from_signed_number(value);
}

[[nodiscard]] std::expected<BigInt, bigint::ParseError>
BigInt::get_from_string(const std::string& str) noexcept {
	MaybeBigIntC result = maybe_bigint_from_string(str.c_str());

	if(maybe_bigint_is_error(result)) {
		return std::unexpected<bigint::ParseError>{ bigint::ParseError{
			maybe_bigint_get_error(result) } };
	}

	return std::expected<BigInt, bigint::ParseError>{ maybe_bigint_get_value(result) };
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
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] bool BigInt::operator/(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] bool BigInt::operator%(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] bool BigInt::operator^(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] BigInt& BigInt::operator-() {
	bigint_negate(&(this->m_c_value));
	return *this;
}

[[nodiscard]] BigInt& BigInt::operator+=(const BigInt& value2) {
	// TODO
	UNUSED(value2);
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] BigInt& BigInt::operator-=(const BigInt& value2) {
	// TODO
	UNUSED(value2);
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] BigInt& BigInt::operator*=(const BigInt& value2) {
	// TODO
	UNUSED(value2);
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] BigInt& BigInt::operator/=(const BigInt& value2) {
	// TODO
	UNUSED(value2);
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] BigInt& BigInt::operator%=(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] BigInt& BigInt::operator^=(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] std::ostream& BigInt::operator<<(std::ostream& os) const {
	std::ios_base::fmtflags flags = os.flags();

	bool prefix = (flags & std::ios_base::showbase) != 0;

	bool trim_first_number = (flags & BigInt::trim_first_number) != 0;

	bool add_gaps = (flags & BigInt::add_gaps) != 0;

	if((flags & std::ios_base::basefield) == std::ios_base::hex) {

		bool uppercase = (flags & std::ios_base::uppercase) != 0;

		char* temp =
		    bigint_to_string_hex(m_c_value, prefix, add_gaps, trim_first_number, uppercase);
		os << temp;
		free(temp);

	} else if((flags & std::ios_base::basefield) == BigInt::bin) {
		char* temp = bigint_to_string_bin(m_c_value, prefix, add_gaps, trim_first_number);
		os << temp;
		free(temp);
	} else {
		char* temp = bigint_to_string(m_c_value);
		os << temp;
		free(temp);
	}

	return os;
}

[[nodiscard]] std::istream& BigInt::operator>>(std::istream& is) const {
	// TODO
	UNUSED(is);
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] BigInt BigInt::operator<<(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] BigInt BigInt::operator>>(const BigInt& value2) const {
	// TODO
	UNUSED(value2);
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] BigInt& BigInt::operator>>=(const BigInt& value2) {
	// TODO
	UNUSED(value2);
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] BigInt& BigInt::operator<<=(const BigInt& value2) {
	// TODO
	UNUSED(value2);
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] BigInt& BigInt::operator++() {
	// TODO
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] BigInt& BigInt::operator--() {
	// TODO
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] BigInt BigInt::operator++(int) {
	// TODO
	UNREACHABLE_WITH_MSG("TODO");
}

[[nodiscard]] BigInt BigInt::operator--(int) {
	// TODO
	UNREACHABLE_WITH_MSG("TODO");
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
