#define _POSIX_C_SOURCE 200809L
#include "scope.h"
#include <stdlib.h>
#include <string.h>

IdentScope* ident_scope_create(IdentScopeLevel level, IdentScope* parent) {
    IdentScope* scope = calloc(1, sizeof(IdentScope));
    scope->level = level;
    scope->parent = parent;
    scope->entries = NULL;
    return scope;
}

void ident_scope_free(IdentScope* scope) {
    if (!scope) return;
    
    IdentScopeEntry* entry = scope->entries;
    while (entry) {
        IdentScopeEntry* next = entry->next;
        free(entry->name);
        free(entry);
        entry = next;
    }
    free(scope);
}

void ident_scope_add(IdentScope* scope, const char* name, IdentScopeLevel level, bool is_function, bool is_parameter) {
    if (!scope || !name) return;
    
    IdentScopeEntry* entry = calloc(1, sizeof(IdentScopeEntry));
    entry->name = strdup(name);
    entry->level = level;
    entry->is_function = is_function;
    entry->is_parameter = is_parameter;
    entry->next = scope->entries;
    scope->entries = entry;
}

IdentScopeEntry* ident_scope_lookup_local(IdentScope* scope, const char* name) {
    if (!scope || !name) return NULL;
    
    for (IdentScopeEntry* entry = scope->entries; entry; entry = entry->next) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
    }
    return NULL;
}

IdentScopeEntry* ident_scope_lookup(IdentScope* scope, const char* name) {
    if (!scope || !name) return NULL;
    
    // Check current scope
    IdentScopeEntry* entry = ident_scope_lookup_local(scope, name);
    if (entry) return entry;
    
    // Check parent scopes
    if (scope->parent) {
        return ident_scope_lookup(scope->parent, name);
    }
    
    return NULL;
}
