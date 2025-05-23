#include "parser.h"

#include <Windows.h>

#include "utils.h"
#include "module.h"
#include "stack_walk.h"

extern u8 expect_single_step;
extern DWORD continue_status;

enum TOKEN_TYPE {
    // Commands
    TOKEN_CONTINUE,
    TOKEN_CALLSTACK,
    TOKEN_LISTMODULE,
    TOKEN_DUMP_SYMBOLS,
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

#define DECLARE_COMMAND(x) { .start = x, .size = sizeof(x) }

static struct command commands[] = {
    { .start = "g", .size = 1 },
    { .start = "k", .size = 1 },
    { .start = "lm", .size = 2 },
    DECLARE_COMMAND("x"),
    { .start = "r", .size = 1 },
    { .start = "t", .size = 1 },
    { .start = "db", .size = 2 },
    { .start = "q", .size = 1 },
};

_Static_assert((sizeof(commands) / sizeof(commands[0])) == TOKEN_QUIT + 1, "Missing commands");

void lexer(const char* str_cmd, size_t str_cmd_len, struct token* tokens) {
    const char* cur = str_cmd;
    const char* end = str_cmd + str_cmd_len;
    struct token* cur_token = tokens;
    str_cmd_len = 0;

    while(cur < end) {
        cur_token->type = TOKEN_UNKNOWN;
        if(*cur == '0' && cur[1] == 'x') {
            const char* start = cur;
            cur += 2;

            while(is_hexadecimal_digit(*cur) && cur < end) {
                cur++;
            }

            cur_token->lexeme = start;
            cur_token->type = TOKEN_HEXADECIMAL_NUMBER;
            cur_token->size = (u8)(cur - start);
            cur_token++;
        } else if(is_decimal_digit(*cur)) {
            const char* start = cur;
            cur++;

            while(is_decimal_digit(*cur) && cur < end) {
                cur++;
            }

            cur_token->lexeme = start;
            cur_token->type = TOKEN_DECIMAL_NUMBER;
            cur_token->size = (u8)(cur - start);
            cur_token++;
        } else if(is_alpha(*cur)) {
            const char* start = cur;
            cur++;
            while(is_alpha(*cur) && cur < end) {
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

struct ast_node* parse_command(struct token* tokens, struct ast_node* nodes) {
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
        case TOKEN_CALLSTACK:
            tokens++;
            print("TOKEN_CALLSTACK\n");
            cur_node->token = cur_token;
            cur_node->left = NULL;
            cur_node->right = NULL;
            return cur_node;
        case TOKEN_LISTMODULE:
            tokens++;
            print("TOKEN_LISTMODULE\n");
            cur_node->token = cur_token;
            cur_node->left = NULL;
            cur_node->right = NULL;
            return cur_node;
        case TOKEN_DUMP_SYMBOLS:
            tokens++;
            print("TOKEN_DUMP_SYMBOLS\n");
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
            cur_node->left = parse_primary(tokens++, ++nodes);
            cur_node->right = parse_primary(tokens++, ++nodes);
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
    extern DEBUG_EVENT dbg_event;
    extern u8 process_commands;
    extern u8 is_open;

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
    } else if (cur_node.token->type == TOKEN_CALLSTACK) {
        walk_stack(&ctx);
    } else if (cur_node.token->type == TOKEN_LISTMODULE) {
        for (u8 index = 0U; index < 64U; index++) {
            if (((1ULL << index) & modules_list) == 0U) {
                struct module* module = &modules[index];
                print("Module Name : %s\n", module->name);
                print("Module Base : %xu\n", module->base_addr);
                print("Module Size : %xu\n\n", module->nt_header.OptionalHeader.SizeOfImage);
            }
        }
    } else if (cur_node.token->type == TOKEN_PRINT_REG) {
        print("RIP : %xu\n", ctx.Rip);
        print("RCX : %xu\n", ctx.Rcx);
        print("RDX : %xu\n", ctx.Rdx);
        print("RBX : %xu\n", ctx.Rbx);
        print("RSP : %xu\n", ctx.Rsp);
        print("RBP : %xu\n", ctx.Rbp);
        print("RSI : %xu\n", ctx.Rsi);
        print("RDI : %xu\n", ctx.Rdi);
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
        u64 addr = eval_number(cur_node.left);
        u64 size = eval_number(cur_node.right);
        u8* buf = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        if(read_memory((const void*)addr, (void*)buf, size)) {
            for (u32 index = 0U; index < size; index += 8U) {
                print("%xb %xb %xb %xb %xb %xb %xb %xb\n", buf[index], buf[index + 1U], buf[index + 2U], buf[index + 3U], buf[index + 4U], buf[index + 5U], buf[index + 6U], buf[index + 7U]);
            }
            print("\n");
        } else {
            
            print("Unable to read memory at this address, error %u\n", GetLastError());
        }

        VirtualFree(buf, size, MEM_RELEASE);
    } else if (cur_node.token->type == TOKEN_DUMP_SYMBOLS) {
        for (u8 index = 0U; index < 64U; index++) {
            if (((1ULL << index) & modules_list) == 0U) {
                struct module* module = &modules[index];
#ifdef HASHMAP
                iterate(&module->symbols);
#else
                for (u32 sym_index = 0U; sym_index < module->functions_count; sym_index++) {
                    print("%s\n", module->functions_name[index]);
                }
#endif
            }
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
