#include "module.h"

#include "utils.h"

struct module modules[64U] = { 0U };
u64 modules_list = ~0ULL;

u8 add_module() {
// struct module* add_module() {
    assert(!modules_list);
    u32 module_index = 0U;
    _BitScanForward64((unsigned long*)&module_index, modules_list);
    modules_list ^= (u64)(0x1 << module_index);

    return (u8)module_index;
};

void remove_module(u8 index) {
    modules_list |= (u64)(0x1 << (((i8)index) - 1U));
}
