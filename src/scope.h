// Identifier scope tracking for proper module prefixing
#ifndef IDENT_SCOPE_H
#define IDENT_SCOPE_H

#include <stdbool.h>

typedef enum {
    IDENT_SCOPE_LOCAL,      // Function parameters and local variables
    IDENT_SCOPE_MODULE,     // Module-level variables and functions
    IDENT_SCOPE_GLOBAL      // Global scope
} IdentScopeLevel;

typedef struct IdentScopeEntry {
    char* name;
    IdentScopeLevel level;
    bool is_function;
    bool is_parameter;
    struct IdentScopeEntry* next;
} IdentScopeEntry;

typedef struct IdentScope {
    IdentScopeEntry* entries;
    struct IdentScope* parent;
    IdentScopeLevel level;
} IdentScope;

// Scope management
IdentScope* ident_scope_create(IdentScopeLevel level, IdentScope* parent);
void ident_scope_free(IdentScope* scope);
void ident_scope_add(IdentScope* scope, const char* name, IdentScopeLevel level, bool is_function, bool is_parameter);
IdentScopeEntry* ident_scope_lookup(IdentScope* scope, const char* name);
IdentScopeEntry* ident_scope_lookup_local(IdentScope* scope, const char* name);

#endif
