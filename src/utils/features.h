
#pragma once

#ifndef BIGINT_C_LIB_INTERNAL_USAGE
#error "only internal usage allowed"
#endif

#include "./utils.h"

typedef enum {
	OptimizationLevelNone = 0,
	// 86_64
	OptimizationLevelAMD64SSE2 = 2,
	OptimizationLevelAMD64AVX2 = 3,
	OptimizationLevelAMD64AVX512 = 4,
	// aarch64
	OptimizationLevelARM64NEON = 8,
	OptimizationLevelARM64SVE = 9,
	// riscv64
	OptimizationLevelRISCV64RVV = 12,
	// helper value
	OptimizationLevelUninitialized = 32
} OptimizationLevel;

/**
 * @brief Get the best optimization level for the current platform
 * @note this is NOT CACHED an performs some operations
 *
 * @return OptimizationLevel
 */
NODISCARD BIGINT_C_ONLY_LOCAL OptimizationLevel get_best_optimization_level_raw(void);

/**
 * @brief Get the best optimization level for the current platform
 * @note this is cached and is faster than @link{get_best_optimization_level_raw}
 *
 * @return OptimizationLevel
 */
NODISCARD BIGINT_C_ONLY_LOCAL OptimizationLevel get_best_optimization_level(void);
