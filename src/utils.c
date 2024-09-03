#if defined (_DEBUG)
#define assert(x) if (!(x)) __debugbreak();
#else
#define assert(x) (x)
#endif
void print(const char* format, ...);

u8 is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

u8 is_decimal_digit(char c) {
    return '0' <= c && c <= '9';
}

u8 is_hexadecimal_digit(char c) {
    return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
}

void u64toa(u64 number, char* str, u32 base) {
    static char codes[16U] = {
        '0',
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
        '9',
        'A',
        'B',
        'C',
        'D',
        'E',
        'F',
    };
    u32 remainder = 0U;
    u32 size = 0U;

    if (number == 0) {
        str[0U] = codes[0U];
        return;
    }

    for(; number != 0U; size++) {
        remainder = number % base;
        number /= base;
        str[size] = codes[remainder];
    }

    for(u32 i = 0; i < size / 2U; i++) {
        char c = str[i];
        str[i] = str[size - i - 1];
        str[size - i - 1] = c;
    }
}

u64 atou64(const char* str, u32 len) {
    u64 number = 0U;
    u32 base = 10U;
    const char* c = str;

    if (str[0] == '0' && str[1] == 'x') {
        base = 16U;
        c += 2U;
    }

    for (; (u32)(c - str) < len; c++) {
        number *= base;
        u32 offset = 0U;
        if ('0' <= *c && *c <= '9') {
            offset = 48U;
        } else if ('a' <= *c && *c <= 'f') {
            offset = 87U;
        } else if ('A' <= *c && *c <= 'F') {
            offset = 55U;
        } else {
            return 0U;
        }

        number += (*c - offset);
    }

    return number;
}

void print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char formatted_str[4096U] = { 0 };
    STRSAFE_LPSTR cur_char = formatted_str;

    const char* cur_fmt = format;
    while(*cur_fmt != '\0') {
        if(*cur_fmt == '%') {
            cur_fmt++;
            if (*cur_fmt == 's') {
                const char* str = va_arg(args, const char*);
                if(str != NULL) {
                    size_t length = 0U;
                    StringCchLengthA(str, sizeof(formatted_str), &length);
                    StringCchCatA(cur_char, length + 1U, str);
                    cur_char += length;
                }
            } else if(*cur_fmt == 'c') {
                char c = va_arg(args, char);
                StringCchCatA(cur_char, 1U, &c);
                cur_char += 1U;
            } else if (*cur_fmt == 'x' && is_alpha(cur_fmt[1])) {
                u32 base = 16U;
                *cur_char = '0';
                cur_char++;
                *cur_char = 'x';
                cur_char++;
                cur_fmt++;

                if(*cur_fmt == 'b') {
                    u8 number = va_arg(args, u8);
                    u64toa((u64)number, cur_char, base);
                    size_t length = 0U;
                    StringCchLengthA(formatted_str, sizeof(formatted_str), &length);
                    cur_char = formatted_str + length;
                } else if(*cur_fmt == 'u') {
                    u64 number = va_arg(args, u64);
                    u64toa(number, cur_char, base);
                    size_t length = 0U;
                    StringCchLengthA(formatted_str, sizeof(formatted_str), &length);
                    cur_char = formatted_str + length;
                }
            } else if(*cur_fmt == 'b') {
                u8 number = va_arg(args, u8);
                u64toa((u64)number, cur_char, 10U);
                size_t length = 0U;
                StringCchLengthA(formatted_str, sizeof(formatted_str), &length);
                cur_char = formatted_str + length;
            } else if(*cur_fmt == 'u') {
                u64 number = va_arg(args, u64);
                u64toa(number, cur_char, 10U);
                size_t length = 0U;
                StringCchLengthA(formatted_str, sizeof(formatted_str), &length);
                cur_char = formatted_str + length;
            }
        } else {
            *cur_char = *cur_fmt;
            cur_char++;
        }
        cur_fmt++;
    }
    va_end(args);

    HANDLE standard_err = GetStdHandle(STD_ERROR_HANDLE);
    size_t length = 0U;
    StringCchLengthA(formatted_str, sizeof(formatted_str), &length);
    SUCCEEDED(WriteFile(standard_err, formatted_str, (DWORD)length, NULL, NULL));
}
