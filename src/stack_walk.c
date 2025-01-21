// enum UNWIND_INFO_FLAGS {
//     UNW_FLAG_EHANDLER,
//     UNW_FLAG_UHANDLER,
//     UNW_FLAG_CHAININFO,
// };

enum UNWIND_OP_TYPE {
    UWOP_PUSH_NONVOL,
    UWOP_ALLOC_LARGE,
    UWOP_ALLOC_SMALL,
    UWOP_SET_FPREG,
    UWOP_SAVE_NONVOL,
    UWOP_SAVE_NONVOL_FAR,
    UWOP_SAVE_XMM128 = 8U,
    UWOP_SAVE_XMM128_FAR,
    UWOP_PUSH_MACHFRAME,
};

struct UNWIND_CODE {
    u8 offset_in_prolog;
    u8 unwind_operation_code    : 4;
    u8 op_info                  : 4;
};

struct UNWIND_INFO {
    u8 version               : 3;
    u8 flags                 : 5;
    u8 size_of_prolog;
    u8 count_of_unwind_codes;
    u8 frame_register        : 4;
    u8 frame_register_offset : 4;
    struct UNWIND_CODE unwind_code[1U];
    // Optional Exception handler of chained unwind info
    //
    // Exception handler
    // ULONG address_of_exception_handler;
    // language specific handler data;
    //
    // Chained unwind info
    // ULONG function_start;
    // ULONG function_end;
    // ULONG unwind_info_address;
};

enum UNWIND_OP_INFO_TYPE {
    RAX,
    RCX,
    RDX,
    RBX,
    RSP,
    RBP,
    RSI,
    RDI,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
};

struct module* get_addr_module(u64 addr) {
    for (u8 index = 0U; index < 64U; index++) {
        if (((1ULL << index) & modules_list) != 0U) {
            continue;
        }

        struct module* module = &modules[index];
        u64 offset = addr - (u64)module->base_addr;
        if (0U <= offset && offset < module->nt_header.OptionalHeader.SizeOfImage) {
            return module;
        }
    }

    return NULL;
}

void print_reg(enum UNWIND_OP_INFO_TYPE op_info) {
    switch (op_info) {
        case RAX:
            print("RAX\n");
            break;
        case RCX:
            print("RCX\n");
            break;
        case RDX:
            print("RDX\n");
            break;
        case RBX:
            print("RBX\n");
            break;
        case RSP:
            print("RSP\n");
            break;
        case RBP:
            print("RBP\n");
            break;
        case RSI:
            print("RSI\n");
            break;
        case RDI:
            print("RDI\n");
            break;
        case R8:
            print("R8\n");
            break;
        case R9:
            print("R9\n");
            break;
        case R10:
            print("R10\n");
            break;
        case R11:
            print("R11\n");
            break;
        case R12:
            print("R12\n");
            break;
        case R13:
            print("R13\n");
            break;
        case R14:
            print("R14\n");
            break;
        case R15:
            print("R15\n");
            break;
    }
}

struct element* find_function(struct module* module, u64 rip_offset) {
    for (u32 sym_index = 0U; sym_index < module->symbols.capacity; sym_index++) {
        struct element* cur_sym = &module->symbols.arr[sym_index];
        if (cur_sym->start != 0U && cur_sym->start <= rip_offset && rip_offset < cur_sym->end) {
            return cur_sym;
        }
    }
    return NULL;
}

void walk_stack(CONTEXT* ctx) {
    u64 addr = ctx->Rip;
    u64 rsp = ctx->Rsp;
    u64 rbp = ctx->Rbp;
    struct module* module = NULL;
    struct UNWIND_INFO* u_info = VirtualAlloc(NULL, 4096U, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    while (addr) {
        module = get_addr_module(addr);
        // print("Module name %s\n", module->name);
        // print("Module address %xu\n", (u64)module->base_addr);
        // print("Module size %xu\n", module->nt_header.OptionalHeader.SizeOfImage);
        // print("Module code address %xu\n", (u64)module->base_addr + module->nt_header.OptionalHeader.BaseOfCode);
        // print("Module code size %xu\n", module->nt_header.OptionalHeader.SizeOfCode);
        // print("Function address %xu\n", addr);
        // print("Function offset in module %xu\n", addr - (u64)module->base_addr);

        struct element* sym = find_function(module, addr - (u64)module->base_addr);
        if (sym == NULL) {
            print("No symbol found at address %xu\n", addr);

            addr = *(u64*)rsp;
            continue;
        }

        u8 buffer[4096U];
        read_memory((void*)sym->name, buffer, 4096U);
        // print("Buffer is %s!%s\n", module->name, buffer);
        print("%s!%s\n", module->name, sym->name);
        // print("rsp value is %xu\n", rsp);
        // print("rbp value is %xu\n", rbp);

        if (!read_memory((const void*)((u64)module->base_addr + (u64)sym->unwind_offset), (void*)u_info, 4096U)) {
            print("Cannot read unwind info in for function %s\n", sym->name);
            continue;
        }

        size_t total_size = sizeof(*u_info) + (u_info->count_of_unwind_codes - 1U) * sizeof(struct UNWIND_CODE);
        assert(total_size < 4096U);

        // TODO: Do this once for the first function
        if (sym->start <= addr && addr < u_info->size_of_prolog) {
            // print("RIP is in prolog\n");
        } else if (1) {
            // print("RIP is in epilog\n");
            // u8* byte_code = (u8*)addr;
            addr = (u64)module->base_addr + sym->start + u_info->size_of_prolog;
        } else {
            // print("RIP is in body\n");
        }

        for (u32 code_index = 0U; code_index < u_info->count_of_unwind_codes;) {
            struct UNWIND_CODE* cur_code = &u_info->unwind_code[code_index];
            switch (cur_code->unwind_operation_code) {
                case UWOP_PUSH_NONVOL:
                    // print("push non volatile\n");
                    // print("push ");
                    // print_reg(cur_code->op_info);
                    rsp += 8U;
                    code_index++;
                    break;
                case UWOP_ALLOC_LARGE:
                    // print("alloc large\n");
                    if (cur_code->op_info == 0U) {
                        u16 size = *(u16*)&u_info->unwind_code[code_index + 1U];
                        if (u_info->frame_register) {
                            // print("sub rbp %xu\n", size);
                            rbp += size;
                        } else {
                            // print("sub rsp %xu\n", size);
                            rsp += size;
                        }
                        code_index += 2U;
                    } else {
                        u32 size = *(u32*)&u_info->unwind_code[code_index + 1U];
                        if (u_info->frame_register) {
                            // print("sub rbp %xu\n", size);
                            rbp += size;
                        } else {
                            // print("sub rsp %xu\n", size);
                            rsp += size;
                        }
                        code_index += 3U;
                    }
                    break;
                case UWOP_ALLOC_SMALL:
                    // print("alloc small\n");
                    u32 size = cur_code->op_info * 8U + 8U;
                    if (u_info->frame_register) {
                        // print("sub rbp %xu\n", size);
                        rbp += size;
                    } else {
                        // print("sub rsp %xu\n", size);
                        rsp += size;
                    }
                    code_index++;
                    break;
                case UWOP_SET_FPREG:
                    // print("set frame point reg\n");
                    // print("mov rbp rsp - %xu\n", u_info->frame_register_offset * 16U);
                    code_index++;
                    break;
                case UWOP_SAVE_NONVOL:
                    // print("save non volatile\n");
                    if (u_info->frame_register) {
                        // print("mov rbp ");
                    } else {
                        // print("mov rsp ");
                    }
                    // print_reg(cur_code->op_info);
                    code_index += 2U;
                    break;
                case UWOP_SAVE_NONVOL_FAR:
                    // print("save non volatile far\n");
                    code_index += 3U;
                    break;
                case UWOP_SAVE_XMM128:
                    // print("save xmm128\n");
                    code_index += 2U;
                    break;
                case UWOP_SAVE_XMM128_FAR:
                    // print("save xmm128 far\n");
                    code_index += 3U;
                    break;
                case UWOP_PUSH_MACHFRAME:
                    // print("push mach frame\n");
                    code_index++;
                    break;
                default:
                    // print("bad operation code\n");
                    code_index++;
            }
        }
        u64 val;
        read_memory((void*)rsp, &val, sizeof(u64));
        rsp += 8U;
        // print("called address is %xu\n", val);
        addr = val;
    }
    VirtualFree(u_info, 4096U, MEM_RELEASE);
}
