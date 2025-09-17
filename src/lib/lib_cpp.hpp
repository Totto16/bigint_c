
#pragma once

#ifndef __cplusplus
#error "Only allowed in c++ mode"
#endif

#include "./lib.h"

#include <stdexcept>
#include <string>

struct BigInt {
  private:
	BigIntImpl m_c_value;

  public:
	BigInt(BigIntImpl&& c_value) noexcept : m_c_value{ c_value } {

		c_value.numbers = nullptr;
		c_value.number_count = 0;
	}

	explicit BigInt(const std::string& str) {
		MaybeBigInt result = maybe_bigint_from_string(str.c_str());

		if(maybe_bigint_is_error(result)) {
			throw std::runtime_error(std::string{ maybe_bigint_get_error(result) });
		}

		m_c_value = maybe_bigint_get_value(result);
	}

	~BigInt() {
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

	[[nodiscard]] operator const BigIntImpl&() const { return m_c_value; }

	[[nodiscard]] const BigIntImpl& underlying() const { return m_c_value; }

	[[nodiscard]] uint8_t operator<=>(const BigInt& value2) const {
		return bigint_compare_bigint(this->m_c_value, value2.m_c_value);
	}

	[[nodiscard]] bool operator==(const BigInt& value2) const {
		return bigint_eq_bigint(this->m_c_value, value2.m_c_value);
	}

	[[nodiscard]] bool operator!=(const BigInt& value2) const { return !(*this == value2); }

	[[nodiscard]] bool operator>=(const BigInt& value2) const { return (*this <=> value2) >= 0; }

	[[nodiscard]] bool operator>(const BigInt& value2) const { return (*this <=> value2) > 0; }

	[[nodiscard]] bool operator<=(const BigInt& value2) const { return (*this <=> value2) <= 0; }

	[[nodiscard]] bool operator<(const BigInt& value2) const { return (*this <=> value2) < 0; }
};
