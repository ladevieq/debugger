#pragma once

#include "types.h"

#if defined (_DEBUG)
#define assert(x) if (!(x)) __debugbreak();
#else
#define assert(x) (x)
#endif

void print(const char* format, ...);

u8 is_alpha(char c);

u8 is_decimal_digit(char c);

u8 is_hexadecimal_digit(char c);

void u64toa(u64 number, char* str, u32 base);

u64 atou64(const char* str, u32 len);

u8 read_memory(const void* addr, void* buffer, size_t size);
