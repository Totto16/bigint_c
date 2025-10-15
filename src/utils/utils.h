#pragma once

#if __STDC_VERSION__ >= 202311L || defined(__cplusplus)
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

#if __STDC_VERSION__ >= 202311L || defined(__cplusplus)
#define STATIC_ASSERT(check, message) static_assert(check, message)
#elif __STDC_VERSION__ < 201112L
// empty, as not supported
#define STATIC_ASSERT(check, message)
#else
#define STATIC_ASSERT(check, message) _Static_assert(check, message)
#endif

#define UNUSED(v) ((void)(v))

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
