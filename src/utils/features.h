
#pragma once

#ifndef BIGINT_C_LIB_INTERNAL_USAGE
#error "only internal usage allowed"
#endif

#include "./utils.h"

typedef enum {
	OptimizationLevelNone = 0,
	// 86_64
	OptimizationLevel_AMD64_SSE2 = 2,
	OptimizationLevel_AMD64_AVX2,
	OptimizationLevel_AMD64_AVX512,
	// aarch64
	OptimizationLevel_ARM64_NEON = 8,
	OptimizationLevel_ARM64_SVE,
	// riscv64
	OptimizationLevel_RISCV64_RVV = 12,
	// helper value
	OptimizationLevelUninitialized = 32
} OptimizationLevel;

/**
 * @brief Get the best optimization level for the current platform
 * @note this is NOT CACHED an performs some operations
 *
 * @return OptimizationLevel
 */
NODISCARD BIGINT_C_ONLY_LOCAL OptimizationLevel get_best_optimization_level_raw();

/**
 * @brief Get the best optimization level for the current platform
 * @note this is cached and is faster than @link{get_best_optimization_level_raw}
 *
 * @return OptimizationLevel
 */
NODISCARD BIGINT_C_ONLY_LOCAL OptimizationLevel get_best_optimization_level(void);
