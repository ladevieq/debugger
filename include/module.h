#pragma once

#include <Windows.h>

#include "types.h"

struct module {
    void* base_addr;
    IMAGE_DOS_HEADER dos_header;
    IMAGE_NT_HEADERS64 nt_header;
    u8 name[256U];
};

extern struct module modules[64U];
extern u64 modules_list;

extern u8 add_module();

extern void remove_module(u8 index);
