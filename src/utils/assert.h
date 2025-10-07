#pragma once

#include <stdbool.h>

#include "./utils.h"

#if defined(_MSC_VER)
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN __attribute__((noreturn))
#endif

BIGINT_C_ONLY_LOCAL NO_RETURN void custom_panic(const char* file_path, int line,
                                                const char* message);

BIGINT_C_ONLY_LOCAL void custom_assert(const char* file, int line, bool cond, const char* message);

#define UNREACHABLE() UNREACHABLE_WITH_MSG("")

// cool trick from here:
// https://stackoverflow.com/questions/777261/avoiding-unused-variables-warnings-when-using-assert-in-a-release-build
#ifdef NDEBUG
#define ASSERT(x, msg) /* NOLINT(readability-identifier-naming) */ \
	do {               /*NOLINT(cppcoreguidelines-avoid-do-while)*/ \
		UNUSED((x)); \
		UNUSED((msg)); \
	} while(false)

#define UNREACHABLE_WITH_MSG(msg) \
	do { /*NOLINT(cppcoreguidelines-avoid-do-while)*/ \
		fprintf(stderr, "[%s %s:%d]: UNREACHABLE: %s", __func__, __FILE__, __LINE__, msg); \
		exit(EXIT_FAILURE); \
	} while(false)
#else

#include <assert.h>
#define ASSERT(cond, message) custom_assert(__FILE__, __LINE__, cond, message)

#define UNREACHABLE_WITH_MSG(msg) \
	do { /*NOLINT(cppcoreguidelines-avoid-do-while)*/ \
		custom_panic(__FILE__, __LINE__, \
		             "UNREACHABLE: " msg); /*NOLINT(cert-dcl03-c,misc-static-assert)*/ \
	} while(false)

#endif
