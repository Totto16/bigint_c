

#define BIGINT_C_CPP_ACCESS_TO_UNDERLYING_C_DATA
#include "./helper.hpp"

#if defined(_MSC_VER)
#define USE_IMPLEMENTATION 1
#else
#define USE_IMPLEMENTATION 0
#endif

#if !defined(USE_IMPLEMENTATION)
#error "DEFINE USE_IMPLEMENTATION"
#elif USE_IMPLEMENTATION == 0
#include <gmp.h>
#elif USE_IMPLEMENTATION == 1
#include <tommath.h>
#else
#error "unknown USE_IMPLEMENTATION"
#endif

#include <stdexcept>

BigIntTest::BigIntTest(bool positive, std::vector<uint64_t> values) noexcept
    : m_positive{ positive }, m_values{ std::move(values) } {}

BigIntTest::BigIntTest(const BigInt& big_int_c) noexcept
    : m_positive{ big_int_c.underlying().positive }, m_values{} {

	for(size_t i = 0; i < big_int_c.underlying().number_count; ++i) {
		m_values.push_back(big_int_c.underlying().numbers[i]);
	}
}

BigIntTest::~BigIntTest() = default;

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

// helper thought just for the tests
[[nodiscard]] bool operator==(const BigInt& value1, const BigIntTest& value2) {

	if(value1.underlying().positive != value2.positive()) {
		return false;
	}

	if(value1.underlying().number_count != value2.values().size()) {
		return false;
	}

	for(size_t i = 0; i < value1.underlying().number_count; ++i) {
		if(value1.underlying().numbers[i] != value2.values().at(i)) {
			return false;
		}
	}

	return true;
}

[[nodiscard]] bool bigint::operator==(const bigint::ParseError& error1,
                                      const bigint::ParseError& error2) {
	if(error1.index() != error2.index()) {
		return false;
	}

	if(error1.symbol() != error2.symbol()) {
		return false;
	}

	return (std::string{ error1.message() } == std::string{ error2.message() });
}

[[nodiscard]] bool BigIntTest::is_special_separator(char value) {
	return value == '_' || value == '\'' || value == ',' || value == '.';
}

#if USE_IMPLEMENTATION == 0

static constexpr uint8_t CHUNK_BITS = 64;

static void initialize_bigint_from_gmp(BigIntTest& test, mpz_t&& number) {
	size_t num_chunks = (mpz_sizeinbase(number, 2) + CHUNK_BITS - 1) / CHUNK_BITS;
	std::vector<uint64_t> values{};
	values.resize(num_chunks);

	mpz_t temp;
	mpz_init_set(temp, number);

	// TODO: maybe use mpz_export

	for(size_t i = 0; i < num_chunks; ++i) {
		values.at(i) = mpz_get_ui(temp);

		// Shift right by 64 bits, don't perform arithmetic shifts (affects negative values), see
		// https://gmplib.org/manual/Integer-Division
		mpz_tdiv_q_2exp(temp, temp, CHUNK_BITS);
	}

	// note: 0 is always positive here
	bool positive = mpz_sgn(number) >= 0;

	mpz_clear(number);
	mpz_clear(temp);

	test = BigIntTest(positive, std::move(values));
}

namespace {

class MPZWrapper {
  private:
	mpz_t* m_value;

  public:
	MPZWrapper() {
		m_value = (mpz_t*)malloc(sizeof(mpz_t));
		mpz_init(*m_value);
	}

	MPZWrapper(const MPZWrapper&) = delete;
	MPZWrapper& operator=(const MPZWrapper&) = delete;

	[[nodiscard]] const mpz_t& get() const { return *m_value; }
	[[nodiscard]] const mpz_t& operator*() const { return *m_value; }

	[[nodiscard]] mpz_t& get() { return *m_value; }
	[[nodiscard]] mpz_t& operator*() { return *m_value; }

	~MPZWrapper() {
		if(m_value != nullptr) {
			mpz_clear(*m_value);
			free(m_value);
		}
	}

	MPZWrapper(MPZWrapper&& number) noexcept : m_value{ number.m_value } {
		number.m_value = nullptr;
	}

	MPZWrapper& operator=(MPZWrapper&& number) noexcept {

		if(this != &number) {
			this->m_value = number.m_value;
			number.m_value = nullptr;
		}
		return *this;
	}
};

} // namespace

static MPZWrapper get_gmp_value_from_bigint(const BigIntTest& test) {

	MPZWrapper bigint{};

	int order = -1; // -1 means the least significant uint64_t comes first, as we store it.
	int endian = 0; // host endian
	int nails = 0;  // use all 64 bits of the number

	// see: https://gmplib.org/manual/Integer-Import-and-Export
	mpz_import(*bigint, test.values().size(), order, sizeof(uint64_t), endian, nails,
	           (void*)(test.values().data()));

	// reverse signedness, if necessary
	if(!test.positive() && mpz_sgn(*bigint) > 0) {
		mpz_neg(*bigint, *bigint);
	} else if(test.positive() && mpz_sgn(*bigint) < 0) {
		mpz_neg(*bigint, *bigint);
	}

	return bigint;
}

// the same algorithm as in c, but using a external library, to verify the test result
// see https://gmplib.org/manual/Concept-Index for gmp docs
BigIntTest::BigIntTest(const std::string& str) : m_values{} {

	// preprocess the string, to allow the same syntax as in the c library
	std::string cleaned_str = str;
	std::erase_if(cleaned_str, [](char digit) -> bool {
		// Remove special characters like separators and also the "+" sign, as gmp doesn't allow
		// that
		return BigIntTest::is_special_separator(digit) || digit == '+';
	});

	mpz_t bigint;
	mpz_init(bigint);

	// Parse the decimal string into the big integer
	if(mpz_set_str(bigint, cleaned_str.c_str(), 10) != 0) {
		mpz_clear(bigint);
		throw std::runtime_error("Failed to parse bigint string: " + str +
		                         " (cleaned: " + cleaned_str + ")");
	}

	initialize_bigint_from_gmp(*this, std::move(bigint));
}

BigIntTest::BigIntTest(const uint64_t& number) : m_values{} {
	mpz_t bigint;
	mpz_init(bigint);

	mpz_set_ui(bigint, number);

	initialize_bigint_from_gmp(*this, std::move(bigint));
}
BigIntTest::BigIntTest(const int64_t& number) : m_values{} {
	mpz_t bigint;
	mpz_init(bigint);

	mpz_set_si(bigint, number);

	initialize_bigint_from_gmp(*this, std::move(bigint));
}

[[nodiscard]] std::string BigIntTest::to_string() const {
	MPZWrapper number = get_gmp_value_from_bigint(*this);

	size_t needed_size =
	    mpz_sizeinbase(*number, 10) + 2; // one for the eventual "-" and one for the 0 terminator

	char* buffer = (char*)malloc(needed_size * sizeof(char));

	if(buffer == nullptr) {
		throw std::runtime_error("string conversion error");
	}

	char* result = mpz_get_str(buffer, 10, *number);

	if(result == nullptr) {

		throw std::runtime_error("string conversion error");
	}

	std::string str{ result };

	free(result);

	return str;
}

[[nodiscard]] BigIntTest BigIntTest::operator+(const BigIntTest& value2) const {

	const MPZWrapper number1 = get_gmp_value_from_bigint(*this);

	const MPZWrapper number2 = get_gmp_value_from_bigint(value2);

	// see: https://gmplib.org/manual/Integer-Arithmetic
	mpz_t result_number;
	mpz_init(result_number);

	mpz_add(result_number, *number1, *number2);

	BigIntTest result{ false, {} };
	initialize_bigint_from_gmp(result, std::move(result_number));

	return result;
}

[[nodiscard]] BigIntTest BigIntTest::operator-(const BigIntTest& value2) const {

	const MPZWrapper number1 = get_gmp_value_from_bigint(*this);

	const MPZWrapper number2 = get_gmp_value_from_bigint(value2);

	// see: https://gmplib.org/manual/Integer-Arithmetic
	mpz_t result_number;
	mpz_init(result_number);

	mpz_sub(result_number, *number1, *number2);

	BigIntTest result{ false, {} };
	initialize_bigint_from_gmp(result, std::move(result_number));

	return result;
}

#elif USE_IMPLEMENTATION == 1
static void initialize_bigint_from_tommath(BigIntTest& test, mp_int&& number) {

	uint8_t digit_size = sizeof(mp_digit);

	if(!(digit_size == 4 || digit_size == 8)) {
		throw new std::runtime_error("expected 32 or 64 bits per value!");
	}

	size_t mp_chuncks = number.used;

	size_t num_chunks = digit_size == 4 ? ((mp_chuncks + 1) / 2) : mp_chuncks;
	std::vector<uint64_t> values{};
	values.resize(num_chunks);

	size_t mp_index = 0;
	for(size_t i = num_chunks; i != 0; --i, ++mp_index) {

		uint64_t word = 0;
		if(digit_size == 4) {
			word = number.dp[mp_index];
			if(mp_index + 1 < mp_chuncks) {
				word |= static_cast<uint64_t>(number.dp[mp_index + 1]) << 32;
			}
		} else {
			word = number.dp[mp_index];
		}

		values.at(i - 1) = word;
	}

	bool positive = !mp_isneg((&number));

	mp_clear(&number);

	test = BigIntTest(positive, std::move(values));
}

#define CHECK_MP_ERROR(err) \
	do { \
		if(err != MP_OKAY) { \
			throw new std::runtime_error{ mp_error_to_string(err) }; \
		} \
	} while(false)

namespace {

class MPWrapper {
  private:
	std::shared_ptr<mp_int> m_value;

  public:
	MPWrapper() : m_value{ nullptr } {
		mp_int* value = new mp_int{};

		mp_err error = mp_init(value);
		if(error != MP_OKAY) {
			delete value;
			throw new std::runtime_error{ mp_error_to_string(error) };
		}

		m_value = std::shared_ptr<mp_int>(value, [](mp_int* p) {
			mp_clear(p);
			delete p;
		});
	}

	MPWrapper(const MPWrapper&) = default;
	MPWrapper& operator=(const MPWrapper&) = default;

	[[nodiscard]] const mp_int* get() const { return m_value.get(); }
	[[nodiscard]] const mp_int* operator*() const { return m_value.get(); }

	[[nodiscard]] mp_int* get() { return m_value.get(); }
	[[nodiscard]] mp_int* operator*() { return m_value.get(); }

	~MPWrapper() = default;

	MPWrapper(MPWrapper&& number) noexcept = default;

	MPWrapper& operator=(MPWrapper&& number) = default;
};

} // namespace

static MPWrapper get_tommath_value_from_bigint(const BigIntTest& test) {

	uint8_t digit_size = sizeof(mp_digit);

	if(!(digit_size == 4 || digit_size == 8)) {
		throw new std::runtime_error("expected 32 or 64 bits per value!");
	}

	int shift_size = digit_size == 4 ? 2 : 1;

	MPWrapper bigint{};

	mp_zero(*bigint);

	for(size_t i = 0; i < test.values().size(); ++i) {
		mp_int temp, shift;
		mp_err error = mp_init(&temp);
		CHECK_MP_ERROR(error);

		error = mp_init(&shift);
		CHECK_MP_ERROR(error);

		mp_set_u64(&temp, test.values()[i]);

		error = mp_lshd(&temp, shift_size);
		CHECK_MP_ERROR(error);

		if(i > 0) {
			mp_lshd(&temp, shift_size);
		}

		// result = result + shift
		error = mp_add(*bigint, &temp, *bigint);
		CHECK_MP_ERROR(error);

		mp_clear(&temp);
		mp_clear(&shift);
	}

	(*bigint)->sign = test.positive() ? MP_ZPOS : MP_NEG;

	return bigint;
}

// the same algorithm as in c, but using a external library, to verify the test result
// see https://gmplib.org/manual/Concept-Index for gmp docs
BigIntTest::BigIntTest(const std::string& str) : m_values{} {

	// preprocess the string, to allow the same syntax as in the c library
	std::string cleaned_str = str;
	std::erase_if(cleaned_str, [](char digit) -> bool {
		// Remove special characters like separators and also the "+" sign, as gmp doesn't allow
		// that
		return BigIntTest::is_special_separator(digit) || digit == '+';
	});

	mp_int bigint;
	mp_err error = mp_init(&bigint);
	CHECK_MP_ERROR(error);

	// Parse the decimal string into the big integer
	error = mp_read_radix(&bigint, cleaned_str.c_str(), 10);
	if(error != MP_OKAY) {
		mp_clear(&bigint);
		throw std::runtime_error("Failed to parse bigint string: " + str +
		                         " (cleaned: " + cleaned_str + ")" + mp_error_to_string(error));
	}

	initialize_bigint_from_tommath(*this, std::move(bigint));
}

BigIntTest::BigIntTest(const uint64_t& number) : m_values{} {
	mp_int bigint;
	mp_err error = mp_init(&bigint);
	CHECK_MP_ERROR(error);

	mp_set_u64(&bigint, number);

	initialize_bigint_from_tommath(*this, std::move(bigint));
}
BigIntTest::BigIntTest(const int64_t& number) : m_values{} {
	mp_int bigint;
	mp_err error = mp_init(&bigint);
	CHECK_MP_ERROR(error);

	mp_set_i64(&bigint, number);

	initialize_bigint_from_tommath(*this, std::move(bigint));
}

[[nodiscard]] std::string BigIntTest::to_string() const {
	MPWrapper number = get_tommath_value_from_bigint(*this);

	size_t needed_size = 0;
	mp_err error = mp_radix_size(*number, 10, &needed_size);
	CHECK_MP_ERROR(error);

	needed_size += 1; // for the \0 terminator

	char* buffer = (char*)malloc(needed_size * sizeof(char));

	if(buffer == nullptr) {
		throw std::runtime_error("string conversion error");
	}

	size_t written = 0;

	error = mp_to_radix(*number, buffer, needed_size, &written, 10);
	CHECK_MP_ERROR(error);

	std::string str{ buffer };

	free(buffer);

	return str;
}

[[nodiscard]] BigIntTest BigIntTest::operator+(const BigIntTest& value2) const {

	const MPWrapper number1 = get_tommath_value_from_bigint(*this);

	const MPWrapper number2 = get_tommath_value_from_bigint(value2);

	mp_int result_number;
	mp_err error = mp_init(&result_number);
	if(error != MP_OKAY) {
		throw new std::runtime_error{ mp_error_to_string(error) };
	}

	error = mp_add(*number1, *number2, &result_number);
	if(error != MP_OKAY) {
		mp_clear(&result_number);
		throw new std::runtime_error{ mp_error_to_string(error) };
	}

	BigIntTest result{ false, {} };
	initialize_bigint_from_tommath(result, std::move(result_number));

	return result;
}

[[nodiscard]] BigIntTest BigIntTest::operator-(const BigIntTest& value2) const {

	const MPWrapper number1 = get_tommath_value_from_bigint(*this);

	const MPWrapper number2 = get_tommath_value_from_bigint(value2);

	mp_int result_number;
	mp_err error = mp_init(&result_number);
	if(error != MP_OKAY) {
		throw new std::runtime_error{ mp_error_to_string(error) };
	}

	error = mp_sub(*number1, *number2, &result_number);
	if(error != MP_OKAY) {
		mp_clear(&result_number);
		throw new std::runtime_error{ mp_error_to_string(error) };
	}

	BigIntTest result{ false, {} };
	initialize_bigint_from_tommath(result, std::move(result_number));

	return result;
}
#endif
