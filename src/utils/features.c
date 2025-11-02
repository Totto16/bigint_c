
#define BIGINT_C_LIB_INTERNAL_USAGE
#include "./features.h"
#undef BIGINT_C_LIB_INTERNAL_USAGE

NODISCARD BIGINT_C_ONLY_LOCAL OptimizationLevel get_best_optimization_level_raw(void) {

	// first get the compile time availability, this is the least optimization the processor
	// supports, if we compile with -march=native even the best!
	const OptimizationLevel optimization_level =

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)

	// check from best to least best
#if defined(__AVX512F__)
	    OptimizationLevel_AMD64_AVX512
#elif defined(__AVX2__)
	    OptimizationLevel_AMD64_AVX2
#elif defined(__SSE2__)
	    OptimizationLevel_AMD64_SSE2
#else
	    OptimizationLevelNone
#endif

#elif defined(__aarch64__)
#if defined(__ARM_FEATURE_SVE2) && __ARM_FEATURE_SVE2 == 1
	    OptimizationLevel_ARM64_SVE2
#elif defined(__ARM_FEATURE_SVE) && __ARM_FEATURE_SVE == 1
	    OptimizationLevel_ARM64_SVE
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
	    OptimizationLevel_ARM64_NEON
#else
	    OptimizationLevelNone
#endif
#else
	    OptimizationLevelNone
#endif

	    ;

	// now detect the runtime best optimization

#if defined(__GNUC__)

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)

	__builtin_cpu_init();

	if(optimization_level <= OptimizationLevel_AMD64_AVX2) {
		if(__builtin_cpu_supports("avx512f")) {
			return OptimizationLevel_AMD64_AVX512;
		}
	} else {
		// optimization_level >= OptimizationLevel_AMD64_AVX512
		return optimization_level;
	}

	if(optimization_level <= OptimizationLevel_AMD64_SSE2) {
		if(__builtin_cpu_supports("avx2")) {
			return OptimizationLevel_AMD64_AVX2;
		}
	} else {
		// optimization_level >= OptimizationLevel_AMD64_AVX2
		return optimization_level;
	}

	if(optimization_level <= OptimizationLevelNone) {
		if(__builtin_cpu_supports("sse2")) {
			return OptimizationLevel_AMD64_SSE2;
		}
	}

	// optimization_level >= OptimizationLevel_AMD64_SSE2
	return optimization_level;

#elif defined(__aarch64__)

	unsigned long hwcap = getauxval(AT_HWCAP);
	unsigned long hwcap2 = getauxval(AT_HWCAP2);

	if(optimization_level <= OptimizationLevel_ARM64_SVE) {
		if((hwcap2 & HWCAP2_SVE2) != 0) {
			return OptimizationLevel_ARM64_SVE2;
		}
	} else {
		// optimization_level >= OptimizationLevel_ARM64_SVE2
		return optimization_level;
	}

	if(optimization_level <= OptimizationLevel_ARM64_NEON) {
		if((hwcap & HWCAP_SVE) != 0) {
			return OptimizationLevel_ARM64_SVE;
		}
	} else {
		// optimization_level >= OptimizationLevel_ARM64_SVE
		return optimization_level;
	}

	if(optimization_level <= OptimizationLevelNone) {
		// HWCAP_ASIMD => NEON
		if((hwcap & HWCAP_ASIMD) != 0) {
			return OptimizationLevel_ARM64_NEON;
		}
	}

	// optimization_level >= OptimizationLevel_ARM64_NEON
	return optimization_level;

#else
// TODO: only aarch64 depends on linux, the rest can be used under mingw too!
#error "NOT supported on linux atm"
#endif

#else
#error "NOT yet IMPLEMENTED"
#endif

	return optimization_level;
}

static OptimizationLevel g_global_detected_optimization_level = OptimizationLevelUninitialized;

NODISCARD BIGINT_C_ONLY_LOCAL OptimizationLevel get_best_optimization_level(void) {

	if(g_global_detected_optimization_level != OptimizationLevelUninitialized) {
		return g_global_detected_optimization_level;
	}

	g_global_detected_optimization_level = get_best_optimization_level_raw();

	return g_global_detected_optimization_level;
}
