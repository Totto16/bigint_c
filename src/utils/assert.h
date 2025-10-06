#pragma once

#if defined(_MSC_VER)
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN __attribute__((noreturn))
#endif

NO_RETURN void custom_panic(const char* file_path, int line, const char* message);

void custom_assert(const char* file, int line, bool cond, const char* message);
