#include "optional.h"
#include "arc_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Generic optional constructors
WynOptional* wyn_some(void* value, size_t size) {
    WynOptional* opt = malloc(sizeof(WynOptional));
    if (!opt) {
        fprintf(stderr, "Error: Failed to allocate memory for optional\n");
        exit(1);
    }
    
    opt->has_value = true;
    opt->value_size = size;
    opt->value = malloc(size);
    if (!opt->value) {
        free(opt);
        fprintf(stderr, "Error: Failed to allocate memory for optional value\n");
        exit(1);
    }
    
    memcpy(opt->value, value, size);
    return opt;
}

WynOptional* wyn_none(void) {
    WynOptional* opt = malloc(sizeof(WynOptional));
    if (!opt) {
        fprintf(stderr, "Error: Failed to allocate memory for optional\n");
        exit(1);
    }
    
    opt->has_value = false;
    opt->value = NULL;
    opt->value_size = 0;
    return opt;
}

// Optional type operations
bool wyn_optional_is_some(WynOptional* opt) {
    return opt && opt->has_value;
}

bool wyn_optional_is_none(WynOptional* opt) {
    return !opt || !opt->has_value;
}

void* wyn_optional_unwrap(WynOptional* opt) {
    if (!opt || !opt->has_value) {
        fprintf(stderr, "Error: Attempted to unwrap None value\n");
        exit(1);
    }
    return opt->value;
}

void* wyn_optional_unwrap_or(WynOptional* opt, void* default_value) {
    if (!opt || !opt->has_value) {
        return default_value;
    }
    return opt->value;
}

// Pattern matching support
bool wyn_optional_match_some(WynOptional* opt, void** out_value) {
    if (opt && opt->has_value) {
        if (out_value) *out_value = opt->value;
        return true;
    }
    return false;
}

bool wyn_optional_match_none(WynOptional* opt) {
    return !opt || !opt->has_value;
}

// Memory management
void wyn_optional_free(WynOptional* opt) {
    if (opt) {
        if (opt->value) {
            free(opt->value);
        }
        free(opt);
    }
}

#define TYPE_GENERIC 999  // Temporary definition

// Type-specific constructors
WynOptional* some_int(int value) {
    return wyn_some(&value, sizeof(int));
}

WynOptional* some_float(float value) {
    return wyn_some(&value, sizeof(float));
}

WynOptional* some_string(const char* value) {
    if (!value) return wyn_none();
    
    WynOptional* opt = malloc(sizeof(WynOptional));
    opt->has_value = true;
    opt->value_size = strlen(value) + 1;
    opt->value = malloc(opt->value_size);
    strcpy((char*)opt->value, value);
    return opt;
}

WynOptional* some_bool(bool value) {
    return wyn_some(&value, sizeof(bool));
}

WynOptional* none_int(void) {
    return wyn_none();
}

WynOptional* none_float(void) {
    return wyn_none();
}

WynOptional* none_string(void) {
    return wyn_none();
}

WynOptional* none_bool(void) {
    return wyn_none();
}

// Type-specific unwrappers
int unwrap_int(WynOptional* opt) {
    void* value = wyn_optional_unwrap(opt);
    return *(int*)value;
}

float unwrap_float(WynOptional* opt) {
    void* value = wyn_optional_unwrap(opt);
    return *(float*)value;
}

const char* unwrap_string(WynOptional* opt) {
    void* value = wyn_optional_unwrap(opt);
    return (const char*)value;
}

bool unwrap_bool(WynOptional* opt) {
    void* value = wyn_optional_unwrap(opt);
    return *(bool*)value;
}

// Convenience functions for codegen
WynOptional* some(int value) {
    // Generic some() function for integers
    return some_int(value);
}

WynOptional* none(void) {
    return wyn_none();
}

// Task 3.5: Additional Option combinators
void* wyn_optional_expect(WynOptional* opt, const char* message) {
    if (!opt || !opt->has_value) {
        fprintf(stderr, "Error: %s\n", message ? message : "Expected Some value, got None");
        exit(1);
    }
    return opt->value;
}

WynOptional* wyn_optional_or_else(WynOptional* opt, WynOptional* (*fallback_fn)(void)) {
    if (opt && opt->has_value) {
        return opt;
    }
    return fallback_fn ? fallback_fn() : wyn_none();
}