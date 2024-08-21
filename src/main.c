#include <Windows.h>
#include <winbase.h>
#include <strsafe.h>
#include <winuser.h>

#include "types.h"
#include "utils.c"

static u8 is_open = TRUE;
static u8 process_commands = TRUE;
static u8 expect_single_step = FALSE;
static DWORD continue_status = DBG_EXCEPTION_NOT_HANDLED;
static HANDLE h_input = NULL;

enum CMD_TYPE {
    CMD_CONTINUE,
    CMD_PRINT_REG,
    CMD_STEP_INTO,
    CMD_QUIT,
    CMD_DUMP_MEMORY,
    CMD_UNKNOWN,
};

enum TOKEN_TYPE {
    TOKEN_CONTINUE,
    TOKEN_PRINT_REG,
    TOKEN_STEP_INTO,
    TOKEN_DUMP_MEMORY,
    TOKEN_QUIT,

    TOKEN_DECIMAL_NUMBER,
    TOKEN_HEXADECIMAL_NUMBER,

    TOKEN_UNKNOWN,
};

struct token {
    enum TOKEN_TYPE type;
    const char* lexeme;
    u8 size;
};

struct command {
    enum CMD_TYPE type;
    struct token name;
    struct token args[2];
};

static char* commands[] = {
    "g",
    "r",
    "t",
    "db",
    "q",
};

struct cmd {
    const char* start;
    u8 size;
};

static struct cmd cmds[] = {
    { .start = "g", .size = 1 },
    { .start = "r", .size = 1 },
    { .start = "t", .size = 1 },
    { .start = "q", .size = 1 },
    { .start = "db", .size = 2 },
};

struct ast_node {
    struct token token;
    u8 left;
};

_Static_assert((sizeof(commands) / sizeof(commands[0])) == CMD_UNKNOWN, "Missing commands");

u8 is_end(char c) {
    return c == '\0';
}

u8 is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

u8 is_decimal_digit(char c) {
    return '0' <= c && c <= '9';
}

u8 is_hexadecimal_digit(char c) {
    return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
}

void lexer(const char* str_cmd, size_t str_cmd_len, struct token* tokens) {
    const char* cur = str_cmd;
    struct token* cur_token = tokens;
    str_cmd_len = 0;

    while(*cur != '\0') {
        cur_token->type = TOKEN_UNKNOWN;
        if(*cur == '0' && cur[1] == 'x') {
            const char* start = cur;
            cur += 2;

            while(is_hexadecimal_digit(*cur) && !is_end(*cur)) {
                cur++;
            }

            cur_token->lexeme = start;
            cur_token->type = TOKEN_HEXADECIMAL_NUMBER;
            cur_token->size = (u8)(cur - start);
            cur_token++;
        } else if(is_decimal_digit(*cur)) {
            const char* start = cur;
            cur++;

            while(is_decimal_digit(*cur) && !is_end(*cur)) {
                cur++;
            }

            cur_token->lexeme = start;
            cur_token->type = TOKEN_DECIMAL_NUMBER;
            cur_token->size = (u8)(cur - start);
            cur_token++;
        } else if(is_alpha(*cur)) {
            const char* start = cur;
            cur++;
            while(is_alpha(*cur) && !is_end(*cur)) {
                cur++;
            }

            cur_token->lexeme = start;
            cur_token->size = (u8)(cur - start);

            for(int i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
                const char* cur_cmd = cmds[i].start;
                if (CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, cur_cmd, cmds[i].size, cur_token->lexeme, cur_token->size) == CSTR_EQUAL) {
                    cur_token->type = i;
                    cur_token++;
                    break;
                }
            }
        } else {
            cur++;
        }
    }
}

void parse_command(struct token* tokens, struct ast_node* nodes)
{
    if (tokens[0].type <= TOKEN_DECIMAL_NUMBER) {
        nodes[0].token = tokens[0];
    } else {
        print("%s must be a command\n", tokens[0].lexeme);
    }
}

void parser(struct token* tokens, struct ast_node* nodes) {
    if (tokens[0].type <= TOKEN_DECIMAL_NUMBER) {
        nodes[0].token = tokens[0];
    }
}

void tokenize(const char* str_cmd, size_t str_cmd_len, struct command* cmd) {
    const char* cur = str_cmd;
    str_cmd_len = 0;

    cmd->type = CMD_UNKNOWN;
    cmd->name.lexeme = NULL;
    cmd->name.size = 0;

    for(int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        char* cur_cmd = commands[i];
        size_t cur_cmd_len = 0U;
        StringCchLengthA(cur_cmd, 16U, &cur_cmd_len);
        if (CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, cur_cmd,(int)cur_cmd_len, str_cmd, (int)cur_cmd_len) == CSTR_EQUAL) {
            cmd->name.lexeme = cur;
            cmd->name.size = (u8)cur_cmd_len;
            cmd->type = i;
            cur += cur_cmd_len;
            break;
        }
    }

    if (cmd->type < CMD_DUMP_MEMORY) {
        return;
    }

    struct token* cur_args = cmd->args;
    while(*cur != '\0' && *cur != '\r' && *cur != '\n'
        || cur_args - cmd->args >= sizeof(cmd->args) / sizeof(cmd->args[0])) {
        if (*cur != ' ') {
            cur_args->lexeme = cur;
            cur++;
            while (*cur != ' ' && *cur != '\r') {
                cur++;
            }
            cur_args->size = (u8)(cur - cur_args->lexeme);
            cur_args++;
        }
        else {
            cur++;
        }
    }
}

void dump_cmd(struct command* cmd) {
    switch(cmd->type) {
        case CMD_CONTINUE: {
            print("%s\n", "continue");
            break;
        }
        case CMD_UNKNOWN:
            print("%s\n", "unknown command encountered");
            break;
        default:
            break;
    }
}

struct command command_parser(const char* str_cmd, size_t str_cmd_len) {
    struct command cmd;
    cmd.type = 0;
    struct token tokens[64U];

    lexer(str_cmd, str_cmd_len, tokens);

    u32 index = 0;
    while(tokens[index].type != TOKEN_UNKNOWN) {
        if(tokens[index].type == TOKEN_DECIMAL_NUMBER) {
            print("Decimal umber\n");
        } else if(tokens[index].type == TOKEN_HEXADECIMAL_NUMBER) {
            print("Hexadecimal number\n");
        } else if(tokens[index].type == TOKEN_CONTINUE) {
            print("Continue\n");
        } else if(tokens[index].type == TOKEN_PRINT_REG) {
            print("Print reg\n");
        } else if(tokens[index].type == TOKEN_STEP_INTO) {
            print("Print step into\n");
        } else if(tokens[index].type == TOKEN_QUIT) {
            print("Print quit\n");
        } else if(tokens[index].type == TOKEN_DUMP_MEMORY) {
            print("Print dump memory\n");
        }
        index++;
    }

    // tokenize(str_cmd, str_cmd_len, &cmd);
    // dump_cmd(&cmd);

    return cmd;
}

void interp_command(struct command* cmd, LPDEBUG_EVENT dbg_event) {
    CONTEXT ctx = {
        .ContextFlags = CONTEXT_ALL,
    };
    HANDLE h_thread = OpenThread(
        THREAD_GET_CONTEXT | THREAD_SET_CONTEXT,
        FALSE,
        dbg_event->dwThreadId
    );
    GetThreadContext(h_thread, &ctx);

    if (cmd->type == CMD_CONTINUE) {
        ContinueDebugEvent(
            dbg_event->dwProcessId,
            dbg_event->dwThreadId,
            continue_status
        );
        process_commands = FALSE;
    } else if (cmd->type == CMD_PRINT_REG) {
        print("RIP : %x\n", ctx.Rip);
        print("RAX : %x\n", ctx.Rax);
        print("RCX : %x\n", ctx.Rcx);
        print("RDX : %x\n", ctx.Rdx);
        print("RBX : %x\n", ctx.Rbx);
        print("RSP : %x\n", ctx.Rsp);
        print("RBP : %x\n", ctx.Rbp);
        print("RSI : %x\n", ctx.Rsi);
        print("RDI : %x\n", ctx.Rdi);
        process_commands = TRUE;
    } else if (cmd->type == CMD_STEP_INTO) {
        ctx.EFlags |= 0x100; // TRAP_FLAG
        if(SetThreadContext(h_thread, &ctx) == 0) {
            __debugbreak();
        }
        ContinueDebugEvent(
            dbg_event->dwProcessId,
            dbg_event->dwThreadId,
            continue_status
        );
        expect_single_step = TRUE;
        process_commands = FALSE;
    } else if (cmd->type == CMD_QUIT) {
        is_open = FALSE;
        process_commands = FALSE;
    } else if (cmd->type == CMD_DUMP_MEMORY) {
        // ReadProcessMemory(dbg_event.)
    } else {
        print("Command unknown\n");
        process_commands = TRUE;
    }

    CloseHandle(h_thread);
}

int mainCRTStartup() {

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

    h_input = GetStdHandle(STD_INPUT_HANDLE);
    if (h_input == NULL) {
        TerminateProcess(proc_info.hProcess, 1U);
        return 1U;
    }
    DWORD console_mode = ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT;
    SetConsoleMode(h_input, console_mode);

    while(is_open) {
        DEBUG_EVENT dbg_event;
        WaitForDebugEventEx(&dbg_event, INFINITE);

        continue_status = DBG_EXCEPTION_NOT_HANDLED;

        switch (dbg_event.dwDebugEventCode) {
            case CREATE_PROCESS_DEBUG_EVENT: {
                OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, dbg_event.dwThreadId);
                print("%s\n", "CreateProcessDebugEvent");
                break;
            }
            case CREATE_THREAD_DEBUG_EVENT: {
                print("%s\n", "CreateThreadDebugEvent");
                break;
            }
            case EXCEPTION_DEBUG_EVENT: {
                print("%s : ", "ExceptionDebugEvent");

                if (dbg_event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP && expect_single_step) {
                    print("%s\n", "Single Step");
                    continue_status = DBG_CONTINUE;
                    expect_single_step = FALSE;
                } else if (dbg_event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT) {
                    print("%s\n", "Breakpoint");
                    continue_status = DBG_CONTINUE;
                } else {
                    print("%u\n", dbg_event.u.Exception.ExceptionRecord.ExceptionCode);
                }

                break;
            }
            case EXIT_PROCESS_DEBUG_EVENT: {
                print("%s\n", "ExitProcessDebugEvent");
                TerminateProcess(NULL, 0U);
                return 0;
                break;
            }
            case EXIT_THREAD_DEBUG_EVENT: {
                print("%s\n", "ExitThreadDebugEvent");
                break;
            }
            case LOAD_DLL_DEBUG_EVENT: {
                // print("%s%s\n", "Loading DLL : ", (char*)dbg_event.u.LoadDll.lpImageName);
                print("%s%s\n", "Loading DLL : ", NULL);
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

        process_commands = TRUE;
        while(process_commands) {
            char input_buffer[16];
            DWORD input_size = 0U;
            ReadConsole(h_input, input_buffer, sizeof(input_buffer), &input_size, NULL);

            struct command cmd = command_parser(input_buffer, input_size);
            cmd.type = 0;
            // interp_command(&cmd, &dbg_event);
        }
    }

    TerminateProcess(proc_info.hProcess, 0U);
    return 0;
}
