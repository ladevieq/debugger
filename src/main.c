#include <Windows.h>
#include <intrin.h>
#include <strsafe.h>

#include "types.h"

#include "utils.h"
#include "events.h"
#include "parser.h"

u8 is_open = TRUE;
u8 process_commands = TRUE;

HANDLE h_input = NULL;
HANDLE h_output = NULL;
DEBUG_EVENT dbg_event;
PROCESS_INFORMATION proc_info = { 0 };
DWORD continue_status = DBG_EXCEPTION_NOT_HANDLED;

void dbg_loop() {
    while(is_open) {
        WaitForDebugEventEx(&dbg_event, INFINITE);

        continue_status = DBG_EXCEPTION_NOT_HANDLED;

        switch (dbg_event.dwDebugEventCode) {
            case CREATE_PROCESS_DEBUG_EVENT:
                handle_create_process();
                break;
            case CREATE_THREAD_DEBUG_EVENT:
                handle_create_thread();
                break;
            case EXCEPTION_DEBUG_EVENT:
                handle_exception();
                break;
            case EXIT_PROCESS_DEBUG_EVENT:
                handle_exit_process();
                break;
            case EXIT_THREAD_DEBUG_EVENT:
                handle_exit_thread();
                break;
            case LOAD_DLL_DEBUG_EVENT:
                handle_load_dll();
                break;
            case OUTPUT_DEBUG_STRING_EVENT:
                handle_output_debug_string();
                break;
            case RIP_EVENT:
                handle_rip();
                break;
            case UNLOAD_DLL_DEBUG_EVENT:
                handle_unload_dll();
                break;
        }

        process_commands = TRUE;
        while(process_commands) {
            const char write_buffer[] = "> ";
            DWORD c_written = 0U;
            WriteConsole(h_output, write_buffer, sizeof(write_buffer), &c_written, NULL);
            char input_buffer[64] = { 0 };
            DWORD input_size = 0U;
            ReadConsole(h_input, input_buffer, sizeof(input_buffer), &input_size, NULL);

            interpreter(input_buffer, input_size);
        }
    }
}

int mainCRTStartup() {

    STARTUPINFOW startup_info = { 0 };
    startup_info.cb = sizeof(startup_info);

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

    h_output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h_output == NULL) {
        TerminateProcess(proc_info.hProcess, 1U);
        return 1U;
    }

    h_input = GetStdHandle(STD_INPUT_HANDLE);
    if (h_input == NULL) {
        TerminateProcess(proc_info.hProcess, 1U);
        return 1U;
    }
    DWORD console_mode = ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT;
    SetConsoleMode(h_input, console_mode);

    dbg_loop();

    TerminateProcess(proc_info.hProcess, 0U);
    return 0;
}
