#ifndef WYN_HASHSET_H
#define WYN_HASHSET_H

typedef struct WynHashSet WynHashSet;

WynHashSet* hashset_new(void);
void hashset_add(WynHashSet* set, const char* key);
int hashset_contains(WynHashSet* set, const char* key);
void hashset_remove(WynHashSet* set, const char* key);
void hashset_free(WynHashSet* set);

// Wrapper functions
void wyn_hashset_insert(WynHashSet* set, const char* key);
int wyn_hashset_contains(WynHashSet* set, const char* key);
void wyn_hashset_remove(WynHashSet* set, const char* key);
int wyn_hashset_len(WynHashSet* set);
int wyn_hashset_is_empty(WynHashSet* set);
void wyn_hashset_clear(WynHashSet* set);
WynHashSet* wyn_hashset_union(WynHashSet* set1, WynHashSet* set2);
WynHashSet* wyn_hashset_intersection(WynHashSet* set1, WynHashSet* set2);
WynHashSet* wyn_hashset_difference(WynHashSet* set1, WynHashSet* set2);
int wyn_hashset_is_subset(WynHashSet* set1, WynHashSet* set2);
int wyn_hashset_is_disjoint(WynHashSet* set1, WynHashSet* set2);

#endif
