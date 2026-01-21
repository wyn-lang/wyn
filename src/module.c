#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "module.h"

static char* module_paths[16];
static int module_path_count = 0;

// Circular import detection
static char* loading_stack[32];
static int loading_stack_count = 0;

void add_module_path(const char* path) {
    if (module_path_count < 16) {
        module_paths[module_path_count++] = strdup(path);
    }
}

static bool is_in_loading_stack(const char* module_name) {
    for (int i = 0; i < loading_stack_count; i++) {
        if (strcmp(loading_stack[i], module_name) == 0) {
            return true;
        }
    }
    return false;
}

static void push_loading_stack(const char* module_name) {
    if (loading_stack_count < 32) {
        loading_stack[loading_stack_count++] = strdup(module_name);
    }
}

static void pop_loading_stack() {
    if (loading_stack_count > 0) {
        free(loading_stack[--loading_stack_count]);
    }
}

static void print_circular_import_error(const char* module_name) {
    fprintf(stderr, "Error: Circular import detected: ");
    for (int i = 0; i < loading_stack_count; i++) {
        fprintf(stderr, "%s -> ", loading_stack[i]);
    }
    fprintf(stderr, "%s\n", module_name);
}

// Check if module is built-in (has C implementation)
bool is_builtin_module(const char* module_name) {
    const char* builtins[] = {"math", NULL};
    for (int i = 0; builtins[i] != NULL; i++) {
        if (strcmp(module_name, builtins[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Pre-scan source for imports and load them
void preload_imports(const char* source) {
    const char* p = source;
    while (*p) {
        // Look for "import " keyword
        if (strncmp(p, "import ", 7) == 0) {
            p += 7;
            // Skip whitespace
            while (*p == ' ' || *p == '\t') p++;
            
            // Extract module name
            const char* start = p;
            while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_') p++;
            
            if (p > start) {
                char module_name[256];
                int len = p - start;
                if (len < 256) {
                    memcpy(module_name, start, len);
                    module_name[len] = '\0';
                    
                    // Skip optional "as alias"
                    while (*p == ' ' || *p == '\t') p++;
                    if (strncmp(p, "as ", 3) == 0) {
                        p += 3;
                        // Skip alias name
                        while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_') p++;
                    }
                    
                    // Always try to load user modules first
                    // User modules override built-ins
                    load_module(module_name);
                }
            }
        }
        p++;
    }
}

static char source_directory[512] = ".";

void set_source_directory(const char* source_file) {
    // Extract directory from source file path
    const char* last_slash = strrchr(source_file, '/');
    if (last_slash) {
        int len = last_slash - source_file;
        if (len < 512) {
            memcpy(source_directory, source_file, len);
            source_directory[len] = '\0';
        }
    } else {
        strcpy(source_directory, ".");
    }
}

char* resolve_module_path(const char* module_name) {
    char path[512];
    struct stat st;
    
    // 1. Source file directory
    snprintf(path, sizeof(path), "%s/%s.wyn", source_directory, module_name);
    if (stat(path, &st) == 0) return strdup(path);
    
    // 2. Source file directory + modules/
    snprintf(path, sizeof(path), "%s/modules/%s.wyn", source_directory, module_name);
    if (stat(path, &st) == 0) return strdup(path);
    
    // 3. Current directory
    snprintf(path, sizeof(path), "%s.wyn", module_name);
    if (stat(path, &st) == 0) return strdup(path);
    
    // 4. ./modules/ directory
    snprintf(path, sizeof(path), "./modules/%s.wyn", module_name);
    if (stat(path, &st) == 0) return strdup(path);
    
    // 5. ./wyn_modules/ directory
    snprintf(path, sizeof(path), "./wyn_modules/%s.wyn", module_name);
    if (stat(path, &st) == 0) return strdup(path);
    
    // 6. User packages: ~/.wyn/packages/module_name/module_name.wyn
    const char* home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path), "%s/.wyn/packages/%s/%s.wyn", home, module_name, module_name);
        if (stat(path, &st) == 0) return strdup(path);
        
        // 7. User modules: ~/.wyn/modules/
        snprintf(path, sizeof(path), "%s/.wyn/modules/%s.wyn", home, module_name);
        if (stat(path, &st) == 0) return strdup(path);
    }
    
    // 8. System modules
    snprintf(path, sizeof(path), "/usr/local/lib/wyn/modules/%s.wyn", module_name);
    if (stat(path, &st) == 0) return strdup(path);
    
    // 9. Standard library (relative to compiler)
    snprintf(path, sizeof(path), "./stdlib/%s.wyn", module_name);
    if (stat(path, &st) == 0) return strdup(path);
    
    snprintf(path, sizeof(path), "../stdlib/%s.wyn", module_name);
    if (stat(path, &st) == 0) return strdup(path);
    
    // 10. Test directory (for development)
    snprintf(path, sizeof(path), "/tmp/wyn_modules/%s.wyn", module_name);
    if (stat(path, &st) == 0) return strdup(path);
    
    // 11. Custom module paths
    for (int i = 0; i < module_path_count; i++) {
        snprintf(path, sizeof(path), "%s/%s.wyn", module_paths[i], module_name);
        if (stat(path, &st) == 0) return strdup(path);
    }
    
    return NULL;
}

Program* load_module(const char* module_name) {
    // Check if already loaded
    extern bool is_module_loaded(const char* name);
    extern Program* get_module(const char* name);
    
    if (is_module_loaded(module_name)) {
        return get_module(module_name);
    }
    
    // Check for circular import
    if (is_in_loading_stack(module_name)) {
        print_circular_import_error(module_name);
        return NULL;
    }
    
    // Add to loading stack
    push_loading_stack(module_name);
    
    char* path = resolve_module_path(module_name);
    if (!path) {
        fprintf(stderr, "Error: Module '%s' not found\n", module_name);
        pop_loading_stack();
        return NULL;
    }
    
    // Read module file
    FILE* f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: Could not open module '%s'\n", path);
        free(path);
        pop_loading_stack();
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* source = malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);
    free(path);
    
    // Recursively preload any imports in this module
    preload_imports(source);
    
    // Save parser state
    extern void save_parser_state();
    extern void restore_parser_state();
    
    save_parser_state();
    
    // Parse module with fresh state
    extern void init_lexer(const char* source);
    extern void init_parser();
    extern Program* parse_program();
    
    init_lexer(source);
    init_parser();
    Program* prog = parse_program();
    
    // Restore original parser state
    restore_parser_state();
    
    // DON'T free source - the AST tokens point to it!
    // It will be freed when the program exits
    
    // Register module
    if (prog) {
        extern void register_module(const char* name, Program* ast);
        register_module(module_name, prog);
    }
    
    // Remove from loading stack
    pop_loading_stack();
    
    return prog;
}

// Type check all loaded modules (call after init_checker)
void check_all_modules(void) {
    extern void check_stmt(void* stmt, void* scope);
    extern SymbolTable* get_global_scope();
    extern int get_module_count();
    extern Program* get_module_at(int index);
    
    SymbolTable* scope = get_global_scope();
    if (!scope) return;
    
    int count = get_module_count();
    for (int i = 0; i < count; i++) {
        Program* prog = get_module_at(i);
        if (prog) {
            for (int j = 0; j < prog->count; j++) {
                check_stmt(prog->stmts[j], scope);
            }
        }
    }
}
