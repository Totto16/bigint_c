
#pragma once

#ifndef __cplusplus
#error "Only allowed in c++ mode"
#endif

#include "./lib.h"

#include <expected>
#include <stdexcept>
#include <string>

namespace std {
template <> struct hash<BigIntC> {
	std::size_t operator()(const BigIntC& value) const noexcept {
		std::size_t hash = std::hash<bool>()(value.positive);

		hash = hash ^ value.number_count;

		// see: https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector
		for(size_t i = 0; i < value.number_count; ++i) {
			hash = hash ^
			       std::hash<uint64_t>()(value.numbers[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		}

		return hash;
	}
};
} // namespace std

struct BigInt {
  private:
	BigIntC m_c_value;

  public:
	BigInt(BigIntC&& c_value) noexcept : m_c_value{ c_value } {

		c_value.numbers = nullptr;
		c_value.number_count = 0;
	}

	BigInt(uint64_t value) noexcept { m_c_value = bigint_from_unsigned_number(value); }

	BigInt(int64_t value) noexcept { m_c_value = bigint_from_signed_number(value); }

	// TODO: make a constexpres literal in c++

	[[nodiscard]] static std::expected<BigInt, std::string>
	get_from_string(const std::string& str) noexcept {
		MaybeBigIntC result = maybe_bigint_from_string(str.c_str());

		if(maybe_bigint_is_error(result)) {
			return std::unexpected<std::string>{ std::string{ maybe_bigint_get_error(result) } };
		}

		return maybe_bigint_get_value(result);
	}

	explicit BigInt(const std::string& str) {
		MaybeBigIntC result = maybe_bigint_from_string(str.c_str());

		if(maybe_bigint_is_error(result)) {
			throw std::runtime_error(std::string{ maybe_bigint_get_error(result) });
		}

		m_c_value = maybe_bigint_get_value(result);
	}

	~BigInt() noexcept {
		if(m_c_value.numbers != nullptr) {
			free(m_c_value.numbers);
		}
	}

	BigInt(const BigInt&) = delete;
	BigInt& operator=(const BigInt&) = delete;

	BigInt(BigInt&& big_int) noexcept : m_c_value{ big_int.m_c_value } {

		big_int.m_c_value.number_count = 0;
		big_int.m_c_value.numbers = nullptr;
	}

	BigInt& operator=(BigInt&& big_int) noexcept {

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
	[[nodiscard]] operator const BigIntC&() const { return m_c_value; }

	[[nodiscard]] const BigIntC& underlying() const { return m_c_value; }
#endif

	[[nodiscard]] std::strong_ordering operator<=>(const BigInt& value2) const {
		int8_t value = bigint_compare_bigint(this->m_c_value, value2.m_c_value);

		if(value == 0) {
			return std::strong_ordering::equal;
		}

		return value > 0 ? std::strong_ordering::greater : std::strong_ordering::less;
	}

	[[nodiscard]] bool operator==(const BigInt& value2) const {
		return bigint_eq_bigint(this->m_c_value, value2.m_c_value);
	}

	[[nodiscard]] bool operator!=(const BigInt& value2) const { return !(*this == value2); }

	[[nodiscard]] bool operator>=(const BigInt& value2) const { return (*this <=> value2) >= 0; }

	[[nodiscard]] bool operator>(const BigInt& value2) const { return (*this <=> value2) > 0; }

	[[nodiscard]] bool operator<=(const BigInt& value2) const { return (*this <=> value2) <= 0; }

	[[nodiscard]] bool operator<(const BigInt& value2) const { return (*this <=> value2) < 0; }

	[[nodiscard]] bool operator+(const BigInt& value2) const {
		// TODO
		UNUSED(value2);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] bool operator-(const BigInt& value2) const {
		// TODO
		UNUSED(value2);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] bool operator*(const BigInt& value2) const {
		// TODO
		UNUSED(value2);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] bool operator/(const BigInt& value2) const {
		// TODO
		UNUSED(value2);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] bool operator%(const BigInt& value2) const {
		// TODO
		UNUSED(value2);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] bool operator^(const BigInt& value2) const {
		// TODO
		UNUSED(value2);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] BigInt& operator-() {
		// TODO
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] BigInt& operator+=(const BigInt& value2) {
		// TODO
		UNUSED(value2);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] BigInt& operator-=(const BigInt& value2) {
		// TODO
		UNUSED(value2);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] BigInt& operator*=(const BigInt& value2) {
		// TODO
		UNUSED(value2);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] BigInt& operator/=(const BigInt& value2) {
		// TODO
		UNUSED(value2);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] BigInt& operator%=(const BigInt& value2) const {
		// TODO
		UNUSED(value2);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] BigInt& operator^=(const BigInt& value2) const {
		// TODO
		UNUSED(value2);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] std::ostream& operator<<(std::ostream& os) const {
		// TODO
		UNUSED(os);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] std::istream& operator>>(std::istream& is) const {
		// TODO
		UNUSED(is);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] bool operator>>=(const BigInt& value2) const {
		// TODO
		UNUSED(value2);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] bool operator<<=(const BigInt& value2) const {
		// TODO
		UNUSED(value2);
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] BigInt& operator++() {
		// TODO
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] BigInt& operator--() {
		// TODO
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] BigInt operator++(int) {
		// TODO
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] BigInt operator--(int) {
		// TODO
		UNREACHABLE_WITH_MSG("TODO");
	}

	[[nodiscard]] std::string to_string() const {
		return std::string{ bigint_to_string(m_c_value) };
	}

	[[nodiscard]] explicit operator std::string() { return this->to_string(); }

	[[nodiscard]] std::size_t hash() const { return std::hash<BigIntC>()(m_c_value); }
};

namespace std {

template <> struct hash<BigInt> {
	std::size_t operator()(const BigInt& value) const noexcept { return value.hash(); }
};

std::string to_string(const BigInt& value);

} // namespace std

#ifdef BIGINT_C_HIDE_C_LIB_FNS_AND_TYPES_IN_CPP

#define BigIntC undef
#define MaybeBigIntC undef
#define maybe_bigint_is_error undef
#define maybe_bigint_get_value undef
#define maybe_bigint_get_error undef
#define maybe_bigint_from_string undef
#define bigint_from_unsigned_number undef
#define bigint_from_signed_number undef
#define free_bigint undef
#define free_bigint_without_reset undef
#define bigint_to_string undef
#define bigint_add_bigint undef
#define bigint_sub_bigint undef
#define bigint_eq_bigint undef
#define bigint_compare_bigint undef

#endif
