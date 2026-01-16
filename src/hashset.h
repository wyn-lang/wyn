#ifndef WYN_HASHSET_H
#define WYN_HASHSET_H

typedef struct WynHashSet WynHashSet;

WynHashSet* hashset_new(void);
void hashset_add(WynHashSet* set, const char* key);
int hashset_contains(WynHashSet* set, const char* key);
void hashset_remove(WynHashSet* set, const char* key);
void hashset_free(WynHashSet* set);

#endif
