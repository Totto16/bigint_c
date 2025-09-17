

#pragma once

#if defined(__cplusplus) && !defined(BIGINT_C_NO_CPP)
#define BIGINT_USE_CPP
#endif

#ifdef BIGINT_USE_CPP
extern "C" {
#endif

#include "../lib/lib.h"

#ifdef BIGINT_USE_CPP
}

#include "../lib/lib_cpp.hpp"

#endif
