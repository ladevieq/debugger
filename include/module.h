#pragma once

#include <Windows.h>

#include "types.h"
#include "hashmap.h"

struct module {
    void* base_addr;
    u8**    functions_name;
    u32*    functions_start;
    u32*    functions_end;
    u32*    functions_unwind;
    u32     functions_count;
    u32     functions_capacity;
    // u32     functions_name_capacity;
    // u32     function_start_capacity;
    // u32     function_end_capacity;
    // u32     function_unwind_capacity;
    
    IMAGE_DOS_HEADER dos_header;
    IMAGE_NT_HEADERS64 nt_header;
#ifdef HASHMAP
    struct hashmap symbols;
#endif
    u8 name[256U];
};


extern struct module modules[64U];
extern u64 modules_list;

extern u8 add_module();

extern void remove_module(u8 index);

#ifdef HASHMAP
extern struct element* find_function(struct module* module, u32 rip_offset);
#else
extern u32 find_function(struct module* module, u64 rip_offset);
#endif // HASHMAP
