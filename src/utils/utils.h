

#pragma once

#if __STDC_VERSION__ >= 202000 || __cplusplus
#define NODISCARD [[nodiscard]]
#else
// see e.g. https://www.gnu.org/software/gnulib/manual/html_node/Attributes.html
#define NODISCARD __attribute__((__warn_unused_result__))
#endif

#define UNUSED(v) ((void)(v))

// cool trick from here:
// https://stackoverflow.com/questions/777261/avoiding-unused-variables-warnings-when-using-assert-in-a-release-build
#ifdef NDEBUG
#define ASSERT(x, msg) /* NOLINT(readability-identifier-naming) */ \
	do { \
		UNUSED((x)); \
		UNUSED((msg)); \
	} while(false)

#else

#include "./assert.h"
#define ASSERT(cond, message) custom_assert(__FILE__, __LINE__, cond, message)

#endif

#ifdef NDEBUG
#define UNREACHABLE() \
	do { \
		fprintf(stderr, "[%s %s:%d]: UNREACHABLE", __func__, __FILE__, __LINE__); \
		exit(EXIT_FAILURE); \
	} while(false)
#else

#define UNREACHABLE() \
	do { \
		ASSERT(false, "UNREACHABLE"); /*NOLINT(cert-dcl03-c,misc-static-assert)*/ \
	} while(false)

#endif
