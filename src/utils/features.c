
#define BIGINT_C_LIB_INTERNAL_USAGE
#include "./features.h"
#undef BIGINT_C_LIB_INTERNAL_USAGE

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)

#elif defined(__aarch64__)

#if defined(_MSC_VER) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#error "Arm64 is not supported on windows"
#else
#include <asm/hwcap.h>
#include <sys/auxv.h>
#endif
#elif defined(__riscv) && __riscv_xlen == 64

#if defined(_MSC_VER) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#error "RiscV64 is not supported on windows"
#endif

#if __has_include(<sys/hwprobe.h>)
#include <sys/hwprobe.h>
#include <sys/syscall.h>
#include <unistd.h>
#else
#include <asm/hwprobe.h>
#include <sys/syscall.h>
#define _GNU_SOURCE 1
#include <unistd.h>
#undef _GNU_SOURCE
#endif

#endif

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

#if defined(__ARM_FEATURE_SVE) && __ARM_FEATURE_SVE == 1
	    OptimizationLevel_ARM64_SVE
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
	    OptimizationLevel_ARM64_NEON
#else
	    OptimizationLevelNone
#endif

#elif defined(__riscv) && __riscv_xlen == 64

#if defined(__riscv_vector)
	    OptimizationLevel_RISCV64_RVV
#else
	    OptimizationLevelNone
#endif

#else
	    OptimizationLevelNone
#endif

	    ;

	// now detect the runtime best optimization

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)

#if defined(__GNUC__)

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

#elif defined(_MSC_VER) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || \
    defined(__NT__)
	// TODO: support runtime detection
	// do nothing, fall back to compile time detection
#else
#error "only gcc / clang runtime detection for x86_64 supported"
#endif

#elif defined(__aarch64__)

#if defined(__linux__)

	unsigned long hwcap = getauxval(AT_HWCAP);

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
#error "only linux runtime detection for arm64 supported"
#endif

#elif defined(__riscv) && __riscv_xlen == 64

#if defined(__linux__)

	// see: https://docs.kernel.org/arch/riscv/hwprobe.html

#ifdef RISCV_HWPROBE_EXT_ZVE64X
	// if we don't have RISCV_HWPROBE_EXT_ZVE64X, alias the linux kernel or the libc includes are
	// too old, we can#t support the runtime detection!

	struct riscv_hwprobe probe = {
		.key = RISCV_HWPROBE_KEY_IMA_EXT_0, // ISA extensions
	};

#if __has_include(<sys/hwprobe.h>)
	long ret = sys_riscv_hwprobe(&probe, 1, 0, NULL, 0);
#else
	long ret = syscall(__NR_riscv_hwprobe, &probe, 1, 0, NULL, 0);
#endif

	if(ret < 0) {
		// an error occurred, use compile time detected result
		return optimization_level;
	}

	unsigned long exts = probe.value;

#endif

#ifdef RISCV_HWPROBE_EXT_ZVE64X
	if(optimization_level <= OptimizationLevelNone) {

		// need runtime RVV (IMA_V) and E64 (zve64x) support,
		if((exts & RISCV_HWPROBE_IMA_V) != 0 && (exts & RISCV_HWPROBE_EXT_ZVE64X) != 0) {
			return OptimizationLevel_RISCV64_RVV;
		}

	} else {
		// optimization_level >= OptimizationLevel_RISCV64_RVV
		return optimization_level;
	}
#endif

	// optimization_level >= OptimizationLevel_RISCV64_RVV
	return optimization_level;

#else
#error "only linux runtime detection for riscv64 supported"
#endif

#else
// no runtime detection supported, but just using compile time detection
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
