

#include "./helper.hpp"

#include <gmp.h>

#include <stdexcept>

BigIntTest::BigIntTest(bool positive, std::vector<uint64_t> values) noexcept
    : m_positive{ positive }, m_values{ std::move(values) } {}

BigIntTest::BigIntTest(const BigInt& big_int_c) noexcept
    : m_positive{ big_int_c.underlying().positive }, m_values{} {

	for(size_t i = 0; i < big_int_c.underlying().number_count; ++i) {
		m_values.push_back(big_int_c.underlying().numbers[i]);
	}
}

BigIntTest::BigIntTest(BigIntTest&& big_int) noexcept
    : m_positive{ big_int.m_positive }, m_values{ std::move(big_int.m_values) } {}

BigIntTest& BigIntTest::operator=(BigIntTest&& big_int) noexcept {

	if(this != &big_int) {
		m_values = std::move(big_int.m_values);
		m_positive = big_int.m_positive;
	}
	return *this;
}

[[nodiscard]] bool BigIntTest::positive() const {
	return m_positive;
}

[[nodiscard]] const std::vector<uint64_t>& BigIntTest::values() const {
	return m_values;
}

// helper thought just for the tests
[[nodiscard]] bool operator==(const BigInt& value1, const BigIntTest& value2) {
	return value1.underlying() == value2;
}

[[nodiscard]] bool operator==(const BigIntImpl& value1, const BigIntTest& value2) {

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

static constexpr uint8_t CHUNK_BITS = 64;

// the same algorithm as in c, but using a external library, to verify the test result
// see https://gmplib.org/manual/Concept-Index for gmp docs
BigIntTest::BigIntTest(const std::string& str) : m_values{} {

	mpz_t big;
	mpz_init(big);

	// Parse the decimal string into the big integer
	if(mpz_set_str(big, str.c_str(), 10) != 0) {
		mpz_clear(big);
		throw std::runtime_error("Failed to parse bigint string");
	}

	size_t num_chunks = (mpz_sizeinbase(big, 2) + CHUNK_BITS - 1) / CHUNK_BITS;
	m_values.resize(num_chunks);

	mpz_t temp;
	mpz_init_set(temp, big);

	for(size_t i = 0; i < num_chunks; ++i) {
		m_values.at(i) = mpz_get_ui(temp);

		// Shift right by 64 bits, don't perform arithmetic shifts (affects negative values), see
		// https://gmplib.org/manual/Integer-Division
		mpz_tdiv_q_2exp(temp, temp, CHUNK_BITS);
	}

	// note: 0 is always positive here
	m_positive = mpz_sgn(big) >= 0;

	mpz_clear(big);
	mpz_clear(temp);
}
