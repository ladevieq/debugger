#include <Windows.h>
#include <strsafe.h>

#include "types.h"

#include "utils.c"

static HANDLE h_input = NULL;
static u8 is_open = TRUE;
static u8 process_commands = TRUE;
static u8 expect_single_step = FALSE;
static DWORD continue_status = DBG_EXCEPTION_NOT_HANDLED;
static DEBUG_EVENT dbg_event;
static PROCESS_INFORMATION proc_info;

enum TOKEN_TYPE {
    // Commands
    TOKEN_CONTINUE,
    TOKEN_PRINT_REG,
    TOKEN_STEP_INTO,
    TOKEN_DUMP_MEMORY,
    TOKEN_QUIT,

    // Primaries
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
    const char* start;
    u8 size;
};

struct ast_node {
    struct token* token;
    struct ast_node* left;
    struct ast_node* right;
};

static struct command commands[] = {
    { .start = "g", .size = 1 },
    { .start = "r", .size = 1 },
    { .start = "t", .size = 1 },
    { .start = "db", .size = 2 },
    { .start = "q", .size = 1 },
};

_Static_assert((sizeof(commands) / sizeof(commands[0])) == TOKEN_QUIT + 1, "Missing commands");

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

            for(int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
                struct command cur_cmd = commands[i];
                if (CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, cur_cmd.start, cur_cmd.size, cur_token->lexeme, cur_token->size) == CSTR_EQUAL) {
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

struct ast_node* parse_primary(struct token* tokens, struct ast_node* nodes) {
    struct ast_node* cur_node = &nodes[0];
    struct token* cur_token = &tokens[0];
    if (cur_token->type == TOKEN_HEXADECIMAL_NUMBER ||
        cur_token->type == TOKEN_DECIMAL_NUMBER) {
        cur_node->token = cur_token;
        cur_node->left = NULL;
        cur_node->right = NULL;
        return cur_node;
    } else {
        print("Expecting a primary token\n");
        return NULL;
    }
}

struct ast_node* parse_command(struct token* tokens, struct ast_node* nodes)
{
    struct ast_node* cur_node = &nodes[0];
    struct token* cur_token = &tokens[0];
    switch(cur_token->type) {
        case TOKEN_CONTINUE:
            tokens++;
            print("TOKEN_CONTINUE\n");
            cur_node->token = cur_token;
            cur_node->left = NULL;
            cur_node->right = NULL;
            return cur_node;
        case TOKEN_PRINT_REG:
            tokens++;
            print("TOKEN_PRINT_REG\n");
            cur_node->token = cur_token;
            cur_node->left = NULL;
            cur_node->right = NULL;
            return cur_node;
        case TOKEN_STEP_INTO:
            tokens++;
            print("TOKEN_STEP_INTO\n");
            cur_node->token = cur_token;
            cur_node->left = NULL;
            cur_node->right = NULL;
            return cur_node;
        case TOKEN_DUMP_MEMORY:
            tokens++;
            print("TOKEN_DUMP_MEMORY\n");
            cur_node->token = cur_token;
            cur_node->left = NULL;
            cur_node->right = parse_primary(tokens, ++nodes);
            return cur_node;
        case TOKEN_QUIT:
            tokens++;
            print("TOKEN_QUIT\n");
            cur_node->token = cur_token;
            cur_node->left = NULL;
            cur_node->right = NULL;
            return cur_node;
        default:
            print("%s must be a command\n", cur_token->lexeme);
            return NULL;
    }
}

u64 eval_number(const struct ast_node* node) {
    if (node->token->type == TOKEN_DECIMAL_NUMBER ||
        node->token->type == TOKEN_HEXADECIMAL_NUMBER) {
        return atou64(node->token->lexeme, node->token->size);
    }
    return ~0U;
}

void run(struct ast_node* root) {
    struct ast_node cur_node = root[0];
    CONTEXT ctx = {
        .ContextFlags = CONTEXT_ALL,
    };
    HANDLE h_thread = OpenThread(
        THREAD_GET_CONTEXT | THREAD_SET_CONTEXT,
        FALSE,
        dbg_event.dwThreadId
    );
    GetThreadContext(h_thread, &ctx);

    if (cur_node.token->type == TOKEN_CONTINUE) {
        ContinueDebugEvent(
            dbg_event.dwProcessId,
            dbg_event.dwThreadId,
            continue_status
        );
        process_commands = FALSE;
    } else if (cur_node.token->type == TOKEN_PRINT_REG) {
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
    } else if (cur_node.token->type == TOKEN_STEP_INTO) {
        ctx.EFlags |= 0x100; // TRAP_FLAG
        if(SetThreadContext(h_thread, &ctx) == 0) {
            __debugbreak();
        }
        ContinueDebugEvent(
            dbg_event.dwProcessId,
            dbg_event.dwThreadId,
            continue_status
        );
        expect_single_step = TRUE;
        process_commands = FALSE;
    } else if (cur_node.token->type == TOKEN_QUIT) {
        is_open = FALSE;
        process_commands = FALSE;
    } else if (cur_node.token->type == TOKEN_DUMP_MEMORY) {
        u64 addr = eval_number(cur_node.right);
        u8 buf[16U] = {};
        size_t b_read = 0U;

        HRESULT hr = ReadProcessMemory(
            proc_info.hProcess,
            (void*)addr,
            buf,
            sizeof(buf),
            &b_read
        );

        if (hr > 0) {
            print("%b %b %b %b %b %b %b %b %b %b %b %b %b %b %b %b\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
        } else {
            print("Unable to read memory at this address\n");
        }

    } else {
        print("Command unknown\n");
        process_commands = TRUE;
    }

    CloseHandle(h_thread);
}

u8 interpreter(const char* str_cmd, size_t str_cmd_len) {
    struct token tokens[64U];
    struct ast_node nodes[64U];

    lexer(str_cmd, str_cmd_len, tokens);

    struct ast_node* root = parse_command(tokens, nodes);

    if(root == NULL) {
        print("Ill formed command\n");
        return FALSE;
    }

    run(root);
    return TRUE;
}

int mainCRTStartup() {

    STARTUPINFOW startup_info = {
        .cb = sizeof(startup_info),
    };

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

            interpreter(input_buffer, input_size);
        }
    }

    TerminateProcess(proc_info.hProcess, 0U);
    return 0;
}
