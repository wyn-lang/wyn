#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define HASHMAP_INITIAL_SIZE 16
#define HASHMAP_LOAD_FACTOR 0.75

typedef struct HashMapEntry {
    char* key;
    void* value;
    struct HashMapEntry* next;
} HashMapEntry;

typedef struct HashMap {
    HashMapEntry** buckets;
    size_t size;
    size_t capacity;
} HashMap;

// Simple hash function (djb2)
static uint32_t hash_string(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// Create new HashMap
void* hashmap_new() {
    HashMap* map = malloc(sizeof(HashMap));
    if (!map) return NULL;
    
    map->capacity = HASHMAP_INITIAL_SIZE;
    map->size = 0;
    map->buckets = calloc(map->capacity, sizeof(HashMapEntry*));
    
    if (!map->buckets) {
        free(map);
        return NULL;
    }
    
    return map;
}

// Resize HashMap when load factor exceeded
static void hashmap_resize(HashMap* map) {
    if (!map) return;
    
    size_t old_capacity = map->capacity;
    HashMapEntry** old_buckets = map->buckets;
    
    map->capacity *= 2;
    map->buckets = calloc(map->capacity, sizeof(HashMapEntry*));
    if (!map->buckets) {
        map->buckets = old_buckets;
        map->capacity = old_capacity;
        return;
    }
    
    map->size = 0;
    
    // Rehash all entries
    for (size_t i = 0; i < old_capacity; i++) {
        HashMapEntry* entry = old_buckets[i];
        while (entry) {
            HashMapEntry* next = entry->next;
            
            uint32_t hash = hash_string(entry->key);
            size_t index = hash % map->capacity;
            
            entry->next = map->buckets[index];
            map->buckets[index] = entry;
            map->size++;
            
            entry = next;
        }
    }
    
    free(old_buckets);
}

// Set key-value pair
void hashmap_put(void* map_ptr, const char* key, const char* value) {
    HashMap* map = (HashMap*)map_ptr;
    if (!map || !key) return;
    
    // Check if resize needed
    if ((double)map->size / map->capacity >= HASHMAP_LOAD_FACTOR) {
        hashmap_resize(map);
    }
    
    uint32_t hash = hash_string(key);
    size_t index = hash % map->capacity;
    
    // Check if key already exists
    HashMapEntry* entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            free(entry->value);  // Free old value
            entry->value = strdup(value);  // Store copy of string
            return;
        }
        entry = entry->next;
    }
    
    // Create new entry
    HashMapEntry* new_entry = malloc(sizeof(HashMapEntry));
    if (!new_entry) return;
    
    new_entry->key = strdup(key);
    new_entry->value = strdup(value);  // Store copy of string
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;
}

// Get value by key
char* hashmap_get(void* map_ptr, const char* key) {
    HashMap* map = (HashMap*)map_ptr;
    if (!map || !key) return NULL;
    
    uint32_t hash = hash_string(key);
    size_t index = hash % map->capacity;
    
    HashMapEntry* entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return (char*)entry->value;
        }
        entry = entry->next;
    }
    
    return NULL;
}

// Check if key exists
int64_t hashmap_contains(void* map_ptr, const char* key) {
    HashMap* map = (HashMap*)map_ptr;
    if (!map || !key) return 0;
    
    uint32_t hash = hash_string(key);
    size_t index = hash % map->capacity;
    
    HashMapEntry* entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return 1;
        }
        entry = entry->next;
    }
    
    return 0;
}

// Remove key-value pair
void hashmap_remove(void* map_ptr, const char* key) {
    HashMap* map = (HashMap*)map_ptr;
    if (!map || !key) return;
    
    uint32_t hash = hash_string(key);
    size_t index = hash % map->capacity;
    
    HashMapEntry* entry = map->buckets[index];
    HashMapEntry* prev = NULL;
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                map->buckets[index] = entry->next;
            }
            
            free(entry->key);
            free(entry->value);  // Free string value
            free(entry);
            map->size--;
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

// Get HashMap size
int64_t hashmap_size(void* map_ptr) {
    HashMap* map = (HashMap*)map_ptr;
    return map ? (int64_t)map->size : 0;
}

// Clear all entries
void hashmap_clear(void* map_ptr) {
    HashMap* map = (HashMap*)map_ptr;
    if (!map) return;
    
    for (size_t i = 0; i < map->capacity; i++) {
        HashMapEntry* entry = map->buckets[i];
        while (entry) {
            HashMapEntry* next = entry->next;
            free(entry->key);
            free(entry->value);  // Free string value
            free(entry);
            entry = next;
        }
        map->buckets[i] = NULL;
    }
    
    map->size = 0;
}

// Get all keys as array
void* hashmap_keys(void* map_ptr) {
    HashMap* map = (HashMap*)map_ptr;
    if (!map) return NULL;
    
    // Create array: [length, capacity, key0, key1, ...]
    char** keys = malloc((map->size + 2) * sizeof(char*));
    if (!keys) return NULL;
    
    keys[0] = (char*)(intptr_t)map->size;  // length
    keys[1] = (char*)(intptr_t)map->size;  // capacity
    
    size_t key_index = 2;
    for (size_t i = 0; i < map->capacity; i++) {
        HashMapEntry* entry = map->buckets[i];
        while (entry) {
            keys[key_index++] = strdup(entry->key);
            entry = entry->next;
        }
    }
    
    return keys;
}

// Get all values as array
void* hashmap_values(void* map_ptr) {
    HashMap* map = (HashMap*)map_ptr;
    if (!map) return NULL;
    
    // Create array: [length, capacity, value0, value1, ...]
    void** values = malloc((map->size + 2) * sizeof(void*));
    if (!values) return NULL;
    
    values[0] = (void*)(intptr_t)map->size;  // length
    values[1] = (void*)(intptr_t)map->size;  // capacity
    
    size_t value_index = 2;
    for (size_t i = 0; i < map->capacity; i++) {
        HashMapEntry* entry = map->buckets[i];
        while (entry) {
            values[value_index++] = entry->value;
            entry = entry->next;
        }
    }
    
    return values;
}

// Free HashMap
void hashmap_free(void* map_ptr) {
    HashMap* map = (HashMap*)map_ptr;
    if (!map) return;
    
    hashmap_clear(map);
    free(map->buckets);
    free(map);
}