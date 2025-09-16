

#pragma once

#include <bigint_c.h>

#include <vector>

struct BigIntCPP {
  private:
	bool m_positive;
	std::vector<uint64_t> m_values;

  public:
	BigIntCPP(bool positive, std::vector<uint64_t> values);

	explicit BigIntCPP(const BigInt& big_int_c);

	BigIntCPP(const BigIntCPP&) = delete;
	BigIntCPP& operator=(const BigIntCPP&) = delete;
	BigIntCPP(BigIntCPP&& big_int) noexcept;
	BigIntCPP& operator=(BigIntCPP&& big_int) noexcept;

	[[nodiscard]] bool positive() const;
	[[nodiscard]] const std::vector<uint64_t>& values() const;
};

// helper thought just for the tests
[[nodiscard]] bool operator==(const BigInt& value1, const BigIntCPP& value2);
