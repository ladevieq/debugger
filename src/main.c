#include <Windows.h>
#include <strsafe.h>
#include <stdint.h>
#include <winbase.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

void print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    const uint32_t max_size = 4096U;
    STRSAFE_LPSTR formatted_str = (char*)VirtualAlloc(NULL, max_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    STRSAFE_LPSTR cur_char = formatted_str;

    const char* cur = format;
    while(*cur != '\0') {
        if(*cur == '%') {
            cur++;
            if (*cur == 's') {
                const char* str = va_arg(args, const char*);
                size_t length;
                StringCchLength(str, max_size, &length);
                StringCchCatA(cur_char, length + 1U, str);
                cur_char += length;
            } else if (*cur == 'c') {
                char c = va_arg(args, char);
                StringCchCatA(cur_char, 1U, &c);
                cur_char += 1U;
            }
        } else {
            *cur_char = *cur;
            cur_char++;
        }
        cur++;
    }

    va_end(args);

    HANDLE standard_err = GetStdHandle(STD_ERROR_HANDLE);
    SUCCEEDED(WriteFile(standard_err, formatted_str, 30, NULL, NULL));

    VirtualFree((void*)formatted_str, 0U, MEM_RELEASE);
}

int mainCRTStartup() {
    u8 is_open = TRUE;
    char input_buffer[16];

    STARTUPINFOW startup_info = {
        .cb = sizeof(startup_info),
    };
    PROCESS_INFORMATION proc_info;

    BOOL ret = CreateProcessW(
        NULL,
        L"./test.exe",
        NULL,
        NULL,
        FALSE,
        DEBUG_ONLY_THIS_PROCESS,
        NULL,
        NULL,
        &startup_info,
        &proc_info
    );

    if (ret == 0) {
        print("%s\n", "Failed to create process");
        return 0;
    }

    HANDLE h_input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD console_mode = ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT;
    SetConsoleMode(h_input, console_mode);

    while(is_open) {
        DWORD input_size = 0U;

        DEBUG_EVENT dbg_event;
        WaitForDebugEventEx(&dbg_event, INFINITE);

        switch (dbg_event.dwDebugEventCode) {
            case CREATE_PROCESS_DEBUG_EVENT: {
                print("%s\n", "CreateProcessDebugEvent");
                break;
            }
            case CREATE_THREAD_DEBUG_EVENT: {
                print("%s\n", "CreateThreadDebugEvent");
                break;
            }
            case EXCEPTION_DEBUG_EVENT: {
                print("%s\n", "ExceptionDebugEvent");
                break;
            }
            case EXIT_PROCESS_DEBUG_EVENT: {
                print("%s\n", "ExitProcessDebugEvent");
                break;
            }
            case EXIT_THREAD_DEBUG_EVENT: {
                print("%s\n", "ExitThreadDebugEvent");
                break;
            }
            case LOAD_DLL_DEBUG_EVENT: {
                print("%s%s\n", "Loading DLL : ", (char*)dbg_event.u.LoadDll.lpImageName);
                break;
            }
            case OUTPUT_DEBUG_STRING_EVENT: {
                print("%s\n", "OutputDebugStringDebugEvent");
                break;
            }
            case RIP_EVENT: {
                print("%s\n", "RIPEvent");
                break;
            }
            case UNLOAD_DLL_DEBUG_EVENT: {
                print("%s\n", "UnloadDLLDebugEvent");
                break;
            }
        }

        ReadConsole(h_input, input_buffer, sizeof(input_buffer), &input_size, NULL);
        // print("%s%s", "ReadConsole :", input_buffer);

        if (input_buffer[0] == 'g') {
            ContinueDebugEvent(
                dbg_event.dwProcessId,
                dbg_event.dwThreadId,
                DBG_EXCEPTION_NOT_HANDLED
            );
        }
    }

    return 0;
}
