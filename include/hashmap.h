#pragma once

#include "types.h"

struct element {
    u64 name;
    u32 start;
    u32 end;
    u32 unwind_offset;
    u32 psl;
};

struct hashmap {
    u32 capacity;
    u32 size;

    struct element* arr;
};

extern struct hashmap create_unordered_flat();

extern struct element* insert_elem(struct hashmap* map, u32 start, u32 end, u64 name);

extern struct element* find_elem(struct hashmap* map, u32 start);

extern void iterate(struct hashmap* map);
