void handle_create_process() {
    print("%s\n", "CreateProcessDebugEvent");
}

void handle_create_thread() {
    print("%s\n", "CreateThreadDebugEvent");
}

void handle_exception() {
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
    const void* dll_base_addr = dll_info.lpBaseOfDll;
    if (dll_name_addr == NULL) {
        print("%s\n", "Loading DLL : error read dll name");
        return;
    }

    u8* addr = NULL;
    if (!read_memory(dll_name_addr, (void*)&addr, 8U)) {
        print("%s\n", "Loading DLL : error reading dll name");
        return;
    }

    u8 dll_name[1024U] = { 0U };
    if (!read_memory((const void*)addr, dll_name, sizeof(dll_name))) {
        print("Cannot read dll name from proc memory\n");
    }

    if (dll_info.fUnicode) {
        u8 dll_name_ansi[1024U] = { 0U };
        WideCharToMultiByte(
            CP_ACP,
            0,
            (wchar_t*)dll_name,
            -1,
            (char*)dll_name_ansi,
            sizeof(dll_name_ansi),
            NULL,
            NULL
        );
        StringCchCopyA((char*)dll_name, sizeof(dll_name), (const char*)dll_name_ansi);
    }
    print("%s%s\n", "Loading DLL : ", (char*)dll_name);

    IMAGE_DOS_HEADER dos_header = { 0U };
    if (!read_memory(dll_base_addr, (void*)&dos_header, sizeof(dos_header))) {
        print("Cannot read DOS header for module %s at address %x\n", dll_name, dll_base_addr);
    }

    IMAGE_NT_HEADERS64 nt_header = { 0U };
    if (!read_memory((const void*)((u64)dll_base_addr + dos_header.e_lfanew), (void*)&nt_header, sizeof(nt_header))) {
        print("Cannot read NT header for module %s at address %x\n", dll_name, dll_base_addr);
    }

    IMAGE_DATA_DIRECTORY export_table = nt_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    IMAGE_EXPORT_DIRECTORY export_dir = { 0U };

    if (!read_memory((const void*)((u64)dll_base_addr + (u64)export_table.VirtualAddress), (void*)&export_dir, export_table.Size)) {
        print("Cannot read export table for module at address %x\n", dll_base_addr);
    }
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
    if (read_memory(str_addr, dbg_str, str_size)) {
        print("%s\n", "OutputDebugStringDebugEvent : error read debug string");
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
