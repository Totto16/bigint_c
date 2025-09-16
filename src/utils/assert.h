#pragma once

__attribute__((noreturn)) void custom_panic(const char* file_path, int line, const char* message);

void custom_assert(const char* file, int line, bool cond, const char* message);
