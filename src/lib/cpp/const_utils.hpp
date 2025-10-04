#pragma once

// TODO
// #include "./utils.hpp"

#include <algorithm>
#include <cassert>
#include <type_traits>

namespace const_utils {

[[nodiscard]] constexpr bool is_constant_evaluated() noexcept {

#if defined(__cpp_if_consteval) && __cpp_if_consteval >= 201811L

	if consteval {
		return true;
	}

	return false;
#else
	return std::is_constant_evaluated();
#endif
}

} // namespace const_utils

// use C++26 feature, if available:
#if __cpp_static_assert >= 202306L
#define CONSTEVAL_ONLY_STATIC_ASSERT(CHECK, MSG) /*NOLINT(cppcoreguidelines-macro-usage)*/ \
	((CHECK) ? void(0) : [] { \
		/* If you see this really bad c++ error message, follow the origin of MSG, to see the real \
		 * error message, c++ error messages suck xD */ \
		throw(MSG); \
	}())

// This doesn't work, since CHECK is in most cases not a constant expression so not constant
// evaluatable inside the if constexpr, and therefore static_assert(false,..) would trigger always
/*
#define CONSTEVAL_ONLY_STATIC_ASSERT(CHECK, MSG)
(if constexpr (!(CHECK)) { static_assert(false, MSG); }())
s*/

#else
#define CONSTEVAL_ONLY_STATIC_ASSERT(CHECK, MSG) /*NOLINT(cppcoreguidelines-macro-usage)*/ \
	((CHECK) ? void(0) : [] { \
		/* If you see this really bad c++ error message, follow the origin of MSG, to see the real \
		 * error message, c++ error messages suck xD */ \
		throw(MSG); \
	}())
#endif

#define CONSTEVAL_STATIC_ASSERT(CHECK, MSG) /*NOLINT(cppcoreguidelines-macro-usage)*/ \
	do {                                    /*NOLINT(cppcoreguidelines-avoid-do-while)*/ \
		if(const_utils::is_constant_evaluated()) { \
			CONSTEVAL_ONLY_STATIC_ASSERT(CHECK, MSG); \
		} else { \
			assert((CHECK) && (MSG)); \
		} \
	} while(false)

namespace const_utils {

#define PROPAGATE(val, V, E) /*NOLINT(cppcoreguidelines-macro-usage)*/ \
	do {                     /*NOLINT(cppcoreguidelines-avoid-do-while)*/ \
		if(not((val).has_value())) { \
			return const_utils::Expected<V, E>::error_result((val).error()); \
		} \
	} while(false)

// represents a sort of constexpr std::Expected
template <typename V, typename E>
    requires std::is_default_constructible_v<V> && std::is_default_constructible_v<E>
struct Expected {
  private:
	bool m_has_value;
	V m_value;
	E m_error;

	constexpr Expected(bool has_value, const V& value,
	                   const E& error) // NOLINT(modernize-pass-by-value)
	    : m_has_value{ has_value }, m_value{ value }, m_error{ error } {}

  public:
	[[nodiscard]] constexpr static Expected<V, E> good_result(const V& type) {
		return { true, type, E{} };
	}

	[[nodiscard]] constexpr static Expected<V, E> error_result(const E& error) {
		return { false, V{}, error };
	}

	[[nodiscard]] constexpr bool has_value() const { return m_has_value; }

	[[nodiscard]] constexpr V value() const {
		CONSTEVAL_STATIC_ASSERT((has_value()), "value() call on Expected without value");

		return m_value;
	}

	[[nodiscard]] constexpr E error() const {
		CONSTEVAL_STATIC_ASSERT((not has_value()), "error() call on Expected without error");

		return m_error;
	}
};

// A string that can be used as a template parameter.
template <std::size_t N> struct ConstString {
	char str[N]{};

	static constexpr std::size_t size = N - 1;

	consteval ConstString() {}
	consteval ConstString(const char (&new_str)[N]) {
		if(new_str[N - 1] != '\0') {
			CONSTEVAL_STATIC_ASSERT(false, "Expected null terminated array (CString)");
		};
		std::copy_n(new_str, size, str);
	}
};

template <typename T, size_t N> struct VectorLike {
  private:
	std::array<T, N> values;
	size_t m_size;

	// capacity is N

  public:
	using value_type = T;

	[[nodiscard]] constexpr size_t size() const { return this->m_size; }

	[[nodiscard]] constexpr size_t capacity() const { return N; }

	constexpr void push_back(T value) {
		if(m_size >= N) {
			CONSTEVAL_STATIC_ASSERT(false, "Have no more capacity in const 'vector'");
		}

		values.at(m_size) = value;
		m_size++;
	}

	constexpr void emplace_back(T&& value) {
		if(m_size >= N) {
			CONSTEVAL_STATIC_ASSERT(false, "Have no more capacity in const 'vector'");
		}

		values.at(m_size) = std::move(value);
		m_size++;
	}

	constexpr const T& operator[](size_t index) const {
		if(index >= m_size) {
			CONSTEVAL_STATIC_ASSERT(false, "index out of bound in const 'vector'");
		}

		return values.at(index);
	}

	constexpr T& operator[](size_t index) {
		if(index >= m_size) {
			CONSTEVAL_STATIC_ASSERT(false, "index out of bound in const 'vector'");
		}

		return values.at(index);
	}

	constexpr void resize(size_t new_size) {
		if(new_size > N) {
			CONSTEVAL_STATIC_ASSERT(false, "Can't allocate that much in const 'vector'");
		}

		m_size = new_size;
	}

	constexpr void pop_back() {
		if(m_size == 0) {
			CONSTEVAL_STATIC_ASSERT(false, "Can't pop an empty const 'vector'");
		}

		m_size--;
	}
};

} // namespace const_utils
