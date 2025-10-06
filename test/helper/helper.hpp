#pragma once

#include <bigint_c.h>

#include <string>
#include <vector>

struct BigIntTest {
  private:
	bool m_positive;
	std::vector<uint64_t> m_values;

  public:
	BigIntTest(bool positive, std::vector<uint64_t> values) noexcept;

	explicit BigIntTest(const BigInt& big_int_c) noexcept;
	explicit BigIntTest(const std::string& str);

	explicit BigIntTest(const uint64_t& number);
	explicit BigIntTest(const int64_t& number);

	BigIntTest(const BigIntTest&) = delete;
	BigIntTest& operator=(const BigIntTest&) = delete;
	BigIntTest(BigIntTest&& big_int) noexcept;
	BigIntTest& operator=(BigIntTest&& big_int) noexcept;
	~BigIntTest();

	[[nodiscard]] bool positive() const;
	[[nodiscard]] const std::vector<uint64_t>& values() const;

	[[nodiscard]] std::string to_string() const;

	[[nodiscard]] static bool is_special_separator(char value);

	[[nodiscard]] BigIntTest operator+(const BigIntTest& value2) const;

	[[nodiscard]] BigIntTest operator-(const BigIntTest& value2) const;
};

struct BigIntDebug {
	const BigInt& value;

	BigIntDebug(const BigInt& value) : value{ value } {};
};

// helper thought just for the tests
[[nodiscard]] bool operator==(const BigInt& value1, const BigIntTest& value2);

namespace bigint {

[[nodiscard]] bool operator==(const ParseError& error1, const ParseError& error2);

} // namespace bigint
