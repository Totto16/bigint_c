

#include "./helper.hpp"

BigIntCPP::BigIntCPP(bool positive, std::vector<uint64_t> values)
    : m_positive{ positive }, m_values{ std::move(values) } {}

BigIntCPP::BigIntCPP(const BigInt& big_int_c) : m_positive{ big_int_c.positive }, m_values{} {

	for(size_t i = 0; i < big_int_c.number_count; ++i) {
		m_values.push_back(big_int_c.numbers[i]);
	}
}

BigIntCPP::BigIntCPP(BigIntCPP&& big_int) noexcept
    : m_positive{ big_int.m_positive }, m_values{ std::move(big_int.m_values) } {}

BigIntCPP& BigIntCPP::operator=(BigIntCPP&& big_int) noexcept {

	if(this != &big_int) {
		m_values = std::move(big_int.m_values);
		m_positive = big_int.m_positive;
	}
	return *this;
}

[[nodiscard]] bool BigIntCPP::positive() const {
	return m_positive;
}

[[nodiscard]] const std::vector<uint64_t>& BigIntCPP::values() const {
	return m_values;
}

// helper thought just for the tests
[[nodiscard]] bool operator==(const BigInt& value1, const BigIntCPP& value2) {

	if(value1.positive != value2.positive()) {
		return false;
	}

	if(value1.number_count != value2.values().size()) {
		return false;
	}

	for(size_t i = 0; i < value1.number_count; ++i) {
		if(value1.numbers[i] != value2.values().at(i)) {
			return false;
		}
	}

	return true;
}
