#include <Windows.h>

#include "types.h"
#include "utils.h"
#include "module.h"
#include "pdb.h"

u8 expect_single_step = FALSE;

extern DEBUG_EVENT dbg_event;

static void read_name_uni(const void* name_addr, const char* name, size_t name_size) {
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

void load_symbols(const void* base_addr, const struct module* module) {
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

    // Export table
    IMAGE_DATA_DIRECTORY export_table = module->nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    IMAGE_EXPORT_DIRECTORY export_dir = { 0U };
    const void* export_dir_start = (const void*)((u64)base_addr + (u64)export_table.VirtualAddress);
    const void* export_dir_end = (const void*)((u64)export_dir_start + export_table.Size);

    if (export_table.Size != 0U) {
        if (!read_memory((const void*)((u64)base_addr + (u64)export_table.VirtualAddress), (void*)&export_dir, sizeof(export_dir))) {
            print("Cannot read export dir for module %s at address %x\n", (char*)module->name, base_addr);
            return;
        }

        u16 name_ordinals[8192];
        if (!read_memory((const void*)((u64)base_addr + (u64)export_dir.AddressOfNameOrdinals), (void*)&name_ordinals, export_dir.NumberOfNames * sizeof(name_ordinals[0]))) {
            print("Cannot read name ordinals from export directory for module %s at address %x\n", (char*)module->name, base_addr);
            return;
        }

        u32 names[8192];
        if (!read_memory((const void*)((u64)base_addr + (u64)export_dir.AddressOfNames), (void*)&names, export_dir.NumberOfNames * sizeof(names[0]))) {
            print("Cannot read names from export directory for module %s at address %x\n", (char*)module->name, base_addr);
            return;
        }

        print("Found %u functions\n", export_dir.NumberOfNames);
        for (u32 i = 0U; i < export_dir.NumberOfNames; i++) {
            const char* func_name = (const char*)((u64)base_addr + names[name_ordinals[i]]);
            if (export_dir_end < (void*)func_name || (void*)func_name < export_dir_start) {
                print("%u : Fowarded function\n", i);
            } else {
                print("%u : Function name : %s\n", i, func_name);
            }
        }
    }


    // Private symbols
    IMAGE_DATA_DIRECTORY debug_table = module->nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    IMAGE_DEBUG_DIRECTORY debug_entries[20U] = { 0U };

    if (debug_table.VirtualAddress && !read_memory((const void*)((u64)base_addr + (u64)debug_table.VirtualAddress), debug_entries, debug_table.Size)) {
        print("Cannot read image debug infos for module %s at address %x\n", module->name, base_addr);
        return;
    }

    struct codeview_header {
        u32 signature;
        u32 guid[4U];
        u32 age;
        char path;
    };

    for (u32 i = 0U; i < debug_table.Size / sizeof(debug_entries[0U]); i++) {
        if (debug_entries[i].Type != IMAGE_DEBUG_TYPE_CODEVIEW) {
            print("Image debug type different from CodeView\n");
            continue;
        }

        struct codeview_header header = { 0U };
        if (!read_memory((const void*)((u64)base_addr + (u64)debug_entries[i].AddressOfRawData), (void*)&header, debug_entries[i].SizeOfData)) {
            print("Cannot read code view header for module %s at address %x\n", (char*)module->name, base_addr);
        }

        print("PDB path : %s\n", &header.path);
        open_pdb(&header.path);
    }
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
