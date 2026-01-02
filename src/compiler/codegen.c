
// ============================================================================
// Variable Capture Analysis for spawn
// ============================================================================

static void find_used_vars_expr(Expr* e, char vars[][MAX_IDENT_LEN], int* count, int max) {
    if (!e || *count >= max) return;
    
    if (e->kind == EXPR_IDENT) {
        // Check if already in list
        for (int i = 0; i < *count; i++) {
            if (strcmp(vars[i], e->ident) == 0) return;
        }
        strcpy(vars[(*count)++], e->ident);
        return;
    }
    
    switch (e->kind) {
        case EXPR_BINARY:
            find_used_vars_expr(e->binary.left, vars, count, max);
            find_used_vars_expr(e->binary.right, vars, count, max);
            break;
        case EXPR_UNARY:
            find_used_vars_expr(e->unary.operand, vars, count, max);
            break;
        case EXPR_CALL:
            for (int i = 0; i < e->call.arg_count; i++)
                find_used_vars_expr(e->call.args[i], vars, count, max);
            break;
        case EXPR_INDEX:
            find_used_vars_expr(e->index.object, vars, count, max);
            find_used_vars_expr(e->index.index, vars, count, max);
            break;
        default: break;
    }
}

static void find_used_vars_stmt(Stmt* s, char vars[][MAX_IDENT_LEN], int* count, int max) {
    if (!s || *count >= max) return;
    
    switch (s->kind) {
        case STMT_LET:
            find_used_vars_expr(s->let.value, vars, count, max);
            break;
        case STMT_ASSIGN:
            find_used_vars_expr(s->assign.target, vars, count, max);
            find_used_vars_expr(s->assign.value, vars, count, max);
            break;
        case STMT_EXPR:
            find_used_vars_expr(s->expr.expr, vars, count, max);
            break;
        case STMT_RETURN:
            find_used_vars_expr(s->ret.value, vars, count, max);
            break;
        case STMT_WHILE:
            find_used_vars_expr(s->while_stmt.cond, vars, count, max);
            for (int i = 0; i < s->while_stmt.body_count; i++)
                find_used_vars_stmt(s->while_stmt.body[i], vars, count, max);
            break;
        default: break;
    }
}

static void analyze_spawn_stmt(Stmt* s) {
    if (!s) return;
    
    if (s->kind == STMT_SPAWN) {
        char used_vars[64][MAX_IDENT_LEN];
        int used_count = 0;
        
        for (int k = 0; k < s->spawn.body_count; k++) {
            find_used_vars_stmt(s->spawn.body[k], used_vars, &used_count, 64);
        }
        
        if (used_count > 0) {
            s->spawn.captured_vars = malloc(sizeof(char*) * used_count);
            s->spawn.captured_types = malloc(sizeof(Type*) * used_count);
            s->spawn.captured_count = used_count;
            for (int k = 0; k < used_count; k++) {
                s->spawn.captured_vars[k] = strdup(used_vars[k]);
                s->spawn.captured_types[k] = NULL;  // Will be filled in during codegen
            }
        }
        return;
    }
    
    // Recursively search in nested statements
    switch (s->kind) {
        case STMT_FOR:
            for (int i = 0; i < s->for_stmt.body_count; i++)
                analyze_spawn_stmt(s->for_stmt.body[i]);
            break;
        case STMT_WHILE:
            for (int i = 0; i < s->while_stmt.body_count; i++)
                analyze_spawn_stmt(s->while_stmt.body[i]);
            break;
        case STMT_IF:
            for (int i = 0; i < s->if_stmt.then_count; i++)
                analyze_spawn_stmt(s->if_stmt.then_block[i]);
            for (int i = 0; i < s->if_stmt.else_count; i++)
                analyze_spawn_stmt(s->if_stmt.else_block[i]);
            break;
        default: break;
    }
}

static void analyze_spawn_captures(Module* m) {
    for (int i = 0; i < m->function_count; i++) {
        FnDef* fn = &m->functions[i];
        for (int j = 0; j < fn->body_count; j++) {
            analyze_spawn_stmt(fn->body[j]);
        }
    }
}


// ============================================================================
// Removed 6,127 lines of x86/ARM codegen (lines 4946-11074)
// ============================================================================

typedef enum { ARCH_ARM64, ARCH_X86_64 } Arch;
typedef enum { OS_MACOS, OS_LINUX } TargetOS;

void print_usage(const char* prog) {
    printf("Wyn Bootstrap Compiler (Stage 0) v0.1.0\n");
    printf("Usage: %s [options] <input.wyn>\n", prog);
    printf("\nOptions:\n");
    printf("  -o <file>       Output file (default: a.out)\n");
    printf("  -c              Compile only, don't link\n");
    printf("  -S              Output assembly\n");
    printf("  --target <arch> Target architecture (arm64, x86_64)\n");
    printf("  --target-os <os> Target OS (macos, linux)\n");
    printf("  --emit-ir       Output intermediate representation\n");
    printf("  -O0             No optimization (default)\n");
    printf("  -O1             Basic optimization\n");
    printf("  -O2             Full optimization\n");
    printf("  --stage1-tc     Use Stage 1 type checker (enhanced)\n");
    printf("  --stage1-opt    Enable Stage 1 optimizations\n");
    printf("  --version       Show version\n");
    printf("  --help          Show this help\n");
}

char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fatal("cannot open file: %s", path);
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* buf = malloc(size + 1);
    if (!buf) {
        fatal("out of memory");
    }
    
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    
    // Skip shebang line if present (#!/usr/bin/env wyn run)
    if (size >= 2 && buf[0] == '#' && buf[1] == '!') {
        char* newline = strchr(buf, '\n');
        if (newline) {
            // Return pointer after the shebang line
            char* content = strdup(newline + 1);
            free(buf);
            return content;
        }
    }
    
    return buf;
}

// Resolve imports by loading and merging imported modules
static void resolve_imports(Module* m, const char* base_path) {
    char dir[256] = "";
    char root[256] = "";
    // Extract directory from base_path
    const char* last_slash = strrchr(base_path, '/');
    if (last_slash) {
        int len = last_slash - base_path + 1;
        strncpy(dir, base_path, len);
        dir[len] = '\0';
        // Try to find project root (go up until we find std/)
        strcpy(root, dir);
    }
    
    for (int i = 0; i < m->import_count; i++) {
        char full_path[512];
        Import* imp = &m->imports[i];
        FILE* f = NULL;
        
        // Convert dots to slashes for file path
        char file_path[MAX_STRING_LEN];
        strcpy(file_path, imp->path);
        for (int k = 0; file_path[k]; k++) {
            if (file_path[k] == '.') file_path[k] = '/';
        }
        
        // Try relative path first
        if (imp->path[0] == '.' && imp->path[1] == '/') {
            snprintf(full_path, 512, "%s%s.wyn", dir, imp->path + 2);
            f = fopen(full_path, "rb");
        }
        
        // Try std/ directory for stdlib modules
        if (!f) {
            snprintf(full_path, 512, "std/%s.wyn", file_path);
            f = fopen(full_path, "rb");
        }
        
        // Try ~/.wyn/packages/ for installed packages
        if (!f) {
            const char* home = getenv("HOME");
            if (home) {
                snprintf(full_path, 512, "%s/.wyn/packages/%s/src/%s.wyn", home, file_path, file_path);
                f = fopen(full_path, "rb");
                
                // Also try without /src/
                if (!f) {
                    snprintf(full_path, 512, "%s/.wyn/packages/%s/%s.wyn", home, file_path, file_path);
                    f = fopen(full_path, "rb");
                }
            }
        }
        
        // Try root-level module
        if (!f && dir[0]) {
            char try_path[512];
            strcpy(try_path, dir);
            for (int depth = 0; depth < 5 && !f; depth++) {
                snprintf(full_path, 512, "%s%s.wyn", try_path, file_path);
                f = fopen(full_path, "rb");
                if (!f) {
                    snprintf(full_path, 512, "%sstd/%s.wyn", try_path, file_path);
                    f = fopen(full_path, "rb");
                }
                if (!f) strcat(try_path, "../");
            }
        }
        
        // Try current directory if no dir specified
        if (!f && !dir[0]) {
            snprintf(full_path, 512, "%s.wyn", file_path);
            f = fopen(full_path, "rb");
        }
        
        // Try std/ in current dir (legacy)
        if (!f) {
            snprintf(full_path, 512, "%sstd/%s.wyn", dir, file_path);
            f = fopen(full_path, "rb");
        }
        
        // Try std/ in parent dir (legacy)
        if (!f) {
            snprintf(full_path, 512, "%s../std/%s.wyn", dir, file_path);
            f = fopen(full_path, "rb");
        }
        
        // Try absolute std path (legacy - for project root)
        if (!f && dir[0]) {
            char try_path[512];
            strcpy(try_path, dir);
            for (int depth = 0; depth < 5 && !f; depth++) {
                snprintf(full_path, 512, "%sstd/%s.wyn", try_path, file_path);
                f = fopen(full_path, "rb");
                if (!f) strcat(try_path, "../");
            }
        }
        
        if (!f) continue;
        
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* src = malloc(size + 1);
        fread(src, 1, size, f);
        src[size] = '\0';
        fclose(f);
        
        Lexer* lex = lexer_new(src, full_path);
        Parser* p = parser_new(lex, full_path);
        Module* imported = parse_module(p);
        
        if (!p->had_error) {
            // Merge functions with module prefix
            for (int j = 0; j < imported->function_count; j++) {
                FnDef prefixed_fn = imported->functions[j];
                // Create module.function name
                char prefixed_name[MAX_IDENT_LEN];
                snprintf(prefixed_name, MAX_IDENT_LEN, "%s.%s", imp->path, imported->functions[j].name);
                strcpy(prefixed_fn.name, prefixed_name);
                m->functions[m->function_count++] = prefixed_fn;
            }
            // Merge structs
            for (int j = 0; j < imported->struct_count; j++) {
                m->structs[m->struct_count++] = imported->structs[j];
            }
        }
    }
    
    // Auto-import core modules if not already imported
    const char* core_modules[] = {"array", "fs", "os", "string", "path"};
    int core_count = 5;
    
    for (int c = 0; c < core_count; c++) {
        const char* core_module = core_modules[c];
        
        // Check if already imported
        bool already_imported = false;
        for (int i = 0; i < m->import_count; i++) {
            if (strcmp(m->imports[i].path, core_module) == 0) {
                already_imported = true;
                break;
            }
        }
        
        if (!already_imported) {
            // Auto-import this core module
            char full_path[512];
            snprintf(full_path, 512, "std/%s.wyn", core_module);
            FILE* f = fopen(full_path, "rb");
            
            if (f) {
                fseek(f, 0, SEEK_END);
                long size = ftell(f);
                fseek(f, 0, SEEK_SET);
                char* src = malloc(size + 1);
                fread(src, 1, size, f);
                src[size] = '\0';
                fclose(f);
                
                Lexer* lex = lexer_new(src, full_path);
                Parser* p = parser_new(lex, full_path);
                Module* imported = parse_module(p);
                
                if (!p->had_error) {
                    // Merge functions
                    for (int j = 0; j < imported->function_count; j++) {
                        m->functions[m->function_count++] = imported->functions[j];
                    }
                    // Merge structs
                    for (int j = 0; j < imported->struct_count; j++) {
                        m->structs[m->struct_count++] = imported->structs[j];
                    }
                    
                    // Add to imports list for tracking
                    strcpy(m->imports[m->import_count].path, core_module);
                    m->imports[m->import_count].alias[0] = '\0';
                    m->import_count++;
                }
            }
        }
    }
}

// Complete LLVM Backend for Wyn Compiler
// Insert this before main() in stage0.c

// String table for deduplication
typedef struct {
    char strings[256][MAX_STRING_LEN];
    int count;
} StringTable;

static int string_table_add(StringTable* st, const char* str) {
    for (int i = 0; i < st->count; i++) {
        if (strcmp(st->strings[i], str) == 0) return i;
    }
    strcpy(st->strings[st->count], str);
    return st->count++;
}

static int string_table_find(StringTable* st, const char* str) {
    for (int i = 0; i < st->count; i++) {
        if (strcmp(st->strings[i], str) == 0) return i;
    }
    return -1;
}

// Collect all strings from AST
static void collect_strings_expr(StringTable* st, Expr* e);
static void collect_strings_stmt(StringTable* st, Stmt* s);

static void collect_strings_expr(StringTable* st, Expr* e) {
    if (!e) return;
    switch (e->kind) {
        case EXPR_STRING:
            string_table_add(st, e->str_val);
            break;
        case EXPR_BINARY:
            collect_strings_expr(st, e->binary.left);
            collect_strings_expr(st, e->binary.right);
            break;
        case EXPR_UNARY:
            collect_strings_expr(st, e->unary.operand);
            break;
        case EXPR_CALL:
            collect_strings_expr(st, e->call.func);
            for (int i = 0; i < e->call.arg_count; i++) {
                collect_strings_expr(st, e->call.args[i]);
            }
            break;
        case EXPR_IF:
            collect_strings_expr(st, e->if_expr.cond);
            collect_strings_expr(st, e->if_expr.then_expr);
            collect_strings_expr(st, e->if_expr.else_expr);
            break;
        case EXPR_TERNARY:
            collect_strings_expr(st, e->ternary.cond);
            collect_strings_expr(st, e->ternary.true_val);
            collect_strings_expr(st, e->ternary.false_val);
            break;
        case EXPR_INDEX:
            collect_strings_expr(st, e->index.object);
            collect_strings_expr(st, e->index.index);
            break;
        case EXPR_FIELD:
            collect_strings_expr(st, e->field.object);
            break;
        case EXPR_ARRAY:
            for (int i = 0; i < e->array.count; i++) {
                collect_strings_expr(st, e->array.elements[i]);
            }
            break;
        case EXPR_OK:
        case EXPR_ERR:
        case EXPR_SOME:
            if (e->some.value) {
                collect_strings_expr(st, e->some.value);
            }
            break;
        case EXPR_NONE:
            // No strings to collect
            break;
        default:
            break;
    }
}

static void collect_strings_stmt(StringTable* st, Stmt* s) {
    if (!s) return;
    switch (s->kind) {
        case STMT_LET:
            if (s->let.value) collect_strings_expr(st, s->let.value);
            break;
        case STMT_ASSIGN:
            collect_strings_expr(st, s->assign.target);
            collect_strings_expr(st, s->assign.value);
            break;
        case STMT_EXPR:
            collect_strings_expr(st, s->expr.expr);
            break;
        case STMT_RETURN:
            if (s->ret.value) collect_strings_expr(st, s->ret.value);
            break;
        case STMT_IF:
            collect_strings_expr(st, s->if_stmt.cond);
            for (int i = 0; i < s->if_stmt.then_count; i++) {
                collect_strings_stmt(st, s->if_stmt.then_block[i]);
            }
            for (int i = 0; i < s->if_stmt.else_count; i++) {
                collect_strings_stmt(st, s->if_stmt.else_block[i]);
            }
            break;
        case STMT_WHILE:
            collect_strings_expr(st, s->while_stmt.cond);
            for (int i = 0; i < s->while_stmt.body_count; i++) {
                collect_strings_stmt(st, s->while_stmt.body[i]);
            }
            break;
        case STMT_FOR:
            collect_strings_expr(st, s->for_stmt.iter);
            for (int i = 0; i < s->for_stmt.body_count; i++) {
                collect_strings_stmt(st, s->for_stmt.body[i]);
            }
            break;
        case STMT_BLOCK:
            for (int i = 0; i < s->block.count; i++) {
                collect_strings_stmt(st, s->block.stmts[i]);
            }
            break;
        case STMT_SPAWN:
            for (int i = 0; i < s->spawn.body_count; i++) {
                collect_strings_stmt(st, s->spawn.body[i]);
            }
            break;
        case STMT_MATCH:
            collect_strings_expr(st, s->match_stmt.value);
            for (int i = 0; i < s->match_stmt.arm_count; i++) {
                collect_strings_expr(st, s->match_stmt.patterns[i]);
                for (int j = 0; j < s->match_stmt.arm_counts[i]; j++) {
                    collect_strings_stmt(st, s->match_stmt.arms[i][j]);
                }
            }
            break;
        default:
            break;
    }
}

// LLVM IR Generator
typedef struct {
    FILE* out;
    Module* module;
    StringTable* strings;
    int label_count;
    int temp_count;
    int loop_start_label;
    int loop_end_label;
    char vars[256][MAX_IDENT_LEN];
    int var_regs[256];
    Type* var_types[256];  // Track variable types
    int var_count;
    Stmt* spawn_blocks[256];  // Track spawn blocks to emit later
    int spawn_count;
    Expr* lambdas[256];  // Track lambda expressions to emit later
    int lambda_count;
    FnDef* current_function;  // Track current function being compiled
    // Generic instantiation tracking
    struct {
        FnDef* generic_fn;
        Type* concrete_type;
        char instantiated_name[MAX_IDENT_LEN];
    } generic_instances[256];
    int generic_instance_count;
} LLVMGen;

static void llvm_emit(LLVMGen* lg, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(lg->out, fmt, args);
    va_end(args);
    fprintf(lg->out, "\n");
}

static void llvm_emit_raw(LLVMGen* lg, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(lg->out, fmt, args);
    va_end(args);
}

static int llvm_new_temp(LLVMGen* lg) { return lg->temp_count++; }
static int llvm_new_label(LLVMGen* lg) { return lg->label_count++; }

static int llvm_find_var(LLVMGen* lg, const char* name) {
    for (int i = lg->var_count - 1; i >= 0; i--) {
        if (strcmp(lg->vars[i], name) == 0) return lg->var_regs[i];
    }
    return -1;
}

static int llvm_find_enum_variant(LLVMGen* lg, const char* name) {
    for (int i = 0; i < lg->module->enum_count; i++) {
        for (int j = 0; j < lg->module->enums[i].variant_count; j++) {
            if (strcmp(lg->module->enums[i].variants[j].name, name) == 0) {
                // If variant has explicit value, use it; otherwise use index
                return lg->module->enums[i].variants[j].has_value ? 
                       lg->module->enums[i].variants[j].value : j;
            }
        }
    }
    return -1; // Not found
}

static void llvm_add_var(LLVMGen* lg, const char* name, int reg, Type* type) {
    strcpy(lg->vars[lg->var_count], name);
    lg->var_regs[lg->var_count] = reg;
    lg->var_types[lg->var_count] = type;
    lg->var_count++;
}

static Type* llvm_get_var_type(LLVMGen* lg, const char* name) {
    if (!lg || !name || name[0] == '\0') return NULL;
    for (int i = lg->var_count - 1; i >= 0; i--) {
        if (lg->vars[i] && strcmp(lg->vars[i], name) == 0) return lg->var_types[i];
    }
    return NULL;
}

// Helper to detect if expression should use float operations
static bool llvm_is_float_expr(LLVMGen* lg, Expr* e) {
    if (!e) return false;
    switch (e->kind) {
        case EXPR_FLOAT: return true;
        case EXPR_INT: return false;
        case EXPR_IDENT: {
            // Check if variable is float type
            Type* t = llvm_get_var_type(lg, e->ident);
            if (t && t->kind == TYPE_FLOAT) return true;
            // Conservative heuristic for untyped variables
            if (!e->ident || e->ident[0] == '\0') return false;
            return strstr(e->ident, "float") != NULL || 
                   (strlen(e->ident) > 1 && e->ident[strlen(e->ident)-1] == 'f' && strcmp(e->ident, "self") != 0);
        }
        case EXPR_BINARY:
            return llvm_is_float_expr(lg, e->binary.left) || llvm_is_float_expr(lg, e->binary.right);
        default: return false;
    }
}

static bool llvm_is_string_expr(LLVMGen* lg, Expr* e) {
    if (!e) return false;
    if (e->kind == EXPR_STRING) return true;
    if (e->kind == EXPR_IDENT) {
        Type* t = llvm_get_var_type(lg, e->ident);
        return t && t->kind == TYPE_STR;
    }
    if (e->kind == EXPR_INDEX) {
        // Check if this is indexing into a string (returns single character string)
        if (e->index.object->kind == EXPR_STRING) return true;
        if (e->index.object->kind == EXPR_IDENT) {
            Type* obj_type = llvm_get_var_type(lg, e->index.object->ident);
            // String indexing returns a string (single character)
            if (obj_type && obj_type->kind == TYPE_STR) return true;
            // String array indexing returns a string
            return obj_type && obj_type->kind == TYPE_ARRAY && obj_type->inner && obj_type->inner->kind == TYPE_STR;
        }
        return false;
    }
    if (e->kind == EXPR_BINARY && e->binary.op == TOK_PLUS) {
        return llvm_is_string_expr(lg, e->binary.left) || llvm_is_string_expr(lg, e->binary.right);
    }
    if (e->kind == EXPR_SLICE) {
        // String slicing returns a string
        if (e->slice.object->kind == EXPR_STRING) return true;
        if (e->slice.object->kind == EXPR_IDENT) {
            Type* t = llvm_get_var_type(lg, e->slice.object->ident);
            return t && t->kind == TYPE_STR;
        }
        return false;
    }
    if (e->kind == EXPR_TERNARY) {
        // Ternary is string if either branch is string
        return llvm_is_string_expr(lg, e->ternary.true_val) || llvm_is_string_expr(lg, e->ternary.false_val);
    }
    if (e->kind == EXPR_CALL) {
        if (e->call.func->kind == EXPR_IDENT) {
            const char* func = e->call.func->ident;
            if (strcmp(func, "chr") == 0 || strcmp(func, "int_to_str") == 0 ||
                strcmp(func, "str_concat") == 0 || strcmp(func, "substring") == 0) {
                return true;
            }
            
            // Check user-defined functions
            for (int i = 0; i < lg->module->function_count; i++) {
                FnDef* fn = &lg->module->functions[i];
                if (strcmp(fn->name, func) == 0) {
                    return fn->return_type && fn->return_type->kind == TYPE_STR;
                }
            }
        } else if (e->call.func->kind == EXPR_FIELD) {
            const char* func = e->call.func->field.field;
            
            // Check if this is a module.function() call that returns a string
            if (e->call.func->field.object->kind == EXPR_IDENT) {
                const char* module = e->call.func->field.object->ident;
                const char* builtin_name = map_module_function(module, func);
                if (builtin_name) {
                    // Check if this module function returns a string
                    Type* ret_type = get_builtin_return_type(builtin_name);
                    if (ret_type && ret_type->kind == TYPE_STR) {
                        return true;
                    }
                }
            }
            
            // Check if this is a string-returning method (regardless of object)
            if (strcmp(func, "upper") == 0 || strcmp(func, "lower") == 0 ||
                strcmp(func, "trim") == 0 || strcmp(func, "read") == 0 ||
                strcmp(func, "sqlite_query") == 0 || strcmp(func, "exec_output") == 0 ||
                strcmp(func, "http_get") == 0 || strcmp(func, "server_read_request") == 0 ||
                strcmp(func, "parse_method") == 0 || strcmp(func, "parse_path") == 0 ||
                strcmp(func, "parse_body") == 0 || strcmp(func, "output") == 0 ||
                strcmp(func, "replace") == 0 || strcmp(func, "to_str") == 0 ||
                strcmp(func, "join") == 0 || strcmp(func, "get_str") == 0 ||
                strcmp(func, "substring") == 0 || strcmp(func, "char_at") == 0) {
                return true;
            }
            
            // Check for extension methods that return strings
            if (lg && lg->module && lg->module->function_count > 0) {
                for (int i = 0; i < lg->module->function_count; i++) {
                    FnDef* fn = &lg->module->functions[i];
                    // Skip the current function to avoid recursion
                    if (fn && fn->is_extension && fn->extension_method && fn->extension_method[0] != '\0' && 
                        strcmp(fn->extension_method, func) == 0 &&
                        fn != lg->current_function) {
                        return fn->return_type && fn->return_type->kind == TYPE_STR;
                    }
                }
            }
        }
    }
    return false;
}

static bool llvm_is_array_expr(LLVMGen* lg, Expr* e) {
    if (!e) return false;
    if (e->kind == EXPR_ARRAY) return true;
    if (e->kind == EXPR_COMPREHENSION) return true;  // List comprehensions return arrays
    if (e->kind == EXPR_IDENT) {
        Type* t = llvm_get_var_type(lg, e->ident);
        return t && t->kind == TYPE_ARRAY;
    }
    if (e->kind == EXPR_SLICE) {
        // Array slicing returns an array
        if (e->slice.object->kind == EXPR_ARRAY) return true;
        if (e->slice.object->kind == EXPR_IDENT) {
            Type* t = llvm_get_var_type(lg, e->slice.object->ident);
            return t && t->kind == TYPE_ARRAY;
        }
        return false;
    }
    if (e->kind == EXPR_CALL) {
        if (e->call.func->kind == EXPR_IDENT) {
            const char* func = e->call.func->ident;
            if (strcmp(func, "str_split") == 0) return true;
            
            // Check if this is a user-defined function that returns an array
            for (int i = 0; i < lg->module->function_count; i++) {
                if (strcmp(lg->module->functions[i].name, func) == 0) {
                    FnDef* fn_def = &lg->module->functions[i];
                    if (fn_def->return_type && fn_def->return_type->kind == TYPE_ARRAY) {
                        return true;
                    }
                    break;
                }
            }
        } else if (e->call.func->kind == EXPR_FIELD) {
            const char* module = e->call.func->field.object->ident;
            const char* func = e->call.func->field.field;
            // Check result module functions (return arrays)
            if (strcmp(module, "result") == 0) {
                return (strcmp(func, "make_ok") == 0 || strcmp(func, "make_err") == 0 ||
                       strcmp(func, "make_some") == 0 || strcmp(func, "make_none") == 0);
            }
            // Check array module functions
            if (strcmp(module, "array") == 0) {
                return (strcmp(func, "reverse") == 0 || strcmp(func, "append") == 0 ||
                       strcmp(func, "prepend") == 0 || strcmp(func, "filter") == 0 ||
                       strcmp(func, "map") == 0 || strcmp(func, "slice") == 0);
            }
            // Check other array-returning functions
            return (strcmp(func, "glob") == 0 || strcmp(func, "split") == 0 ||
                   strcmp(func, "list_files") == 0 || strcmp(func, "list_dir") == 0);
        }
    }
    return false;
}

// Helper function to find struct definition
static StructDef* llvm_lookup_struct(LLVMGen* lg, const char* name) {
    for (int i = 0; i < lg->module->struct_count; i++) {
        if (strcmp(lg->module->structs[i].name, name) == 0) {
            return &lg->module->structs[i];
        }
    }
    return NULL;
}

// Helper function to get field offset in struct
static int llvm_struct_field_offset(StructDef* s, const char* field) {
    for (int i = 0; i < s->field_count; i++) {
        if (strcmp(s->fields[i].name, field) == 0) {
            return i;
        }
    }
    return -1;
}

static void llvm_expr(LLVMGen* lg, Expr* e, int* result_reg);
static void llvm_stmt(LLVMGen* lg, Stmt* s);

static void llvm_expr(LLVMGen* lg, Expr* e, int* result_reg) {
    switch (e->kind) {
        case EXPR_INT:
            *result_reg = -e->int_val - 1000000;  // Encode constants
            break;
            
        case EXPR_BOOL:
            *result_reg = -(e->bool_val ? 1 : 0) - 1000000;
            break;
            
        case EXPR_FLOAT: {
            // Generate proper double constant
            int t = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = fadd double 0.0, %f", t, e->float_val);
            *result_reg = t;
            break;
        }
            
        case EXPR_STRING: {
            int str_id = string_table_find(lg->strings, e->str_val);
            int t = llvm_new_temp(lg);
            int len = strlen(e->str_val);
            llvm_emit(lg, "  %%%d = getelementptr [%d x i8], [%d x i8]* @.str.%d, i32 0, i32 0",
                     t, len + 1, len + 1, str_id);
            *result_reg = t;
            break;
        }
        
        case EXPR_IDENT: {
            int var_reg = llvm_find_var(lg, e->ident);
            if (var_reg >= 0) {
                Type* var_type = llvm_get_var_type(lg, e->ident);
                
                // Check if this is a struct variable
                bool is_struct_var = var_type && var_type->kind == TYPE_NAMED;
                
                if (is_struct_var) {
                    // For struct variables, return the pointer directly (no load)
                    *result_reg = var_reg;
                } else {
                    int t = llvm_new_temp(lg);
                    
                    if (var_type) {
                        if (var_type->kind == TYPE_FLOAT) {
                            llvm_emit(lg, "  %%%d = load double, double* %%%d", t, var_reg);
                        } else if (var_type->kind == TYPE_ARRAY) {
                            llvm_emit(lg, "  %%%d = load i64*, i64** %%%d", t, var_reg);
                        } else if (var_type->kind == TYPE_STR) {
                            llvm_emit(lg, "  %%%d = load i8*, i8** %%%d", t, var_reg);
                        } else {
                            llvm_emit(lg, "  %%%d = load i64, i64* %%%d", t, var_reg);
                        }
                    } else {
                        // Fallback to i64 if type unknown
                        llvm_emit(lg, "  %%%d = load i64, i64* %%%d", t, var_reg);
                    }
                    *result_reg = t;
                }
            } else {
                // Check if it's an enum variant
                int enum_value = llvm_find_enum_variant(lg, e->ident);
                if (enum_value >= 0) {
                    // Encode enum value as constant
                    *result_reg = -enum_value - 1000000;
                } else {
                    // Variable not found - generate zero register
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = add i64 0, 0", t);
                    *result_reg = t;
                }
            }
            break;
        }
        
        case EXPR_BINARY: {
            int left_reg, right_reg;
            llvm_expr(lg, e->binary.left, &left_reg);
            llvm_expr(lg, e->binary.right, &right_reg);
            
            // Check if this should be float arithmetic
            bool is_float_op = false;  // Temporarily disabled to debug crash
            // bool is_float_op = llvm_is_float_expr(lg, e->binary.left) || llvm_is_float_expr(lg, e->binary.right);
            
            // Handle constants
            char left_str[64], right_str[64];
            if (left_reg <= -1000000) {
                if (is_float_op) {
                    // Convert integer constant to float
                    int conv_reg = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = sitofp i64 %lld to double", conv_reg, (long long)(-left_reg - 1000000));
                    snprintf(left_str, 64, "%%%d", conv_reg);
                } else {
                    snprintf(left_str, 64, "%lld", (long long)(-left_reg - 1000000));
                }
            } else {
                snprintf(left_str, 64, "%%%d", left_reg);
            }
            if (right_reg <= -1000000) {
                if (is_float_op) {
                    // Convert integer constant to float
                    int conv_reg = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = sitofp i64 %lld to double", conv_reg, (long long)(-right_reg - 1000000));
                    snprintf(right_str, 64, "%%%d", conv_reg);
                } else {
                    snprintf(right_str, 64, "%lld", (long long)(-right_reg - 1000000));
                }
            } else {
                snprintf(right_str, 64, "%%%d", right_reg);
            }
            
            int t;
            if (is_float_op) {
                t = llvm_new_temp(lg);
                // Float arithmetic
                switch (e->binary.op) {
                    case TOK_PLUS:
                        llvm_emit(lg, "  %%%d = fadd double %s, %s", t, left_str, right_str);
                        break;
                    case TOK_MINUS:
                        llvm_emit(lg, "  %%%d = fsub double %s, %s", t, left_str, right_str);
                        break;
                    case TOK_STAR:
                        llvm_emit(lg, "  %%%d = fmul double %s, %s", t, left_str, right_str);
                        break;
                    case TOK_SLASH:
                        llvm_emit(lg, "  %%%d = fdiv double %s, %s", t, left_str, right_str);
                        break;
                    case TOK_LT:
                        llvm_emit(lg, "  %%%d = fcmp olt double %s, %s", t, left_str, right_str);
                        break;
                    case TOK_GT:
                        llvm_emit(lg, "  %%%d = fcmp ogt double %s, %s", t, left_str, right_str);
                        break;
                    case TOK_LTEQ:
                        llvm_emit(lg, "  %%%d = fcmp ole double %s, %s", t, left_str, right_str);
                        break;
                    case TOK_GTEQ:
                        llvm_emit(lg, "  %%%d = fcmp oge double %s, %s", t, left_str, right_str);
                        break;
                    case TOK_EQEQ:
                        llvm_emit(lg, "  %%%d = fcmp oeq double %s, %s", t, left_str, right_str);
                        break;
                    case TOK_NOTEQ:
                        llvm_emit(lg, "  %%%d = fcmp one double %s, %s", t, left_str, right_str);
                        break;
                    default:
                        // Unsupported float operation - generate zero register
                        t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = fadd double 0.0, 0.0", t);
                        *result_reg = t;
                        return;
                }
            } else {
                // Integer arithmetic
                // For most operations, allocate t first
                // For TOK_STARSTAR, t is allocated at the end
                if (e->binary.op != TOK_STARSTAR) {
                    t = llvm_new_temp(lg);
                }
                
                switch (e->binary.op) {
                    case TOK_PLUS: {
                        // Check for string concatenation
                        bool left_is_str = (e->binary.left->kind == EXPR_STRING);
                        bool right_is_str = (e->binary.right->kind == EXPR_STRING);
                        
                        // Check variable types
                        if (e->binary.left->kind == EXPR_IDENT) {
                            Type* t = llvm_get_var_type(lg, e->binary.left->ident);
                            if (t && t->kind == TYPE_STR) left_is_str = true;
                        }
                        if (e->binary.right->kind == EXPR_IDENT) {
                            Type* t = llvm_get_var_type(lg, e->binary.right->ident);
                            if (t && t->kind == TYPE_STR) right_is_str = true;
                        }
                        
                        if (left_is_str || right_is_str) {
                            // String concatenation
                            t = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = call i8* @str_concat(i8* %s, i8* %s)", t, left_str, right_str);
                        } else {
                            // Integer addition
                            llvm_emit(lg, "  %%%d = add i64 %s, %s", t, left_str, right_str);
                        }
                        break;
                    }
                    case TOK_MINUS:
                        llvm_emit(lg, "  %%%d = sub i64 %s, %s", t, left_str, right_str);
                        break;
                    case TOK_STAR:
                        llvm_emit(lg, "  %%%d = mul i64 %s, %s", t, left_str, right_str);
                        break;
                    case TOK_SLASH:
                        llvm_emit(lg, "  %%%d = sdiv i64 %s, %s", t, left_str, right_str);
                        break;
                    case TOK_PERCENT:
                        llvm_emit(lg, "  %%%d = srem i64 %s, %s", t, left_str, right_str);
                        break;
                    case TOK_LT:
                        llvm_emit(lg, "  %%%d = icmp slt i64 %s, %s", t, left_str, right_str);
                        break;
                    case TOK_GT:
                        llvm_emit(lg, "  %%%d = icmp sgt i64 %s, %s", t, left_str, right_str);
                        break;
                    case TOK_LTEQ:
                        llvm_emit(lg, "  %%%d = icmp sle i64 %s, %s", t, left_str, right_str);
                        break;
                    case TOK_GTEQ:
                        llvm_emit(lg, "  %%%d = icmp sge i64 %s, %s", t, left_str, right_str);
                        break;
                    case TOK_EQEQ: {
                        // Check if comparing strings
                        bool left_is_str = (e->binary.left->kind == EXPR_STRING) ||
                                          (e->binary.left->kind == EXPR_IDENT && 
                                           llvm_get_var_type(lg, e->binary.left->ident) &&
                                           llvm_get_var_type(lg, e->binary.left->ident)->kind == TYPE_STR);
                        bool right_is_str = (e->binary.right->kind == EXPR_STRING) ||
                                           (e->binary.right->kind == EXPR_IDENT && 
                                            llvm_get_var_type(lg, e->binary.right->ident) &&
                                            llvm_get_var_type(lg, e->binary.right->ident)->kind == TYPE_STR);
                        
                        if (left_is_str || right_is_str) {
                            // String comparison - call str_cmp and check if result == 0
                            int cmp_reg = llvm_new_temp(lg);
                            t = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = call i64 @str_cmp(i8* %s, i8* %s)", cmp_reg, left_str, right_str);
                            llvm_emit(lg, "  %%%d = icmp eq i64 %%%d, 0", t, cmp_reg);
                        } else {
                            llvm_emit(lg, "  %%%d = icmp eq i64 %s, %s", t, left_str, right_str);
                        }
                        break;
                    }
                    case TOK_NOTEQ: {
                        // Check if comparing strings
                        bool left_is_str = (e->binary.left->kind == EXPR_STRING) ||
                                          (e->binary.left->kind == EXPR_IDENT && 
                                           llvm_get_var_type(lg, e->binary.left->ident) &&
                                           llvm_get_var_type(lg, e->binary.left->ident)->kind == TYPE_STR);
                        bool right_is_str = (e->binary.right->kind == EXPR_STRING) ||
                                           (e->binary.right->kind == EXPR_IDENT && 
                                            llvm_get_var_type(lg, e->binary.right->ident) &&
                                            llvm_get_var_type(lg, e->binary.right->ident)->kind == TYPE_STR);
                        
                        if (left_is_str || right_is_str) {
                            // String comparison - call str_cmp and check if result != 0
                            int cmp_reg = llvm_new_temp(lg);
                            t = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = call i64 @str_cmp(i8* %s, i8* %s)", cmp_reg, left_str, right_str);
                            llvm_emit(lg, "  %%%d = icmp ne i64 %%%d, 0", t, cmp_reg);
                        } else {
                            llvm_emit(lg, "  %%%d = icmp ne i64 %s, %s", t, left_str, right_str);
                        }
                        break;
                    }
                    case TOK_IN: {
                        // Array membership: element in array
                        // Call array_contains builtin
                        t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = call i64 @array_contains(i64* %s, i64 %s)", t, right_str, left_str);
                        int bool_result = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = icmp ne i64 %%%d, 0", bool_result, t);
                        *result_reg = bool_result;
                        return;
                    }
                    case TOK_PIPE:
                        llvm_emit(lg, "  %%%d = or i64 %s, %s", t, left_str, right_str);
                        break;
                    case TOK_AMPERSAND:
                        llvm_emit(lg, "  %%%d = and i64 %s, %s", t, left_str, right_str);
                        break;
                    case TOK_CARET:
                        llvm_emit(lg, "  %%%d = xor i64 %s, %s", t, left_str, right_str);
                        break;
                    case TOK_LTLT:
                        llvm_emit(lg, "  %%%d = shl i64 %s, %s", t, left_str, right_str);
                        break;
                    case TOK_GTGT:
                        llvm_emit(lg, "  %%%d = ashr i64 %s, %s", t, left_str, right_str);
                        break;
                    case TOK_STARSTAR: {
                        // Power operator: use loop-based implementation
                        int base_reg, exp_reg, result_reg_pow;
                        int loop_label, body_label, end_label;
                        
                        // Allocate variables
                        base_reg = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = alloca i64", base_reg);
                        exp_reg = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = alloca i64", exp_reg);
                        result_reg_pow = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = alloca i64", result_reg_pow);
                        
                        // Store base and exponent
                        llvm_emit(lg, "  store i64 %s, i64* %%%d", left_str, base_reg);
                        llvm_emit(lg, "  store i64 %s, i64* %%%d", right_str, exp_reg);
                        llvm_emit(lg, "  store i64 1, i64* %%%d", result_reg_pow);
                        
                        // Loop
                        loop_label = llvm_new_label(lg);
                        body_label = llvm_new_label(lg);
                        end_label = llvm_new_label(lg);
                        llvm_emit(lg, "  br label %%L%d", loop_label);
                        llvm_emit(lg, "L%d:", loop_label);
                        
                        int exp_val = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = load i64, i64* %%%d", exp_val, exp_reg);
                        int cond = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = icmp sgt i64 %%%d, 0", cond, exp_val);
                        llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cond, body_label, end_label);
                        
                        llvm_emit(lg, "L%d:", body_label);
                        int curr_result = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = load i64, i64* %%%d", curr_result, result_reg_pow);
                        int base_val = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = load i64, i64* %%%d", base_val, base_reg);
                        int new_result = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = mul i64 %%%d, %%%d", new_result, curr_result, base_val);
                        llvm_emit(lg, "  store i64 %%%d, i64* %%%d", new_result, result_reg_pow);
                        int new_exp = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = sub i64 %%%d, 1", new_exp, exp_val);
                        llvm_emit(lg, "  store i64 %%%d, i64* %%%d", new_exp, exp_reg);
                        llvm_emit(lg, "  br label %%L%d", loop_label);
                        
                        llvm_emit(lg, "L%d:", end_label);
                        // Allocate t NOW, after all the loop code
                        t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = load i64, i64* %%%d", t, result_reg_pow);
                        *result_reg = t;
                        return;
                    }
                    default:
                        // Unsupported integer operation - generate zero register
                        t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = add i64 0, 0", t);
                        *result_reg = t;
                        return;
                }
            }
            *result_reg = t;
            break;
        }
        
        case EXPR_UNARY: {
            int operand_reg;
            llvm_expr(lg, e->unary.operand, &operand_reg);
            
            char operand_str[64];
            if (operand_reg <= -1000000) {
                snprintf(operand_str, 64, "%lld", (long long)(-operand_reg - 1000000));
            } else {
                snprintf(operand_str, 64, "%%%d", operand_reg);
            }
            
            int t = llvm_new_temp(lg);
            if (e->unary.op == TOK_MINUS) {
                llvm_emit(lg, "  %%%d = sub i64 0, %s", t, operand_str);
            } else if (e->unary.op == TOK_BANG) {
                llvm_emit(lg, "  %%%d = xor i1 %s, true", t, operand_str);
            } else if (e->unary.op == TOK_TILDE) {
                llvm_emit(lg, "  %%%d = xor i64 %s, -1", t, operand_str);
            }
            *result_reg = t;
            break;
        }
        
        case EXPR_CALL: {
            // Check for module.function() calls or chained method calls
            if (e->call.func->kind == EXPR_FIELD) {
                // Could be: module.function(), module.submodule.function(), or result.method()
                const char* function = e->call.func->field.field;
                const char* builtin_name = NULL;
                
                // Handle nested module paths (e.g., compiler.lexer.tokenize)
                if (e->call.func->field.object->kind == EXPR_FIELD) {
                    // Build full module path
                    char full_path[MAX_IDENT_LEN] = "";
                    Expr* curr = e->call.func->field.object;
                    char parts[10][MAX_IDENT_LEN];
                    int pcount = 0;
                    
                    while (curr && pcount < 10) {
                        if (curr->kind == EXPR_IDENT) {
                            strcpy(parts[pcount++], curr->ident);
                            break;
                        } else if (curr->kind == EXPR_FIELD) {
                            strcpy(parts[pcount++], curr->field.field);
                            curr = curr->field.object;
                        } else break;
                    }
                    
                    if (pcount > 0) {
                        strcpy(full_path, parts[pcount - 1]);
                        for (int i = pcount - 2; i >= 0; i--) {
                            strcat(full_path, ".");
                            strcat(full_path, parts[i]);
                        }
                        strcat(full_path, ".");
                        strcat(full_path, function);
                        
                        // Look up the function
                        for (int i = 0; i < lg->module->function_count; i++) {
                            if (strcmp(lg->module->functions[i].name, full_path) == 0) {
                                // Found it! Generate the call
                                char args[1024] = "";
                                for (int j = 0; j < e->call.arg_count; j++) {
                                    int arg_reg;
                                    llvm_expr(lg, e->call.args[j], &arg_reg);
                                    char arg_str[128];
                                    bool is_str = (e->call.args[j]->kind == EXPR_STRING ||
                                                  (e->call.args[j]->kind == EXPR_IDENT &&
                                                   llvm_get_var_type(lg, e->call.args[j]->ident) &&
                                                   llvm_get_var_type(lg, e->call.args[j]->ident)->kind == TYPE_STR));
                                    if (arg_reg <= -1000000) {
                                        snprintf(arg_str, 128, "%si64 %lld", (j > 0 ? ", " : ""), (long long)(-arg_reg - 1000000));
                                    } else if (is_str) {
                                        snprintf(arg_str, 128, "%si8* %%%d", (j > 0 ? ", " : ""), arg_reg);
                                    } else {
                                        snprintf(arg_str, 128, "%si64 %%%d", (j > 0 ? ", " : ""), arg_reg);
                                    }
                                    strcat(args, arg_str);
                                }
                                
                                int t = llvm_new_temp(lg);
                                FnDef* fn = &lg->module->functions[i];
                                bool ret_str = (fn->return_type && fn->return_type->kind == TYPE_STR);
                                
                                if (ret_str) {
                                    llvm_emit(lg, "  %%%d = call i8* @\"%s\"(%s)", t, full_path, args);
                                } else {
                                    llvm_emit(lg, "  %%%d = call i64 @\"%s\"(%s)", t, full_path, args);
                                }
                                *result_reg = t;
                                return;
                            }
                        }
                    }
                }
                
                // Check if it's a simple module.function call
                if (e->call.func->field.object->kind == EXPR_IDENT) {
                    const char* module = e->call.func->field.object->ident;
                    builtin_name = map_module_function(module, function);
                    
                    // If not builtin, check for user-defined module.function
                    if (!builtin_name) {
                        char prefixed_name[MAX_IDENT_LEN];
                        // Convert module.function to module/function.function for lookup
                        char module_path[MAX_IDENT_LEN];
                        strcpy(module_path, module);
                        // Convert dots to slashes in module path
                        for (int k = 0; module_path[k]; k++) {
                            if (module_path[k] == '.') module_path[k] = '/';
                        }
                        snprintf(prefixed_name, MAX_IDENT_LEN, "%s.%s", module_path, function);
                        
                        // Check if this function exists in our module
                        bool found_user_fn = false;
                        for (int i = 0; i < lg->module->function_count; i++) {
                            if (strcmp(lg->module->functions[i].name, prefixed_name) == 0) {
                                found_user_fn = true;
                                break;
                            }
                        }
                        
                        if (found_user_fn) {
                            // Handle user-defined function call
                            char args[1024] = "";
                            for (int i = 0; i < e->call.arg_count; i++) {
                                int arg_reg;
                                llvm_expr(lg, e->call.args[i], &arg_reg);
                                
                                char arg_str[128];
                                // Check if argument is a string
                                bool is_string_arg = (e->call.args[i]->kind == EXPR_STRING ||
                                                     (e->call.args[i]->kind == EXPR_IDENT &&
                                                      llvm_get_var_type(lg, e->call.args[i]->ident) &&
                                                      llvm_get_var_type(lg, e->call.args[i]->ident)->kind == TYPE_STR));
                                
                                if (arg_reg <= -1000000) {
                                    snprintf(arg_str, 128, "%si64 %lld", (i > 0 ? ", " : ""), (long long)(-arg_reg - 1000000));
                                } else if (is_string_arg) {
                                    snprintf(arg_str, 128, "%si8* %%%d", (i > 0 ? ", " : ""), arg_reg);
                                } else {
                                    snprintf(arg_str, 128, "%si64 %%%d", (i > 0 ? ", " : ""), arg_reg);
                                }
                                strcat(args, arg_str);
                            }
                            
                            int t = llvm_new_temp(lg);
                            // Find the function to check return type
                            FnDef* user_fn = NULL;
                            for (int j = 0; j < lg->module->function_count; j++) {
                                if (strcmp(lg->module->functions[j].name, prefixed_name) == 0) {
                                    user_fn = &lg->module->functions[j];
                                    break;
                                }
                            }
                            
                            // Check if function returns string
                            bool returns_string = (user_fn && user_fn->return_type && user_fn->return_type->kind == TYPE_STR);
                            
                            if (returns_string) {
                                llvm_emit(lg, "  %%%d = call i8* @\"%s\"(%s)", t, prefixed_name, args);
                            } else {
                                llvm_emit(lg, "  %%%d = call i64 @\"%s\"(%s)", t, prefixed_name, args);
                            }
                            *result_reg = t;
                            break;
                        }
                    }
                }
                
                // If not a module function, check if it's a method on a call result (chaining)
                if (!builtin_name && e->call.func->field.object->kind == EXPR_CALL) {
                    // This is method chaining: result.method()
                    // Evaluate the object (previous call)
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    // Now call the method with obj_reg as first argument
                    // Map the method name (e.g., "arg" -> "command_arg_chain")
                    builtin_name = map_module_function("os", function);  // Assume os for now
                    
                    if (builtin_name) {
                        // Build arguments with object as first arg
                        char args[1024];
                        snprintf(args, sizeof(args), "i64 %%%d", obj_reg);
                        
                        for (int i = 0; i < e->call.arg_count; i++) {
                            int arg_reg;
                            llvm_expr(lg, e->call.args[i], &arg_reg);
                            
                            char arg_str[128];
                            bool is_string_arg = llvm_is_string_expr(lg, e->call.args[i]);
                            bool is_array_arg = llvm_is_array_expr(lg, e->call.args[i]);
                            
                            if (is_string_arg) {
                                snprintf(arg_str, 128, ", i8* %%%d", arg_reg);
                            } else if (is_array_arg) {
                                snprintf(arg_str, 128, ", i64* %%%d", arg_reg);
                            } else if (arg_reg <= -1000000) {
                                snprintf(arg_str, 128, ", i64 %lld", (long long)(-arg_reg - 1000000));
                            } else {
                                snprintf(arg_str, 128, ", i64 %%%d", arg_reg);
                            }
                            strcat(args, arg_str);
                        }
                        
                        // Determine return type
                        bool returns_string = (strcmp(function, "output") == 0);
                        
                        int t = llvm_new_temp(lg);
                        if (returns_string) {
                            llvm_emit(lg, "  %%%d = call i8* @%s(%s)", t, builtin_name, args);
                        } else {
                            llvm_emit(lg, "  %%%d = call i64 @%s(%s)", t, builtin_name, args);
                        }
                        *result_reg = t;
                        break;
                    }
                }
                
                if (builtin_name) {
                    // Map to builtin function call with proper types
                    char args[1024] = "";
                    for (int i = 0; i < e->call.arg_count; i++) {
                        int arg_reg;
                        llvm_expr(lg, e->call.args[i], &arg_reg);
                        
                        char arg_str[128];
                        // Check if argument is a string (use i8* type)
                        bool is_string_arg = (e->call.args[i]->kind == EXPR_STRING ||
                                             (e->call.args[i]->kind == EXPR_IDENT &&
                                              llvm_get_var_type(lg, e->call.args[i]->ident) &&
                                              llvm_get_var_type(lg, e->call.args[i]->ident)->kind == TYPE_STR));
                        
                        // Check if argument is an array (use i64* type)
                        bool is_array_arg = (e->call.args[i]->kind == EXPR_ARRAY ||
                                            (e->call.args[i]->kind == EXPR_IDENT &&
                                             llvm_get_var_type(lg, e->call.args[i]->ident) &&
                                             llvm_get_var_type(lg, e->call.args[i]->ident)->kind == TYPE_ARRAY));
                        
                        // Check if argument is result of string-returning function
                        if (e->call.args[i]->kind == EXPR_CALL && e->call.args[i]->call.func->kind == EXPR_IDENT) {
                            const char* called_func = e->call.args[i]->call.func->ident;
                            if (strcmp(called_func, "str_concat") == 0 || 
                                strcmp(called_func, "str_replace") == 0 ||
                                strcmp(called_func, "substring") == 0 ||
                                strcmp(called_func, "chr") == 0 ||
                                strcmp(called_func, "int_to_str") == 0) {
                                is_string_arg = true;
                            }
                        }
                        
                        // Check if this function expects string arguments
                        bool force_string = false;
                        if (strcmp(builtin_name, "exec_wyn") == 0 && i == 0) force_string = true;
                        if (strcmp(builtin_name, "setenv_wyn") == 0) force_string = true;
                        if (strcmp(builtin_name, "chdir_wyn") == 0 && i == 0) force_string = true;
                        
                        if (is_string_arg || force_string) {
                            snprintf(arg_str, 128, "i8* %%%d", arg_reg);
                        } else if (is_array_arg) {
                            snprintf(arg_str, 128, "i64* %%%d", arg_reg);
                        } else if (arg_reg <= -1000000) {
                            snprintf(arg_str, 128, "i64 %lld", (long long)(-arg_reg - 1000000));
                        } else {
                            snprintf(arg_str, 128, "i64 %%%d", arg_reg);
                        }
                        if (i > 0) strcat(args, ", ");
                        strcat(args, arg_str);
                    }
                    
                    // Determine return type based on function
                    bool returns_string = (strcmp(function, "chr") == 0 || 
                                          strcmp(function, "substring") == 0 ||
                                          strcmp(function, "concat") == 0 ||
                                          strcmp(function, "char_at") == 0 ||
                                          strcmp(function, "int_to_str") == 0 ||
                                          strcmp(function, "upper") == 0 ||
                                          strcmp(function, "lower") == 0 ||
                                          strcmp(function, "trim") == 0 ||
                                          strcmp(function, "read") == 0 ||
                                          strcmp(function, "getenv") == 0 ||
                                          strcmp(function, "cwd") == 0 ||
                                          strcmp(function, "exec_output") == 0 ||
                                          strcmp(function, "exec_args") == 0 ||
                                          strcmp(function, "parse") == 0 ||
                                          strcmp(function, "stringify") == 0 ||
                                          strcmp(function, "get_string") == 0 ||
                                          strcmp(function, "yaml_parse") == 0 ||
                                          strcmp(function, "toml_parse") == 0 ||
                                          strcmp(function, "http_get") == 0 ||
                                          strcmp(function, "http_post") == 0 ||
                                          strcmp(function, "tcp_recv") == 0 ||
                                          strcmp(function, "hostname") == 0 ||
                                          strcmp(function, "http_put") == 0 ||
                                          strcmp(function, "http_delete") == 0 ||
                                          strcmp(function, "http_head") == 0 ||
                                          strcmp(function, "http_get_headers") == 0 ||
                                          strcmp(function, "udp_recv") == 0 ||
                                          strcmp(function, "resolve_host") == 0 ||
                                          strcmp(function, "get_local_ip") == 0 ||
                                          strcmp(function, "format_timestamp") == 0 ||
                                          strcmp(function, "format_iso8601") == 0 ||
                                          strcmp(function, "base64_encode") == 0 ||
                                          strcmp(function, "base64_decode") == 0 ||
                                          strcmp(function, "hex_encode") == 0 ||
                                          strcmp(function, "hex_decode") == 0 ||
                                          strcmp(function, "url_encode") == 0 ||
                                          strcmp(function, "url_decode") == 0 ||
                                          strcmp(function, "sha256") == 0 ||
                                          strcmp(function, "md5") == 0 ||
                                          strcmp(function, "sha1") == 0 ||
                                          strcmp(function, "find") == 0 ||
                                          strcmp(function, "replace") == 0 ||
                                          strcmp(function, "hashmap_new") == 0 ||
                                          strcmp(function, "hashmap_get") == 0 ||
                                          strcmp(function, "set_new") == 0 ||
                                          strcmp(function, "encrypt_aes256") == 0 ||
                                          strcmp(function, "decrypt_aes256") == 0 ||
                                          strcmp(function, "generate_random_bytes") == 0 ||
                                          strcmp(function, "hmac_sha256") == 0 ||
                                          strcmp(function, "server_read_request") == 0 ||
                                          strcmp(function, "parse_method") == 0 ||
                                          strcmp(function, "parse_path") == 0 ||
                                          strcmp(function, "parse_body") == 0 ||
                                          strcmp(function, "sqlite_query") == 0 ||
                                          strcmp(function, "output") == 0 ||
                                          strcmp(function, "to_str") == 0);
                    
                    bool returns_array = (strcmp(function, "split") == 0 ||
                                         strcmp(function, "glob") == 0 ||
                                         strcmp(function, "reverse") == 0 ||
                                         strcmp(function, "append") == 0 ||
                                         strcmp(function, "prepend") == 0 ||
                                         strcmp(function, "make_ok") == 0 ||
                                         strcmp(function, "make_err") == 0 ||
                                         strcmp(function, "make_some") == 0 ||
                                         strcmp(function, "make_none") == 0);
                    
                    bool returns_pointer = (strcmp(builtin_name, "sb_new_wyn") == 0);
                    
                    bool returns_void = (strcmp(builtin_name, "sb_append_wyn") == 0);
                    
                    int t = llvm_new_temp(lg);
                    if (returns_string) {
                        llvm_emit(lg, "  %%%d = call i8* @%s(%s)", t, builtin_name, args);
                    } else if (returns_array) {
                        llvm_emit(lg, "  %%%d = call i64* @%s(%s)", t, builtin_name, args);
                    } else if (returns_pointer) {
                        llvm_emit(lg, "  %%%d = call i8* @%s(%s)", t, builtin_name, args);
                        // Convert pointer to i64 for storage
                        int converted_reg = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = ptrtoint i8* %%%d to i64", converted_reg, t);
                        t = converted_reg;
                    } else if (returns_void) {
                        llvm_emit(lg, "  call void @%s(%s)", builtin_name, args);
                        // Generate a dummy register for void functions
                        llvm_emit(lg, "  %%%d = add i64 0, 0", t);
                    } else {
                        llvm_emit(lg, "  %%%d = call i64 @%s(%s)", t, builtin_name, args);
                    }
                    *result_reg = t;
                    break;
                }
            }
            
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "print") == 0) {
                for (int i = 0; i < e->call.arg_count; i++) {
                    int arg_reg;
                    llvm_expr(lg, e->call.args[i], &arg_reg);
                    
                    int t = llvm_new_temp(lg);  // Capture printf return value
                    if (e->call.args[i]->kind == EXPR_STRING) {
                        llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* %%%d)", t, arg_reg);
                    } else if (e->call.args[i]->kind == EXPR_BINARY && e->call.args[i]->binary.op == TOK_PLUS) {
                        // Check if binary expression is string concatenation
                        bool left_is_str = (e->call.args[i]->binary.left->kind == EXPR_STRING) ||
                                          (e->call.args[i]->binary.left->kind == EXPR_IDENT && 
                                           llvm_get_var_type(lg, e->call.args[i]->binary.left->ident) &&
                                           llvm_get_var_type(lg, e->call.args[i]->binary.left->ident)->kind == TYPE_STR);
                        bool right_is_str = (e->call.args[i]->binary.right->kind == EXPR_STRING) ||
                                           (e->call.args[i]->binary.right->kind == EXPR_IDENT && 
                                            llvm_get_var_type(lg, e->call.args[i]->binary.right->ident) &&
                                            llvm_get_var_type(lg, e->call.args[i]->binary.right->ident)->kind == TYPE_STR);
                        if (left_is_str || right_is_str) {
                            // String concatenation result - print as string
                            llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* %%%d)", t, arg_reg);
                        } else {
                            // Integer addition result - print as integer
                            llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([3 x i8], [3 x i8]* @.fmt.int, i32 0, i32 0), i64 %%%d)", t, arg_reg);
                        }
                    } else if (e->call.args[i]->kind == EXPR_CALL) {
                        // Check if this is a string-returning call (including chained)
                        if (llvm_is_string_expr(lg, e->call.args[i])) {
                            llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* %%%d)", t, arg_reg);
                        } else {
                            llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([3 x i8], [3 x i8]* @.fmt.int, i32 0, i32 0), i64 %%%d)", t, arg_reg);
                        }
                    } else if (e->call.args[i]->kind == EXPR_SLICE) {
                        // Check if this is string or array slicing
                        bool is_string_slice = false;
                        if (e->call.args[i]->slice.object->kind == EXPR_STRING) {
                            is_string_slice = true;
                        } else if (e->call.args[i]->slice.object->kind == EXPR_IDENT) {
                            Type* obj_type = llvm_get_var_type(lg, e->call.args[i]->slice.object->ident);
                            is_string_slice = (obj_type && obj_type->kind == TYPE_STR);
                        }
                        
                        if (is_string_slice) {
                            // String slice - convert i64 back to i8* for printing
                            llvm_emit(lg, "  %%%d = inttoptr i64 %%%d to i8*", t, arg_reg);
                            int printf_reg = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* %%%d)", printf_reg, t);
                        } else {
                            // Array slice - convert pointer to integer and print
                            llvm_emit(lg, "  %%%d = ptrtoint i64* %%%d to i64", t, arg_reg);
                            int printf_reg = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([3 x i8], [3 x i8]* @.fmt.int, i32 0, i32 0), i64 %%%d)", printf_reg, t);
                        }
                    } else if (e->call.args[i]->kind == EXPR_FLOAT) {
                        llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([3 x i8], [3 x i8]* @.fmt.float, i32 0, i32 0), double %f)", 
                                 t, e->call.args[i]->float_val);
                    } else if (e->call.args[i]->kind == EXPR_INT) {
                        llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([3 x i8], [3 x i8]* @.fmt.int, i32 0, i32 0), i64 %lld)", 
                                 t, e->call.args[i]->int_val);
                    } else if (e->call.args[i]->kind == EXPR_IDENT) {
                        // Check actual type from type tracking
                        Type* var_type = llvm_get_var_type(lg, e->call.args[i]->ident);
                        if (var_type && var_type->kind == TYPE_FLOAT) {
                            llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([3 x i8], [3 x i8]* @.fmt.float, i32 0, i32 0), double %%%d)", t, arg_reg);
                        } else if (var_type && var_type->kind == TYPE_STR) {
                            llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* %%%d)", t, arg_reg);
                        } else {
                            llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([3 x i8], [3 x i8]* @.fmt.int, i32 0, i32 0), i64 %%%d)", t, arg_reg);
                        }
                    } else if (llvm_is_float_expr(lg, e->call.args[i])) {
                        // Float expression
                        char arg_str[64];
                        if (arg_reg <= -1000000) {
                            snprintf(arg_str, 64, "%lld", (long long)(-arg_reg - 1000000));
                        } else {
                            snprintf(arg_str, 64, "%%%d", arg_reg);
                        }
                        llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([3 x i8], [3 x i8]* @.fmt.float, i32 0, i32 0), double %s)", t, arg_str);
                    } else if (llvm_is_string_expr(lg, e->call.args[i])) {
                        // String expression (including array indexing)
                        llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* %%%d)", t, arg_reg);
                    } else {
                        char arg_str[64];
                        if (arg_reg <= -1000000) {
                            snprintf(arg_str, 64, "%lld", (long long)(-arg_reg - 1000000));
                            llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([3 x i8], [3 x i8]* @.fmt.int, i32 0, i32 0), i64 %s)", t, arg_str);
                        } else {
                            // Check if this might be a pointer that needs casting
                            if (llvm_is_array_expr(lg, e->call.args[i])) {
                                // Cast pointer to i64 for printing
                                int cast_reg = llvm_new_temp(lg);
                                llvm_emit(lg, "  %%%d = ptrtoint ptr %%%d to i64", cast_reg, arg_reg);
                                llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([3 x i8], [3 x i8]* @.fmt.int, i32 0, i32 0), i64 %%%d)", t, cast_reg);
                            } else {
                                snprintf(arg_str, 64, "%%%d", arg_reg);
                                llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([3 x i8], [3 x i8]* @.fmt.int, i32 0, i32 0), i64 %s)", t, arg_str);
                            }
                        }
                    }
                }
                // Add newline after print
                int nl = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([2 x i8], [2 x i8]* @.fmt.newline, i32 0, i32 0))", nl);
                // Print returns void, generate zero register for consistency
                int t = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = add i64 0, 0", t);
                *result_reg = t;
                break;
            }
            
            // print_raw: print without newline
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "print_raw") == 0) {
                for (int i = 0; i < e->call.arg_count; i++) {
                    int arg_reg;
                    llvm_expr(lg, e->call.args[i], &arg_reg);
                    
                    int t = llvm_new_temp(lg);
                    if (e->call.args[i]->kind == EXPR_STRING) {
                        llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* %%%d)", t, arg_reg);
                    } else if (llvm_is_float_expr(lg, e->call.args[i])) {
                        char arg_str[64];
                        if (arg_reg <= -1000000) {
                            snprintf(arg_str, 64, "%f", (double)(-arg_reg - 1000000));
                        } else {
                            snprintf(arg_str, 64, "%%%d", arg_reg);
                        }
                        llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([3 x i8], [3 x i8]* @.fmt.float, i32 0, i32 0), double %s)", t, arg_str);
                    } else {
                        char arg_str[64];
                        if (arg_reg <= -1000000) {
                            snprintf(arg_str, 64, "%lld", (long long)(-arg_reg - 1000000));
                        } else {
                            snprintf(arg_str, 64, "%%%d", arg_reg);
                        }
                        llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([3 x i8], [3 x i8]* @.fmt.int, i32 0, i32 0), i64 %s)", t, arg_str);
                    }
                }
                // No newline for print_raw
                int t = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = add i64 0, 0", t);
                *result_reg = t;
                break;
            }
            
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "println") == 0) {
                // println: same as print but add newline
                for (int i = 0; i < e->call.arg_count; i++) {
                    int arg_reg;
                    llvm_expr(lg, e->call.args[i], &arg_reg);
                    
                    int t = llvm_new_temp(lg);
                    if (e->call.args[i]->kind == EXPR_STRING) {
                        llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* %%%d)", t, arg_reg);
                    } else if (e->call.args[i]->kind == EXPR_INT) {
                        llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.fmt.int, i32 0, i32 0), i64 %lld)", 
                                 t, e->call.args[i]->int_val);
                    } else if (e->call.args[i]->kind == EXPR_IDENT) {
                        Type* var_type = llvm_get_var_type(lg, e->call.args[i]->ident);
                        if (var_type && var_type->kind == TYPE_STR) {
                            llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* %%%d)", t, arg_reg);
                        } else {
                            llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.fmt.int, i32 0, i32 0), i64 %%%d)", t, arg_reg);
                        }
                    } else {
                        llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @.fmt.int, i32 0, i32 0), i64 %%%d)", t, arg_reg);
                    }
                }
                // Print newline
                int nl = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr ([2 x i8], [2 x i8]* @.fmt.newline, i32 0, i32 0))", nl);
                int t = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = add i64 0, 0", t);
                *result_reg = t;
            } else if (e->call.func->kind == EXPR_IDENT) {
                const char* func_name = e->call.func->ident;
                char actual_func_name[MAX_IDENT_LEN];
                strcpy(actual_func_name, func_name);
                
                // Handle default parameters for user-defined functions
                FnDef* fn_def = NULL;
                for (int i = 0; i < lg->module->function_count; i++) {
                    if (strcmp(lg->module->functions[i].name, func_name) == 0) {
                        fn_def = &lg->module->functions[i];
                        break;
                    }
                }
                
                // Check if this is a generic function call
                if (fn_def && fn_def->generic_count > 0 && e->call.type_arg_count > 0) {
                    // Create instantiated name: func_int, func_str, etc.
                    snprintf(actual_func_name, MAX_IDENT_LEN, "%s_", func_name);
                    
                    // Append type name (simplified - only handle int, str, bool for now)
                    if (e->call.type_args[0]->kind == TYPE_INT) {
                        strcat(actual_func_name, "int");
                    } else if (e->call.type_args[0]->kind == TYPE_STR) {
                        strcat(actual_func_name, "str");
                    } else if (e->call.type_args[0]->kind == TYPE_BOOL) {
                        strcat(actual_func_name, "bool");
                    } else {
                        strcat(actual_func_name, "generic");
                    }
                    
                    // Check if already instantiated
                    bool already_instantiated = false;
                    for (int i = 0; i < lg->generic_instance_count; i++) {
                        if (strcmp(lg->generic_instances[i].instantiated_name, actual_func_name) == 0) {
                            already_instantiated = true;
                            break;
                        }
                    }
                    
                    // Track this instantiation
                    if (!already_instantiated && lg->generic_instance_count < 256) {
                        lg->generic_instances[lg->generic_instance_count].generic_fn = fn_def;
                        lg->generic_instances[lg->generic_instance_count].concrete_type = e->call.type_args[0];
                        strcpy(lg->generic_instances[lg->generic_instance_count].instantiated_name, actual_func_name);
                        lg->generic_instance_count++;
                    }
                }
                
                // Fill in default arguments if needed
                Expr** actual_args = e->call.args;
                int actual_arg_count = e->call.arg_count;
                
                if (fn_def && e->call.arg_count < fn_def->param_count) {
                    // Allocate new args array with defaults filled in
                    actual_args = malloc(sizeof(Expr*) * fn_def->param_count);
                    for (int i = 0; i < fn_def->param_count; i++) {
                        if (i < e->call.arg_count) {
                            actual_args[i] = e->call.args[i];
                        } else {
                            actual_args[i] = fn_def->params[i].default_val;
                        }
                    }
                    actual_arg_count = fn_def->param_count;
                }
                
                // Check if it's a builtin function
                bool is_string_builtin = (strcmp(func_name, "chr") == 0 ||
                                         strcmp(func_name, "substring") == 0 ||
                                         strcmp(func_name, "str_concat") == 0 ||
                                         strcmp(func_name, "str_replace") == 0 ||
                                         strcmp(func_name, "char_at") == 0 ||
                                         strcmp(func_name, "int_to_str") == 0 ||
                                         strcmp(func_name, "to_string") == 0 ||
                                         strcmp(func_name, "str_substr") == 0 ||
                                         strcmp(func_name, "str_upper") == 0 ||
                                         strcmp(func_name, "str_lower") == 0 ||
                                         strcmp(func_name, "str_trim") == 0 ||
                                         strcmp(func_name, "read_file") == 0 ||
                                         strcmp(func_name, "getenv_wyn") == 0 ||
                                         strcmp(func_name, "cwd_wyn") == 0 ||
                                         strcmp(func_name, "parse_json") == 0 ||
                                         strcmp(func_name, "stringify_json") == 0 ||
                                         strcmp(func_name, "get_string_json") == 0 ||
                                         strcmp(func_name, "yaml_parse") == 0 ||
                                         strcmp(func_name, "toml_parse") == 0 ||
                                         strcmp(func_name, "http_get_wyn") == 0 ||
                                         strcmp(func_name, "http_post_wyn") == 0 ||
                                         strcmp(func_name, "tcp_recv_wyn") == 0 ||
                                         strcmp(func_name, "hostname_wyn") == 0 ||
                                         strcmp(func_name, "http_put_wyn") == 0 ||
                                         strcmp(func_name, "http_delete_wyn") == 0 ||
                                         strcmp(func_name, "http_head_wyn") == 0 ||
                                         strcmp(func_name, "http_get_headers_wyn") == 0 ||
                                         strcmp(func_name, "udp_recv_wyn") == 0 ||
                                         strcmp(func_name, "resolve_host_wyn") == 0 ||
                                         strcmp(func_name, "get_local_ip_wyn") == 0 ||
                                         strcmp(func_name, "format_timestamp") == 0 ||
                                         strcmp(func_name, "format_iso8601") == 0 ||
                                         strcmp(func_name, "hashmap_new") == 0 ||
                                         strcmp(func_name, "hashmap_get") == 0 ||
                                         strcmp(func_name, "set_new") == 0 ||
                                         strcmp(func_name, "encrypt_aes256") == 0 ||
                                         strcmp(func_name, "decrypt_aes256") == 0 ||
                                         strcmp(func_name, "generate_random_bytes") == 0 ||
                                         strcmp(func_name, "hmac_sha256") == 0 ||
                                         strcmp(func_name, "http_server_read_request") == 0 ||
                                         strcmp(func_name, "http_parse_method") == 0 ||
                                         strcmp(func_name, "http_parse_path") == 0 ||
                                         strcmp(func_name, "http_parse_body") == 0);
                
                bool is_int_builtin = (strcmp(func_name, "ord") == 0 ||
                                      strcmp(func_name, "str_find") == 0 ||
                                      strcmp(func_name, "str_cmp") == 0 ||
                                      strcmp(func_name, "clock") == 0 ||
                                      strcmp(func_name, "random") == 0 ||
                                      strcmp(func_name, "min") == 0 ||
                                      strcmp(func_name, "max") == 0 ||
                                      strcmp(func_name, "abs") == 0);
                
                bool is_array_builtin = (strcmp(func_name, "str_split") == 0 ||
                                         strcmp(func_name, "file_glob") == 0);
                
                // Build arguments
                char args[1024] = "";
                if (strcmp(func_name, "assert") != 0) {
                    // Normal argument processing for non-assert functions
                    for (int i = 0; i < actual_arg_count; i++) {
                        int arg_reg;
                        llvm_expr(lg, actual_args[i], &arg_reg);
                        
                        char arg_str[128];
                        // Check if argument should be i8* (string)
                        bool is_string_arg = (actual_args[i]->kind == EXPR_STRING ||
                                             (actual_args[i]->kind == EXPR_IDENT &&
                                              llvm_get_var_type(lg, actual_args[i]->ident) &&
                                              llvm_get_var_type(lg, actual_args[i]->ident)->kind == TYPE_STR));
                        
                        // Check if argument should be i64* (array)
                        bool is_array_arg = (actual_args[i]->kind == EXPR_ARRAY ||
                                            (actual_args[i]->kind == EXPR_IDENT &&
                                             llvm_get_var_type(lg, actual_args[i]->ident) &&
                                             llvm_get_var_type(lg, actual_args[i]->ident)->kind == TYPE_ARRAY));
                        
                        // Check if argument is a string-returning function call
                        if (actual_args[i]->kind == EXPR_CALL) {
                            if (actual_args[i]->call.func->kind == EXPR_IDENT) {
                                const char* called_func = actual_args[i]->call.func->ident;
                                if (strcmp(called_func, "str_concat") == 0 || 
                                    strcmp(called_func, "str_replace") == 0 ||
                                    strcmp(called_func, "substring") == 0 ||
                                    strcmp(called_func, "chr") == 0 ||
                                    strcmp(called_func, "int_to_str") == 0) {
                                    is_string_arg = true;
                                }
                            } else if (actual_args[i]->call.func->kind == EXPR_FIELD) {
                                // Check module.function calls
                                Type* ret_type = get_builtin_return_type(
                                    map_module_function(
                                        actual_args[i]->call.func->field.object->ident,
                                        actual_args[i]->call.func->field.field
                                    )
                                );
                                if (ret_type && ret_type->kind == TYPE_STR) {
                                    is_string_arg = true;
                                }
                            }
                        }
                        
                        // Special case: ord() first arg is always i8*, str_find/str_cmp first two args are i8*
                        bool force_string_arg = false;
                        if (strcmp(func_name, "ord") == 0 && i == 0) force_string_arg = true;
                        if ((strcmp(func_name, "str_find") == 0 || strcmp(func_name, "str_cmp") == 0) && i < 2) force_string_arg = true;
                        
                        // exec_wyn takes string argument
                        if (strcmp(func_name, "exec_wyn") == 0 && i == 0) force_string_arg = true;
                        
                        if (is_string_arg || is_string_builtin || force_string_arg) {
                            if (actual_args[i]->kind == EXPR_STRING || is_string_arg || force_string_arg) {
                                snprintf(arg_str, 128, "i8* %%%d", arg_reg);
                            } else if (arg_reg <= -1000000) {
                                snprintf(arg_str, 128, "i64 %lld", (long long)(-arg_reg - 1000000));
                            } else {
                                snprintf(arg_str, 128, "i64 %%%d", arg_reg);
                            }
                        } else if (is_array_arg) {
                            snprintf(arg_str, 128, "i64* %%%d", arg_reg);
                        } else if (arg_reg <= -1000000) {
                            snprintf(arg_str, 128, "i64 %lld", (long long)(-arg_reg - 1000000));
                        } else {
                            snprintf(arg_str, 128, "i64 %%%d", arg_reg);
                        }
                        if (i > 0) strcat(args, ", ");
                        strcat(args, arg_str);
                    }
                    
                    int t = llvm_new_temp(lg);
                    // Map function names to C runtime names
                    const char* runtime_name = actual_func_name;
                    if (strcmp(actual_func_name, "clock") == 0) runtime_name = "clock_wyn";
                    if (strcmp(actual_func_name, "random") == 0) runtime_name = "random_wyn";
                    if (strcmp(actual_func_name, "exit") == 0) runtime_name = "exit_wyn";
                    if (strcmp(actual_func_name, "min") == 0) runtime_name = "min_int";
                    if (strcmp(actual_func_name, "max") == 0) runtime_name = "max_int";
                    if (strcmp(actual_func_name, "abs") == 0) runtime_name = "abs_int";
                    if (strcmp(actual_func_name, "to_string") == 0) runtime_name = "int_to_str";
                    
                    // Quote function name if it contains dots
                    char quoted_runtime_name[MAX_IDENT_LEN + 2];
                    if (strchr(runtime_name, '.')) {
                        snprintf(quoted_runtime_name, sizeof(quoted_runtime_name), "\"%s\"", runtime_name);
                    } else {
                        strcpy(quoted_runtime_name, runtime_name);
                    }
                    
                    if (is_string_builtin) {
                        int t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = call i8* @%s(%s)", t, quoted_runtime_name, args);
                        *result_reg = t;
                    } else if (is_array_builtin) {
                        int t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = call i64* @%s(%s)", t, quoted_runtime_name, args);
                        *result_reg = t;
                    } else {
                        // Check if this is a declared function or a variable (function pointer)
                        bool is_declared_function = false;
                        for (int i = 0; i < lg->module->function_count; i++) {
                            if (strcmp(lg->module->functions[i].name, func_name) == 0) {
                                is_declared_function = true;
                                break;
                            }
                        }
                        
                        int var_reg = -1;
                        if (!is_declared_function) {
                            for (int i = 0; i < lg->var_count; i++) {
                                if (strcmp(lg->vars[i], func_name) == 0) {
                                    var_reg = lg->var_regs[i];
                                    break;
                                }
                            }
                        }
                        
                        if (var_reg >= 0) {
                            // Indirect call through function pointer
                            int ptr_reg = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = load i64, i64* %%%d", ptr_reg, var_reg);
                            
                            int fn_reg = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = inttoptr i64 %%%d to i64 (", fn_reg, ptr_reg);
                            for (int i = 0; i < actual_arg_count; i++) {
                                if (i > 0) llvm_emit_raw(lg, ", ");
                                llvm_emit_raw(lg, "i64");
                            }
                            llvm_emit_raw(lg, ")*\n");
                            
                            int t = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = call i64 %%%d(%s)", t, fn_reg, args);
                            *result_reg = t;
                        } else {
                            // Direct call to named function
                            int t = llvm_new_temp(lg);
                            
                            // Determine return type from function definition
                            const char* ret_type = "i64";
                            if (fn_def && fn_def->return_type) {
                                if (fn_def->return_type->kind == TYPE_STR) {
                                    ret_type = "i8*";
                                } else if (fn_def->return_type->kind == TYPE_ARRAY) {
                                    ret_type = "i64*";
                                } else if (fn_def->return_type->kind == TYPE_RESULT) {
                                    ret_type = "i64";  // Result types use simple integer encoding
                                } else if (fn_def->return_type->kind == TYPE_OPTIONAL) {
                                    ret_type = "i64";  // Option types use simple integer encoding
                                } else if (fn_def->return_type->kind == TYPE_FLOAT) {
                                    ret_type = "double";
                                }
                            }
                            
                            llvm_emit(lg, "  %%%d = call %s @%s(%s)", t, ret_type, quoted_runtime_name, args);
                            *result_reg = t;
                        }
                    }
                } else {
                    // Special handling for assert - convert i1 arguments to i64 if needed
                    char converted_args[1024] = "";
                    for (int i = 0; i < actual_arg_count; i++) {
                        int arg_reg;
                        llvm_expr(lg, actual_args[i], &arg_reg);
                        
                        char arg_str[128];
                        if (arg_reg <= -1000000) {
                            snprintf(arg_str, 128, "i64 %lld", (long long)(-arg_reg - 1000000));
                        } else {
                            // Only convert if this is a direct comparison (i1), not a loaded variable (i64)
                            bool is_direct_comparison = (actual_args[i]->kind == EXPR_BINARY && 
                                                        (actual_args[i]->binary.op == TOK_EQEQ || 
                                                         actual_args[i]->binary.op == TOK_NOTEQ ||
                                                         actual_args[i]->binary.op == TOK_LT ||
                                                         actual_args[i]->binary.op == TOK_GT ||
                                                         actual_args[i]->binary.op == TOK_LTEQ ||
                                                         actual_args[i]->binary.op == TOK_GTEQ ||
                                                         actual_args[i]->binary.op == TOK_IN));
                            
                            if (is_direct_comparison) {
                                // Convert i1 to i64
                                int conv_reg = llvm_new_temp(lg);
                                llvm_emit(lg, "  %%%d = zext i1 %%%d to i64", conv_reg, arg_reg);
                                snprintf(arg_str, 128, "i64 %%%d", conv_reg);
                            } else {
                                // Already i64 (loaded from variable or direct value)
                                snprintf(arg_str, 128, "i64 %%%d", arg_reg);
                            }
                        }
                        if (i > 0) strcat(converted_args, ", ");
                        strcat(converted_args, arg_str);
                    }
                    
                    int t = llvm_new_temp(lg);
                    
                    // Check if func_name is a declared function or a variable
                    bool is_declared_function = false;
                    for (int i = 0; i < lg->module->function_count; i++) {
                        if (strcmp(lg->module->functions[i].name, func_name) == 0) {
                            is_declared_function = true;
                            break;
                        }
                    }
                    
                    // If not a declared function, check if it's a variable (function pointer)
                    int var_reg = -1;
                    if (!is_declared_function) {
                        for (int i = 0; i < lg->var_count; i++) {
                            if (strcmp(lg->vars[i], func_name) == 0) {
                                var_reg = lg->var_regs[i];
                                break;
                            }
                        }
                    }
                    
                    if (var_reg >= 0) {
                        // Indirect call through function pointer
                        int ptr_reg = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = load i64, i64* %%%d", ptr_reg, var_reg);
                        
                        // Cast i64 to function pointer type
                        int fn_reg = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = inttoptr i64 %%%d to i64 (", fn_reg, ptr_reg);
                        for (int i = 0; i < actual_arg_count; i++) {
                            if (i > 0) llvm_emit_raw(lg, ", ");
                            llvm_emit_raw(lg, "i64");
                        }
                        llvm_emit_raw(lg, ")*\n");
                        
                        // Call through function pointer
                        llvm_emit(lg, "  %%%d = call i64 %%%d(%s)", t, fn_reg, converted_args);
                    } else {
                        // Direct call to named function
                        // Determine return type from function definition
                        const char* ret_type = "i64";
                        if (fn_def && fn_def->return_type) {
                            if (fn_def->return_type->kind == TYPE_STR) {
                                ret_type = "i8*";
                            } else if (fn_def->return_type->kind == TYPE_ARRAY) {
                                ret_type = "i64*";
                            } else if (fn_def->return_type->kind == TYPE_RESULT) {
                                ret_type = "i64";  // Result types use simple integer encoding
                            } else if (fn_def->return_type->kind == TYPE_OPTIONAL) {
                                ret_type = "i64";  // Option types use simple integer encoding
                            } else if (fn_def->return_type->kind == TYPE_FLOAT) {
                                ret_type = "double";
                            }
                        }
                        
                        // Quote function name if it contains dots
                        char quoted_func_name[MAX_IDENT_LEN + 2];
                        if (strchr(func_name, '.')) {
                            snprintf(quoted_func_name, sizeof(quoted_func_name), "\"%s\"", func_name);
                        } else {
                            strcpy(quoted_func_name, func_name);
                        }
                        
                        llvm_emit(lg, "  %%%d = call %s @%s(%s)", t, ret_type, quoted_func_name, converted_args);
                    }
                    *result_reg = t;
                }
                
                // Clean up allocated args array if we filled in defaults
                if (actual_args != e->call.args) {
                    free(actual_args);
                }
            } else if (e->call.func->kind == EXPR_FIELD) {
                // Method call: obj.method()
                const char* method_name = e->call.func->field.field;
                
                if (strcmp(method_name, "len") == 0) {
                    // Handle .len() for strings and arrays
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    // Check if it's a string
                    Type* obj_type = NULL;
                    if (e->call.func->field.object->kind == EXPR_IDENT) {
                        obj_type = llvm_get_var_type(lg, e->call.func->field.object->ident);
                    } else if (e->call.func->field.object->kind == EXPR_STRING) {
                        obj_type = new_type(TYPE_STR);
                    } else if (llvm_is_string_expr(lg, e->call.func->field.object)) {
                        obj_type = new_type(TYPE_STR);
                    }
                    
                    if (obj_type && obj_type->kind == TYPE_STR) {
                        // String: call str_len
                        int t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = call i64 @str_len(i8* %%%d)", t, obj_reg);
                        *result_reg = t;
                    } else {
                        // Array: load length from first element
                        int len_ptr = llvm_new_temp(lg);
                        int t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = getelementptr i64, ptr %%%d, i64 0", len_ptr, obj_reg);
                        llvm_emit(lg, "  %%%d = load i64, i64* %%%d", t, len_ptr);
                        *result_reg = t;
                    }
                } else if (strcmp(method_name, "abs") == 0) {
                    // Handle num.abs()
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    int cmp = llvm_new_temp(lg);
                    int neg = llvm_new_temp(lg);
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = icmp slt i64 %%%d, 0", cmp, obj_reg);
                    llvm_emit(lg, "  %%%d = sub i64 0, %%%d", neg, obj_reg);
                    llvm_emit(lg, "  %%%d = select i1 %%%d, i64 %%%d, i64 %%%d", t, cmp, neg, obj_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "to_int") == 0) {
                    // Handle str.to_int()
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64 @str_to_int(i8* %%%d)", t, obj_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "to_str") == 0) {
                    // Check if this is a StringBuilder to_str
                    bool is_stringbuilder = false;
                    if (e->call.func->field.object->kind == EXPR_IDENT) {
                        // Simple heuristic: if variable name starts with "sb" or contains "stringbuilder", assume it's a StringBuilder
                        const char* var_name = e->call.func->field.object->ident;
                        if (strncmp(var_name, "sb", 2) == 0 || strstr(var_name, "stringbuilder") != NULL) {
                            is_stringbuilder = true;
                        } else {
                            // Also check the type
                            Type* obj_type = llvm_get_var_type(lg, var_name);
                            if (obj_type && obj_type->kind == TYPE_ANY) {
                                is_stringbuilder = true;
                            }
                        }
                    }
                    
                    if (is_stringbuilder) {
                        // Handle sb.to_str() - convert i64 back to pointer
                        int obj_reg;
                        llvm_expr(lg, e->call.func->field.object, &obj_reg);
                        
                        // Convert i64 back to pointer
                        int ptr_reg = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = inttoptr i64 %%%d to i8*", ptr_reg, obj_reg);
                        
                        // Call sb_to_str_wyn(ptr) -> returns string
                        int t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = call i8* @sb_to_str_wyn(i8* %%%d)", t, ptr_reg);
                        *result_reg = t;
                    } else {
                        // Handle num.to_str() or str.to_str()
                        int obj_reg;
                        llvm_expr(lg, e->call.func->field.object, &obj_reg);
                        
                        // Check if object is a string
                        bool is_string = llvm_is_string_expr(lg, e->call.func->field.object);
                        
                        if (is_string) {
                            // String.to_str() just returns self
                            *result_reg = obj_reg;
                        } else {
                            // Int/bool.to_str() converts to string
                            int t = llvm_new_temp(lg);
                            if (obj_reg <= -1000000) {
                                // Constant value
                                long long const_val = -obj_reg - 1000000;
                                llvm_emit(lg, "  %%%d = call i8* @int_to_str(i64 %lld)", t, const_val);
                            } else {
                                llvm_emit(lg, "  %%%d = call i8* @int_to_str(i64 %%%d)", t, obj_reg);
                            }
                            *result_reg = t;
                        }
                    }
                } else if (strcmp(method_name, "upper") == 0) {
                    // Handle str.upper()
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i8* @str_upper(i8* %%%d)", t, obj_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "lower") == 0) {
                    // Handle str.lower()
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i8* @str_lower(i8* %%%d)", t, obj_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "trim") == 0) {
                    // Handle str.trim()
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i8* @str_trim(i8* %%%d)", t, obj_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "contains") == 0) {
                    // Handle str.contains(substr) or arr.contains(item)
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    // Determine if object is array or string
                    bool is_array = false;
                    if (e->call.func->field.object->kind == EXPR_IDENT) {
                        Type* obj_type = llvm_get_var_type(lg, e->call.func->field.object->ident);
                        if (obj_type && obj_type->kind == TYPE_ARRAY) {
                            is_array = true;
                        }
                    } else if (e->call.func->field.object->kind == EXPR_ARRAY) {
                        is_array = true;
                    } else if (llvm_is_array_expr(lg, e->call.func->field.object)) {
                        is_array = true;
                    }
                    
                    if (is_array) {
                        // Array contains
                        if (e->call.arg_count > 0) {
                            int item_reg;
                            llvm_expr(lg, e->call.args[0], &item_reg);
                            
                            // Check if item is a string (needs ptrtoint)
                            bool is_string_item = llvm_is_string_expr(lg, e->call.args[0]);
                            if (is_string_item) {
                                int ptr_to_int = llvm_new_temp(lg);
                                llvm_emit(lg, "  %%%d = ptrtoint i8* %%%d to i64", ptr_to_int, item_reg);
                                int t = llvm_new_temp(lg);
                                llvm_emit(lg, "  %%%d = call i64 @array_contains_elem(i64* %%%d, i64 %%%d)", t, obj_reg, ptr_to_int);
                                *result_reg = t;
                            } else if (item_reg <= -1000000) {
                                long long const_val = -item_reg - 1000000;
                                int t = llvm_new_temp(lg);
                                llvm_emit(lg, "  %%%d = call i64 @array_contains_elem(i64* %%%d, i64 %lld)", t, obj_reg, const_val);
                                *result_reg = t;
                            } else {
                                int t = llvm_new_temp(lg);
                                llvm_emit(lg, "  %%%d = call i64 @array_contains_elem(i64* %%%d, i64 %%%d)", t, obj_reg, item_reg);
                                *result_reg = t;
                            }
                        } else {
                            // No argument provided, return 0 (false)
                            int t = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = add i64 0, 0", t);
                            *result_reg = t;
                        }
                    } else {
                        // String contains
                        if (e->call.arg_count > 0) {
                            int arg_reg;
                            llvm_expr(lg, e->call.args[0], &arg_reg);
                            int t = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = call i64 @str_contains(i8* %%%d, i8* %%%d)", t, obj_reg, arg_reg);
                            *result_reg = t;
                        } else {
                            // No argument provided, return 0 (false)
                            int t = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = add i64 0, 0", t);
                            *result_reg = t;
                        }
                    }
                } else if (strcmp(method_name, "starts_with") == 0) {
                    // Handle str.starts_with(prefix)
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int arg_reg;
                    if (e->call.arg_count > 0) {
                        llvm_expr(lg, e->call.args[0], &arg_reg);
                    }
                    
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64 @str_starts_with(i8* %%%d, i8* %%%d)", t, obj_reg, arg_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "ends_with") == 0) {
                    // Handle str.ends_with(suffix)
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int arg_reg;
                    if (e->call.arg_count > 0) {
                        llvm_expr(lg, e->call.args[0], &arg_reg);
                    }
                    
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64 @str_ends_with(i8* %%%d, i8* %%%d)", t, obj_reg, arg_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "split") == 0) {
                    // Handle str.split(sep)
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int sep_reg;
                    if (e->call.arg_count > 0) {
                        llvm_expr(lg, e->call.args[0], &sep_reg);
                    }
                    
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64* @str_split(i8* %%%d, i8* %%%d)", t, obj_reg, sep_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "replace") == 0) {
                    // Handle str.replace(old, new)
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int old_reg, new_reg;
                    if (e->call.arg_count > 0) {
                        llvm_expr(lg, e->call.args[0], &old_reg);
                    }
                    if (e->call.arg_count > 1) {
                        llvm_expr(lg, e->call.args[1], &new_reg);
                    }
                    
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i8* @str_replace(i8* %%%d, i8* %%%d, i8* %%%d)", t, obj_reg, old_reg, new_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "find") == 0) {
                    // Handle str.find(substring)
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int arg_reg;
                    if (e->call.arg_count > 0) {
                        llvm_expr(lg, e->call.args[0], &arg_reg);
                    }
                    
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64 @str_find(i8* %%%d, i8* %%%d)", t, obj_reg, arg_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "substring") == 0) {
                    // Handle str.substring(start, length)
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int start_reg, length_reg;
                    if (e->call.arg_count > 0) {
                        llvm_expr(lg, e->call.args[0], &start_reg);
                    }
                    if (e->call.arg_count > 1) {
                        llvm_expr(lg, e->call.args[1], &length_reg);
                    }
                    
                    int t = llvm_new_temp(lg);
                    if (start_reg <= -1000000 && length_reg <= -1000000) {
                        // Both constants
                        long long start_val = -start_reg - 1000000;
                        long long length_val = -length_reg - 1000000;
                        llvm_emit(lg, "  %%%d = call i8* @str_substring(i8* %%%d, i64 %lld, i64 %lld)", t, obj_reg, start_val, length_val);
                    } else if (start_reg <= -1000000) {
                        // Start is constant
                        long long start_val = -start_reg - 1000000;
                        llvm_emit(lg, "  %%%d = call i8* @str_substring(i8* %%%d, i64 %lld, i64 %%%d)", t, obj_reg, start_val, length_reg);
                    } else if (length_reg <= -1000000) {
                        // Length is constant
                        long long length_val = -length_reg - 1000000;
                        llvm_emit(lg, "  %%%d = call i8* @str_substring(i8* %%%d, i64 %%%d, i64 %lld)", t, obj_reg, start_reg, length_val);
                    } else {
                        // Both are registers
                        llvm_emit(lg, "  %%%d = call i8* @str_substring(i8* %%%d, i64 %%%d, i64 %%%d)", t, obj_reg, start_reg, length_reg);
                    }
                    *result_reg = t;
                } else if (strcmp(method_name, "char_at") == 0) {
                    // Handle str.char_at(index)
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int index_reg;
                    if (e->call.arg_count > 0) {
                        llvm_expr(lg, e->call.args[0], &index_reg);
                    }
                    
                    int t = llvm_new_temp(lg);
                    if (index_reg <= -1000000) {
                        // Index is constant
                        long long index_val = -index_reg - 1000000;
                        llvm_emit(lg, "  %%%d = call i8* @char_at(i8* %%%d, i64 %lld)", t, obj_reg, index_val);
                    } else {
                        // Index is register
                        llvm_emit(lg, "  %%%d = call i8* @char_at(i8* %%%d, i64 %%%d)", t, obj_reg, index_reg);
                    }
                    *result_reg = t;
                } else if (strcmp(method_name, "get") == 0) {
                    // Handle arr.get(index)
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int index_reg;
                    if (e->call.arg_count > 0) {
                        llvm_expr(lg, e->call.args[0], &index_reg);
                    }
                    
                    // Array access: arr[index+2] (skip length and capacity)
                    int offset_reg = llvm_new_temp(lg);
                    if (index_reg <= -1000000) {
                        // Constant index
                        long long const_idx = -index_reg - 1000000;
                        llvm_emit(lg, "  %%%d = add i64 %lld, 2", offset_reg, const_idx);
                    } else {
                        llvm_emit(lg, "  %%%d = add i64 %%%d, 2", offset_reg, index_reg);
                    }
                    
                    int elem_ptr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = getelementptr i64, ptr %%%d, i64 %%%d", elem_ptr, obj_reg, offset_reg);
                    
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64, i64* %%%d", t, elem_ptr);
                    *result_reg = t;
                } else if (strcmp(method_name, "get_str") == 0) {
                    // Handle arr.get_str(index) - for string arrays
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int index_reg;
                    if (e->call.arg_count > 0) {
                        llvm_expr(lg, e->call.args[0], &index_reg);
                    }
                    
                    // Array access: arr[index+2]
                    int offset_reg = llvm_new_temp(lg);
                    if (index_reg <= -1000000) {
                        long long const_idx = -index_reg - 1000000;
                        llvm_emit(lg, "  %%%d = add i64 %lld, 2", offset_reg, const_idx);
                    } else {
                        llvm_emit(lg, "  %%%d = add i64 %%%d, 2", offset_reg, index_reg);
                    }
                    
                    int elem_ptr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = getelementptr i64, ptr %%%d, i64 %%%d", elem_ptr, obj_reg, offset_reg);
                    
                    int i64_val = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64, i64* %%%d", i64_val, elem_ptr);
                    
                    // Cast i64 to i8* for string
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = inttoptr i64 %%%d to i8*", t, i64_val);
                    *result_reg = t;
                } else if (strcmp(method_name, "floor") == 0) {
                    // Handle float.floor()
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64 @floor_wyn(i64 %%%d)", t, obj_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "ceil") == 0) {
                    // Handle float.ceil()
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64 @ceil_wyn(i64 %%%d)", t, obj_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "round") == 0) {
                    // Handle float.round()
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64 @round_wyn(i64 %%%d)", t, obj_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "reverse") == 0) {
                    // Handle arr.reverse()
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64* @array_reverse(i64* %%%d)", t, obj_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "append") == 0) {
                    // Check if this is a StringBuilder append
                    bool is_stringbuilder = false;
                    if (e->call.func->field.object->kind == EXPR_IDENT) {
                        // Simple heuristic: if variable name starts with "sb" or contains "stringbuilder", assume it's a StringBuilder
                        const char* var_name = e->call.func->field.object->ident;
                        if (strncmp(var_name, "sb", 2) == 0 || strstr(var_name, "stringbuilder") != NULL) {
                            is_stringbuilder = true;
                        } else {
                            // Also check the type
                            Type* obj_type = llvm_get_var_type(lg, var_name);
                            if (obj_type && obj_type->kind == TYPE_ANY) {
                                is_stringbuilder = true;
                            }
                        }
                    }
                    
                    if (is_stringbuilder) {
                        // Handle sb.append(str) - convert i64 back to pointer
                        int obj_reg;
                        llvm_expr(lg, e->call.func->field.object, &obj_reg);
                        
                        // Convert i64 back to pointer
                        int ptr_reg = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = inttoptr i64 %%%d to i8*", ptr_reg, obj_reg);
                        
                        // Get string argument
                        int str_reg;
                        if (e->call.arg_count > 0) {
                            llvm_expr(lg, e->call.args[0], &str_reg);
                        }
                        
                        // Call sb_append_wyn(ptr, str)
                        llvm_emit(lg, "  call void @sb_append_wyn(i8* %%%d, i8* %%%d)", ptr_reg, str_reg);
                        
                        // Generate dummy result for void function
                        int t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = add i64 0, 0", t);
                        *result_reg = t;
                    } else {
                        // Handle arr.append(item)
                        int obj_reg;
                        llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int item_reg;
                    if (e->call.arg_count > 0) {
                        llvm_expr(lg, e->call.args[0], &item_reg);
                    }
                    
                    if (item_reg <= -1000000) {
                        long long const_val = -item_reg - 1000000;
                        int t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = call i64* @array_append_elem(i64* %%%d, i64 %lld)", t, obj_reg, const_val);
                        *result_reg = t;
                    } else {
                        // Check if the item is a string (needs to be converted to i64)
                        bool is_string_item = llvm_is_string_expr(lg, e->call.args[0]);
                        if (is_string_item) {
                            // Convert string pointer to i64 for storage in array
                            int ptr_to_int = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = ptrtoint i8* %%%d to i64", ptr_to_int, item_reg);
                            int t = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = call i64* @array_append_elem(i64* %%%d, i64 %%%d)", t, obj_reg, ptr_to_int);
                            *result_reg = t;
                        } else {
                            int t = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = call i64* @array_append_elem(i64* %%%d, i64 %%%d)", t, obj_reg, item_reg);
                            *result_reg = t;
                        }
                    }
                }
                } else if (strcmp(method_name, "prepend") == 0) {
                    // Handle arr.prepend(item)
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int t = llvm_new_temp(lg);
                    if (e->call.arg_count > 0) {
                        int item_reg;
                        llvm_expr(lg, e->call.args[0], &item_reg);
                        
                        if (item_reg <= -1000000) {
                            long long const_val = -item_reg - 1000000;
                            llvm_emit(lg, "  %%%d = call i64* @array_prepend_elem(i64* %%%d, i64 %lld)", t, obj_reg, const_val);
                        } else {
                            llvm_emit(lg, "  %%%d = call i64* @array_prepend_elem(i64* %%%d, i64 %%%d)", t, obj_reg, item_reg);
                        }
                    } else {
                        // No argument provided, return original array
                        llvm_emit(lg, "  %%%d = add i64* %%%d, 0", t, obj_reg);
                    }
                    *result_reg = t;
                } else if (strcmp(method_name, "filter") == 0) {
                    // Handle arr.filter() - simplified version
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64* @array_filter_simple(i64* %%%d)", t, obj_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "map") == 0) {
                    // Handle arr.map() - simplified version
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64* @array_map_simple(i64* %%%d)", t, obj_reg);
                    *result_reg = t;
                } else if (strcmp(method_name, "join") == 0) {
                    // Handle arr.join(sep)
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int sep_reg;
                    if (e->call.arg_count > 0) {
                        llvm_expr(lg, e->call.args[0], &sep_reg);
                    }
                    
                    // Check if this is a string array
                    bool is_string_array = false;
                    if (e->call.func->field.object->kind == EXPR_IDENT) {
                        Type* obj_type = llvm_get_var_type(lg, e->call.func->field.object->ident);
                        is_string_array = obj_type && obj_type->kind == TYPE_ARRAY && 
                                         obj_type->inner && obj_type->inner->kind == TYPE_STR;
                    } else if (e->call.func->field.object->kind == EXPR_ARRAY) {
                        // Check if array literal contains strings
                        if (e->call.func->field.object->array.count > 0) {
                            is_string_array = (e->call.func->field.object->array.elements[0]->kind == EXPR_STRING);
                        }
                    }
                    
                    int t = llvm_new_temp(lg);
                    if (is_string_array) {
                        llvm_emit(lg, "  %%%d = call i8* @array_join_str(i64* %%%d, i8* %%%d)", t, obj_reg, sep_reg);
                    } else {
                        llvm_emit(lg, "  %%%d = call i8* @array_join(i64* %%%d, i8* %%%d)", t, obj_reg, sep_reg);
                    }
                    *result_reg = t;
                } else {
                    // Check for extension methods
                    const char* method_name = e->call.func->field.field;
                    bool found_extension = false;
                    
                    for (int i = 0; i < lg->module->function_count; i++) {
                        FnDef* fn = &lg->module->functions[i];
                        if (fn && fn->is_extension && fn->extension_method && fn->extension_method[0] != '\0' &&
                            strcmp(fn->extension_method, method_name) == 0) {
                            // Call extension method with object as first parameter
                            int obj_reg;
                            llvm_expr(lg, e->call.func->field.object, &obj_reg);
                            
                            // Build argument list
                            char args[2048];
                            bool is_string_obj = llvm_is_string_expr(lg, e->call.func->field.object);
                            
                            if (is_string_obj) {
                                snprintf(args, sizeof(args), "i8* %%%d", obj_reg);
                            } else if (obj_reg <= -1000000) {
                                // Constant object (like 5.squared())
                                snprintf(args, sizeof(args), "i64 %lld", (long long)(-obj_reg - 1000000));
                            } else {
                                snprintf(args, sizeof(args), "i64 %%%d", obj_reg);
                            }
                            
                            // Add remaining arguments
                            for (int j = 0; j < e->call.arg_count; j++) {
                                int arg_reg;
                                llvm_expr(lg, e->call.args[j], &arg_reg);
                                
                                char arg_str[128];
                                bool is_string_arg = llvm_is_string_expr(lg, e->call.args[j]);
                                
                                if (is_string_arg) {
                                    snprintf(arg_str, 128, ", i8* %%%d", arg_reg);
                                } else if (arg_reg <= -1000000) {
                                    snprintf(arg_str, 128, ", i64 %lld", (long long)(-arg_reg - 1000000));
                                } else {
                                    snprintf(arg_str, 128, ", i64 %%%d", arg_reg);
                                }
                                strcat(args, arg_str);
                            }
                            
                            // Determine return type
                            int t = llvm_new_temp(lg);
                            if (fn->return_type && fn->return_type->kind == TYPE_STR) {
                                llvm_emit(lg, "  %%%d = call i8* @%s(%s)", t, fn->name, args);
                            } else if (fn->return_type && fn->return_type->kind == TYPE_ARRAY) {
                                llvm_emit(lg, "  %%%d = call i64* @%s(%s)", t, fn->name, args);
                            } else {
                                llvm_emit(lg, "  %%%d = call i64 @%s(%s)", t, fn->name, args);
                            }
                            *result_reg = t;
                            found_extension = true;
                            break;
                        }
                    }
                    
                    if (!found_extension) {
                        // Unknown method - generate zero register
                        int t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = add i64 0, 0", t);
                        *result_reg = t;
                    }
                }
            } else {
                // Unknown function call - generate zero register
                int t = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = add i64 0, 0", t);
                *result_reg = t;
            }
            break;
        }
        
        case EXPR_ARRAY: {
            // Simple array: allocate space and store elements
            int count = e->array.count;
            int arr_reg = llvm_new_temp(lg);
            
            // Allocate array: [length, capacity, elem0, elem1, ...]
            int size = (count + 2) * 8;  // +2 for length and capacity
            llvm_emit(lg, "  %%%d = alloca i8, i64 %d", arr_reg, size);
            
            // Cast to i64*
            int ptr_reg = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = bitcast i8* %%%d to i64*", ptr_reg, arr_reg);
            
            // Store length
            int len_ptr = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = getelementptr i64, ptr %%%d, i64 0", len_ptr, ptr_reg);
            llvm_emit(lg, "  store i64 %d, i64* %%%d", count, len_ptr);
            
            // Store capacity
            int cap_ptr = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = getelementptr i64, ptr %%%d, i64 1", cap_ptr, ptr_reg);
            llvm_emit(lg, "  store i64 %d, i64* %%%d", count, cap_ptr);
            
            // Store elements
            for (int i = 0; i < count; i++) {
                int elem_reg;
                llvm_expr(lg, e->array.elements[i], &elem_reg);
                
                int elem_ptr = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = getelementptr i64, ptr %%%d, i64 %d", elem_ptr, ptr_reg, i + 2);
                
                // Check if element is a string
                bool is_string_elem = (e->array.elements[i]->kind == EXPR_STRING);
                
                if (elem_reg <= -1000000) {
                    llvm_emit(lg, "  store i64 %lld, i64* %%%d", (long long)(-elem_reg - 1000000), elem_ptr);
                } else if (is_string_elem) {
                    // Cast i8* to i64 for storage
                    int cast_reg = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = ptrtoint i8* %%%d to i64", cast_reg, elem_reg);
                    llvm_emit(lg, "  store i64 %%%d, i64* %%%d", cast_reg, elem_ptr);
                } else {
                    llvm_emit(lg, "  store i64 %%%d, i64* %%%d", elem_reg, elem_ptr);
                }
            }
            
            *result_reg = ptr_reg;
            break;
        }
        
        case EXPR_INDEX: {
            // Check if we're indexing a string
            bool is_string_obj = false;
            if (e->index.object->kind == EXPR_IDENT) {
                Type* obj_type = llvm_get_var_type(lg, e->index.object->ident);
                is_string_obj = obj_type && obj_type->kind == TYPE_STR;
            } else if (e->index.object->kind == EXPR_STRING) {
                is_string_obj = true;
            }
            
            if (is_string_obj) {
                // String indexing: str[idx] -> char_at(str, idx)
                int str_reg, idx_reg;
                llvm_expr(lg, e->index.object, &str_reg);
                llvm_expr(lg, e->index.index, &idx_reg);
                
                int t = llvm_new_temp(lg);
                if (idx_reg <= -1000000) {
                    // Constant index
                    long long idx_val = -idx_reg - 1000000;
                    llvm_emit(lg, "  %%%d = call i8* @char_at(i8* %%%d, i64 %lld)", t, str_reg, idx_val);
                } else {
                    // Variable index
                    llvm_emit(lg, "  %%%d = call i8* @char_at(i8* %%%d, i64 %%%d)", t, str_reg, idx_reg);
                }
                *result_reg = t;
            } else {
                // Array indexing: arr[idx]
                int arr_reg, idx_reg;
                llvm_expr(lg, e->index.object, &arr_reg);
                llvm_expr(lg, e->index.index, &idx_reg);
                
                int final_idx;
                if (idx_reg <= -1000000) {
                    // Constant index
                    long long idx_val = -idx_reg - 1000000;
                    if (idx_val < 0) {
                        // Negative constant index: convert to positive
                        // Load array length
                        int len_ptr = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = getelementptr i64, ptr %%%d, i64 0", len_ptr, arr_reg);
                        int len_reg = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = load i64, i64* %%%d", len_reg, len_ptr);
                        
                        // Calculate positive index: len + idx
                        final_idx = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = add i64 %%%d, %lld", final_idx, len_reg, idx_val);
                    } else {
                        // Positive constant index
                        final_idx = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = add i64 %lld, 0", final_idx, idx_val);
                    }
                } else {
                    // Variable index - need runtime check for negative
                    int len_ptr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = getelementptr i64, ptr %%%d, i64 0", len_ptr, arr_reg);
                    int len_reg = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64, i64* %%%d", len_reg, len_ptr);
                    
                    // Check if index is negative
                    int is_neg = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = icmp slt i64 %%%d, 0", is_neg, idx_reg);
                    
                    // Calculate len + idx for negative case
                    int pos_idx = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = add i64 %%%d, %%%d", pos_idx, len_reg, idx_reg);
                    
                    // Select between original index and positive index
                    final_idx = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = select i1 %%%d, i64 %%%d, i64 %%%d", final_idx, is_neg, pos_idx, idx_reg);
                }
                
                // Add 2 to skip length and capacity
                int adj_idx = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = add i64 %%%d, 2", adj_idx, final_idx);
                
                // Get element pointer
                int elem_ptr = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = getelementptr i64, ptr %%%d, i64 %%%d", elem_ptr, arr_reg, adj_idx);
                
                // Load element
                int t = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = load i64, i64* %%%d", t, elem_ptr);
                
                // Check if this is a string array - need to cast i64 to i8*
                Type* arr_type = NULL;
                if (e->index.object->kind == EXPR_IDENT) {
                    arr_type = llvm_get_var_type(lg, e->index.object->ident);
                }
                
                if (arr_type && arr_type->kind == TYPE_ARRAY && arr_type->inner && arr_type->inner->kind == TYPE_STR) {
                    // Cast i64 to i8* for string arrays
                    int str_reg = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = inttoptr i64 %%%d to i8*", str_reg, t);
                    *result_reg = str_reg;
                } else {
                    *result_reg = t;
                }
            }
            break;
        }
        
        case EXPR_STRUCT: {
            // Struct literal creation: Point { x: 10, y: 20 }
            StructDef* s = llvm_lookup_struct(lg, e->struct_lit.name);
            if (!s) {
                *result_reg = -1;
                break;
            }
            
            // Allocate struct on stack
            int struct_reg = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = alloca %%struct.%s", struct_reg, s->name);
            
            // Initialize fields
            for (int i = 0; i < e->struct_lit.count; i++) {
                int field_offset = llvm_struct_field_offset(s, e->struct_lit.fields[i]);
                if (field_offset >= 0) {
                    // Get field value
                    int val_reg;
                    llvm_expr(lg, e->struct_lit.values[i], &val_reg);
                    
                    // Get field pointer
                    int field_ptr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = getelementptr %%struct.%s, %%struct.%s* %%%d, i32 0, i32 %d", 
                             field_ptr, s->name, s->name, struct_reg, field_offset);
                    
                    // Store value
                    if (val_reg <= -1000000) {
                        llvm_emit(lg, "  store i64 %lld, i64* %%%d", (long long)(-val_reg - 1000000), field_ptr);
                    } else {
                        llvm_emit(lg, "  store i64 %%%d, i64* %%%d", val_reg, field_ptr);
                    }
                }
            }
            
            *result_reg = struct_reg;
            break;
        }
        
        case EXPR_FIELD: {
            // Field access: obj.field
            int obj_reg;
            llvm_expr(lg, e->field.object, &obj_reg);
            
            // Find struct type - for now assume it's from variable name
            StructDef* s = NULL;
            if (e->field.object->kind == EXPR_IDENT) {
                // Look through all structs to find one with this field
                for (int i = 0; i < lg->module->struct_count; i++) {
                    for (int j = 0; j < lg->module->structs[i].field_count; j++) {
                        if (strcmp(lg->module->structs[i].fields[j].name, e->field.field) == 0) {
                            s = &lg->module->structs[i];
                            break;
                        }
                    }
                    if (s) break;
                }
            }
            
            if (s) {
                int field_offset = llvm_struct_field_offset(s, e->field.field);
                if (field_offset >= 0) {
                    // Get field pointer
                    int field_ptr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = getelementptr %%struct.%s, %%struct.%s* %%%d, i32 0, i32 %d", 
                             field_ptr, s->name, s->name, obj_reg, field_offset);
                    
                    // Load field value
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64, i64* %%%d", t, field_ptr);
                    *result_reg = t;
                } else {
                    *result_reg = -1;
                }
            } else {
                // Unknown struct - generate zero register
                int t = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = add i64 0, 0", t);
                *result_reg = t;
            }
            break;
        }
        
        case EXPR_SLICE: {
            // String/array slicing: obj[start:end]
            int obj_reg;
            llvm_expr(lg, e->slice.object, &obj_reg);
            
            int start_reg;
            if (e->slice.start) {
                llvm_expr(lg, e->slice.start, &start_reg);
            } else {
                start_reg = -1000000;  // 0 constant
            }
            
            int end_reg;
            if (e->slice.end) {
                llvm_expr(lg, e->slice.end, &end_reg);
            } else {
                end_reg = -2000000;  // 1000000 constant (large number, will be clamped to string length)
            }
            
            // Determine if this is string or array slicing
            Type* obj_type = NULL;
            if (e->slice.object->kind == EXPR_IDENT) {
                obj_type = llvm_get_var_type(lg, e->slice.object->ident);
            } else if (e->slice.object->kind == EXPR_STRING) {
                obj_type = new_type(TYPE_STR);
            }
            
            int t = llvm_new_temp(lg);
            if (obj_type && obj_type->kind == TYPE_ARRAY) {
                // Array slicing
                char start_str[32], end_str[32];
                if (start_reg <= -1000000) {
                    snprintf(start_str, 32, "i64 %lld", (long long)(-start_reg - 1000000));
                } else {
                    snprintf(start_str, 32, "i64 %%%d", start_reg);
                }
                if (end_reg <= -1000000) {
                    snprintf(end_str, 32, "i64 %lld", (long long)(-end_reg - 1000000));
                } else {
                    snprintf(end_str, 32, "i64 %%%d", end_reg);
                }
                llvm_emit(lg, "  %%%d = call i64* @array_slice(i64* %%%d, %s, %s)", t, obj_reg, start_str, end_str);
            } else {
                // String slicing
                char start_str[32], end_str[32];
                if (start_reg <= -1000000) {
                    snprintf(start_str, 32, "i64 %lld", (long long)(-start_reg - 1000000));
                } else {
                    snprintf(start_str, 32, "i64 %%%d", start_reg);
                }
                if (end_reg <= -1000000) {
                    snprintf(end_str, 32, "i64 %lld", (long long)(-end_reg - 1000000));
                } else {
                    snprintf(end_str, 32, "i64 %%%d", end_reg);
                }
                llvm_emit(lg, "  %%%d = call i8* @str_slice(i8* %%%d, %s, %s)", t, obj_reg, start_str, end_str);
                // Convert i8* to i64 for storage compatibility
                int cast_reg = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = ptrtoint i8* %%%d to i64", cast_reg, t);
                t = cast_reg;
            }
            
            *result_reg = t;
            break;
        }
        
        case EXPR_AWAIT: {
            // Await expression: await future
            // Evaluate the future expression
            int future_reg;
            llvm_expr(lg, e->await_expr.future, &future_reg);
            
            // Call __wyn_await to wait for result
            int t = llvm_new_temp(lg);
            if (future_reg <= -1000000) {
                llvm_emit(lg, "  %%%d = call i64 @__wyn_await(i64 %lld)", t, (long long)(-future_reg - 1000000));
            } else {
                llvm_emit(lg, "  %%%d = call i64 @__wyn_await(i64 %%%d)", t, future_reg);
            }
            *result_reg = t;
            break;
        }
        
        case EXPR_TERNARY: {
            // Ternary operator: cond ? true_val : false_val
            int cond_reg;
            llvm_expr(lg, e->ternary.cond, &cond_reg);
            
            // Create labels
            int label_true = lg->label_count++;
            int label_false = lg->label_count++;
            int label_end = lg->label_count++;
            
            // Check if condition is already i1 (from comparison) or needs conversion
            bool cond_is_comparison = (e->ternary.cond->kind == EXPR_BINARY &&
                                      (e->ternary.cond->binary.op == TOK_EQEQ ||
                                       e->ternary.cond->binary.op == TOK_NOTEQ ||
                                       e->ternary.cond->binary.op == TOK_LT ||
                                       e->ternary.cond->binary.op == TOK_GT ||
                                       e->ternary.cond->binary.op == TOK_LTEQ ||
                                       e->ternary.cond->binary.op == TOK_GTEQ));
            
            // Branch on condition
            if (cond_reg <= -1000000) {
                int cond_bool = (-cond_reg - 1000000) != 0 ? 1 : 0;
                llvm_emit(lg, "  br i1 %d, label %%L%d, label %%L%d", cond_bool, label_true, label_false);
            } else if (cond_is_comparison) {
                // Condition is already i1, use directly
                llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cond_reg, label_true, label_false);
            } else {
                // Condition is i64, convert to i1
                int cond_i1 = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = icmp ne i64 %%%d, 0", cond_i1, cond_reg);
                llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cond_i1, label_true, label_false);
            }
            
            // True branch
            llvm_emit(lg, "L%d:", label_true);
            int true_reg;
            llvm_expr(lg, e->ternary.true_val, &true_reg);
            llvm_emit(lg, "  br label %%L%d", label_end);
            
            // False branch
            llvm_emit(lg, "L%d:", label_false);
            int false_reg;
            llvm_expr(lg, e->ternary.false_val, &false_reg);
            llvm_emit(lg, "  br label %%L%d", label_end);
            
            // End label with phi node
            llvm_emit(lg, "L%d:", label_end);
            int t = llvm_new_temp(lg);
            
            // Determine phi type from true/false values
            bool is_string_result = llvm_is_string_expr(lg, e->ternary.true_val);
            const char* phi_type = is_string_result ? "i8*" : "i64";
            
            char true_str[32], false_str[32];
            if (true_reg <= -1000000) {
                snprintf(true_str, 32, "%lld", (long long)(-true_reg - 1000000));
            } else {
                snprintf(true_str, 32, "%%%d", true_reg);
            }
            if (false_reg <= -1000000) {
                snprintf(false_str, 32, "%lld", (long long)(-false_reg - 1000000));
            } else {
                snprintf(false_str, 32, "%%%d", false_reg);
            }
            
            llvm_emit(lg, "  %%%d = phi %s [ %s, %%L%d ], [ %s, %%L%d ]", 
                     t, phi_type, true_str, label_true, false_str, label_false);
            *result_reg = t;
            break;
        }
        
        case EXPR_COMPREHENSION: {
            // List comprehension: [expr for var in iter if condition]
            
            // Create empty array
            int result_arr = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = call i64* @array_new()", result_arr);
            
            // Allocate pointer to track current array
            int arr_ptr = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = alloca i64*", arr_ptr);
            llvm_emit(lg, "  store i64* %%%d, i64** %%%d", result_arr, arr_ptr);
            
            // Check if iterator is a range
            bool is_range = e->comprehension.iter->kind == EXPR_BINARY &&
                           (e->comprehension.iter->binary.op == TOK_DOTDOT || 
                            e->comprehension.iter->binary.op == TOK_DOTDOTEQ);
            
            if (is_range) {
                // Range comprehension: [expr for var in start..end]
                bool inclusive = e->comprehension.iter->binary.op == TOK_DOTDOTEQ;
                
                // Get start and end values
                int start_val, end_val;
                llvm_expr(lg, e->comprehension.iter->binary.left, &start_val);
                llvm_expr(lg, e->comprehension.iter->binary.right, &end_val);
                
                // Loop variable
                int loop_var = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = alloca i64", loop_var);
                
                // Initialize loop variable with start
                if (start_val <= -1000000) {
                    llvm_emit(lg, "  store i64 %lld, i64* %%%d", (long long)(-start_val - 1000000), loop_var);
                } else {
                    llvm_emit(lg, "  store i64 %%%d, i64* %%%d", start_val, loop_var);
                }
                
                // Labels
                int loop_start = lg->label_count++;
                int loop_body = lg->label_count++;
                int loop_end = lg->label_count++;
                
                llvm_emit(lg, "  br label %%L%d", loop_start);
                llvm_emit(lg, "L%d:", loop_start);
                
                // Check condition
                int curr_val = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = load i64, i64* %%%d", curr_val, loop_var);
                
                int loop_cond = llvm_new_temp(lg);
                if (inclusive) {
                    if (end_val <= -1000000) {
                        llvm_emit(lg, "  %%%d = icmp sle i64 %%%d, %lld", loop_cond, curr_val, (long long)(-end_val - 1000000));
                    } else {
                        llvm_emit(lg, "  %%%d = icmp sle i64 %%%d, %%%d", loop_cond, curr_val, end_val);
                    }
                } else {
                    if (end_val <= -1000000) {
                        llvm_emit(lg, "  %%%d = icmp slt i64 %%%d, %lld", loop_cond, curr_val, (long long)(-end_val - 1000000));
                    } else {
                        llvm_emit(lg, "  %%%d = icmp slt i64 %%%d, %%%d", loop_cond, curr_val, end_val);
                    }
                }
                llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", loop_cond, loop_body, loop_end);
                
                llvm_emit(lg, "L%d:", loop_body);
                
                // Add loop variable to scope
                int saved_var_count = lg->var_count;
                llvm_add_var(lg, e->comprehension.var, loop_var, new_type(TYPE_INT));
                
                // Handle filter
                if (e->comprehension.condition) {
                    int filter_reg;
                    llvm_expr(lg, e->comprehension.condition, &filter_reg);
                    
                    int append_label = lg->label_count++;
                    int skip_label = lg->label_count++;
                    
                    bool is_comparison = (e->comprehension.condition->kind == EXPR_BINARY);
                    if (is_comparison) {
                        llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", filter_reg, append_label, skip_label);
                    } else {
                        int cond_i1 = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = icmp ne i64 %%%d, 0", cond_i1, filter_reg);
                        llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cond_i1, append_label, skip_label);
                    }
                    
                    llvm_emit(lg, "L%d:", append_label);
                    
                    // Evaluate expression
                    int expr_reg;
                    llvm_expr(lg, e->comprehension.expr, &expr_reg);
                    
                    // Load current array, append, store back
                    int curr_arr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64*, i64** %%%d", curr_arr, arr_ptr);
                    int new_arr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64* @array_append_elem(i64* %%%d, i64 %%%d)", new_arr, curr_arr, expr_reg);
                    llvm_emit(lg, "  store i64* %%%d, i64** %%%d", new_arr, arr_ptr);
                    
                    llvm_emit(lg, "  br label %%L%d", skip_label);
                    llvm_emit(lg, "L%d:", skip_label);
                } else {
                    // No filter
                    int expr_reg;
                    llvm_expr(lg, e->comprehension.expr, &expr_reg);
                    
                    int curr_arr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64*, i64** %%%d", curr_arr, arr_ptr);
                    int new_arr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64* @array_append_elem(i64* %%%d, i64 %%%d)", new_arr, curr_arr, expr_reg);
                    llvm_emit(lg, "  store i64* %%%d, i64** %%%d", new_arr, arr_ptr);
                }
                
                // Remove loop variable
                lg->var_count = saved_var_count;
                
                // Increment
                int next_val = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = add i64 %%%d, 1", next_val, curr_val);
                llvm_emit(lg, "  store i64 %%%d, i64* %%%d", next_val, loop_var);
                llvm_emit(lg, "  br label %%L%d", loop_start);
                
                // End
                llvm_emit(lg, "L%d:", loop_end);
                
            } else {
                // Array comprehension: [expr for var in array]
                int iter_reg;
                llvm_expr(lg, e->comprehension.iter, &iter_reg);
                
                // Get length
                int len_ptr = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = getelementptr i64, ptr %%%d, i64 0", len_ptr, iter_reg);
                int len_val = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = load i64, i64* %%%d", len_val, len_ptr);
                
                // Loop index
                int loop_idx = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = alloca i64", loop_idx);
                llvm_emit(lg, "  store i64 0, i64* %%%d", loop_idx);
                
                // Labels
                int loop_start = lg->label_count++;
                int loop_body = lg->label_count++;
                int loop_end = lg->label_count++;
                
                llvm_emit(lg, "  br label %%L%d", loop_start);
                llvm_emit(lg, "L%d:", loop_start);
                
                // Check condition
                int i_val = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = load i64, i64* %%%d", i_val, loop_idx);
                int loop_cond = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = icmp slt i64 %%%d, %%%d", loop_cond, i_val, len_val);
                llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", loop_cond, loop_body, loop_end);
                
                llvm_emit(lg, "L%d:", loop_body);
                
                // Get element
                int idx_plus_2 = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = add i64 %%%d, 2", idx_plus_2, i_val);
                int elem_ptr = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = getelementptr i64, ptr %%%d, i64 %%%d", elem_ptr, iter_reg, idx_plus_2);
                int elem_val = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = load i64, i64* %%%d", elem_val, elem_ptr);
                
                // Add loop variable
                int var_alloca = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = alloca i64", var_alloca);
                llvm_emit(lg, "  store i64 %%%d, i64* %%%d", elem_val, var_alloca);
                int saved_var_count = lg->var_count;
                llvm_add_var(lg, e->comprehension.var, var_alloca, new_type(TYPE_INT));
                
                // Handle filter
                if (e->comprehension.condition) {
                    int filter_reg;
                    llvm_expr(lg, e->comprehension.condition, &filter_reg);
                    
                    int append_label = lg->label_count++;
                    int skip_label = lg->label_count++;
                    
                    bool is_comparison = (e->comprehension.condition->kind == EXPR_BINARY);
                    if (is_comparison) {
                        llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", filter_reg, append_label, skip_label);
                    } else {
                        int cond_i1 = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = icmp ne i64 %%%d, 0", cond_i1, filter_reg);
                        llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cond_i1, append_label, skip_label);
                    }
                    
                    llvm_emit(lg, "L%d:", append_label);
                    
                    // Evaluate expression
                    int expr_reg;
                    llvm_expr(lg, e->comprehension.expr, &expr_reg);
                    
                    // Load current array, append, store back
                    int curr_arr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64*, i64** %%%d", curr_arr, arr_ptr);
                    int new_arr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64* @array_append_elem(i64* %%%d, i64 %%%d)", new_arr, curr_arr, expr_reg);
                    llvm_emit(lg, "  store i64* %%%d, i64** %%%d", new_arr, arr_ptr);
                    
                    llvm_emit(lg, "  br label %%L%d", skip_label);
                    llvm_emit(lg, "L%d:", skip_label);
                } else {
                    // No filter
                    int expr_reg;
                    llvm_expr(lg, e->comprehension.expr, &expr_reg);
                    
                    int curr_arr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64*, i64** %%%d", curr_arr, arr_ptr);
                    int new_arr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64* @array_append_elem(i64* %%%d, i64 %%%d)", new_arr, curr_arr, expr_reg);
                    llvm_emit(lg, "  store i64* %%%d, i64** %%%d", new_arr, arr_ptr);
                }
                
                // Remove loop variable
                lg->var_count = saved_var_count;
                
                // Increment
                int next_i = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = add i64 %%%d, 1", next_i, i_val);
                llvm_emit(lg, "  store i64 %%%d, i64* %%%d", next_i, loop_idx);
                llvm_emit(lg, "  br label %%L%d", loop_start);
                
                // End
                llvm_emit(lg, "L%d:", loop_end);
            }
            
            // Load final result
            int final_arr = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = load i64*, i64** %%%d", final_arr, arr_ptr);
            
            *result_reg = final_arr;
            break;
        }
        
        case EXPR_LAMBDA: {
            // Lambda expression: store for later emission, return function pointer
            lg->lambdas[lg->lambda_count++] = e;
            int t = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = ptrtoint i64 (", t);
            // Build parameter types
            for (int i = 0; i < e->lambda.param_count; i++) {
                if (i > 0) llvm_emit_raw(lg, ", ");
                llvm_emit_raw(lg, "i64");
            }
            llvm_emit_raw(lg, ")* @__lambda_%d to i64\n", e->lambda.id);
            *result_reg = t;
            break;
        }
        
        case EXPR_MATCH: {
            // Match expression: match value { pattern => expr, ... }
            int end_label = llvm_new_label(lg);
            int result_alloca = llvm_new_temp(lg);
            
            // Allocate result variable
            llvm_emit(lg, "  %%%d = alloca i64", result_alloca);
            
            // Evaluate match value once
            int match_reg;
            llvm_expr(lg, e->match_expr.value, &match_reg);
            
            for (int i = 0; i < e->match_expr.arm_count; i++) {
                int next_label = (i < e->match_expr.arm_count - 1) ? llvm_new_label(lg) : end_label;
                Expr* pat = e->match_expr.patterns[i];
                
                // Check if this is a wildcard pattern
                bool is_wildcard = pat->kind == EXPR_IDENT && strcmp(pat->ident, "_") == 0;
                
                // Check if this is a binding pattern (ok(v), err(msg), some(v))
                bool is_binding_pattern = (pat->kind == EXPR_SOME || pat->kind == EXPR_OK || pat->kind == EXPR_ERR) &&
                                         pat->some.value && pat->some.value->kind == EXPR_IDENT;
                
                if (!is_wildcard) {
                    int arm_label = llvm_new_label(lg);
                    
                    if (is_binding_pattern) {
                        // For binding patterns, we need to check the tag and extract the value
                        if (pat->kind == EXPR_OK) {
                            // Check if match value represents "ok" (non-negative for now)
                            int cmp_reg = llvm_new_temp(lg);
                            if (match_reg <= -1000000) {
                                long long match_val = -match_reg - 1000000;
                                if (match_val >= 0) {
                                    llvm_emit(lg, "  br label %%L%d", arm_label);
                                } else {
                                    llvm_emit(lg, "  br label %%L%d", next_label);
                                    continue;
                                }
                            } else {
                                llvm_emit(lg, "  %%%d = icmp sge i64 %%%d, 0", cmp_reg, match_reg);
                                llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp_reg, arm_label, next_label);
                            }
                        } else if (pat->kind == EXPR_ERR) {
                            // Check if match value represents "err" (negative for now)
                            int cmp_reg = llvm_new_temp(lg);
                            if (match_reg <= -1000000) {
                                long long match_val = -match_reg - 1000000;
                                if (match_val < 0) {
                                    llvm_emit(lg, "  br label %%L%d", arm_label);
                                } else {
                                    llvm_emit(lg, "  br label %%L%d", next_label);
                                    continue;
                                }
                            } else {
                                llvm_emit(lg, "  %%%d = icmp slt i64 %%%d, 0", cmp_reg, match_reg);
                                llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp_reg, arm_label, next_label);
                            }
                        } else if (pat->kind == EXPR_SOME) {
                            // For Option types, check if not none (non-zero for now)
                            int cmp_reg = llvm_new_temp(lg);
                            if (match_reg <= -1000000) {
                                long long match_val = -match_reg - 1000000;
                                if (match_val != 0) {
                                    llvm_emit(lg, "  br label %%L%d", arm_label);
                                } else {
                                    llvm_emit(lg, "  br label %%L%d", next_label);
                                    continue;
                                }
                            } else {
                                llvm_emit(lg, "  %%%d = icmp ne i64 %%%d, 0", cmp_reg, match_reg);
                                llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp_reg, arm_label, next_label);
                            }
                        }
                    } else {
                        // Regular pattern matching
                        int pat_reg;
                        llvm_expr(lg, pat, &pat_reg);
                        
                        int cmp_reg = llvm_new_temp(lg);
                        
                        if (match_reg <= -1000000 && pat_reg <= -1000000) {
                            // Both constants
                            long long match_val = -match_reg - 1000000;
                            long long pat_val = -pat_reg - 1000000;
                            if (match_val == pat_val) {
                                llvm_emit(lg, "  br label %%L%d", arm_label);
                            } else {
                                llvm_emit(lg, "  br label %%L%d", next_label);
                                continue;
                            }
                        } else if (match_reg <= -1000000) {
                            // Match value is constant
                            llvm_emit(lg, "  %%%d = icmp eq i64 %%%d, %lld", cmp_reg, pat_reg, (long long)(-match_reg - 1000000));
                            llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp_reg, arm_label, next_label);
                        } else if (pat_reg <= -1000000) {
                            // Pattern is constant
                            llvm_emit(lg, "  %%%d = icmp eq i64 %%%d, %lld", cmp_reg, match_reg, (long long)(-pat_reg - 1000000));
                            llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp_reg, arm_label, next_label);
                        } else {
                            // Both are registers
                            llvm_emit(lg, "  %%%d = icmp eq i64 %%%d, %%%d", cmp_reg, match_reg, pat_reg);
                            llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp_reg, arm_label, next_label);
                        }
                    }
                    
                    llvm_emit(lg, "L%d:", arm_label);
                    
                    // Add variable binding if present
                    if (e->match_expr.bindings[i][0] != '\0') {
                        // Create variable for the bound value
                        int var_reg = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = alloca i64", var_reg);
                        
                        // For now, just store the match value directly
                        if (match_reg <= -1000000) {
                            llvm_emit(lg, "  store i64 %lld, i64* %%%d", (long long)(-match_reg - 1000000), var_reg);
                        } else {
                            llvm_emit(lg, "  store i64 %%%d, i64* %%%d", match_reg, var_reg);
                        }
                        
                        // Add variable to scope
                        llvm_add_var(lg, e->match_expr.bindings[i], var_reg, new_type(TYPE_INT));
                    }
                } else {
                    // Wildcard matches everything - no comparison needed
                }
                
                // Evaluate arm expression and store result
                int arm_reg;
                llvm_expr(lg, e->match_expr.arms[i], &arm_reg);
                
                if (arm_reg <= -1000000) {
                    llvm_emit(lg, "  store i64 %lld, i64* %%%d", (long long)(-arm_reg - 1000000), result_alloca);
                } else {
                    llvm_emit(lg, "  store i64 %%%d, i64* %%%d", arm_reg, result_alloca);
                }
                llvm_emit(lg, "  br label %%L%d", end_label);
                
                // Emit next label only if not the last iteration
                if (i < e->match_expr.arm_count - 1) {
                    llvm_emit(lg, "L%d:", next_label);
                }
            }
            
            // End label
            llvm_emit(lg, "L%d:", end_label);
            
            // Load final result
            int final_reg = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = load i64, i64* %%%d", final_reg, result_alloca);
            *result_reg = final_reg;
            break;
        }
        
        case EXPR_UNWRAP: {
            // Unwrap operator: expr!
            // Calls result_unwrap_simple to extract value from Result
            int operand_reg;
            llvm_expr(lg, e->unary.operand, &operand_reg);
            
            int t = llvm_new_temp(lg);
            if (operand_reg <= -1000000) {
                // Constant value - just return it
                llvm_emit(lg, "  %%%d = add i64 0, %lld", t, (long long)(-operand_reg - 1000000));
            } else {
                // Call unwrap function
                llvm_emit(lg, "  %%%d = call i64 @result_unwrap_simple(i64 %%%d)", t, operand_reg);
            }
            *result_reg = t;
            break;
        }
        
        case EXPR_OK: {
            // ok(value) - create Result with Ok status (simple encoding)
            int value_reg;
            llvm_expr(lg, e->some.value, &value_reg);
            
            int t = llvm_new_temp(lg);
            char value_str[64];
            if (value_reg <= -1000000) {
                snprintf(value_str, 64, "i64 %lld", (long long)(-value_reg - 1000000));
            } else {
                snprintf(value_str, 64, "i64 %%%d", value_reg);
            }
            llvm_emit(lg, "  %%%d = call i64 @result_ok_simple(%s)", t, value_str);
            *result_reg = t;
            break;
        }
        
        case EXPR_ERR: {
            // err(value) - create Result with Err status (simple encoding)
            int value_reg;
            llvm_expr(lg, e->some.value, &value_reg);
            
            int t = llvm_new_temp(lg);
            
            // For string errors, use the negative of the string ID as error code
            if (e->some.value->kind == EXPR_STRING) {
                int str_id = string_table_find(lg->strings, e->some.value->str_val);
                llvm_emit(lg, "  %%%d = call i64 @result_err_simple(i64 %d)", t, -str_id);
            } else {
                // Check if this is a string expression that results in a pointer
                bool is_string_expr = llvm_is_string_expr(lg, e->some.value);
                
                if (is_string_expr) {
                    // Convert string pointer to integer for simple Result encoding
                    int ptr_to_int = llvm_new_temp(lg);
                    if (value_reg <= -1000000) {
                        llvm_emit(lg, "  %%%d = add i64 0, %lld", ptr_to_int, (long long)(-value_reg - 1000000));
                    } else {
                        llvm_emit(lg, "  %%%d = ptrtoint i8* %%%d to i64", ptr_to_int, value_reg);
                    }
                    llvm_emit(lg, "  %%%d = call i64 @result_err_simple(i64 %%%d)", t, ptr_to_int);
                } else {
                    // Integer error code
                    char value_str[64];
                    if (value_reg <= -1000000) {
                        snprintf(value_str, 64, "i64 %lld", (long long)(-value_reg - 1000000));
                    } else {
                        snprintf(value_str, 64, "i64 %%%d", value_reg);
                    }
                    llvm_emit(lg, "  %%%d = call i64 @result_err_simple(%s)", t, value_str);
                }
            }
            *result_reg = t;
            break;
        }
        
        case EXPR_SOME: {
            // some(value) - create Option with Some status (simple encoding)
            int value_reg;
            llvm_expr(lg, e->some.value, &value_reg);
            
            // For string literals, use the string ID as the value
            if (e->some.value->kind == EXPR_STRING) {
                int str_id = string_table_find(lg->strings, e->some.value->str_val);
                int t = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = call i64 @option_some_simple(i64 %d)", t, str_id);
                *result_reg = t;
            } else {
                // For other values, check if it's a string expression and convert pointer to int
                bool is_string_expr = llvm_is_string_expr(lg, e->some.value);
                if (is_string_expr) {
                    // Convert pointer to integer
                    int ptr_to_int_reg = llvm_new_temp(lg);
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = ptrtoint i8* %%%d to i64", ptr_to_int_reg, value_reg);
                    llvm_emit(lg, "  %%%d = call i64 @option_some_simple(i64 %%%d)", t, ptr_to_int_reg);
                    *result_reg = t;
                } else {
                    int t = llvm_new_temp(lg);
                    char value_str[64];
                    if (value_reg <= -1000000) {
                        snprintf(value_str, 64, "i64 %lld", (long long)(-value_reg - 1000000));
                    } else {
                        snprintf(value_str, 64, "i64 %%%d", value_reg);
                    }
                    llvm_emit(lg, "  %%%d = call i64 @option_some_simple(%s)", t, value_str);
                    *result_reg = t;
                }
            }
            break;
        }
        
        case EXPR_NONE: {
            // none - create Option with None status (simple encoding)
            int t = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = call i64 @option_none_simple()", t);
            *result_reg = t;
            break;
        }
        
        default:
            // Unhandled expression type - generate zero register
            {
                int t = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = add i64 0, 0", t);
                *result_reg = t;
            }
            break;
    }
}

static void llvm_stmt(LLVMGen* lg, Stmt* s) {
    switch (s->kind) {
        case STMT_LET: {
            int alloca_reg = llvm_new_temp(lg);
            
            // Check if this is a struct variable
            bool is_struct_var = false;
            StructDef* struct_def = NULL;
            
            // Check explicit type annotation
            if (s->let.type && s->let.type->kind == TYPE_NAMED) {
                struct_def = llvm_lookup_struct(lg, s->let.type->name);
                is_struct_var = (struct_def != NULL);
            }
            
            // Check inferred type from struct literal
            if (!is_struct_var && s->let.value && s->let.value->kind == EXPR_STRUCT) {
                struct_def = llvm_lookup_struct(lg, s->let.value->struct_lit.name);
                is_struct_var = (struct_def != NULL);
            }
            
            // Check if this is a float variable
            bool is_float_var = (s->let.type && s->let.type->kind == TYPE_FLOAT) ||
                               (s->let.value && llvm_is_float_expr(lg, s->let.value));
            
            // Check if this is an array variable
            bool is_array_var = (s->let.type && s->let.type->kind == TYPE_ARRAY) ||
                               (s->let.value && llvm_is_array_expr(lg, s->let.value));
            
            // Check if this is a string variable
            bool is_string_var = (s->let.type && s->let.type->kind == TYPE_STR) ||
                                (s->let.value && llvm_is_string_expr(lg, s->let.value));
            
            if (is_struct_var) {
                llvm_emit(lg, "  %%%d = alloca %%struct.%s", alloca_reg, struct_def->name);
            } else if (is_float_var) {
                llvm_emit(lg, "  %%%d = alloca double", alloca_reg);
            } else if (is_array_var) {
                llvm_emit(lg, "  %%%d = alloca i64*", alloca_reg);
            } else if (is_string_var) {
                llvm_emit(lg, "  %%%d = alloca i8*", alloca_reg);
            } else {
                llvm_emit(lg, "  %%%d = alloca i64", alloca_reg);
            }
            
            // Infer type from value if no explicit type annotation
            Type* var_type = s->let.type;
            if (!var_type && s->let.value) {
                if (is_string_var) {
                    var_type = new_type(TYPE_STR);
                } else if (is_array_var) {
                    var_type = new_type(TYPE_ARRAY);
                    // Infer element type from array literal
                    if (s->let.value->kind == EXPR_ARRAY && s->let.value->array.count > 0) {
                        if (s->let.value->array.elements[0]->kind == EXPR_STRING) {
                            var_type->inner = new_type(TYPE_STR);
                        } else {
                            var_type->inner = new_type(TYPE_INT);
                        }
                    }
                } else if (is_float_var) {
                    var_type = new_type(TYPE_FLOAT);
                } else {
                    var_type = new_type(TYPE_INT);
                }
            }
            
            llvm_add_var(lg, s->let.name, alloca_reg, var_type);
            
            if (s->let.value) {
                if (is_struct_var && s->let.value->kind == EXPR_STRUCT) {
                    // Handle struct literal assignment - initialize fields directly in the allocated struct
                    for (int i = 0; i < s->let.value->struct_lit.count; i++) {
                        int field_offset = llvm_struct_field_offset(struct_def, s->let.value->struct_lit.fields[i]);
                        if (field_offset >= 0) {
                            int val_reg;
                            llvm_expr(lg, s->let.value->struct_lit.values[i], &val_reg);
                            
                            int field_ptr = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = getelementptr %%struct.%s, %%struct.%s* %%%d, i32 0, i32 %d", 
                                     field_ptr, struct_def->name, struct_def->name, alloca_reg, field_offset);
                            
                            if (val_reg <= -1000000) {
                                llvm_emit(lg, "  store i64 %lld, i64* %%%d", (long long)(-val_reg - 1000000), field_ptr);
                            } else {
                                llvm_emit(lg, "  store i64 %%%d, i64* %%%d", val_reg, field_ptr);
                            }
                        }
                    }
                } else {
                    // Regular variable assignment (for both struct and non-struct variables)
                    int val_reg;
                    llvm_expr(lg, s->let.value, &val_reg);
                    if (is_float_var) {
                        if (val_reg <= -1000000) {
                            int conv_reg = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = sitofp i64 %lld to double", conv_reg, (long long)(-val_reg - 1000000));
                            llvm_emit(lg, "  store double %%%d, double* %%%d", conv_reg, alloca_reg);
                        } else {
                            llvm_emit(lg, "  store double %%%d, double* %%%d", val_reg, alloca_reg);
                        }
                    } else if (is_array_var) {
                        llvm_emit(lg, "  store i64* %%%d, i64** %%%d", val_reg, alloca_reg);
                    } else if (is_string_var) {
                        // Check if value is from array indexing or string slicing (i64 that needs cast to i8*)
                        if (s->let.value->kind == EXPR_INDEX || 
                            (s->let.value->kind == EXPR_SLICE && 
                             ((s->let.value->slice.object->kind == EXPR_STRING) ||
                              (s->let.value->slice.object->kind == EXPR_IDENT && 
                               llvm_get_var_type(lg, s->let.value->slice.object->ident) &&
                               llvm_get_var_type(lg, s->let.value->slice.object->ident)->kind == TYPE_STR)))) {
                            // Cast i64 to i8* for string array elements and sliced strings
                            int cast_reg = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = inttoptr i64 %%%d to i8*", cast_reg, val_reg);
                            llvm_emit(lg, "  store i8* %%%d, i8** %%%d", cast_reg, alloca_reg);
                        } else {
                            llvm_emit(lg, "  store i8* %%%d, i8** %%%d", val_reg, alloca_reg);
                        }
                    } else {
                        if (val_reg <= -1000000) {
                            llvm_emit(lg, "  store i64 %lld, i64* %%%d", (long long)(-val_reg - 1000000), alloca_reg);
                        } else {
                            // For non-constant values, we need to handle potential i1 to i64 conversion
                            // Check if this is a comparison that returns i1
                            bool needs_bool_conv = (s->let.value->kind == EXPR_BINARY && 
                                                   (s->let.value->binary.op == TOK_EQEQ || 
                                                    s->let.value->binary.op == TOK_NOTEQ ||
                                                    s->let.value->binary.op == TOK_LT ||
                                                    s->let.value->binary.op == TOK_GT ||
                                                    s->let.value->binary.op == TOK_LTEQ ||
                                                    s->let.value->binary.op == TOK_GTEQ ||
                                                    s->let.value->binary.op == TOK_IN));
                            
                            if (needs_bool_conv) {
                                // Convert i1 to i64
                                int conv_reg = llvm_new_temp(lg);
                                llvm_emit(lg, "  %%%d = zext i1 %%%d to i64", conv_reg, val_reg);
                                llvm_emit(lg, "  store i64 %%%d, i64* %%%d", conv_reg, alloca_reg);
                            } else {
                                llvm_emit(lg, "  store i64 %%%d, i64* %%%d", val_reg, alloca_reg);
                            }
                        }
                    }
                }
            }
            break;
        }
        
        case STMT_ASSIGN: {
            if (s->assign.target->kind == EXPR_IDENT) {
                // Regular variable assignment
                int var_reg = llvm_find_var(lg, s->assign.target->ident);
                if (var_reg >= 0) {
                    // Check if this is likely a float variable
                    bool is_float_var = strstr(s->assign.target->ident, "float") != NULL || 
                                       (strlen(s->assign.target->ident) > 0 && s->assign.target->ident[strlen(s->assign.target->ident)-1] == 'f');
                    
                    // Handle compound assignments (+=, -=, etc.)
                    if (s->assign.op != TOK_EQ) {
                        // Check if this is an array += operation
                        Type* var_type = llvm_get_var_type(lg, s->assign.target->ident);
                        if (var_type && var_type->kind == TYPE_ARRAY && s->assign.op == TOK_PLUSEQ) {
                            // Array += item means append
                            // Load current array (as i64, then cast to pointer)
                            int curr_i64 = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = load i64, i64* %%%d", curr_i64, var_reg);
                            
                            // Cast to i64* for array_append
                            int curr_ptr = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = inttoptr i64 %%%d to i64*", curr_ptr, curr_i64);
                            
                            // Evaluate item to append
                            int item_reg;
                            llvm_expr(lg, s->assign.value, &item_reg);
                            
                            // Call array_append
                            int result_ptr = llvm_new_temp(lg);
                            bool is_string_item = llvm_is_string_expr(lg, s->assign.value);
                            if (is_string_item) {
                                int ptr_to_int = llvm_new_temp(lg);
                                llvm_emit(lg, "  %%%d = ptrtoint i8* %%%d to i64", ptr_to_int, item_reg);
                                llvm_emit(lg, "  %%%d = call i64* @array_append(i64* %%%d, i64 %%%d)", result_ptr, curr_ptr, ptr_to_int);
                            } else {
                                if (item_reg <= -1000000) {
                                    // Constant item
                                    long long const_val = -item_reg - 1000000;
                                    llvm_emit(lg, "  %%%d = call i64* @array_append(i64* %%%d, i64 %lld)", result_ptr, curr_ptr, const_val);
                                } else {
                                    llvm_emit(lg, "  %%%d = call i64* @array_append(i64* %%%d, i64 %%%d)", result_ptr, curr_ptr, item_reg);
                                }
                            }
                            
                            // Cast back to i64 and store
                            int result_i64 = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = ptrtoint i64* %%%d to i64", result_i64, result_ptr);
                            llvm_emit(lg, "  store i64 %%%d, i64* %%%d", result_i64, var_reg);
                        } else {
                            // Numeric compound assignment
                            // Load current value
                            int curr_reg = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = load i64, i64* %%%d", curr_reg, var_reg);
                            
                            // Evaluate RHS
                            int val_reg;
                            llvm_expr(lg, s->assign.value, &val_reg);
                            
                            // Perform operation
                            int result_reg = llvm_new_temp(lg);
                            if (val_reg <= -1000000) {
                                // RHS is a constant
                                long long const_val = -val_reg - 1000000;
                                if (s->assign.op == TOK_PLUSEQ) {
                                    llvm_emit(lg, "  %%%d = add i64 %%%d, %lld", result_reg, curr_reg, const_val);
                                } else if (s->assign.op == TOK_MINUSEQ) {
                                    llvm_emit(lg, "  %%%d = sub i64 %%%d, %lld", result_reg, curr_reg, const_val);
                                } else if (s->assign.op == TOK_STAREQ) {
                                    llvm_emit(lg, "  %%%d = mul i64 %%%d, %lld", result_reg, curr_reg, const_val);
                                } else if (s->assign.op == TOK_SLASHEQ) {
                                    llvm_emit(lg, "  %%%d = sdiv i64 %%%d, %lld", result_reg, curr_reg, const_val);
                                }
                            } else {
                                // RHS is a register
                                if (s->assign.op == TOK_PLUSEQ) {
                                    llvm_emit(lg, "  %%%d = add i64 %%%d, %%%d", result_reg, curr_reg, val_reg);
                                } else if (s->assign.op == TOK_MINUSEQ) {
                                    llvm_emit(lg, "  %%%d = sub i64 %%%d, %%%d", result_reg, curr_reg, val_reg);
                                } else if (s->assign.op == TOK_STAREQ) {
                                    llvm_emit(lg, "  %%%d = mul i64 %%%d, %%%d", result_reg, curr_reg, val_reg);
                                } else if (s->assign.op == TOK_SLASHEQ) {
                                    llvm_emit(lg, "  %%%d = sdiv i64 %%%d, %%%d", result_reg, curr_reg, val_reg);
                                }
                            }
                            
                            // Store result
                            llvm_emit(lg, "  store i64 %%%d, i64* %%%d", result_reg, var_reg);
                        }
                    } else {
                        // Regular assignment
                        int val_reg;
                        llvm_expr(lg, s->assign.value, &val_reg);
                        
                        if (val_reg <= -1000000) {
                            if (is_float_var) {
                                int conv_reg = llvm_new_temp(lg);
                                llvm_emit(lg, "  %%%d = sitofp i64 %lld to double", conv_reg, (long long)(-val_reg - 1000000));
                                llvm_emit(lg, "  store double %%%d, double* %%%d", conv_reg, var_reg);
                            } else {
                                llvm_emit(lg, "  store i64 %lld, i64* %%%d", (long long)(-val_reg - 1000000), var_reg);
                            }
                        } else {
                            // Get variable type to determine storage type
                            Type* var_type = llvm_get_var_type(lg, s->assign.target->ident);
                            
                            if (var_type && var_type->kind == TYPE_STR) {
                                llvm_emit(lg, "  store i8* %%%d, i8** %%%d", val_reg, var_reg);
                            } else if (var_type && var_type->kind == TYPE_ARRAY) {
                                llvm_emit(lg, "  store i64* %%%d, i64** %%%d", val_reg, var_reg);
                            } else if (is_float_var) {
                                llvm_emit(lg, "  store double %%%d, double* %%%d", val_reg, var_reg);
                            } else {
                                llvm_emit(lg, "  store i64 %%%d, i64* %%%d", val_reg, var_reg);
                            }
                        }
                    }
                }
            } else if (s->assign.target->kind == EXPR_FIELD) {
                // Struct field assignment: obj.field = value
                int obj_reg;
                llvm_expr(lg, s->assign.target->field.object, &obj_reg);
                
                int val_reg;
                llvm_expr(lg, s->assign.value, &val_reg);
                
                // Find struct type
                StructDef* struct_def = NULL;
                if (s->assign.target->field.object->kind == EXPR_IDENT) {
                    for (int i = 0; i < lg->module->struct_count; i++) {
                        for (int j = 0; j < lg->module->structs[i].field_count; j++) {
                            if (strcmp(lg->module->structs[i].fields[j].name, s->assign.target->field.field) == 0) {
                                struct_def = &lg->module->structs[i];
                                break;
                            }
                        }
                        if (struct_def) break;
                    }
                }
                
                if (struct_def) {
                    int field_offset = llvm_struct_field_offset(struct_def, s->assign.target->field.field);
                    if (field_offset >= 0) {
                        int field_ptr = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = getelementptr %%struct.%s, %%struct.%s* %%%d, i32 0, i32 %d", 
                                 field_ptr, struct_def->name, struct_def->name, obj_reg, field_offset);
                        
                        if (val_reg <= -1000000) {
                            llvm_emit(lg, "  store i64 %lld, i64* %%%d", (long long)(-val_reg - 1000000), field_ptr);
                        } else {
                            llvm_emit(lg, "  store i64 %%%d, i64* %%%d", val_reg, field_ptr);
                        }
                    }
                }
            }
            break;
        }
        
        case STMT_EXPR: {
            // Check if this is an array method call that should modify the variable in place
            if (s->expr.expr->kind == EXPR_CALL && 
                s->expr.expr->call.func->kind == EXPR_FIELD &&
                s->expr.expr->call.func->field.object->kind == EXPR_IDENT) {
                
                const char* method_name = s->expr.expr->call.func->field.field;
                const char* var_name = s->expr.expr->call.func->field.object->ident;
                
                // Check if this is an array method that returns a new array
                if (strcmp(method_name, "append") == 0 || strcmp(method_name, "reverse") == 0) {
                    // Generate the method call
                    int result_reg;
                    llvm_expr(lg, s->expr.expr, &result_reg);
                    
                    // Find the variable and update it with the result
                    for (int i = lg->var_count - 1; i >= 0; i--) {
                        if (strcmp(lg->vars[i], var_name) == 0) {
                            // Store the result back to the variable
                            if (result_reg <= -1000000) {
                                llvm_emit(lg, "  store i64 %lld, i64* %%%d", 
                                         (long long)(-result_reg - 1000000), lg->var_regs[i]);
                            } else {
                                llvm_emit(lg, "  store i64* %%%d, i64** %%%d", result_reg, lg->var_regs[i]);
                            }
                            
                            // Update variable type if appending to array
                            if (strcmp(method_name, "append") == 0 && s->expr.expr->call.arg_count > 0) {
                                Type* var_type = lg->var_types[i];
                                if (var_type && var_type->kind == TYPE_ARRAY && (!var_type->inner || var_type->inner->kind == TYPE_ANY)) {
                                    // Check if appending a string
                                    if (llvm_is_string_expr(lg, s->expr.expr->call.args[0])) {
                                        if (!var_type->inner) {
                                            var_type->inner = malloc(sizeof(Type));
                                        }
                                        var_type->inner->kind = TYPE_STR;
                                    } else {
                                        // Appending an integer
                                        if (!var_type->inner) {
                                            var_type->inner = malloc(sizeof(Type));
                                        }
                                        var_type->inner->kind = TYPE_INT;
                                    }
                                }
                            }
                            break;
                        }
                    }
                } else {
                    // Regular expression statement
                    int dummy;
                    llvm_expr(lg, s->expr.expr, &dummy);
                }
            } else {
                // Regular expression statement
                int dummy;
                llvm_expr(lg, s->expr.expr, &dummy);
            }
            break;
        }
        
        case STMT_RETURN:
            if (s->ret.value) {
                int val_reg;
                llvm_expr(lg, s->ret.value, &val_reg);
                
                // Check function's return type
                Type* ret_type = lg->current_function ? lg->current_function->return_type : NULL;
                
                if (val_reg <= -1000000) {
                    llvm_emit(lg, "  ret i64 %lld", (long long)(-val_reg - 1000000));
                } else {
                    // Emit return with correct type
                    // Note: llvm_expr should have already loaded the value
                    // But in some cases (generic instantiated functions), it returns the alloca
                    // We detect this and load if needed
                    if (ret_type && ret_type->kind == TYPE_STR) {
                        llvm_emit(lg, "  ret i8* %%%d", val_reg);
                    } else if (ret_type && ret_type->kind == TYPE_ARRAY) {
                        llvm_emit(lg, "  ret i64* %%%d", val_reg);
                    } else if (ret_type && ret_type->kind == TYPE_FLOAT) {
                        llvm_emit(lg, "  ret double %%%d", val_reg);
                    } else {
                        llvm_emit(lg, "  ret i64 %%%d", val_reg);
                    }
                }
            } else {
                llvm_emit(lg, "  ret i32 0");
            }
            break;
            
        case STMT_IF: {
            int cond_reg;
            llvm_expr(lg, s->if_stmt.cond, &cond_reg);
            
            // Check if condition needs i64 to i1 conversion
            // This happens when condition is a variable or function call, not a direct comparison
            bool needs_i64_to_i1 = (s->if_stmt.cond->kind == EXPR_IDENT) ||
                                   (s->if_stmt.cond->kind == EXPR_CALL);
            
            int final_cond_reg = cond_reg;
            if (needs_i64_to_i1 && cond_reg > -1000000) {
                // Convert i64 to i1 by comparing with 0
                int conv_reg = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = icmp ne i64 %%%d, 0", conv_reg, cond_reg);
                final_cond_reg = conv_reg;
            }
            
            int then_label = llvm_new_label(lg);
            int else_label = llvm_new_label(lg);
            int end_label = llvm_new_label(lg);
            
            llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", final_cond_reg, then_label, 
                     s->if_stmt.else_count > 0 ? else_label : end_label);
            
            llvm_emit(lg, "L%d:", then_label);
            for (int i = 0; i < s->if_stmt.then_count; i++) {
                llvm_stmt(lg, s->if_stmt.then_block[i]);
            }
            bool then_returns = block_ends_with_return(s->if_stmt.then_block, s->if_stmt.then_count);
            bool then_breaks = block_ends_with_break_or_continue(s->if_stmt.then_block, s->if_stmt.then_count);
            if (!then_returns && !then_breaks) {
                llvm_emit(lg, "  br label %%L%d", end_label);
            }
            
            if (s->if_stmt.else_count > 0) {
                llvm_emit(lg, "L%d:", else_label);
                for (int i = 0; i < s->if_stmt.else_count; i++) {
                    llvm_stmt(lg, s->if_stmt.else_block[i]);
                }
                bool else_returns = block_ends_with_return(s->if_stmt.else_block, s->if_stmt.else_count);
                bool else_breaks = block_ends_with_break_or_continue(s->if_stmt.else_block, s->if_stmt.else_count);
                if (!else_returns && !else_breaks) {
                    llvm_emit(lg, "  br label %%L%d", end_label);
                }
            }
            
            llvm_emit(lg, "L%d:", end_label);
            break;
        }
        
        case STMT_WHILE: {
            int cond_label = llvm_new_label(lg);
            int body_label = llvm_new_label(lg);
            int end_label = llvm_new_label(lg);
            
            int old_start = lg->loop_start_label;
            int old_end = lg->loop_end_label;
            lg->loop_start_label = cond_label;
            lg->loop_end_label = end_label;
            
            llvm_emit(lg, "  br label %%L%d", cond_label);
            llvm_emit(lg, "L%d:", cond_label);
            
            int cond_reg;
            llvm_expr(lg, s->while_stmt.cond, &cond_reg);
            llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cond_reg, body_label, end_label);
            
            llvm_emit(lg, "L%d:", body_label);
            for (int i = 0; i < s->while_stmt.body_count; i++) {
                llvm_stmt(lg, s->while_stmt.body[i]);
            }
            llvm_emit(lg, "  br label %%L%d", cond_label);
            
            llvm_emit(lg, "L%d:", end_label);
            
            lg->loop_start_label = old_start;
            lg->loop_end_label = old_end;
            break;
        }
        
        case STMT_FOR: {
            // Check if range (start..end) or array iteration
            bool is_range = s->for_stmt.iter && s->for_stmt.iter->kind == EXPR_BINARY &&
                           (s->for_stmt.iter->binary.op == TOK_DOTDOT || 
                            s->for_stmt.iter->binary.op == TOK_DOTDOTEQ);
            
            if (is_range) {
                // Range iteration: for i in 0..5
                bool inclusive = s->for_stmt.iter->binary.op == TOK_DOTDOTEQ;
                
                // Allocate loop variable
                int var_reg = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = alloca i64", var_reg);
                llvm_add_var(lg, s->for_stmt.var, var_reg, new_type(TYPE_INT));
                
                // Allocate end value
                int end_reg = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = alloca i64", end_reg);
                
                // Initialize start
                int start_val;
                llvm_expr(lg, s->for_stmt.iter->binary.left, &start_val);
                if (start_val <= -1000000) {
                    llvm_emit(lg, "  store i64 %lld, i64* %%%d", (long long)(-start_val - 1000000), var_reg);
                } else {
                    llvm_emit(lg, "  store i64 %%%d, i64* %%%d", start_val, var_reg);
                }
                
                // Initialize end
                int end_val;
                llvm_expr(lg, s->for_stmt.iter->binary.right, &end_val);
                if (end_val <= -1000000) {
                    llvm_emit(lg, "  store i64 %lld, i64* %%%d", (long long)(-end_val - 1000000), end_reg);
                } else {
                    llvm_emit(lg, "  store i64 %%%d, i64* %%%d", end_val, end_reg);
                }
                
                // Create labels
                int cond_label = llvm_new_label(lg);
                int body_label = llvm_new_label(lg);
                int cont_label = llvm_new_label(lg);
                int end_label = llvm_new_label(lg);
                
                int old_start = lg->loop_start_label;
                int old_end = lg->loop_end_label;
                lg->loop_start_label = cont_label;
                lg->loop_end_label = end_label;
                
                // Jump to condition
                llvm_emit(lg, "  br label %%L%d", cond_label);
                
                // Condition check
                llvm_emit(lg, "L%d:", cond_label);
                int curr_val = llvm_new_temp(lg);
                int end_check = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = load i64, i64* %%%d", curr_val, var_reg);
                llvm_emit(lg, "  %%%d = load i64, i64* %%%d", end_check, end_reg);
                
                int cmp = llvm_new_temp(lg);
                if (inclusive) {
                    llvm_emit(lg, "  %%%d = icmp sle i64 %%%d, %%%d", cmp, curr_val, end_check);
                } else {
                    llvm_emit(lg, "  %%%d = icmp slt i64 %%%d, %%%d", cmp, curr_val, end_check);
                }
                llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp, body_label, end_label);
                
                // Body
                llvm_emit(lg, "L%d:", body_label);
                for (int i = 0; i < s->for_stmt.body_count; i++) {
                    llvm_stmt(lg, s->for_stmt.body[i]);
                }
                llvm_emit(lg, "  br label %%L%d", cont_label);
                
                // Continue: increment
                llvm_emit(lg, "L%d:", cont_label);
                int loaded = llvm_new_temp(lg);
                int next_val = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = load i64, i64* %%%d", loaded, var_reg);
                llvm_emit(lg, "  %%%d = add i64 %%%d, 1", next_val, loaded);
                llvm_emit(lg, "  store i64 %%%d, i64* %%%d", next_val, var_reg);
                llvm_emit(lg, "  br label %%L%d", cond_label);
                
                // End
                llvm_emit(lg, "L%d:", end_label);
                
                lg->loop_start_label = old_start;
                lg->loop_end_label = old_end;
            } else {
                // Check if iterating over a string
                bool is_string = llvm_is_string_expr(lg, s->for_stmt.iter);
                
                if (is_string) {
                    // String iteration: for c in "abc"
                    int str_reg;
                    llvm_expr(lg, s->for_stmt.iter, &str_reg);
                    
                    // Allocate loop variable and index
                    int var_reg = llvm_new_temp(lg);
                    int idx_reg = llvm_new_temp(lg);
                    
                    llvm_emit(lg, "  %%%d = alloca i8*", var_reg);
                    llvm_emit(lg, "  %%%d = alloca i64", idx_reg);
                    llvm_add_var(lg, s->for_stmt.var, var_reg, new_type(TYPE_STR));
                    
                    // Initialize index to 0
                    llvm_emit(lg, "  store i64 0, i64* %%%d", idx_reg);
                    
                    // Get string length using str_len
                    int len_reg = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64 @str_len(i8* %%%d)", len_reg, str_reg);
                    
                    // Create labels
                    int cond_label = llvm_new_label(lg);
                    int body_label = llvm_new_label(lg);
                    int cont_label = llvm_new_label(lg);
                    int end_label = llvm_new_label(lg);
                    
                    int old_start = lg->loop_start_label;
                    int old_end = lg->loop_end_label;
                    lg->loop_start_label = cont_label;
                    lg->loop_end_label = end_label;
                    
                    llvm_emit(lg, "  br label %%L%d", cond_label);
                    
                    // Condition: idx < len
                    llvm_emit(lg, "L%d:", cond_label);
                    int curr_idx = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64, i64* %%%d", curr_idx, idx_reg);
                    int cmp = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = icmp slt i64 %%%d, %%%d", cmp, curr_idx, len_reg);
                    llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp, body_label, end_label);
                    
                    // Body: get character at index
                    llvm_emit(lg, "L%d:", body_label);
                    int char_val = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i8* @char_at(i8* %%%d, i64 %%%d)", char_val, str_reg, curr_idx);
                    llvm_emit(lg, "  store i8* %%%d, i8** %%%d", char_val, var_reg);
                    
                    for (int i = 0; i < s->for_stmt.body_count; i++) {
                        llvm_stmt(lg, s->for_stmt.body[i]);
                    }
                    llvm_emit(lg, "  br label %%L%d", cont_label);
                    
                    // Continue: increment index
                    llvm_emit(lg, "L%d:", cont_label);
                    int loaded_idx = llvm_new_temp(lg);
                    int next_idx = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64, i64* %%%d", loaded_idx, idx_reg);
                    llvm_emit(lg, "  %%%d = add i64 %%%d, 1", next_idx, loaded_idx);
                    llvm_emit(lg, "  store i64 %%%d, i64* %%%d", next_idx, idx_reg);
                    llvm_emit(lg, "  br label %%L%d", cond_label);
                    
                    // End
                    llvm_emit(lg, "L%d:", end_label);
                    
                    lg->loop_start_label = old_start;
                    lg->loop_end_label = old_end;
                } else {
                    // Array iteration: for elem in arr
                    int arr_reg;
                    llvm_expr(lg, s->for_stmt.iter, &arr_reg);
                    
                    // Get array type to determine element type
                    Type* arr_type = NULL;
                    if (s->for_stmt.iter->kind == EXPR_IDENT) {
                        arr_type = llvm_get_var_type(lg, s->for_stmt.iter->ident);
                    }
                    bool is_string_array = arr_type && arr_type->kind == TYPE_ARRAY && 
                                          arr_type->inner && arr_type->inner->kind == TYPE_STR;
                    
                    // Allocate loop variable and index
                    int var_reg = llvm_new_temp(lg);
                    int idx_reg = llvm_new_temp(lg);
                    
                    if (is_string_array) {
                        llvm_emit(lg, "  %%%d = alloca i8*", var_reg);
                    } else {
                        llvm_emit(lg, "  %%%d = alloca i64", var_reg);
                    }
                    llvm_emit(lg, "  %%%d = alloca i64", idx_reg);
                    llvm_add_var(lg, s->for_stmt.var, var_reg, arr_type ? arr_type->inner : new_type(TYPE_INT));
                    
                    // Initialize index to 0
                    llvm_emit(lg, "  store i64 0, i64* %%%d", idx_reg);
                    
                    // Get array length
                    int len_ptr = llvm_new_temp(lg);
                    int len_reg = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = getelementptr i64, ptr %%%d, i64 0", len_ptr, arr_reg);
                    llvm_emit(lg, "  %%%d = load i64, i64* %%%d", len_reg, len_ptr);
                    
                    // Create labels
                    int cond_label = llvm_new_label(lg);
                    int body_label = llvm_new_label(lg);
                    int cont_label = llvm_new_label(lg);
                    int end_label = llvm_new_label(lg);
                    
                    int old_start = lg->loop_start_label;
                    int old_end = lg->loop_end_label;
                    lg->loop_start_label = cont_label;
                    lg->loop_end_label = end_label;
                    
                    llvm_emit(lg, "  br label %%L%d", cond_label);
                    
                    // Condition: idx < len
                    llvm_emit(lg, "L%d:", cond_label);
                    int curr_idx = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64, i64* %%%d", curr_idx, idx_reg);
                    int cmp = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = icmp slt i64 %%%d, %%%d", cmp, curr_idx, len_reg);
                    llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp, body_label, end_label);
                    
                    // Body: load element and execute statements
                    llvm_emit(lg, "L%d:", body_label);
                    int adj_idx = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = add i64 %%%d, 2", adj_idx, curr_idx);
                    int elem_ptr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = getelementptr i64, ptr %%%d, i64 %%%d", elem_ptr, arr_reg, adj_idx);
                    int elem_val = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64, i64* %%%d", elem_val, elem_ptr);
                    
                    if (is_string_array) {
                        // Cast i64 to i8* for string elements
                        int str_val = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = inttoptr i64 %%%d to i8*", str_val, elem_val);
                        llvm_emit(lg, "  store i8* %%%d, i8** %%%d", str_val, var_reg);
                    } else {
                        llvm_emit(lg, "  store i64 %%%d, i64* %%%d", elem_val, var_reg);
                    }
                    
                    for (int i = 0; i < s->for_stmt.body_count; i++) {
                        llvm_stmt(lg, s->for_stmt.body[i]);
                    }
                    llvm_emit(lg, "  br label %%L%d", cont_label);
                    
                    // Continue: increment index
                    llvm_emit(lg, "L%d:", cont_label);
                    int loaded_idx = llvm_new_temp(lg);
                    int next_idx = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64, i64* %%%d", loaded_idx, idx_reg);
                    llvm_emit(lg, "  %%%d = add i64 %%%d, 1", next_idx, loaded_idx);
                    llvm_emit(lg, "  store i64 %%%d, i64* %%%d", next_idx, idx_reg);
                    llvm_emit(lg, "  br label %%L%d", cond_label);
                    
                    // End
                    llvm_emit(lg, "L%d:", end_label);
                    
                    lg->loop_start_label = old_start;
                    lg->loop_end_label = old_end;
                }
            }
            break;
        }
        
        case STMT_BREAK:
            if (lg->loop_end_label >= 0) {
                llvm_emit(lg, "  br label %%L%d", lg->loop_end_label);
            }
            break;
            
        case STMT_CONTINUE:
            if (lg->loop_start_label >= 0) {
                llvm_emit(lg, "  br label %%L%d", lg->loop_start_label);
            }
            break;
        
        case STMT_BLOCK:
            for (int i = 0; i < s->block.count; i++) {
                llvm_stmt(lg, s->block.stmts[i]);
            }
            break;
        
        case STMT_MATCH: {
            int end_label = llvm_new_label(lg);
            
            // Evaluate match value once
            int match_reg;
            llvm_expr(lg, s->match_stmt.value, &match_reg);
            
            for (int i = 0; i < s->match_stmt.arm_count; i++) {
                int next_label = llvm_new_label(lg);
                Expr* pat = s->match_stmt.patterns[i];
                
                // Check if pattern is wildcard (_)
                bool is_wildcard = pat->kind == EXPR_IDENT && strcmp(pat->ident, "_") == 0;
                
                // Check if this is a binding pattern (ok(v), err(msg), some(v))
                bool is_binding_pattern = (pat->kind == EXPR_SOME || pat->kind == EXPR_OK || pat->kind == EXPR_ERR) &&
                                         pat->some.value && pat->some.value->kind == EXPR_IDENT;
                
                if (!is_wildcard) {
                    int arm_label = llvm_new_label(lg);
                    
                    if (is_binding_pattern) {
                        // For binding patterns, we need to check the tag and extract the value
                        // For now, assume Result/Option are represented as simple values
                        // This is a simplified implementation - real tagged unions would be more complex
                        
                        if (pat->kind == EXPR_OK) {
                            // Check if match value represents "ok" (non-negative for now)
                            int cmp_reg = llvm_new_temp(lg);
                            if (match_reg <= -1000000) {
                                long long match_val = -match_reg - 1000000;
                                if (match_val >= 0) {
                                    llvm_emit(lg, "  br label %%L%d", arm_label);
                                } else {
                                    llvm_emit(lg, "  br label %%L%d", next_label);
                                    goto next_arm;
                                }
                            } else {
                                llvm_emit(lg, "  %%%d = icmp sge i64 %%%d, 0", cmp_reg, match_reg);
                                llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp_reg, arm_label, next_label);
                            }
                        } else if (pat->kind == EXPR_ERR) {
                            // Check if match value represents "err" (negative for now)
                            int cmp_reg = llvm_new_temp(lg);
                            if (match_reg <= -1000000) {
                                long long match_val = -match_reg - 1000000;
                                if (match_val < 0) {
                                    llvm_emit(lg, "  br label %%L%d", arm_label);
                                } else {
                                    llvm_emit(lg, "  br label %%L%d", next_label);
                                    goto next_arm;
                                }
                            } else {
                                llvm_emit(lg, "  %%%d = icmp slt i64 %%%d, 0", cmp_reg, match_reg);
                                llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp_reg, arm_label, next_label);
                            }
                        } else if (pat->kind == EXPR_SOME) {
                            // For Option types, check if not none (non-zero for now)
                            int cmp_reg = llvm_new_temp(lg);
                            if (match_reg <= -1000000) {
                                long long match_val = -match_reg - 1000000;
                                if (match_val != 0) {
                                    llvm_emit(lg, "  br label %%L%d", arm_label);
                                } else {
                                    llvm_emit(lg, "  br label %%L%d", next_label);
                                    goto next_arm;
                                }
                            } else {
                                llvm_emit(lg, "  %%%d = icmp ne i64 %%%d, 0", cmp_reg, match_reg);
                                llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp_reg, arm_label, next_label);
                            }
                        }
                    } else {
                        // Regular pattern matching
                        int pat_reg;
                        llvm_expr(lg, pat, &pat_reg);
                        
                        int cmp_reg = llvm_new_temp(lg);
                        
                        if (match_reg <= -1000000 && pat_reg <= -1000000) {
                            // Both constants - compare directly
                            long long match_val = -match_reg - 1000000;
                            long long pat_val = -pat_reg - 1000000;
                            if (match_val != pat_val) {
                                llvm_emit(lg, "  br label %%L%d", next_label);
                                goto next_arm;
                            } else {
                                // Pattern matches, go to arm
                                llvm_emit(lg, "  br label %%L%d", arm_label);
                            }
                        } else if (match_reg <= -1000000) {
                            // Match value is constant
                            llvm_emit(lg, "  %%%d = icmp eq i64 %%%d, %lld", cmp_reg, pat_reg, (long long)(-match_reg - 1000000));
                            llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp_reg, arm_label, next_label);
                        } else if (pat_reg <= -1000000) {
                            // Pattern is constant
                            llvm_emit(lg, "  %%%d = icmp eq i64 %%%d, %lld", cmp_reg, match_reg, (long long)(-pat_reg - 1000000));
                            llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp_reg, arm_label, next_label);
                        } else {
                            // Both are registers
                            llvm_emit(lg, "  %%%d = icmp eq i64 %%%d, %%%d", cmp_reg, match_reg, pat_reg);
                            llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cmp_reg, arm_label, next_label);
                        }
                    }
                    
                    llvm_emit(lg, "L%d:", arm_label);
                    
                    // Add variable binding if present
                    if (s->match_stmt.bindings[i][0] != '\0') {
                        // Create variable for the bound value
                        int var_reg = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = alloca i64", var_reg);
                        
                        // For now, just store the match value directly
                        // In a full implementation, this would extract the inner value from tagged unions
                        if (match_reg <= -1000000) {
                            llvm_emit(lg, "  store i64 %lld, i64* %%%d", (long long)(-match_reg - 1000000), var_reg);
                        } else {
                            llvm_emit(lg, "  store i64 %%%d, i64* %%%d", match_reg, var_reg);
                        }
                        
                        // Add variable to scope
                        llvm_add_var(lg, s->match_stmt.bindings[i], var_reg, new_type(TYPE_INT));
                    }
                } else {
                    // Wildcard matches everything - no comparison needed
                }
                
                // Execute arm body
                for (int j = 0; j < s->match_stmt.arm_counts[i]; j++) {
                    llvm_stmt(lg, s->match_stmt.arms[i][j]);
                }
                
                // Jump to end (unless this arm ends with return)
                bool arm_returns = false;
                if (s->match_stmt.arm_counts[i] > 0) {
                    Stmt* last = s->match_stmt.arms[i][s->match_stmt.arm_counts[i] - 1];
                    arm_returns = (last->kind == STMT_RETURN);
                }
                if (!arm_returns) {
                    llvm_emit(lg, "  br label %%L%d", end_label);
                }
                
                next_arm:
                // Always emit the next label, even for the last iteration
                llvm_emit(lg, "L%d:", next_label);
                // If this is the last arm and no pattern matched, fall through to end
                if (i == s->match_stmt.arm_count - 1) {
                    llvm_emit(lg, "  br label %%L%d", end_label);
                }
            }
            
            llvm_emit(lg, "L%d:", end_label);
            break;
        }
        
        case STMT_SPAWN: {
            // Store spawn block for later emission
            int spawn_id = lg->spawn_count;
            lg->spawn_blocks[lg->spawn_count++] = s;
            
            int cap_count = s->spawn.captured_count;
            if (cap_count > 0) {
                // Allocate context struct
                int ctx_t = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = call i8* @malloc(i64 %d)", ctx_t, cap_count * 8);
                
                // Store captured variable addresses in context
                for (int i = 0; i < cap_count; i++) {
                    int var_idx = llvm_find_var(lg, s->spawn.captured_vars[i]);
                    if (var_idx >= 0) {
                        Type* var_type = llvm_get_var_type(lg, s->spawn.captured_vars[i]);
                        
                        // Store type for later use in spawn function
                        s->spawn.captured_types[i] = var_type;
                        
                        int addr_t = llvm_new_temp(lg);
                        int cast_t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = getelementptr i8, i8* %%%d, i64 %d", addr_t, ctx_t, i * 8);
                        
                        if (var_type && var_type->kind == TYPE_STR) {
                            // String variable - store i8**
                            llvm_emit(lg, "  %%%d = bitcast i8* %%%d to i8***", cast_t, addr_t);
                            llvm_emit(lg, "  store i8** %%%d, i8*** %%%d", var_idx, cast_t);
                        } else if (var_type && var_type->kind == TYPE_ARRAY) {
                            // Array variable - store i64***
                            llvm_emit(lg, "  %%%d = bitcast i8* %%%d to i64***", cast_t, addr_t);
                            llvm_emit(lg, "  store i64** %%%d, i64*** %%%d", var_idx, cast_t);
                        } else {
                            // Integer variable - store i64**
                            llvm_emit(lg, "  %%%d = bitcast i8* %%%d to i64**", cast_t, addr_t);
                            llvm_emit(lg, "  store i64* %%%d, i64** %%%d", var_idx, cast_t);
                        }
                    }
                }
                
                // Call __wyn_spawn with function and context
                int t = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = call i64 @__wyn_spawn(i8* bitcast (void (i8*)* @__spawn_%d to i8*), i8* %%%d)", t, spawn_id, ctx_t);
            } else {
                // Call __wyn_spawn with function pointer only
                int t = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = call i64 @__wyn_spawn(i8* bitcast (void (i8*)* @__spawn_%d to i8*), i8* null)", t, spawn_id);
            }
            break;
        }
        
        default:
            break;
    }
}

static void llvm_function(LLVMGen* lg, FnDef* f) {
    // Track current function for return type checking
    lg->current_function = f;
    
    // Generate function signature
    if (strcmp(f->name, "main") == 0) {
        llvm_emit(lg, "define i32 @%s() {", f->name);
        llvm_emit(lg, "entry:");
        
        // Initialize runtime for spawn support
        int t = llvm_new_temp(lg);
        llvm_emit(lg, "  %%%d = call i64 @__wyn_runtime_init()", t);
    } else {
        // Build parameter list
        char params[1024] = "";
        for (int i = 0; i < f->param_count; i++) {
            char param[128];
            
            // For extension methods, first parameter (self) needs correct type
            if (f->is_extension && i == 0 && strcmp(f->params[i].name, "self") == 0) {
                const char* self_type = "i64";
                if (strcmp(f->extension_type, "str") == 0) {
                    self_type = "i8*";
                } else if (strcmp(f->extension_type, "array") == 0) {
                    self_type = "i64*";
                }
                snprintf(param, 128, "%s %%param.%s", self_type, f->params[i].name);
            } else {
                snprintf(param, 128, "i64 %%param.%s", f->params[i].name);
            }
            
            if (i > 0) strcat(params, ", ");
            strcat(params, param);
        }
        
        // Determine return type
        const char* ret_type = "i64";
        if (f->return_type) {
            if (f->return_type->kind == TYPE_STR) {
                ret_type = "i8*";
            } else if (f->return_type->kind == TYPE_ARRAY) {
                ret_type = "i64*";
            } else if (f->return_type->kind == TYPE_RESULT) {
                ret_type = "i64";  // Result types use simple integer encoding
            } else if (f->return_type->kind == TYPE_OPTIONAL) {
                ret_type = "i64";  // Option types use simple integer encoding
            } else if (f->return_type->kind == TYPE_FLOAT) {
                ret_type = "double";
            }
        }
        
        llvm_emit(lg, "define %s @\"%s\"(%s) {", ret_type, f->name, params);
        llvm_emit(lg, "entry:");
    }
    
    // Allocate and store parameters
    for (int i = 0; i < f->param_count; i++) {
        // For extension methods, self parameter needs correct type
        if (f->is_extension && i == 0 && strcmp(f->params[i].name, "self") == 0) {
            if (strcmp(f->extension_type, "str") == 0) {
                // String self parameter
                int alloca_reg = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = alloca i8*", alloca_reg);
                llvm_emit(lg, "  store i8* %%param.%s, i8** %%%d", f->params[i].name, alloca_reg);
                llvm_add_var(lg, f->params[i].name, alloca_reg, new_type(TYPE_STR));
            } else if (strcmp(f->extension_type, "array") == 0) {
                // Array self parameter
                int alloca_reg = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = alloca i64*", alloca_reg);
                llvm_emit(lg, "  store i64* %%param.%s, i64** %%%d", f->params[i].name, alloca_reg);
                Type* arr_type = new_type(TYPE_ARRAY);
                llvm_add_var(lg, f->params[i].name, alloca_reg, arr_type);
            } else {
                // Int, bool, etc - use i64
                int alloca_reg = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = alloca i64", alloca_reg);
                llvm_emit(lg, "  store i64 %%param.%s, i64* %%%d", f->params[i].name, alloca_reg);
                llvm_add_var(lg, f->params[i].name, alloca_reg, f->params[i].type);
            }
        } else {
            // Regular parameter
            int alloca_reg = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = alloca i64", alloca_reg);
            llvm_emit(lg, "  store i64 %%param.%s, i64* %%%d", f->params[i].name, alloca_reg);
            llvm_add_var(lg, f->params[i].name, alloca_reg, f->params[i].type);
        }
    }
    
    for (int i = 0; i < f->body_count; i++) {
        llvm_stmt(lg, f->body[i]);
    }
    
    // Add default return statement
    if (strcmp(f->name, "main") == 0) {
        // Shutdown runtime before returning
        int t = llvm_new_temp(lg);
        llvm_emit(lg, "  %%%d = call i64 @__wyn_runtime_shutdown()", t);
        llvm_emit(lg, "  ret i32 0");
    } else {
        // Check if the last statement is a return
        bool has_return = false;
        if (f->body_count > 0) {
            Stmt* last = f->body[f->body_count - 1];
            has_return = (last->kind == STMT_RETURN);
        }
        if (!has_return) {
            llvm_emit(lg, "  ret i64 0");
        }
    }
    llvm_emit(lg, "}");
}

static void llvm_generate(FILE* out, Module* m, Arch arch, TargetOS os) {
    // Collect all strings
    StringTable strings = {0};
    for (int i = 0; i < m->function_count; i++) {
        FnDef* f = &m->functions[i];
        // Collect strings from default parameter values
        for (int j = 0; j < f->param_count; j++) {
            if (f->params[j].default_val) {
                collect_strings_expr(&strings, f->params[j].default_val);
            }
        }
        // Collect strings from function body
        for (int j = 0; j < f->body_count; j++) {
            collect_strings_stmt(&strings, f->body[j]);
        }
    }
    
    // Collect strings from top-level statements
    for (int i = 0; i < m->top_level_count; i++) {
        collect_strings_stmt(&strings, m->top_level_stmts[i]);
    }
    
    LLVMGen lg = {0};
    lg.out = out;
    lg.module = m;
    lg.strings = &strings;
    lg.loop_start_label = -1;
    lg.loop_end_label = -1;
    
    // Header
    llvm_emit(&lg, "; ModuleID = 'wyn'");
    llvm_emit(&lg, "target triple = \"%s-apple-%s\"", 
             arch == ARCH_ARM64 ? "arm64" : "x86_64",
             os == OS_MACOS ? "macosx11.0.0" : "linux-gnu");
    llvm_emit(&lg, "");
    
    // Emit string constants
    for (int i = 0; i < strings.count; i++) {
        int len = strlen(strings.strings[i]);
        llvm_emit(&lg, "@.str.%d = private unnamed_addr constant [%d x i8] c\"%s\\00\", align 1",
                 i, len + 1, strings.strings[i]);
    }
    
    // Format strings
    llvm_emit(&lg, "@.fmt.int = private unnamed_addr constant [3 x i8] c\"%%d\\00\", align 1");
    llvm_emit(&lg, "@.fmt.float = private unnamed_addr constant [3 x i8] c\"%%f\\00\", align 1");
    llvm_emit(&lg, "@.fmt.newline = private unnamed_addr constant [2 x i8] c\"\\0A\\00\", align 1");
    llvm_emit(&lg, "@.fmt.array = private unnamed_addr constant [6 x i8] c\"Array\\00\", align 1");
    llvm_emit(&lg, "");
    
    // Struct type declarations
    for (int i = 0; i < m->struct_count; i++) {
        StructDef* s = &m->structs[i];
        fprintf(lg.out, "%%struct.%s = type { ", s->name);
        for (int j = 0; j < s->field_count; j++) {
            if (j > 0) fprintf(lg.out, ", ");
            // For now, all fields are i64 (can be extended for other types)
            fprintf(lg.out, "i64");
        }
        fprintf(lg.out, " }\n");
    }
    if (m->struct_count > 0) llvm_emit(&lg, "");
    
    // External declarations
    llvm_emit(&lg, "declare i32 @printf(i8*, ...)");
    llvm_emit(&lg, "declare i8* @malloc(i64)");
    
    // Spawn runtime declarations
    llvm_emit(&lg, "declare i64 @__wyn_spawn(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @__wyn_runtime_init()");
    llvm_emit(&lg, "declare i64 @__wyn_runtime_shutdown()");
    llvm_emit(&lg, "declare void @__wyn_yield()");
    llvm_emit(&lg, "declare i64 @__wyn_await(i64)");
    
    // Channel/Task declarations
    llvm_emit(&lg, "declare i8* @__wyn_channel_new()");
    llvm_emit(&lg, "declare void @__wyn_channel_send(i8*, i64)");
    llvm_emit(&lg, "declare i64 @__wyn_channel_recv(i8*)");
    llvm_emit(&lg, "declare void @__wyn_channel_close(i8*)");
    llvm_emit(&lg, "declare void @__wyn_channel_free(i8*)");
    
    // Builtin function declarations for stdlib
    llvm_emit(&lg, "declare i64 @assert(i64)");
    llvm_emit(&lg, "declare i64 @array_contains(i64*, i64)");
    llvm_emit(&lg, "declare i8* @read_file_builtin(i8*)");
    llvm_emit(&lg, "declare i64 @write_file_builtin(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @file_exists_builtin(i8*)");
    llvm_emit(&lg, "declare i64 @append_file_builtin(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @read_file(i8*)");
    llvm_emit(&lg, "declare i64 @write_file(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @fs_write(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @fs_append(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @fs_delete(i8*)");
    llvm_emit(&lg, "declare i64 @fs_mkdir(i8*)");
    llvm_emit(&lg, "declare i8* @os_exec_capture(i8*)");
    llvm_emit(&lg, "declare i64 @os_exec_check(i8*)");
    llvm_emit(&lg, "declare i8* @path_join(i64*)");
    llvm_emit(&lg, "declare i8* @path_dirname(i8*)");
    llvm_emit(&lg, "declare i8* @path_basename(i8*)");
    llvm_emit(&lg, "declare i8* @path_ext(i8*)");
    llvm_emit(&lg, "declare i64 @file_exists(i8*)");
    llvm_emit(&lg, "declare i64 @append_file(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @file_copy(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @file_move(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @file_chmod(i8*, i64)");
    llvm_emit(&lg, "declare i64* @file_glob(i8*)");
    llvm_emit(&lg, "declare i64 @ord(i8*)");
    llvm_emit(&lg, "declare i8* @chr(i64)");
    llvm_emit(&lg, "declare i8* @substring(i8*, i64, i64)");
    llvm_emit(&lg, "declare i8* @str_substring(i8*, i64, i64)");
    llvm_emit(&lg, "declare i64 @str_find(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @str_concat(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @str_replace(i8*, i8*, i8*)");
    llvm_emit(&lg, "declare i64 @str_cmp(i8*, i8*)");
    llvm_emit(&lg, "declare i64* @str_split(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @format(i8*, i64*)");
    llvm_emit(&lg, "declare i8* @str_format(i8*, i64*)");
    llvm_emit(&lg, "declare i8* @char_at(i8*, i64)");
    llvm_emit(&lg, "declare i8* @int_to_str(i64)");
    llvm_emit(&lg, "declare i64 @str_to_int(i8*)");
    llvm_emit(&lg, "declare i64 @str_len(i8*)");
    llvm_emit(&lg, "declare i8* @str_upper(i8*)");
    llvm_emit(&lg, "declare i8* @str_lower(i8*)");
    llvm_emit(&lg, "declare i8* @str_trim(i8*)");
    llvm_emit(&lg, "declare i64 @str_contains(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @str_starts_with(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @str_ends_with(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @sb_new_wyn()");
    llvm_emit(&lg, "declare void @sb_append_wyn(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @sb_to_str_wyn(i8*)");
    llvm_emit(&lg, "declare i64 @floor_wyn(i64)");
    llvm_emit(&lg, "declare i64 @ceil_wyn(i64)");
    llvm_emit(&lg, "declare i64 @round_wyn(i64)");
    llvm_emit(&lg, "declare i64 @clock_wyn()");
    llvm_emit(&lg, "declare i64 @random_wyn()");
    llvm_emit(&lg, "declare i64 @time_now()");
    llvm_emit(&lg, "declare void @sleep_ms(i64)");
    llvm_emit(&lg, "declare void @exit_wyn(i64)");
    llvm_emit(&lg, "declare void @task_await(i64)");
    llvm_emit(&lg, "declare void @task_join_all()");
    llvm_emit(&lg, "declare i64 @min_int(i64, i64)");
    llvm_emit(&lg, "declare i64 @max_int(i64, i64)");
    llvm_emit(&lg, "declare i64 @abs_int(i64)");
    
    // OS module functions
    llvm_emit(&lg, "declare i8* @getenv_wyn(i8*)");
    llvm_emit(&lg, "declare i64 @setenv_wyn(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @exec_wyn(i8*)");
    llvm_emit(&lg, "declare i8* @exec_output_wyn(i8*)");
    llvm_emit(&lg, "declare i8* @exec_args_wyn(i8*, i64*)");
    llvm_emit(&lg, "declare void @command_start(i8*)");
    llvm_emit(&lg, "declare void @command_arg(i8*)");
    llvm_emit(&lg, "declare i64 @command_run()");
    llvm_emit(&lg, "declare i8* @command_output()");
    llvm_emit(&lg, "declare i64 @command_new(i8*)");
    llvm_emit(&lg, "declare i64 @command_arg_chain(i64, i8*)");
    llvm_emit(&lg, "declare i64 @command_run_chain(i64)");
    llvm_emit(&lg, "declare i8* @command_output_chain(i64)");
    llvm_emit(&lg, "declare i64 @cli_has_flag(i8*)");
    llvm_emit(&lg, "declare i8* @cli_get_arg(i8*)");
    llvm_emit(&lg, "declare i8* @cli_get_positional(i64)");
    llvm_emit(&lg, "declare i64 @cli_arg_count()");
    llvm_emit(&lg, "declare i8* @str_slice(i8*, i64, i64)");
    llvm_emit(&lg, "declare i64* @array_slice(i64*, i64, i64)");
    llvm_emit(&lg, "declare i64* @array_reverse(i64*)");
    llvm_emit(&lg, "declare i8* @array_join(i64*, i8*)");
    llvm_emit(&lg, "declare i8* @array_join_str(i64*, i8*)");
    llvm_emit(&lg, "declare i64* @array_append_elem(i64*, i64)");
    llvm_emit(&lg, "declare i64* @array_prepend_elem(i64*, i64)");
    llvm_emit(&lg, "declare i64 @array_contains_elem(i64*, i64)");
    llvm_emit(&lg, "declare i64* @array_filter_simple(i64*)");
    llvm_emit(&lg, "declare i64* @array_map_simple(i64*)");
    llvm_emit(&lg, "declare i64* @array_new()");
    llvm_emit(&lg, "declare i64 @sqlite_open(i8*)");
    llvm_emit(&lg, "declare i64 @sqlite_exec(i64, i8*)");
    llvm_emit(&lg, "declare i8* @sqlite_query(i64, i8*)");
    llvm_emit(&lg, "declare i64 @sqlite_close(i64)");
    llvm_emit(&lg, "declare i64 @http_router_new()");
    llvm_emit(&lg, "declare i64 @http_router_add(i64, i8*, i8*, i64)");
    llvm_emit(&lg, "declare i64 @http_router_match(i64, i8*, i8*)");
    llvm_emit(&lg, "declare i64 @http_response_json(i64, i64, i8*)");
    llvm_emit(&lg, "declare i64 @http_response_html(i64, i64, i8*)");
    llvm_emit(&lg, "declare i64 @http_response_text(i64, i64, i8*)");
    llvm_emit(&lg, "declare i64 @test_assert_eq(i64, i64, i8*)");
    llvm_emit(&lg, "declare i64 @test_assert_ne(i64, i64, i8*)");
    llvm_emit(&lg, "declare i64 @test_assert_true(i64, i8*)");
    llvm_emit(&lg, "declare i64 @test_assert_false(i64, i8*)");
    llvm_emit(&lg, "declare i64 @test_summary()");
    llvm_emit(&lg, "declare i64 @result_ok_simple(i64)");
    llvm_emit(&lg, "declare i64 @result_err_simple(i64)");
    llvm_emit(&lg, "declare i64 @result_is_ok_simple(i64)");
    llvm_emit(&lg, "declare i64 @result_is_err_simple(i64)");
    llvm_emit(&lg, "declare i64 @result_unwrap_simple(i64)");
    llvm_emit(&lg, "declare i64 @array_len(i64*)");
    llvm_emit(&lg, "declare i64 @array_get(i64*, i64)");
    llvm_emit(&lg, "declare i64* @array_push(i64*, i64)");
    llvm_emit(&lg, "declare void @array_set(i64*, i64, i64)");
    llvm_emit(&lg, "declare i64* @result_ok(i64)");
    llvm_emit(&lg, "declare i64* @result_err(i64)");
    llvm_emit(&lg, "declare i64 @result_is_ok(i64*)");
    llvm_emit(&lg, "declare i64 @result_is_err(i64*)");
    llvm_emit(&lg, "declare i64 @result_unwrap(i64*)");
    llvm_emit(&lg, "declare i64* @option_some(i64)");
    llvm_emit(&lg, "declare i64* @option_none()");
    llvm_emit(&lg, "declare i64 @option_is_some(i64*)");
    llvm_emit(&lg, "declare i64 @option_is_none(i64*)");
    llvm_emit(&lg, "declare i64 @option_unwrap(i64*)");
    llvm_emit(&lg, "declare i64 @option_some_simple(i64)");
    llvm_emit(&lg, "declare i64 @option_none_simple()");
    llvm_emit(&lg, "declare i64 @option_is_some_simple(i64)");
    llvm_emit(&lg, "declare i64 @option_is_none_simple(i64)");
    llvm_emit(&lg, "declare i64 @option_unwrap_simple(i64)");
    llvm_emit(&lg, "declare i8** @args_wyn()");
    llvm_emit(&lg, "declare i8* @cwd_wyn()");
    llvm_emit(&lg, "declare i8* @char_to_string(i8)");
    llvm_emit(&lg, "declare i64 @chdir_wyn(i8*)");
    
    // Config module functions
    llvm_emit(&lg, "declare i8* @yaml_parse(i8*)");
    llvm_emit(&lg, "declare i8* @toml_parse(i8*)");
    
    // JSON module functions
    llvm_emit(&lg, "declare i8* @parse_json(i8*)");
    llvm_emit(&lg, "declare i8* @stringify_json(i8*)");
    llvm_emit(&lg, "declare i8* @get_string_json(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @get_int_json(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @get_bool_json(i8*, i8*)");
    
    // Net module functions
    llvm_emit(&lg, "declare i8* @http_get_wyn(i8*)");
    llvm_emit(&lg, "declare i8* @http_post_wyn(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @tcp_connect_wyn(i8*, i64)");
    llvm_emit(&lg, "declare i64 @tcp_send_wyn(i64, i8*)");
    llvm_emit(&lg, "declare i8* @tcp_recv_wyn(i64, i64)");
    llvm_emit(&lg, "declare void @tcp_close_wyn(i64)");
    
    // Extended IO functions
    llvm_emit(&lg, "declare i64 @delete_file(i8*)");
    llvm_emit(&lg, "declare i64 @file_size(i8*)");
    llvm_emit(&lg, "declare i64 @mkdir_wyn(i8*)");
    llvm_emit(&lg, "declare i64 @rmdir_wyn(i8*)");
    llvm_emit(&lg, "declare i64 @is_dir_wyn(i8*)");
    llvm_emit(&lg, "declare i64 @is_file_wyn(i8*)");
    llvm_emit(&lg, "declare i8* @hostname_wyn()");
    llvm_emit(&lg, "declare i64 @getpid_wyn()");
    llvm_emit(&lg, "declare i8* @http_put_wyn(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @http_delete_wyn(i8*)");
    llvm_emit(&lg, "declare i8* @http_head_wyn(i8*)");
    llvm_emit(&lg, "declare i8* @http_get_headers_wyn(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @udp_send_wyn(i8*, i64, i8*)");
    llvm_emit(&lg, "declare i8* @udp_recv_wyn(i64, i64)");
    llvm_emit(&lg, "declare i8* @resolve_host_wyn(i8*)");
    llvm_emit(&lg, "declare i8* @get_local_ip_wyn()");
    
    // Compress module functions
    llvm_emit(&lg, "declare i64 @gzip_file(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @gunzip_file(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @tar_create(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @tar_extract(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @tar_list(i8*)");
    llvm_emit(&lg, "declare i8* @compress_string(i8*)");
    llvm_emit(&lg, "declare i8* @decompress_string(i8*)");
    
    // Regex module functions
    llvm_emit(&lg, "declare i64 @regex_matches(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @regex_find(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @regex_replace(i8*, i8*, i8*)");
    llvm_emit(&lg, "declare i64 @regex_count(i8*, i8*)");
    
    // Time module functions
    llvm_emit(&lg, "declare i64 @now_unix()");
    llvm_emit(&lg, "declare i64 @now_millis()");
    llvm_emit(&lg, "declare i64 @now_micros()");
    llvm_emit(&lg, "declare void @sleep_seconds(i64)");
    llvm_emit(&lg, "declare i8* @format_timestamp(i64)");
    llvm_emit(&lg, "declare i8* @format_iso8601(i64)");
    llvm_emit(&lg, "declare i64 @parse_timestamp(i8*)");
    
    // Log module functions
    llvm_emit(&lg, "declare void @log_info(i8*)");
    llvm_emit(&lg, "declare void @log_warn(i8*)");
    llvm_emit(&lg, "declare void @log_error(i8*)");
    llvm_emit(&lg, "declare void @log_debug(i8*)");
    llvm_emit(&lg, "declare void @log_with_level(i8*, i8*)");
    
    // Encoding module functions
    llvm_emit(&lg, "declare i8* @base64_encode(i8*)");
    llvm_emit(&lg, "declare i8* @base64_decode(i8*)");
    llvm_emit(&lg, "declare i8* @hex_encode(i8*)");
    llvm_emit(&lg, "declare i8* @hex_decode(i8*)");
    llvm_emit(&lg, "declare i8* @url_encode(i8*)");
    llvm_emit(&lg, "declare i8* @url_decode(i8*)");
    
    // Hash module functions
    llvm_emit(&lg, "declare i8* @sha256_hash(i8*)");
    llvm_emit(&lg, "declare i8* @md5_hash(i8*)");
    llvm_emit(&lg, "declare i8* @sha1_hash(i8*)");
    
    // Collections module functions
    llvm_emit(&lg, "declare i8* @hashmap_new()");
    llvm_emit(&lg, "declare void @hashmap_put(i8*, i8*, i8*)");
    llvm_emit(&lg, "declare i8* @hashmap_get(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @hashmap_contains(i8*, i8*)");
    llvm_emit(&lg, "declare void @hashmap_remove(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @hashmap_size(i8*)");
    llvm_emit(&lg, "declare void @hashmap_clear(i8*)");
    llvm_emit(&lg, "declare i8* @hashmap_keys(i8*)");
    llvm_emit(&lg, "declare i8* @hashmap_values(i8*)");
    llvm_emit(&lg, "declare i8* @set_new()");
    llvm_emit(&lg, "declare void @set_add(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @set_contains(i8*, i8*)");
    llvm_emit(&lg, "declare void @set_remove(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @set_size(i8*)");
    llvm_emit(&lg, "declare void @set_clear(i8*)");
    
    // Crypto module functions
    llvm_emit(&lg, "declare i8* @encrypt_aes256(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @decrypt_aes256(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @generate_random_bytes(i64)");
    llvm_emit(&lg, "declare i8* @hmac_sha256(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @verify_signature(i8*, i8*, i8*)");
    
    // Atomic operations
    llvm_emit(&lg, "declare i64 @wyn_atomic_add(i64*, i64)");
    llvm_emit(&lg, "declare i64 @wyn_atomic_sub(i64*, i64)");
    llvm_emit(&lg, "declare i64 @wyn_atomic_load(i64*)");
    llvm_emit(&lg, "declare void @wyn_atomic_store(i64*, i64)");
    llvm_emit(&lg, "declare i64 @wyn_atomic_cas(i64*, i64, i64)");
    
    // HTTP Server functions
    llvm_emit(&lg, "declare i64 @http_server_create(i64)");
    llvm_emit(&lg, "declare i64 @http_server_accept(i64)");
    llvm_emit(&lg, "declare i8* @http_server_read_request(i64)");
    llvm_emit(&lg, "declare i8* @http_parse_method(i8*)");
    llvm_emit(&lg, "declare i8* @http_parse_path(i8*)");
    llvm_emit(&lg, "declare i8* @http_parse_body(i8*)");
    llvm_emit(&lg, "declare i64 @http_server_send_response(i64, i64, i8*)");
    llvm_emit(&lg, "declare i64 @http_server_send_json(i64, i64, i8*)");
    llvm_emit(&lg, "declare void @http_server_close_client(i64)");
    llvm_emit(&lg, "declare void @http_server_close(i64)");
    llvm_emit(&lg, "");
    
    // Check if we need to generate an implicit main function
    bool has_main = false;
    for (int i = 0; i < m->function_count; i++) {
        if (strcmp(m->functions[i].name, "main") == 0) {
            has_main = true;
            break;
        }
    }
    
    // Generate implicit main function if we have top-level statements and no explicit main
    if (!has_main && m->top_level_count > 0) {
        // Create a synthetic main function
        FnDef* main_fn = &m->functions[m->function_count++];
        strcpy(main_fn->name, "main");
        main_fn->param_count = 0;
        main_fn->return_type = new_type(TYPE_INT);
        main_fn->body = m->top_level_stmts;
        main_fn->body_count = m->top_level_count;
        main_fn->is_pub = false;
        main_fn->is_async = false;
        main_fn->generic_count = 0;
    }
    
    // Generate functions (skip generic functions - they're instantiated separately)
    for (int i = 0; i < m->function_count; i++) {
        if (m->functions[i].generic_count == 0) {
            llvm_function(&lg, &m->functions[i]);
            llvm_emit(&lg, "");
        }
    }
    
    // Generate generic instantiations
    for (int i = 0; i < lg.generic_instance_count; i++) {
        FnDef* generic_fn = lg.generic_instances[i].generic_fn;
        Type* concrete_type = lg.generic_instances[i].concrete_type;
        const char* inst_name = lg.generic_instances[i].instantiated_name;
        
        // Create instantiated function by substituting type
        // For now, just generate a wrapper that calls with concrete type
        // Full implementation would clone and transform the entire function
        
        // Generate signature
        const char* ret_type = "i64";
        if (generic_fn->return_type && generic_fn->return_type->kind == TYPE_GENERIC) {
            // Return type is generic, use concrete type
            if (concrete_type->kind == TYPE_STR) ret_type = "i8*";
            else if (concrete_type->kind == TYPE_ARRAY) ret_type = "i64*";
        }
        
        fprintf(lg.out, "define %s @%s(", ret_type, inst_name);
        for (int j = 0; j < generic_fn->param_count; j++) {
            if (j > 0) fprintf(lg.out, ", ");
            // If parameter type is generic, use concrete type
            if (generic_fn->params[j].type && generic_fn->params[j].type->kind == TYPE_GENERIC) {
                if (concrete_type->kind == TYPE_STR) {
                    fprintf(lg.out, "i8* %%param.%s", generic_fn->params[j].name);
                } else if (concrete_type->kind == TYPE_ARRAY) {
                    fprintf(lg.out, "i64* %%param.%s", generic_fn->params[j].name);
                } else {
                    fprintf(lg.out, "i64 %%param.%s", generic_fn->params[j].name);
                }
            } else {
                fprintf(lg.out, "i64 %%param.%s", generic_fn->params[j].name);
            }
        }
        fprintf(lg.out, ") {\n");
        fprintf(lg.out, "entry:\n");
        
        // Save state
        int saved_temp = lg.temp_count;
        int saved_var = lg.var_count;
        lg.temp_count = 0;
        lg.var_count = 0;
        
        // Set current function for return type
        lg.current_function = generic_fn;
        
        // Allocate and store parameters
        for (int j = 0; j < generic_fn->param_count; j++) {
            int alloca_reg = llvm_new_temp(&lg);
            
            // Determine parameter type
            if (generic_fn->params[j].type && generic_fn->params[j].type->kind == TYPE_GENERIC) {
                if (concrete_type->kind == TYPE_STR) {
                    llvm_emit(&lg, "  %%%d = alloca i8*", alloca_reg);
                    llvm_emit(&lg, "  store i8* %%param.%s, i8** %%%d", generic_fn->params[j].name, alloca_reg);
                } else if (concrete_type->kind == TYPE_ARRAY) {
                    llvm_emit(&lg, "  %%%d = alloca i64*", alloca_reg);
                    llvm_emit(&lg, "  store i64* %%param.%s, i64** %%%d", generic_fn->params[j].name, alloca_reg);
                } else {
                    llvm_emit(&lg, "  %%%d = alloca i64", alloca_reg);
                    llvm_emit(&lg, "  store i64 %%param.%s, i64* %%%d", generic_fn->params[j].name, alloca_reg);
                }
                llvm_add_var(&lg, generic_fn->params[j].name, alloca_reg, concrete_type);
            } else {
                llvm_emit(&lg, "  %%%d = alloca i64", alloca_reg);
                llvm_emit(&lg, "  store i64 %%param.%s, i64* %%%d", generic_fn->params[j].name, alloca_reg);
                llvm_add_var(&lg, generic_fn->params[j].name, alloca_reg, generic_fn->params[j].type);
            }
        }
        
        // Generate body
        for (int j = 0; j < generic_fn->body_count; j++) {
            Stmt* stmt = generic_fn->body[j];
            
            // Special handling for return statements in generic functions
            if (stmt->kind == STMT_RETURN && stmt->ret.value && stmt->ret.value->kind == EXPR_IDENT) {
                // Returning a parameter - need to load it
                int var_reg = llvm_find_var(&lg, stmt->ret.value->ident);
                if (var_reg >= 0) {
                    int loaded = llvm_new_temp(&lg);
                    if (concrete_type->kind == TYPE_STR) {
                        llvm_emit(&lg, "  %%%d = load i8*, i8** %%%d", loaded, var_reg);
                        llvm_emit(&lg, "  ret i8* %%%d", loaded);
                    } else if (concrete_type->kind == TYPE_ARRAY) {
                        llvm_emit(&lg, "  %%%d = load i64*, i64** %%%d", loaded, var_reg);
                        llvm_emit(&lg, "  ret i64* %%%d", loaded);
                    } else {
                        llvm_emit(&lg, "  %%%d = load i64, i64* %%%d", loaded, var_reg);
                        llvm_emit(&lg, "  ret i64 %%%d", loaded);
                    }
                    continue;  // Skip normal statement generation
                }
            }
            
            llvm_stmt(&lg, stmt);
        }
        
        // Restore state
        lg.temp_count = saved_temp;
        lg.var_count = saved_var;
        
        fprintf(lg.out, "}\n\n");
    }
    
    // Generate spawn functions
    for (int i = 0; i < lg.spawn_count; i++) {
        Stmt* s = lg.spawn_blocks[i];
        int cap_count = s->spawn.captured_count;
        
        fprintf(lg.out, "define void @__spawn_%d(i8* %%ctx) {\n", i);
        fprintf(lg.out, "entry:\n");
        
        // Generate spawn body with fresh temp counter but keep string table
        int saved_temp = lg.temp_count;
        int saved_var = lg.var_count;
        lg.temp_count = 1;  // Start from 1 since %ctx is %0
        lg.var_count = 0;  // Spawn blocks have their own scope
        
        // Load captured variables from context
        if (cap_count > 0) {
            for (int j = 0; j < cap_count; j++) {
                Type* var_type = s->spawn.captured_types[j];
                
                // Add variable to spawn function scope
                strcpy(lg.vars[lg.var_count], s->spawn.captured_vars[j]);
                
                // Load variable address from context
                int addr_t = llvm_new_temp(&lg);
                int cast_t = llvm_new_temp(&lg);
                int var_t = llvm_new_temp(&lg);
                lg.var_regs[lg.var_count] = var_t;
                
                fprintf(lg.out, "  %%%d = getelementptr i8, i8* %%ctx, i64 %d\n", addr_t, j * 8);
                
                if (var_type && var_type->kind == TYPE_STR) {
                    // String variable
                    fprintf(lg.out, "  %%%d = bitcast i8* %%%d to i8***\n", cast_t, addr_t);
                    fprintf(lg.out, "  %%%d = load i8**, i8*** %%%d\n", var_t, cast_t);
                    lg.var_types[lg.var_count] = var_type;
                } else if (var_type && var_type->kind == TYPE_ARRAY) {
                    // Array variable
                    fprintf(lg.out, "  %%%d = bitcast i8* %%%d to i64***\n", cast_t, addr_t);
                    fprintf(lg.out, "  %%%d = load i64**, i64*** %%%d\n", var_t, cast_t);
                    lg.var_types[lg.var_count] = var_type;
                } else {
                    // Integer variable (default)
                    fprintf(lg.out, "  %%%d = bitcast i8* %%%d to i64**\n", cast_t, addr_t);
                    fprintf(lg.out, "  %%%d = load i64*, i64** %%%d\n", var_t, cast_t);
                    lg.var_types[lg.var_count] = var_type ? var_type : new_type(TYPE_INT);
                }
                
                lg.var_count++;
            }
        }
        
        for (int j = 0; j < s->spawn.body_count; j++) {
            llvm_stmt(&lg, s->spawn.body[j]);
        }
        
        lg.temp_count = saved_temp;
        lg.var_count = saved_var;
        
        fprintf(lg.out, "  ret void\n");
        fprintf(lg.out, "}\n\n");
    }
    
    // Generate lambda functions
    for (int i = 0; i < lg.lambda_count; i++) {
        Expr* lambda = lg.lambdas[i];
        
        // Function signature
        fprintf(lg.out, "define i64 @__lambda_%d(", lambda->lambda.id);
        for (int j = 0; j < lambda->lambda.param_count; j++) {
            if (j > 0) fprintf(lg.out, ", ");
            fprintf(lg.out, "i64 %%param_%s", lambda->lambda.params[j]);
        }
        fprintf(lg.out, ") {\n");
        fprintf(lg.out, "entry:\n");
        
        // Save and reset state
        int saved_temp = lg.temp_count;
        int saved_var = lg.var_count;
        lg.temp_count = 0;
        lg.var_count = 0;
        
        // Allocate stack space for parameters and store them
        for (int j = 0; j < lambda->lambda.param_count; j++) {
            int alloca_reg = llvm_new_temp(&lg);
            fprintf(lg.out, "  %%%d = alloca i64\n", alloca_reg);
            fprintf(lg.out, "  store i64 %%param_%s, i64* %%%d\n", lambda->lambda.params[j], alloca_reg);
            strcpy(lg.vars[lg.var_count], lambda->lambda.params[j]);
            lg.var_regs[lg.var_count] = alloca_reg;
            lg.var_count++;
        }
        
        // Generate body
        for (int j = 0; j < lambda->lambda.body_count; j++) {
            llvm_stmt(&lg, lambda->lambda.body[j]);
        }
        
        // Restore state
        lg.temp_count = saved_temp;
        lg.var_count = saved_var;
        
        fprintf(lg.out, "}\n\n");
    }
}

#ifdef COMPILER_MAIN
int compiler_main(int argc, char** argv) {
#else
