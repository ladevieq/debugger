#include <Windows.h>

#include "utils.h"
#include "pdb.h"

u8 expect_single_step = FALSE;

extern DEBUG_EVENT dbg_event;

const char* dummy_name = "Function_dummy_name";

void read_name_uni(const void* name_addr, const char* name, size_t name_size) {
    u8* addr = NULL;
    if (!read_memory(name_addr, (void*)&addr, 8U)) {
        print("%s %u\n", "Loading DLL : error reading dll name : ", GetLastError());
        return;
    }

    wchar_t name_uni[256U] = { 0U };
    if (!read_memory((const void*)addr, name_uni, sizeof(name_uni))) {
        print("Cannot read dll name from proc memory %u\n", GetLastError());
        return;
    }

    WideCharToMultiByte(
        CP_ACP,
        0,
        name_uni,
        -1,
        (char*)name,
        (int)name_size,
        NULL,
        NULL
    );
}

void read_name_ansi(const void* name_addr, const char* name, size_t name_size) {
    u8* addr = NULL;
    if (!read_memory(name_addr, (void*)&addr, 8U)) {
        print("%s\n", "Loading DLL : error reading dll name");
        return;
    }

    if (!read_memory((const void*)addr, (char*)name, name_size)) {
        print("Cannot read dll name from proc memory\n");
        return;
    }
}

void load_symbols(const void* base_addr, struct module* module) {
    if (!read_memory(base_addr, (void*)&module->dos_header, sizeof(module->dos_header))) {
        print("Cannot read DOS header for module %s at address %x\n", (char*)module->name, base_addr);
        return;
    }

    if (!read_memory((const void*)((u64)base_addr + module->dos_header.e_lfanew), (void*)&module->nt_header, sizeof(module->nt_header))) {
        print("Cannot read NT header for module %s at address %x\n", (char*)module->name, base_addr);
        return;
    }

    print("Module Base Address %xu\n", base_addr);
    print("Module Size %xu\n", module->nt_header.OptionalHeader.SizeOfImage);

#ifndef HASHMAP
    // Exception directory
    IMAGE_DATA_DIRECTORY exception_entry = module->nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];

    if (exception_entry.Size == 0U) {
        print("Exception directory is empty in module %s\n", module->name);
    }

    RUNTIME_FUNCTION* exception_table = VirtualAlloc(NULL, exception_entry.Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!read_memory((const void*)((u64)base_addr + (u64)exception_entry.VirtualAddress), (void*)exception_table, exception_entry.Size)) {
        print("Cannot read exception table for module %s at address %x\n", (char*)module->name, base_addr);
        return;
    }

    u32 pdata_entries = exception_entry.Size / sizeof(exception_table[0U]);
    module->functions_name = VirtualAlloc(NULL, pdata_entries * sizeof(module->functions_name[0U]), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    module->functions_start = VirtualAlloc(NULL, pdata_entries * sizeof(module->functions_start[0U]), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    module->functions_end = VirtualAlloc(NULL, pdata_entries * sizeof(module->functions_end[0U]), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    module->functions_unwind = VirtualAlloc(NULL, pdata_entries * sizeof(module->functions_unwind[0U]), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    module->functions_count = pdata_entries;
    for (u32 index = 0U; index < pdata_entries; index++) {
        RUNTIME_FUNCTION cur_entry = exception_table[index];
        module->functions_start[index] = cur_entry.BeginAddress;
        module->functions_end[index] = cur_entry.EndAddress;
        module->functions_unwind[index] = cur_entry.UnwindData;
        module->functions_name[index] = (u8*)dummy_name;
    }
#endif

    // Export table
    IMAGE_DATA_DIRECTORY export_table = module->nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    IMAGE_EXPORT_DIRECTORY export_dir = { 0U };
    const void* export_dir_start = (const void*)((u64)base_addr + (u64)export_table.VirtualAddress);
    const void* export_dir_end = (const void*)((u64)export_dir_start + export_table.Size);
    if (export_table.Size != 0U) {
        if (!read_memory(export_dir_start, (void*)&export_dir, sizeof(export_dir))) {
            print("Cannot read export dir for module %s at address %x\n", (char*)module->name, base_addr);
            return;
        }

        size_t ordinals_size = export_dir.NumberOfNames * sizeof(u16);
        u16* ordinals = VirtualAlloc(NULL, ordinals_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!read_memory((const void*)((u64)base_addr + (u64)export_dir.AddressOfNameOrdinals), (void*)ordinals, ordinals_size)) {
            print("Cannot read name ordinals from export directory for module %s at address %x\n", (char*)module->name, base_addr);
            return;
        }

        size_t names_size = export_dir.NumberOfNames * sizeof(u32);
        u32* names_address = VirtualAlloc(NULL, names_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!read_memory((const void*)((u64)base_addr + (u64)export_dir.AddressOfNames), (void*)names_address, names_size)) {
            print("Cannot read names from export directory for module %s at address %x\n", (char*)module->name, base_addr);
            return;
        }

        size_t addresses_size = export_dir.NumberOfFunctions * sizeof(u32);
        u32* addresses = VirtualAlloc(NULL, addresses_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!read_memory((const void*)((u64)base_addr + (u64)export_dir.AddressOfFunctions), (void*)addresses, addresses_size)) {
            print("Cannot read functions addresses from export directory for module %s at address %x\n", (char*)module->name, base_addr);
            return;
        }

#ifndef HASHMAP
        // u32 global_functions_index = 0U;
        for (u32 i = 0U; i < export_dir.NumberOfFunctions; i++) {
            u64 address = (u64)base_addr + addresses[i];
            u32 name_index = 0U;
            u8* name = string_stack_current;
            size_t length = 0U;
            for (; name_index < export_dir.NumberOfNames; name_index++) {
                if (ordinals[name_index] == i) {
                    // TODO: Add string management code
                    assert(string_stack_current - 16U * 4096U < string_stack_base);
                    void* name_address = (void*)((u64)module->base_addr + names_address[name_index]);
                    read_memory(name_address, name, 256U);
                    StringCchLengthA((PCNZCH)name, STRSAFE_MAX_CCH, &length);
                    break;
                }
            }

            if (name_index == export_dir.NumberOfNames) {
                print("%u : Function has no name, function address %xu\n", i, addresses[i]);
                continue;
            }

            if (export_dir_start < (void*)address && (void*)address < export_dir_end) {
                print("%u : Fowarded function : %s\n", i, name);
                continue;
            }

            u32 global_functions_index = 0U;
            for(; global_functions_index < module->functions_count; global_functions_index++) {
                if (module->functions_start[global_functions_index] == addresses[i]) {
                    string_stack_current[length] = '\0';
                    module->functions_name[global_functions_index] = name;
                    string_stack_current += length + 1U;
                    print("%u : Function name : %s, function address %xu\n", i, name, addresses[i]);
                    break;
                }
            }

            if (global_functions_index == module->functions_count) {
                print("Function not found : %s!\n", string_stack_current);
            }
        }
#endif

#ifdef HASHMAP
        print("Found %u functions\n", export_dir.NumberOfFunctions);
        for (u32 i = 0U; i < export_dir.NumberOfFunctions; i++) {
            char func_name[256U] = { 0 };
            u64 func_address = (u64)base_addr + addresses[i];
            u32 name_index = 0U;
            for (; name_index < export_dir.NumberOfNames; name_index++) {
                if (ordinals[name_index] == i) {
                    break;
                }
            }

            u64 name_address = (u64)base_addr + names_address[name_index];
            read_memory((void*)name_address, func_name, sizeof(func_name));

            if (export_dir_start < (void*)func_address && (void*)func_address < export_dir_end) {
                print("%u : Fowarded function : %s\n", i, func_name);
            } else if (name_index && name_index < export_dir.NumberOfNames) {
                print("%u : Function name : %s, function address %xu\n", i, func_name, addresses[i]);
                insert_elem(&module->symbols, addresses[i], 0U, name_address);
            } else {
                print("%u : Function has no name, function address %xu\n", i, addresses[i]);
                insert_elem(&module->symbols, addresses[i], 0U, (u64)dummy_name);
            }
        }
#endif

        VirtualFree(names_address, names_size, MEM_RELEASE);
        VirtualFree(ordinals, ordinals_size, MEM_RELEASE);
        VirtualFree(addresses, addresses_size, MEM_RELEASE);
    }

    // Private symbols
    IMAGE_DATA_DIRECTORY debug_table = module->nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    IMAGE_DEBUG_DIRECTORY debug_entries[20U] = { 0U };

    if (debug_table.VirtualAddress && !read_memory((const void*)((u64)base_addr + (u64)debug_table.VirtualAddress), debug_entries, debug_table.Size)) {
        print("Cannot read image debug infos for module %s at address %x\n", module->name, base_addr);
        return;
    }

    for (u32 i = 0U; i < debug_table.Size / sizeof(debug_entries[0U]); i++) {
        if (debug_entries[i].Type != IMAGE_DEBUG_TYPE_CODEVIEW) {
            print("Image debug type different from CodeView\n");
            continue;
        }

        struct codeview_header {
            u32 signature;
            u32 guid[4U];
            u32 age;
            char path;
        } header = { 0U };
        if (!read_memory((const void*)((u64)base_addr + (u64)debug_entries[i].AddressOfRawData), (void*)&header, debug_entries[i].SizeOfData)) {
            print("Cannot read code view header for module %s at address %x\n", (char*)module->name, base_addr);
        }

        print("PDB path : %s\n", &header.path);
        open_pdb(&header.path, module);
    }


#ifdef HASHMAP
    // Exception directory
    IMAGE_DATA_DIRECTORY exception_entry = module->nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];

    if (exception_entry.Size == 0U) {
        print("Exception directory is empty in module %s\n", module->name);
    }

    RUNTIME_FUNCTION* exception_table = VirtualAlloc(NULL, exception_entry.Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!read_memory((const void*)((u64)base_addr + (u64)exception_entry.VirtualAddress), (void*)exception_table, exception_entry.Size)) {
        print("Cannot read exception table for module %s at address %x\n", (char*)module->name, base_addr);
        return;
    }

    // Find unwind info offset
    u32 pdata_entries = exception_entry.Size / sizeof(exception_table[0U]);
    for (u32 index = 0U; index < pdata_entries; index++) {
        RUNTIME_FUNCTION cur_entry = exception_table[index];

        struct element* elem = find_elem(&module->symbols, cur_entry.BeginAddress);
        if (elem == NULL) {
            elem = insert_elem(&module->symbols, cur_entry.BeginAddress, cur_entry.EndAddress, 0ULL);
        } else {
            if (elem->name == 0U) {
                print("function name %s\n", elem->name);
            }

            elem->end = cur_entry.EndAddress;
        }

        elem->unwind_offset = cur_entry.UnwindData;

        // if (!read_memory((const void*)((u64)base_addr + (u64)cur_entry.UnwindData), (void*)u_info, 4096U)) {
        //     print("Cannot read unwind info in for function %s\n", elem->name);
        //     continue;
        // }

        // size_t total_size = sizeof(*u_info) + (u_info->count_of_unwind_codes - 1U) * sizeof(struct UNWIND_CODE);
        // assert(total_size < 4096U);

        // for (u32 code_index = 0U; code_index < u_info->count_of_unwind_codes; code_index++) {
        //     struct UNWIND_CODE* cur_code = &u_info->unwind_code[code_index];
        //     switch (cur_code->unwind_operation_code) {
        //         case UWOP_PUSH_NONVOL:
        //             print("push non volatile\n");
        //             print("push ");
        //             print_reg(cur_code->op_info);
        //             break;
        //         case UWOP_ALLOC_LARGE:
        //             print("alloc large\n");
        //             if (cur_code->op_info == 0U) {
        //                 u16 size = *(u16*)&u_info->unwind_code[code_index + 1U];
        //                 if (u_info->frame_register) {
        //                     print("sub rbp %xu\n", size);
        //                 } else {
        //                     print("sub rsp %xu\n", size);
        //                 }
        //                 code_index++;
        //             } else {
        //                 u32 size = *(u32*)&u_info->unwind_code[code_index + 1U];
        //                 if (u_info->frame_register) {
        //                     print("sub rbp %xu\n", size);
        //                 } else {
        //                     print("sub rsp %xu\n", size);
        //                 }
        //                 code_index += 2U;
        //             }
        //             break;
        //         case UWOP_ALLOC_SMALL:
        //             print("alloc small\n");
        //             u32 size = cur_code->op_info * 8U + 8U;
        //             if (u_info->frame_register) {
        //                 print("sub rbp %xu\n", size);
        //             } else {
        //                 print("sub rsp %xu\n", size);
        //             }
        //             break;
        //         case UWOP_SET_FPREG:
        //             print("set frame point reg\n");
        //             print("mov rbp rsp - %xu\n", u_info->frame_register_offset * 16U);
        //             break;
        //         case UWOP_SAVE_NONVOL:
        //             print("save non volatile\n");
        //             if (u_info->frame_register) {
        //                 print("mov rbp ");
        //             } else {
        //                 print("mov rsp ");
        //             }
        //             print_reg(cur_code->op_info);
        //             code_index++;
        //             break;
        //         case UWOP_SAVE_NONVOL_FAR:
        //             print("save non volatile far\n");
        //             code_index += 2U;
        //             break;
        //         case UWOP_SAVE_XMM128:
        //             print("save xmm128\n");
        //             code_index++;
        //             break;
        //         case UWOP_SAVE_XMM128_FAR:
        //             print("save xmm128 far\n");
        //             code_index += 2U;
        //             break;
        //         case UWOP_PUSH_MACHFRAME:
        //             print("push mach frame\n");
        //             break;
        //         default:
        //             print("bad operation code\n");
        //     }
        // }
    }
#endif

    // VirtualFree(u_info, 4096U, MEM_RELEASE);
    VirtualFree(exception_table, exception_entry.Size, MEM_RELEASE);
}

void handle_create_process() {
    print("%s\n", "CreateProcessDebugEvent");

    CREATE_PROCESS_DEBUG_INFO proc_dbg_info = dbg_event.u.CreateProcessInfo;

    HANDLE hproc = OpenProcess(PROCESS_VM_READ, FALSE, dbg_event.dwProcessId);
    hproc;

    struct module* module = &modules[add_module()];
    // https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-create_process_debug_info
    // Debuggers must be prepared to handle the case where lpImageName is NULL or *lpImageName (in the address space of the process being debugged) is NULL.
    // Specifically, the system does not provide an image name for a create process event
    // module->base_addr = proc_dbg_info.lpBaseOfImage;
    // read_name_uni(proc_dbg_info.lpImageName, (const char*)module->name, sizeof(module->name));
    module->base_addr = proc_dbg_info.lpBaseOfImage;
    GetFinalPathNameByHandleA(proc_dbg_info.hFile, (LPSTR)module->name, sizeof(module->name), FILE_NAME_NORMALIZED);
    print("%s%s\n", "Loading Process : ", (char*)module->name);

    load_symbols((const void*)proc_dbg_info.lpBaseOfImage, module);
}

void handle_create_thread() {
    print("%s\n", "CreateThreadDebugEvent");
}

void handle_exception() {
    extern DWORD continue_status;

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
}

void handle_exit_process() {
    print("%s\n", "ExitProcessDebugEvent");
    TerminateProcess(NULL, 0U);
}

void handle_exit_thread() {
    print("%s\n", "ExitThreadDebugEvent");
}

void handle_load_dll() {
    LOAD_DLL_DEBUG_INFO dll_info = dbg_event.u.LoadDll;
    const void* dll_name_addr = dll_info.lpImageName;
    void* dll_base_addr = dll_info.lpBaseOfDll;
    if (dll_name_addr == NULL) {
        print("%s\n", "Loading DLL : error reading dll name");
        return;
    }

    struct module* module = &modules[add_module()];
    module->base_addr = dll_base_addr;
    read_name_uni(dll_name_addr, (const char*)module->name, sizeof(module->name));

    print("%s%s\n", "Loading DLL : ", (char*)module->name);

    load_symbols(dll_base_addr, module);
}

void handle_unload_dll() {
    print("%s\n", "UnloadDLLDebugEvent");
}

void handle_output_debug_string() {
    OUTPUT_DEBUG_STRING_INFO dbg_str_event = dbg_event.u.DebugString;
    char* str_addr = dbg_str_event.lpDebugStringData;
    size_t str_size = (size_t)dbg_str_event.nDebugStringLength;
    // assert(str_size > 1024U);
    u8 dbg_str[1024U] = { 0U };
    if (!read_memory(str_addr, dbg_str, str_size)) {
        print("%s %u\n", "OutputDebugStringDebugEvent : error reading debug string : ", GetLastError());
        return;
    }
    if (dbg_str_event.fUnicode) {
        u8 ansi_str[1024U] = { 0U };
        WideCharToMultiByte(
            CP_ACP,
            0,
            (wchar_t*)dbg_str,
            -1,
            (char*)ansi_str,
            sizeof(ansi_str),
            NULL,
            NULL
        );
        print("%s%s\n", "OutputDebugStringDebugEvent : ", ansi_str);
    } else {
        print("%s%s\n", "OutputDebugStringDebugEvent : ", dbg_str);
    }
}

void handle_rip() {
    print("%s\n", "RIPEvent");
}
