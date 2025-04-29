#include "hashmap.h"

#include <Windows.h>

#include "utils.h"

static const u32 initial_size = 4096U * 100U;
static const u32 empty_psl = (u32)-1;
static const float max_load_factor = 0.75f;

void resize(struct hashmap* map) {
    void* alloc = VirtualAlloc((void*)(map->arr + 1U), map->capacity * sizeof(struct element), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (alloc == NULL) {
        alloc = VirtualAlloc(NULL, map->capacity * 2U * sizeof(struct element), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        map->capacity *= 2U;
        // copy elements + rehashing
    } else {
        map->capacity *= 2U;
        // rehashing
    }
}

struct hashmap create_unordered_flat() {
    struct hashmap map = {
        .capacity = initial_size / sizeof(struct element),
        .size = 0U,
        .arr = (struct element*)VirtualAlloc(NULL, initial_size * sizeof(struct element), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE),
    };

    for (u32 index = 0U; index <  map.capacity; index++) {
        map.arr[index].name = 0ULL;
        map.arr[index].start = 0U;
        map.arr[index].end = 0U;
        map.arr[index].psl = 0U;
    }
    return map;
}

struct element* insert_elem(struct hashmap* map, u32 start, u32 end, u64 name) {
    if (map->size >= map->capacity) {
        return NULL;
    }
    float load = map->size / (float)map->capacity;
    if (load >= max_load_factor) {
        resize(map);
    }

    u32 index = start % map->capacity;
    u32 psl = 0U;

    while(map->arr[index + psl].name != 0ULL || (map->arr[index + psl].psl >= psl && map->arr[index + psl].psl != 0U)) {
        psl++;
    }

    map->arr[index + psl].name = name;
    map->arr[index + psl].start = start;
    map->arr[index + psl].end = end;
    map->arr[index + psl].psl = psl;
    map->size++;

    return &map->arr[index + psl];
}

void remove_elem(struct hashmap* map, u32 start) {
    u32 index = start % map->capacity;

    while (map->arr[index].start != start) {
        index++;
    }

    while (map->arr[index].name != 0U && map->arr[index].psl != empty_psl && index < map->capacity) {
        map->arr[index - 1U].psl = map->arr[index].psl - 1U;
        map->arr[index - 1U].start = map->arr[index].start;
        map->arr[index - 1U].end = map->arr[index].end;
        map->arr[index - 1U].name = map->arr[index].name;
        index++;
    }

    map->arr[index].psl = empty_psl;
}

struct element* find_elem(struct hashmap* map, u32 start) {
    u32 index = start % map->capacity;

    while (index < map->capacity) {
        if (map->arr[index].start == start) {
            return &map->arr[index];
        }
        index++;
    }

    return NULL;
}

void iterate(struct hashmap* map) {
    u32 counter = 0U;
    for (u32 index = 0U; index < map->capacity && counter < map->size; index++) {
        if (map->arr[index].name) {
            print("%s\n", map->arr[index].name);
            counter++;
        }
    }
}
