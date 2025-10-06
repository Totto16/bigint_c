#pragma once

#if __STDC_VERSION__ >= 202000 || __cplusplus
#define NODISCARD [[nodiscard]]
#else
// see e.g. https://www.gnu.org/software/gnulib/manual/html_node/Attributes.html
#if defined(_MSC_VER)
#if _MSC_VER >= 1700
#define NODISCARD _Check_return_
#else
// empty, as not supported
#define NODISCARD
#endif
#else
#define NODISCARD __attribute__((__warn_unused_result__))
#endif
#endif

#define UNUSED(v) ((void)(v))

#define UNREACHABLE() UNREACHABLE_WITH_MSG("")

// cool trick from here:
// https://stackoverflow.com/questions/777261/avoiding-unused-variables-warnings-when-using-assert-in-a-release-build
#ifdef NDEBUG
#define ASSERT(x, msg) /* NOLINT(readability-identifier-naming) */ \
	do { \
		UNUSED((x)); \
		UNUSED((msg)); \
	} while(false)

#define UNREACHABLE_WITH_MSG(msg) \
	do { \
		fprintf(stderr, "[%s %s:%d]: UNREACHABLE: %s", __func__, __FILE__, __LINE__, msg); \
		exit(EXIT_FAILURE); \
	} while(false)
#else

#include "./assert.h"
#define ASSERT(cond, message) custom_assert(__FILE__, __LINE__, cond, message)

#define UNREACHABLE_WITH_MSG(msg) \
	do { \
		custom_panic(__FILE__, __LINE__, \
		             "UNREACHABLE: " msg); /*NOLINT(cert-dcl03-c,misc-static-assert)*/ \
	} while(false)

#endif

// see: https://gcc.gnu.org/wiki/Visibility

// clang-format off
#if defined(_MSC_VER) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #define BIGINT_C_ONLY_LOCAL

	// mingw vs msvc
    #if defined(__GNUC__)
        #define BIGINT_C_DLL_EXPORT __attribute__((dllexport))
        #define BIGINT_C_DLL_IMPORT __attribute__((dllimport))
    #else
        #define BIGINT_C_DLL_EXPORT __declspec(dllexport) 
        #define BIGINT_C_DLL_IMPORT __declspec(dllimport) 
    #endif
#else
    #define BIGINT_C_ONLY_LOCAL __attribute__((visibility("hidden")))

    #define BIGINT_C_DLL_EXPORT __attribute__((visibility("default")))
    #define BIGINT_C_DLL_IMPORT __attribute__((visibility("default")))
#endif

#if defined(BIGINT_C_LIB_TYPE) && BIGINT_C_LIB_TYPE == 0
	#if defined(BIGINT_C_LIB_EXPORT)
		#define BIGINT_C_LIB_EXPORTED BIGINT_C_DLL_EXPORT
	#else
		#define BIGINT_C_LIB_EXPORTED BIGINT_C_DLL_IMPORT
	#endif
#else
	#define BIGINT_C_LIB_EXPORTED BIGINT_C_ONLY_LOCAL
#endif

// clang-format on
