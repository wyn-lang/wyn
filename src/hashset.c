#include "hashset.h"
#include <stdlib.h>
#include <string.h>

#define HASHSET_SIZE 128

typedef struct Entry {
    char* key;
    struct Entry* next;
} Entry;

struct WynHashSet {
    Entry* buckets[HASHSET_SIZE];
};

static unsigned int hash(const char* key) {
    unsigned int h = 0;
    while (*key) {
        h = h * 31 + *key++;
    }
    return h % HASHSET_SIZE;
}

WynHashSet* hashset_new(void) {
    WynHashSet* set = calloc(1, sizeof(WynHashSet));
    return set;
}

void hashset_add(WynHashSet* set, const char* key) {
    unsigned int idx = hash(key);
    Entry* entry = set->buckets[idx];
    
    // Check if already exists
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return; // Already in set
        }
        entry = entry->next;
    }
    
    // Add new entry
    Entry* new_entry = malloc(sizeof(Entry));
    new_entry->key = strdup(key);
    new_entry->next = set->buckets[idx];
    set->buckets[idx] = new_entry;
}

int hashset_contains(WynHashSet* set, const char* key) {
    unsigned int idx = hash(key);
    Entry* entry = set->buckets[idx];
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return 1;
        }
        entry = entry->next;
    }
    
    return 0;
}

void hashset_remove(WynHashSet* set, const char* key) {
    unsigned int idx = hash(key);
    Entry* entry = set->buckets[idx];
    Entry* prev = NULL;
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                set->buckets[idx] = entry->next;
            }
            free(entry->key);
            free(entry);
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

void hashset_free(WynHashSet* set) {
    for (int i = 0; i < HASHSET_SIZE; i++) {
        Entry* entry = set->buckets[i];
        while (entry) {
            Entry* next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    free(set);
}
