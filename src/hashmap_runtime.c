// HashMap Runtime Wrappers for Wyn
#include "hashmap.h"
#include <string.h>
#include <stdlib.h>

// Global registry to store HashMap pointers
#define MAX_HASHMAPS 1024
static WynHashMap* hashmap_registry[MAX_HASHMAPS] = {NULL};
static int next_handle = 1;

// Wrappers that use integer handles

int HashMap_new() {
    WynHashMap* map = hashmap_new();
    if (!map) return 0;
    
    // Find free slot
    for (int i = 1; i < MAX_HASHMAPS; i++) {
        if (hashmap_registry[i] == NULL) {
            hashmap_registry[i] = map;
            return i;
        }
    }
    
    // No free slots
    hashmap_free(map);
    return 0;
}

void HashMap_insert(int handle, const char* key, int value) {
    if (handle <= 0 || handle >= MAX_HASHMAPS || !key) return;
    WynHashMap* map = hashmap_registry[handle];
    if (!map) return;
    hashmap_insert_int(map, key, value);
}

int HashMap_get(int handle, const char* key) {
    if (handle <= 0 || handle >= MAX_HASHMAPS || !key) return -1;
    WynHashMap* map = hashmap_registry[handle];
    if (!map) return -1;
    return hashmap_get_int(map, key);
}

int HashMap_contains(int handle, const char* key) {
    if (handle <= 0 || handle >= MAX_HASHMAPS || !key) return 0;
    WynHashMap* map = hashmap_registry[handle];
    if (!map) return 0;
    return hashmap_has(map, key) ? 1 : 0;
}

int HashMap_len(int handle) {
    if (handle <= 0 || handle >= MAX_HASHMAPS) return 0;
    WynHashMap* map = hashmap_registry[handle];
    if (!map) return 0;
    return hashmap_len(map);
}

void HashMap_free(int handle) {
    if (handle <= 0 || handle >= MAX_HASHMAPS) return;
    WynHashMap* map = hashmap_registry[handle];
    if (!map) return;
    hashmap_free(map);
    hashmap_registry[handle] = NULL;
}

// Lowercase versions
int wyn_hashmap_new() {
    return HashMap_new();
}

void wyn_hashmap_insert_int(int handle, const char* key, int value) {
    HashMap_insert(handle, key, value);
}

int wyn_hashmap_get_int(int handle, const char* key) {
    return HashMap_get(handle, key);
}

int wyn_hashmap_has(int handle, const char* key) {
    return HashMap_contains(handle, key);
}

int wyn_hashmap_len(int handle) {
    return HashMap_len(handle);
}

void wyn_hashmap_free(int handle) {
    HashMap_free(handle);
}
