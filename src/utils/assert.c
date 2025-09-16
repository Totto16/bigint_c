
#include "./assert.h"

#include <stdio.h>
#include <stdlib.h>

static __attribute__((noreturn)) void platform_panic(const char* file_path, int line,
                                                     const char* message) {
	fprintf(stderr, "%s:%d: ASSERTION FAILED: %s\n", file_path, line, message);
	abort();
}

void custom_assert(const char* file, int line, bool cond, const char* message) {
	if(!cond) {
		platform_panic(file, line, message);
	}
}
