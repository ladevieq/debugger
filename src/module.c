#include "module.h"

#include "utils.h"

struct module modules[64U] = { 0U };
u64 modules_list = ~0ULL;

// u64 module_symbols

u8 add_module() {
// struct module* add_module() {
    assert(!modules_list);
    u32 module_index = 0U;
    _BitScanForward64((unsigned long*)&module_index, modules_list);
    modules_list ^= (u64)(0x1 << module_index);

#ifdef HASHMAP
    modules[module_index].symbols = create_unordered_flat();
#endif

    return (u8)module_index;
};

void remove_module(u8 index) {
    modules_list |= (u64)(0x1 << (((i8)index) - 1U));
}

#ifdef HASHMAP
struct element* find_function(struct module* module, u32 rip_offset) {
    for (u32 sym_index = 0U; sym_index < module->symbols.capacity; sym_index++) {
        struct element* cur_sym = &module->symbols.arr[sym_index];
        if (cur_sym->start != 0U && cur_sym->start <= rip_offset && rip_offset < cur_sym->end) {
            return cur_sym;
        }
    }
    return NULL;
}
#else
u32 find_function(struct module* module, u64 rip_offset) {
    if (module->functions_count == 0U) {
        return (u32)-1;
    }
    u32 min = 0U, max = module->functions_count - 1U;

    while(min != max) {
        u32 mid = (min + max) / 2U;
        if (module->functions_start[mid] < rip_offset) {
            min = mid + 1U;
        } else {
            max = mid - 1U;
        }
    }

    return min;
}
#endif
