    printf("Imports: %d\n", m->import_count);
    for (int i = 0; i < m->import_count; i++) {
        printf("  import %s", m->imports[i].path);
        if (m->imports[i].alias[0]) printf(" as %s", m->imports[i].alias);
        printf("\n");
    }
    
    printf("Functions: %d\n", m->function_count);
    for (int i = 0; i < m->function_count; i++) {
        FnDef* f = &m->functions[i];
        printf("  %s%sfn %s(", f->is_pub ? "pub " : "", f->is_async ? "async " : "", f->name);
        for (int j = 0; j < f->param_count; j++) {
            if (j > 0) printf(", ");
            printf("%s", f->params[j].name);
            if (f->params[j].type) { printf(": "); print_type(f->params[j].type); }
        }
        printf(")");
        if (f->return_type) { printf(" -> "); print_type(f->return_type); }
        printf(" { %d stmts }\n", f->body_count);
    }
    
    printf("Structs: %d\n", m->struct_count);
    for (int i = 0; i < m->struct_count; i++) {
        StructDef* s = &m->structs[i];
        printf("  %sstruct %s { %d fields, %d methods }\n", 
               s->is_pub ? "pub " : "", s->name, s->field_count, s->method_count);
    }
    
    printf("Enums: %d\n", m->enum_count);
    for (int i = 0; i < m->enum_count; i++) {
        printf("  %senum %s { %d variants }\n", 
               m->enums[i].is_pub ? "pub " : "", m->enums[i].name, m->enums[i].variant_count);
    }
    
    printf("Interfaces: %d\n", m->interface_count);
    for (int i = 0; i < m->interface_count; i++) {
        printf("  %sinterface %s { %d methods }\n",
               m->interfaces[i].is_pub ? "pub " : "", m->interfaces[i].name, m->interfaces[i].method_count);
    }
    
    printf("Tests: %d\n", m->test_count);
    for (int i = 0; i < m->test_count; i++) {
        printf("  test \"%s\"\n", m->tests[i].name);
    }
}

// ============================================================================
// SECTION 3: TYPE CHECKER - Type System
// Lines 2923-5310 (~2400 lines)
// 
// Validates types and resolves symbols.
// Key functions: typecheck_module(), tc_check_expr(), tc_check_stmt()
// ============================================================================

// ============================================================================
// Error Reporting
// ============================================================================

void error(const char* filename, int line, int col, const char* fmt, ...) {
    fprintf(stderr, "%s:%d:%d: error: ", filename, line, col);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

void fatal(const char* fmt, ...) {
    fprintf(stderr, "fatal: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(1);
}

// ============================================================================
// Type Checker
// ============================================================================

typedef struct Symbol {
    char name[MAX_IDENT_LEN];
    Type* type;
    bool is_mut;
    int scope_depth;
} Symbol;

typedef struct TypeChecker {
    Module* module;
    const char* filename;
    Symbol symbols[1024];
    int symbol_count;
    int scope_depth;
    Type* current_return_type;
    FnDef* current_function;
    bool had_error;
} TypeChecker;

static TypeChecker* typechecker_new(Module* m, const char* filename) {
    TypeChecker* tc = calloc(1, sizeof(TypeChecker));
    tc->module = m;
    tc->filename = filename;
    return tc;
}

static void tc_error(TypeChecker* tc, int line, int col, const char* fmt, ...) {
    fprintf(stderr, "\n");
    fprintf(stderr, "╭─ Error in %s:%d:%d\n", tc->filename, line, col);
    fprintf(stderr, "│\n");
    fprintf(stderr, "│  ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fprintf(stderr, "│\n");
    fprintf(stderr, "╰─ Type error\n");
    fprintf(stderr, "\n");
    tc->had_error = true;
}

static void tc_enter_scope(TypeChecker* tc) { tc->scope_depth++; }

static void tc_exit_scope(TypeChecker* tc) {
    while (tc->symbol_count > 0 && tc->symbols[tc->symbol_count - 1].scope_depth == tc->scope_depth) {
        tc->symbol_count--;
    }
    tc->scope_depth--;
}

static void tc_define(TypeChecker* tc, const char* name, Type* type, bool is_mut) {
    Symbol* s = &tc->symbols[tc->symbol_count++];
    strcpy(s->name, name);
    s->type = type;
    s->is_mut = is_mut;
    s->scope_depth = tc->scope_depth;
}

static Symbol* tc_lookup(TypeChecker* tc, const char* name) {
    for (int i = tc->symbol_count - 1; i >= 0; i--) {
        if (strcmp(tc->symbols[i].name, name) == 0) return &tc->symbols[i];
    }
    return NULL;
}

static bool tc_is_module_imported(TypeChecker* tc, const char* module_name) {
    for (int i = 0; i < tc->module->import_count; i++) {
        const char* path = tc->module->imports[i].path;
        // Check if import path matches module name exactly
        // We want "import io" not "import std/io"
        if (strcmp(path, module_name) == 0) return true;
        
        // Also check with dot/slash conversion
        char converted_module[MAX_IDENT_LEN];
        strcpy(converted_module, module_name);
        // Convert dots to slashes
        for (int k = 0; converted_module[k]; k++) {
            if (converted_module[k] == '.') converted_module[k] = '/';
        }
        if (strcmp(path, converted_module) == 0) return true;
    }
    return false;
}

// Helper function to build full module path from field expression
static bool build_module_path(Expr* e, char* path) {
    if (!e) return false;
    
    if (e->kind == EXPR_IDENT) {
        strcpy(path, e->ident);
        return true;
    } else if (e->kind == EXPR_FIELD) {
        char parent_path[MAX_IDENT_LEN];
        if (build_module_path(e->field.object, parent_path)) {
            snprintf(path, MAX_IDENT_LEN, "%s.%s", parent_path, e->field.field);
            return true;
        }
    }
    return false;
}

static FnDef* tc_lookup_fn(TypeChecker* tc, const char* name) {
    for (int i = 0; i < tc->module->function_count; i++) {
        if (strcmp(tc->module->functions[i].name, name) == 0) return &tc->module->functions[i];
    }
    return NULL;
}

static StructDef* tc_lookup_struct(TypeChecker* tc, const char* name) {
    for (int i = 0; i < tc->module->struct_count; i++) {
        if (strcmp(tc->module->structs[i].name, name) == 0) return &tc->module->structs[i];
    }
    return NULL;
}

static bool tc_is_enum_variant(TypeChecker* tc, const char* name) {
    for (int i = 0; i < tc->module->enum_count; i++) {
        for (int j = 0; j < tc->module->enums[i].variant_count; j++) {
            if (strcmp(tc->module->enums[i].variants[j].name, name) == 0) {
                return true;
            }
        }
    }
    return false;
}

static bool types_equal(Type* a, Type* b) {
    if (!a || !b) return true;  // Unknown types are compatible
    if (a->kind == TYPE_ANY || b->kind == TYPE_ANY) return true;
    if (a->kind != b->kind) return false;
    if (a->kind == TYPE_NAMED) return strcmp(a->name, b->name) == 0;
    if (a->kind == TYPE_ARRAY || a->kind == TYPE_OPTIONAL || a->kind == TYPE_POINTER) {
        if (!a->inner || !b->inner) return true;  // Unknown inner types are compatible
        return types_equal(a->inner, b->inner);
    }
    if (a->kind == TYPE_RESULT) {
        // Result types are compatible if both ok and err types match (or are unknown)
        bool ok_match = !a->inner || !b->inner || types_equal(a->inner, b->inner);
        bool err_match = !a->value_type || !b->value_type || types_equal(a->value_type, b->value_type);
        return ok_match && err_match;
    }
    if (a->kind == TYPE_GENERIC) {
        if (strcmp(a->name, b->name) != 0) return false;
        if (a->param_count != b->param_count) return false;
        for (int i = 0; i < a->param_count; i++) {
            if (!types_equal(a->params[i], b->params[i])) return false;
        }
        return true;
    }
    return true;
}

static const char* type_name(Type* t) {
    static char buf[256];
    if (!t) return "unknown";
    switch (t->kind) {
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_STR: return "str";
        case TYPE_BOOL: return "bool";
        case TYPE_BYTE: return "byte";
        case TYPE_ANY: return "any";
        case TYPE_VOID: return "void";
        case TYPE_NAMED: return t->name;
        case TYPE_POINTER:
            if (t->inner) snprintf(buf, 256, "*%s", type_name(t->inner));
            else snprintf(buf, 256, "*");
            return buf;
        case TYPE_ARRAY: 
            if (t->inner) snprintf(buf, 256, "[%s]", type_name(t->inner)); 
            else snprintf(buf, 256, "[]");
            return buf;
        case TYPE_OPTIONAL: 
            if (t->inner) snprintf(buf, 256, "?%s", type_name(t->inner));
            else snprintf(buf, 256, "?");
            return buf;
        case TYPE_RESULT:
            snprintf(buf, 256, "Result[%s, %s]", 
                     t->inner ? type_name(t->inner) : "?",
                     t->value_type ? type_name(t->value_type) : "?");
            return buf;
        case TYPE_GENERIC: {
            int len = snprintf(buf, 256, "%s[", t->name);
            for (int i = 0; i < t->param_count && len < 250; i++) {
                if (i > 0) len += snprintf(buf + len, 256 - len, ", ");
                len += snprintf(buf + len, 256 - len, "%s", type_name(t->params[i]));
            }
            snprintf(buf + len, 256 - len, "]");
            return buf;
        }
        default: return "?";
    }
}

static Type* tc_check_expr(TypeChecker* tc, Expr* e);

static Type* tc_check_expr(TypeChecker* tc, Expr* e) {
    if (!e) return NULL;
    
    switch (e->kind) {
        case EXPR_INT: return new_type(TYPE_INT);
        case EXPR_FLOAT: return new_type(TYPE_FLOAT);
        case EXPR_STRING: return new_type(TYPE_STR);
        case EXPR_BOOL: return new_type(TYPE_BOOL);
        
        case EXPR_IDENT: {
            // _ is a wildcard pattern, always valid
            if (strcmp(e->ident, "_") == 0) return new_type(TYPE_ANY);
            Symbol* s = tc_lookup(tc, e->ident);
            if (!s) {
                // Check if it's an enum variant
                if (tc_is_enum_variant(tc, e->ident)) return new_type(TYPE_INT);
                // Check if it's a function name
                if (tc_lookup_fn(tc, e->ident)) return new_type(TYPE_FUNCTION);
                // Check if it's a builtin function
                if (strcmp(e->ident, "print") == 0 || strcmp(e->ident, "println") == 0 || strcmp(e->ident, "print_raw") == 0 || strcmp(e->ident, "print_str") == 0 ||
                    strcmp(e->ident, "assert") == 0 || strcmp(e->ident, "args") == 0 || 
                    strcmp(e->ident, "syscall") == 0 || strcmp(e->ident, "len") == 0 ||
                    strcmp(e->ident, "ord") == 0 || strcmp(e->ident, "chr") == 0 ||
                    strcmp(e->ident, "min") == 0 || strcmp(e->ident, "max") == 0 ||
                    strcmp(e->ident, "substring") == 0 || strcmp(e->ident, "str_split") == 0 ||
                    strcmp(e->ident, "str_find") == 0 || strcmp(e->ident, "str_concat") == 0 ||
                    strcmp(e->ident, "str_replace") == 0 ||
                    strcmp(e->ident, "str_cmp") == 0 || strcmp(e->ident, "char_at") == 0 ||
                    strcmp(e->ident, "int_to_str") == 0 || strcmp(e->ident, "str_substr") == 0 ||
                    strcmp(e->ident, "str_to_int") == 0 || strcmp(e->ident, "abs") == 0 ||
                    strcmp(e->ident, "sqrtf") == 0 || strcmp(e->ident, "int_to_float") == 0 ||
                    strcmp(e->ident, "float_to_int") == 0 || strcmp(e->ident, "to_string") == 0 ||
                    strcmp(e->ident, "format") == 0 || strcmp(e->ident, "str_format") == 0 ||
                    strcmp(e->ident, "read_file") == 0 || strcmp(e->ident, "write_file") == 0 ||
                    strcmp(e->ident, "append_file") == 0 || strcmp(e->ident, "file_exists") == 0 ||
                    strcmp(e->ident, "file_size") == 0 || strcmp(e->ident, "read_stdin_line") == 0 ||
                    strcmp(e->ident, "mkdir") == 0 || strcmp(e->ident, "rmdir") == 0 ||
                    strcmp(e->ident, "rename_file") == 0 || strcmp(e->ident, "getcwd") == 0 ||
                    strcmp(e->ident, "getpid") == 0 || strcmp(e->ident, "time_now") == 0 ||
                    strcmp(e->ident, "sleep_ms") == 0 || strcmp(e->ident, "sleep") == 0 ||
                    strcmp(e->ident, "clock") == 0 || strcmp(e->ident, "random") == 0 || strcmp(e->ident, "exit") == 0 ||
                    strcmp(e->ident, "task_await") == 0 || strcmp(e->ident, "task_join_all") == 0 ||
                    strcmp(e->ident, "gpu_device_count") == 0) {
                    return new_type(TYPE_FUNCTION);
                }
                
                // Check if this identifier might be part of a module path
                // Look for any imported modules that start with this identifier
                for (int i = 0; i < tc->module->import_count; i++) {
                    const char* import_path = tc->module->imports[i].path;
                    if (strncmp(import_path, e->ident, strlen(e->ident)) == 0 &&
                        (import_path[strlen(e->ident)] == '.' || import_path[strlen(e->ident)] == '\0')) {
                        // This might be part of a module path, return a special type
                        return new_type(TYPE_ANY);
                    }
                }
                
                // Enhanced error with suggestion
                tc_error(tc, e->line, e->column, "undefined variable '%s'", e->ident);
                
                // Suggest similar variables
                for (int i = tc->symbol_count - 1; i >= 0; i--) {
                    if (strstr(tc->symbols[i].name, e->ident) || strstr(e->ident, tc->symbols[i].name)) {
                        fprintf(stderr, "   💡 Did you mean '%s'?\n", tc->symbols[i].name);
                        break;
                    }
                }
                fprintf(stderr, "   💡 Make sure the variable is declared before use\n");
                
                return new_type(TYPE_ANY);
            }
            return s->type;
        }
        
        case EXPR_BINARY: {
            Type* left = tc_check_expr(tc, e->binary.left);
            Type* right = tc_check_expr(tc, e->binary.right);
            TokenKind op = e->binary.op;
            
            // Range operators return array
            if (op == TOK_DOTDOT || op == TOK_DOTDOTEQ) {
                Type* t = new_type(TYPE_ARRAY);
                t->inner = left;
                return t;
            }
            
            // 'in' operator: elem in array -> bool
            if (op == TOK_IN) {
                // Right side should be an array
                return new_type(TYPE_BOOL);
            }
            
            // String concatenation
            if (op == TOK_PLUS && left->kind == TYPE_STR && right->kind == TYPE_STR) {
                return new_type(TYPE_STR);
            }
            
            // Comparison operators return bool
            if (op == TOK_EQEQ || op == TOK_NOTEQ || op == TOK_LT || op == TOK_GT ||
                op == TOK_LTEQ || op == TOK_GTEQ) {
                return new_type(TYPE_BOOL);
            }
            
            // Logical operators
            if (op == TOK_AND || op == TOK_OR) {
                if (left->kind != TYPE_BOOL) tc_error(tc, e->line, e->column, "expected bool, got %s", type_name(left));
                if (right->kind != TYPE_BOOL) tc_error(tc, e->line, e->column, "expected bool, got %s", type_name(right));
                return new_type(TYPE_BOOL);
            }
            
            // Arithmetic - types must match
            if (!types_equal(left, right) && left->kind != TYPE_ANY && right->kind != TYPE_ANY) {
                tc_error(tc, e->line, e->column, "type mismatch: %s vs %s", type_name(left), type_name(right));
            }
            return left;
        }
        
        case EXPR_UNARY: {
            Type* operand = tc_check_expr(tc, e->unary.operand);
            if (e->unary.op == TOK_NOT || e->unary.op == TOK_BANG) return new_type(TYPE_BOOL);
            return operand;
        }
        
        case EXPR_ADDR: {
            Type* operand = tc_check_expr(tc, e->unary.operand);
            Type* ptr = new_type(TYPE_POINTER);
            ptr->inner = operand;
            return ptr;
        }
        
        case EXPR_DEREF: {
            Type* operand = tc_check_expr(tc, e->unary.operand);
            if (operand && operand->kind == TYPE_POINTER && operand->inner) {
                return operand->inner;
            }
            return new_type(TYPE_ANY);
        }
        
        case EXPR_CALL: {
            // Check if this is a module.function() call
            if (e->call.func->kind == EXPR_FIELD && 
                e->call.func->field.object->kind == EXPR_IDENT) {
                const char* module = e->call.func->field.object->ident;
                const char* function = e->call.func->field.field;
                
                // Check if this is 'self' in an extension method
                if (strcmp(module, "self") == 0 &&
                    tc->current_function && tc->current_function->is_extension) {
                    // This is a method call on 'self' in an extension method
                    // Handle it as a normal method call, not a module call
                    Type* func_type = tc_check_expr(tc, e->call.func);
                    (void)func_type;
                    for (int i = 0; i < e->call.arg_count; i++) {
                        tc_check_expr(tc, e->call.args[i]);
                    }
                    // Handle method calls (obj.method())
                    if (e->call.func->kind == EXPR_FIELD) {
                        const char* method_name = e->call.func->field.field;
                        // String methods
                        if (strcmp(method_name, "len") == 0) return new_type(TYPE_INT);
                        if (strcmp(method_name, "upper") == 0) return new_type(TYPE_STR);
                        if (strcmp(method_name, "lower") == 0) return new_type(TYPE_STR);
                        if (strcmp(method_name, "trim") == 0) return new_type(TYPE_STR);
                        if (strcmp(method_name, "contains") == 0) return new_type(TYPE_BOOL);
                        if (strcmp(method_name, "starts_with") == 0) return new_type(TYPE_BOOL);
                        if (strcmp(method_name, "ends_with") == 0) return new_type(TYPE_BOOL);
                        if (strcmp(method_name, "split") == 0) return new_type(TYPE_ARRAY);
                        if (strcmp(method_name, "replace") == 0) return new_type(TYPE_STR);
                        if (strcmp(method_name, "index_of") == 0) return new_type(TYPE_INT);
                        if (strcmp(method_name, "find") == 0) return new_type(TYPE_INT);
                        if (strcmp(method_name, "substring") == 0) return new_type(TYPE_STR);
                        if (strcmp(method_name, "char_at") == 0) return new_type(TYPE_STR);
                        // Int methods
                        if (strcmp(method_name, "abs") == 0) return new_type(TYPE_INT);
                        if (strcmp(method_name, "to_str") == 0) return new_type(TYPE_STR);
                        if (strcmp(method_name, "to_int") == 0) return new_type(TYPE_INT);
                        if (strcmp(method_name, "to_float") == 0) return new_type(TYPE_FLOAT);
                        // Float methods
                        if (strcmp(method_name, "floor") == 0) return new_type(TYPE_INT);
                        if (strcmp(method_name, "ceil") == 0) return new_type(TYPE_INT);
                        if (strcmp(method_name, "round") == 0) return new_type(TYPE_INT);
                        // Array methods
                        if (strcmp(method_name, "get") == 0) return new_type(TYPE_INT);
                        if (strcmp(method_name, "get_str") == 0) return new_type(TYPE_STR);
                        if (strcmp(method_name, "reverse") == 0) return new_type(TYPE_ARRAY);
                        if (strcmp(method_name, "append") == 0) return new_type(TYPE_ARRAY);
                        if (strcmp(method_name, "contains") == 0) return new_type(TYPE_INT);
                        if (strcmp(method_name, "prepend") == 0) return new_type(TYPE_ARRAY);
                        if (strcmp(method_name, "filter") == 0) return new_type(TYPE_ARRAY);
                        if (strcmp(method_name, "map") == 0) return new_type(TYPE_ARRAY);
                        if (strcmp(method_name, "join") == 0) return new_type(TYPE_STR);
                    }
                    return new_type(TYPE_ANY);
                }
                
                const char* builtin_name = map_module_function(module, function);
                if (builtin_name) {
                    // Check if module was imported (required for new import system)
                    if (!tc_is_module_imported(tc, module)) {
                        tc_error(tc, e->line, e->column, "module '%s' not imported (add 'import %s')", module, module);
                        return new_type(TYPE_ANY);
                    }
                    // It's a valid module.function() call - check args
                    for (int i = 0; i < e->call.arg_count; i++) {
                        tc_check_expr(tc, e->call.args[i]);
                    }
                    // Return appropriate type based on function
                    return get_builtin_return_type(builtin_name);
                } else {
                    // Check for user-defined module.function
                    if (tc_is_module_imported(tc, module)) {
                        char prefixed_name[MAX_IDENT_LEN];
                        // Convert module.function to module/function.function for lookup
                        char module_path[MAX_IDENT_LEN];
                        strcpy(module_path, module);
                        // Convert dots to slashes in module path
                        for (int k = 0; module_path[k]; k++) {
                            if (module_path[k] == '.') module_path[k] = '/';
                        }
                        snprintf(prefixed_name, MAX_IDENT_LEN, "%s.%s", module_path, function);
                        FnDef* fn = tc_lookup_fn(tc, prefixed_name);
                        if (fn) {
                            // Check args
                            for (int i = 0; i < e->call.arg_count; i++) {
                                tc_check_expr(tc, e->call.args[i]);
                            }
                            return fn->return_type ? fn->return_type : new_type(TYPE_ANY);
                        } else {
                            tc_error(tc, e->line, e->column, "function '%s' not found in module '%s'", function, module);
                            return new_type(TYPE_ANY);
                        }
                    } else {
                        tc_error(tc, e->line, e->column, "module '%s' not imported (add 'import %s')", module, module);
                        return new_type(TYPE_ANY);
                    }
                }
            }
            
            Type* func_type = tc_check_expr(tc, e->call.func);
            (void)func_type;
            for (int i = 0; i < e->call.arg_count; i++) {
                tc_check_expr(tc, e->call.args[i]);
            }
            // Look up return type if it's a known function
            if (e->call.func->kind == EXPR_IDENT) {
                FnDef* fn = tc_lookup_fn(tc, e->call.func->ident);
                if (fn && fn->return_type) {
                    // If generic call with type args, substitute type parameters
                    if (e->call.type_arg_count > 0 && fn->generic_count > 0) {
                        Type* ret = fn->return_type;
                        // If return type is a generic parameter, substitute it
                        if (ret->kind == TYPE_NAMED) {
                            for (int i = 0; i < fn->generic_count && i < e->call.type_arg_count; i++) {
                                if (strcmp(ret->name, fn->generics[i]) == 0) {
                                    return e->call.type_args[i];
                                }
                            }
                        }
                        return ret;
                    }
                    return fn->return_type;
                }
            }
            // Handle method calls (obj.method())
            if (e->call.func->kind == EXPR_FIELD) {
                const char* method_name = e->call.func->field.field;
                // String methods
                if (strcmp(method_name, "len") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "upper") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "lower") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "trim") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "contains") == 0) return new_type(TYPE_BOOL);
                if (strcmp(method_name, "starts_with") == 0) return new_type(TYPE_BOOL);
                if (strcmp(method_name, "ends_with") == 0) return new_type(TYPE_BOOL);
                if (strcmp(method_name, "split") == 0) return new_type(TYPE_ARRAY);
                if (strcmp(method_name, "replace") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "index_of") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "find") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "substring") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "char_at") == 0) return new_type(TYPE_STR);
                // Int methods
                if (strcmp(method_name, "abs") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "to_str") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "to_int") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "to_float") == 0) return new_type(TYPE_FLOAT);
                // Float methods
                if (strcmp(method_name, "floor") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "ceil") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "round") == 0) return new_type(TYPE_INT);
                // Array methods
                if (strcmp(method_name, "get") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "get_str") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "reverse") == 0) return new_type(TYPE_ARRAY);
                if (strcmp(method_name, "append") == 0) return new_type(TYPE_ARRAY);
                if (strcmp(method_name, "contains") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "prepend") == 0) return new_type(TYPE_ARRAY);
                if (strcmp(method_name, "filter") == 0) return new_type(TYPE_ARRAY);
                if (strcmp(method_name, "map") == 0) return new_type(TYPE_ARRAY);
                if (strcmp(method_name, "join") == 0) return new_type(TYPE_STR);
                
                // Look up extension methods
                Type* obj_type = tc_check_expr(tc, e->call.func->field.object);
                const char* obj_type_name = type_name(obj_type);
                if (obj_type_name) {
                    for (int i = 0; i < tc->module->function_count; i++) {
                        FnDef* fn = &tc->module->functions[i];
                        if (fn && fn->is_extension && fn->extension_type && fn->extension_method &&
                            strcmp(fn->extension_type, obj_type_name) == 0 &&
                            strcmp(fn->extension_method, method_name) == 0) {
                            return fn->return_type ? fn->return_type : new_type(TYPE_ANY);
                        }
                    }
                }
                
                // Look up method in all structs
                for (int i = 0; i < tc->module->struct_count; i++) {
                    StructDef* s = &tc->module->structs[i];
                    for (int j = 0; j < s->method_count; j++) {
                        if (strcmp(s->methods[j].name, method_name) == 0) {
                            return s->methods[j].return_type;
                        }
                    }
                }
            }
            return new_type(TYPE_ANY);
        }
        
        case EXPR_INDEX: {
            Type* obj = tc_check_expr(tc, e->index.object);
            tc_check_expr(tc, e->index.index);
            if (obj->kind == TYPE_ARRAY) return obj->inner;
            if (obj->kind == TYPE_STR) return new_type(TYPE_STR);
            return new_type(TYPE_ANY);
        }
        
        case EXPR_SLICE: {
            Type* obj = tc_check_expr(tc, e->slice.object);
            if (e->slice.start) tc_check_expr(tc, e->slice.start);
            if (e->slice.end) tc_check_expr(tc, e->slice.end);
            if (e->slice.step) tc_check_expr(tc, e->slice.step);
            // Slice returns same type as object (array or string)
            return obj;
        }
        
        case EXPR_AWAIT: {
            // Await unwraps a Future<T> to T
            // For now, just return the inner type (simplified)
            Type* future_type = tc_check_expr(tc, e->await_expr.future);
            // TODO: Proper Future type handling
            return future_type;
        }
        
        case EXPR_FIELD: {
            // Check if this is 'self' in an extension method
            if (e->field.object->kind == EXPR_IDENT && 
                strcmp(e->field.object->ident, "self") == 0 &&
                tc->current_function && tc->current_function->is_extension) {
                // This is a method call on 'self' in an extension method
                // Type check 'self' normally and return the field access
                Type* obj = tc_check_expr(tc, e->field.object);
                if (obj && obj->kind == TYPE_NAMED) {
                    StructDef* s = tc_lookup_struct(tc, obj->name);
                    if (s) {
                        for (int i = 0; i < s->field_count; i++) {
                            if (strcmp(s->fields[i].name, e->field.field) == 0) {
                                return s->fields[i].type;
                            }
                        }
                    }
                }
                return new_type(TYPE_ANY);
            }
            
            // FIXED: Check if object is a VARIABLE first (method call)
            if (e->field.object->kind == EXPR_IDENT) {
                Symbol* sym = tc_lookup(tc, e->field.object->ident);
                if (sym) {
                    // It's a variable - this is a method call
                    return new_type(TYPE_FUNCTION);
                }
                
                // Not a variable - check for module
                if (tc_is_module_imported(tc, e->field.object->ident)) {
                    // This is a module namespace, return function type
                    return new_type(TYPE_FUNCTION);
                }
            }
            
            // Try building full module path for nested cases like compiler.lexer
            char full_path[MAX_IDENT_LEN];
            if (build_module_path(e->field.object, full_path)) {
                if (tc_is_module_imported(tc, full_path)) {
                    // This is a nested module namespace, return function type
                    return new_type(TYPE_FUNCTION);
                }
            }
            
            // Try to type check the object, but don't fail if it's part of a module path
            Type* obj = NULL;
            if (e->field.object->kind == EXPR_IDENT) {
                // For identifiers, check if they might be part of a module path
                // by building the full path and checking if any imported module starts with it
                char test_path[MAX_IDENT_LEN];
                snprintf(test_path, MAX_IDENT_LEN, "%s.%s", e->field.object->ident, e->field.field);
                
                // Check if this path or any extension of it is an imported module
                bool might_be_module = false;
                for (int i = 0; i < tc->module->import_count; i++) {
                    const char* import_path = tc->module->imports[i].path;
                    if (strncmp(import_path, test_path, strlen(test_path)) == 0) {
                        might_be_module = true;
                        break;
                    }
                }
                
                if (might_be_module) {
                    // This might be part of a module path, return function type
                    return new_type(TYPE_FUNCTION);
                } else {
                    // Normal identifier, type check it
                    obj = tc_check_expr(tc, e->field.object);
                }
            } else {
                obj = tc_check_expr(tc, e->field.object);
            }
            
            if (obj && obj->kind == TYPE_NAMED) {
                StructDef* s = tc_lookup_struct(tc, obj->name);
                if (s) {
                    for (int i = 0; i < s->field_count; i++) {
                        if (strcmp(s->fields[i].name, e->field.field) == 0) {
                            return s->fields[i].type;
                        }
                    }
                }
            }
            return new_type(TYPE_ANY);
        }
        
        case EXPR_ARRAY: {
            Type* elem_type = NULL;
            for (int i = 0; i < e->array.count; i++) {
                Type* t = tc_check_expr(tc, e->array.elements[i]);
                if (!elem_type) elem_type = t;
            }
            Type* arr = new_type(TYPE_ARRAY);
            arr->inner = elem_type ? elem_type : new_type(TYPE_ANY);
            return arr;
        }
        
        case EXPR_STRUCT: {
            StructDef* s = tc_lookup_struct(tc, e->struct_lit.name);
            if (!s) {
                tc_error(tc, e->line, e->column, "unknown struct '%s'", e->struct_lit.name);
                return new_type(TYPE_ANY);
            }
            for (int i = 0; i < e->struct_lit.count; i++) {
                tc_check_expr(tc, e->struct_lit.values[i]);
            }
            // Return generic type if type args provided
            if (e->struct_lit.type_arg_count > 0) {
                Type* t = new_type(TYPE_GENERIC);
                strcpy(t->name, e->struct_lit.name);
                t->params = malloc(sizeof(Type*) * e->struct_lit.type_arg_count);
                t->param_count = e->struct_lit.type_arg_count;
                for (int i = 0; i < e->struct_lit.type_arg_count; i++) {
                    t->params[i] = e->struct_lit.type_args[i];
                }
                return t;
            }
            Type* t = new_type(TYPE_NAMED);
            strcpy(t->name, e->struct_lit.name);
            return t;
        }
        
        case EXPR_MAP: {
            for (int i = 0; i < e->map.count; i++) {
                tc_check_expr(tc, e->map.values[i]);
            }
            return new_type(TYPE_MAP);
        }
        
        case EXPR_IF: {
            Type* cond = tc_check_expr(tc, e->if_expr.cond);
            if (cond->kind != TYPE_BOOL) tc_error(tc, e->line, e->column, "condition must be bool");
            Type* then_t = tc_check_expr(tc, e->if_expr.then_expr);
            Type* else_t = tc_check_expr(tc, e->if_expr.else_expr);
            if (!types_equal(then_t, else_t)) {
                tc_error(tc, e->line, e->column, "if branches have different types");
            }
            return then_t;
        }
        
        case EXPR_TERNARY: {
            Type* cond = tc_check_expr(tc, e->ternary.cond);
            if (cond->kind != TYPE_BOOL) tc_error(tc, e->line, e->column, "ternary condition must be bool");
            Type* true_t = tc_check_expr(tc, e->ternary.true_val);
            Type* false_t = tc_check_expr(tc, e->ternary.false_val);
            if (!types_equal(true_t, false_t)) {
                tc_error(tc, e->line, e->column, "ternary branches have different types");
            }
            return true_t;
        }
        
        case EXPR_COMPREHENSION: {
            // List comprehension: [expr for var in iter if condition]
            Type* iter_type = tc_check_expr(tc, e->comprehension.iter);
            if (iter_type->kind != TYPE_ARRAY) {
                tc_error(tc, e->line, e->column, "comprehension iterator must be array");
            }
            
            // Add loop variable to scope
            Type* elem_type = iter_type->inner ? iter_type->inner : new_type(TYPE_INT);
            int saved_symbol_count = tc->symbol_count;
            strcpy(tc->symbols[tc->symbol_count].name, e->comprehension.var);
            tc->symbols[tc->symbol_count].type = elem_type;
            tc->symbols[tc->symbol_count].scope_depth = tc->scope_depth;
            tc->symbol_count++;
            
            // Check expression with loop variable in scope
            Type* expr_type = tc_check_expr(tc, e->comprehension.expr);
            
            // Check condition if present
            if (e->comprehension.condition) {
                Type* cond_type = tc_check_expr(tc, e->comprehension.condition);
                if (cond_type->kind != TYPE_BOOL) {
                    tc_error(tc, e->line, e->column, "comprehension condition must be bool");
                }
            }
            
            // Remove loop variable from scope
            tc->symbol_count = saved_symbol_count;
            
            // Return array of expr type
            Type* result = new_type(TYPE_ARRAY);
            result->inner = expr_type;
            return result;
        }
        
        case EXPR_SOME: {
            Type* inner = tc_check_expr(tc, e->some.value);
            Type* opt = new_type(TYPE_OPTIONAL);
            opt->inner = inner;
            return opt;
        }
        
        case EXPR_NONE: return new_type(TYPE_OPTIONAL);
        
        case EXPR_OK: {
            Type* inner = tc_check_expr(tc, e->some.value);
            Type* res = new_type(TYPE_RESULT);
            res->inner = inner;  // ok value type
            return res;
        }
        
        case EXPR_ERR: {
            Type* inner = tc_check_expr(tc, e->some.value);
            Type* res = new_type(TYPE_RESULT);
            res->value_type = inner;  // error type
            return res;
        }
        
        case EXPR_DEFAULT: {
            tc_check_expr(tc, e->default_expr.value);
            return tc_check_expr(tc, e->default_expr.default_val);
        }
        
        case EXPR_MATCH: {
            Type* match_type = tc_check_expr(tc, e->match_expr.value);
            Type* result_type = NULL;
            for (int i = 0; i < e->match_expr.arm_count; i++) {
                Expr* pat = e->match_expr.patterns[i];
                // Don't type-check binding patterns - they introduce variables
                bool is_binding_pattern = (pat->kind == EXPR_SOME || pat->kind == EXPR_OK || pat->kind == EXPR_ERR) &&
                                         pat->some.value && pat->some.value->kind == EXPR_IDENT;
                if (!is_binding_pattern) {
                    tc_check_expr(tc, pat);
                }
                
                tc_enter_scope(tc);
                // Add binding variable to scope if present
                if (e->match_expr.bindings[i][0] != '\0') {
                    // For some(x)/ok(x), binding gets the inner type
                    // For err(e), binding gets the error type (str for now)
                    Type* bind_type = new_type(TYPE_INT);  // Default
                    if (match_type && match_type->kind == TYPE_OPTIONAL && match_type->inner) {
                        bind_type = match_type->inner;
                    } else if (match_type && match_type->kind == TYPE_RESULT) {
                        if (pat->kind == EXPR_ERR) {
                            bind_type = new_type(TYPE_STR);  // Error type is str
                        } else if (match_type->inner) {
                            bind_type = match_type->inner;
                        }
                    }
                    e->match_expr.binding_types[i] = bind_type;
                    tc_define(tc, e->match_expr.bindings[i], bind_type, false);
                }
                
                Type* arm_type = tc_check_expr(tc, e->match_expr.arms[i]);
                if (result_type == NULL) {
                    result_type = arm_type;
                }
                tc_exit_scope(tc);
            }
            return result_type ? result_type : new_type(TYPE_ANY);
        }
        
        default: return new_type(TYPE_ANY);
    }
}

static void tc_check_stmt(TypeChecker* tc, Stmt* s);
static void tc_check_function_with_self(TypeChecker* tc, FnDef* fn, Type* self_type);

static void tc_check_stmt(TypeChecker* tc, Stmt* s) {
    if (!s) return;
    
    switch (s->kind) {
        case STMT_LET: {
            Type* init_type = s->let.value ? tc_check_expr(tc, s->let.value) : NULL;
            Type* decl_type = s->let.type;
            
            // Allow Result[T, E] where T is expected (auto-unwrap)
            bool types_compatible = types_equal(decl_type, init_type);
            if (!types_compatible && init_type && init_type->kind == TYPE_RESULT && decl_type) {
                // Check if Result's inner type matches declared type
                if (init_type->inner && types_equal(decl_type, init_type->inner)) {
                    types_compatible = true;
                }
            }
            
            if (decl_type && init_type && !types_compatible && 
                init_type->kind != TYPE_ANY && decl_type->kind != TYPE_ANY) {
                tc_error(tc, s->line, s->column, "type mismatch in let: expected %s, got %s",
                         type_name(decl_type), type_name(init_type));
                
                // Add helpful suggestions
                if (decl_type->kind == TYPE_STR && init_type->kind == TYPE_INT) {
                    fprintf(stderr, "   💡 Use int_to_str() to convert int to string\n");
                } else if (decl_type->kind == TYPE_INT && init_type->kind == TYPE_STR) {
                    fprintf(stderr, "   💡 Use str_to_int() to convert string to int\n");
                } else if (decl_type->kind == TYPE_BOOL && init_type->kind == TYPE_INT) {
                    fprintf(stderr, "   💡 Use comparison operators (==, !=, <, >) to get bool\n");
                }
            }
            
            tc_define(tc, s->let.name, decl_type ? decl_type : init_type, s->let.is_mut);
            break;
        }
        
        case STMT_ASSIGN: {
            Type* target = tc_check_expr(tc, s->assign.target);
            Type* value = tc_check_expr(tc, s->assign.value);
            
            // Check mutability
            if (s->assign.target->kind == EXPR_IDENT) {
                Symbol* sym = tc_lookup(tc, s->assign.target->ident);
                if (sym && !sym->is_mut) {
                    tc_error(tc, s->line, s->column, "cannot assign to immutable variable '%s'", 
                             s->assign.target->ident);
                }
            }
            
            // Special case: array += item (append operation)
            if (s->assign.op == TOK_PLUSEQ && target->kind == TYPE_ARRAY) {
                // For arrays, += means append, allow it
                break;
            }
            
            if (!types_equal(target, value) && target->kind != TYPE_ANY && value->kind != TYPE_ANY) {
                tc_error(tc, s->line, s->column, "type mismatch in assignment");
            }
            break;
        }
        
        case STMT_EXPR:
            tc_check_expr(tc, s->expr.expr);
            break;
        
        case STMT_RETURN: {
            Type* ret_type = s->ret.value ? tc_check_expr(tc, s->ret.value) : new_type(TYPE_VOID);
            if (tc->current_return_type && !types_equal(tc->current_return_type, ret_type) &&
                ret_type->kind != TYPE_ANY && tc->current_return_type->kind != TYPE_ANY) {
                tc_error(tc, s->line, s->column, "return type mismatch: expected %s, got %s",
                         type_name(tc->current_return_type), type_name(ret_type));
            }
            break;
        }
        
        case STMT_IF: {
            Type* cond = tc_check_expr(tc, s->if_stmt.cond);
            // Allow Result[bool, E] in conditions (auto-unwrap)
            bool is_bool = (cond->kind == TYPE_BOOL);
            if (!is_bool && cond->kind == TYPE_RESULT && cond->inner && cond->inner->kind == TYPE_BOOL) {
                is_bool = true;
            }
            if (!is_bool) tc_error(tc, s->line, s->column, "condition must be bool");
            tc_enter_scope(tc);
            for (int i = 0; i < s->if_stmt.then_count; i++) tc_check_stmt(tc, s->if_stmt.then_block[i]);
            tc_exit_scope(tc);
            if (s->if_stmt.else_block) {
                tc_enter_scope(tc);
                for (int i = 0; i < s->if_stmt.else_count; i++) tc_check_stmt(tc, s->if_stmt.else_block[i]);
                tc_exit_scope(tc);
            }
            break;
        }
        
        case STMT_WHILE: {
            Type* cond = tc_check_expr(tc, s->while_stmt.cond);
            // Allow Result[bool, E] in conditions (auto-unwrap)
            bool is_bool = (cond->kind == TYPE_BOOL);
            if (!is_bool && cond->kind == TYPE_RESULT && cond->inner && cond->inner->kind == TYPE_BOOL) {
                is_bool = true;
            }
            if (!is_bool) tc_error(tc, s->line, s->column, "condition must be bool");
            tc_enter_scope(tc);
            for (int i = 0; i < s->while_stmt.body_count; i++) tc_check_stmt(tc, s->while_stmt.body[i]);
            tc_exit_scope(tc);
            break;
        }
        
        case STMT_FOR: {
            Type* iter = tc_check_expr(tc, s->for_stmt.iter);
            tc_enter_scope(tc);
            // Infer loop variable type from iterator
            Type* var_type = new_type(TYPE_INT);
            if (iter && iter->kind == TYPE_ARRAY && iter->inner) {
                var_type = iter->inner;
            } else if (iter && iter->kind == TYPE_STR) {
                // String iteration produces single-character strings
                var_type = new_type(TYPE_STR);
            }
            tc_define(tc, s->for_stmt.var, var_type, false);
            for (int i = 0; i < s->for_stmt.body_count; i++) tc_check_stmt(tc, s->for_stmt.body[i]);
            tc_exit_scope(tc);
            break;
        }
        
        case STMT_BLOCK:
            tc_enter_scope(tc);
            for (int i = 0; i < s->block.count; i++) tc_check_stmt(tc, s->block.stmts[i]);
            tc_exit_scope(tc);
            break;
        
        case STMT_DEFER:
            tc_check_expr(tc, s->defer.expr);
            break;
        
        case STMT_SPAWN:
            tc_enter_scope(tc);
            for (int i = 0; i < s->spawn.body_count; i++) tc_check_stmt(tc, s->spawn.body[i]);
            tc_exit_scope(tc);
            break;
        
        case STMT_MATCH: {
            Type* match_type = tc_check_expr(tc, s->match_stmt.value);
            for (int i = 0; i < s->match_stmt.arm_count; i++) {
                Expr* pat = s->match_stmt.patterns[i];
                // Don't type-check binding patterns (some(x), ok(v), err(e)) - they introduce variables
                bool is_binding_pattern = (pat->kind == EXPR_SOME || pat->kind == EXPR_OK || pat->kind == EXPR_ERR) &&
                                          pat->some.value && pat->some.value->kind == EXPR_IDENT;
                if (!is_binding_pattern) {
                    tc_check_expr(tc, pat);
                }
                tc_enter_scope(tc);
                // Add binding variable to scope if present
                if (s->match_stmt.bindings[i][0] != '\0') {
                    // For some(x)/ok(x), binding gets the inner type
                    // For err(e), binding gets the error type (str for now)
                    Type* bind_type = new_type(TYPE_INT);  // Default
                    if (match_type && match_type->kind == TYPE_OPTIONAL && match_type->inner) {
                        bind_type = match_type->inner;
                    } else if (match_type && match_type->kind == TYPE_RESULT) {
                        if (pat->kind == EXPR_ERR) {
                            bind_type = new_type(TYPE_STR);  // Error type is str
                        } else if (match_type->inner) {
                            bind_type = match_type->inner;
                        }
                    }
                    s->match_stmt.binding_types[i] = bind_type;
                    tc_define(tc, s->match_stmt.bindings[i], bind_type, false);
                }
                for (int j = 0; j < s->match_stmt.arm_counts[i]; j++) {
                    tc_check_stmt(tc, s->match_stmt.arms[i][j]);
                }
                tc_exit_scope(tc);
            }
            break;
        }
        
        case STMT_BREAK:
        case STMT_CONTINUE:
            break;
        
        default: break;
    }
}

static void tc_check_function(TypeChecker* tc, FnDef* fn) {
    tc_check_function_with_self(tc, fn, NULL);
}

static void tc_check_function_with_self(TypeChecker* tc, FnDef* fn, Type* self_type) {
    tc_enter_scope(tc);
    
    // Set current function for extension method detection
    tc->current_function = fn;
    
    // Add parameters to scope
    for (int i = 0; i < fn->param_count; i++) {
        Type* param_type = fn->params[i].type;
        // If parameter is 'self' and we have a self_type, use it
        if (strcmp(fn->params[i].name, "self") == 0 && self_type) {
            param_type = self_type;
        }
        tc_define(tc, fn->params[i].name, param_type, false);
    }
    
    tc->current_return_type = fn->return_type;
    
    // Check body
    for (int i = 0; i < fn->body_count; i++) {
        tc_check_stmt(tc, fn->body[i]);
    }
    
    tc->current_return_type = NULL;
    tc->current_function = NULL;
    tc_exit_scope(tc);
}

static bool typecheck_module(TypeChecker* tc) {
    // Check all functions
    for (int i = 0; i < tc->module->function_count; i++) {
        FnDef* fn = &tc->module->functions[i];
        
        // For extension methods, create self_type
        if (fn->is_extension) {
            Type* self_type = NULL;
            if (strcmp(fn->extension_type, "str") == 0) {
                self_type = new_type(TYPE_STR);
            } else if (strcmp(fn->extension_type, "int") == 0) {
                self_type = new_type(TYPE_INT);
            } else if (strcmp(fn->extension_type, "bool") == 0) {
                self_type = new_type(TYPE_BOOL);
            } else if (strcmp(fn->extension_type, "array") == 0) {
                self_type = new_type(TYPE_ARRAY);
            } else {
                self_type = new_type(TYPE_NAMED);
                strcpy(self_type->name, fn->extension_type);
            }
            tc_check_function_with_self(tc, fn, self_type);
        } else {
            tc_check_function(tc, fn);
        }
    }
    
    // Check struct methods
    for (int i = 0; i < tc->module->struct_count; i++) {
        StructDef* s = &tc->module->structs[i];
        Type* self_type = new_type(TYPE_NAMED);
        strcpy(self_type->name, s->name);
        for (int j = 0; j < s->method_count; j++) {
            tc_check_function_with_self(tc, &s->methods[j], self_type);
        }
    }
    
    // Check test bodies
    for (int i = 0; i < tc->module->test_count; i++) {
        tc_enter_scope(tc);
        for (int j = 0; j < tc->module->tests[i].body_count; j++) {
            tc_check_stmt(tc, tc->module->tests[i].body[j]);
        }
        tc_exit_scope(tc);
    }
    
    return !tc->had_error;
}

// ============================================================================
// Stage 1 Type Checker (Enhanced)
// ============================================================================

typedef struct {
    Module* module;
    const char* filename;
    Symbol symbols[2048];
    int symbol_count;
    int scope_depth;
    Type* current_return_type;
    bool had_error;
    bool enable_inference;
} TypeChecker1;

static TypeChecker1* tc1_new(Module* m, const char* filename) {
    TypeChecker1* tc = calloc(1, sizeof(TypeChecker1));
    tc->module = m;
    tc->filename = filename;
    tc->enable_inference = true;
    return tc;
}

static void tc1_error(TypeChecker1* tc, int line, int col, const char* fmt, ...) {
    fprintf(stderr, "%s:%d:%d: error: ", tc->filename, line, col);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    tc->had_error = true;
}

static void tc1_suggest(TypeChecker1* tc, const char* suggestion) {
    fprintf(stderr, "  suggestion: %s\n", suggestion);
}

static void tc1_enter_scope(TypeChecker1* tc) { tc->scope_depth++; }

static void tc1_exit_scope(TypeChecker1* tc) {
    while (tc->symbol_count > 0 && tc->symbols[tc->symbol_count - 1].scope_depth == tc->scope_depth) {
        tc->symbol_count--;
    }
    tc->scope_depth--;
}

static void tc1_define(TypeChecker1* tc, const char* name, Type* type, bool is_mut) {
    Symbol* s = &tc->symbols[tc->symbol_count++];
    strcpy(s->name, name);
    s->type = type;
    s->is_mut = is_mut;
    s->scope_depth = tc->scope_depth;
}

static Symbol* tc1_lookup(TypeChecker1* tc, const char* name) {
    for (int i = tc->symbol_count - 1; i >= 0; i--) {
        if (strcmp(tc->symbols[i].name, name) == 0) return &tc->symbols[i];
    }
    return NULL;
}

static bool tc1_is_builtin(const char* name) {
    // Core builtins
    if (strcmp(name, "print") == 0 || strcmp(name, "println") == 0 || strcmp(name, "print_raw") == 0 || strcmp(name, "print_str") == 0 || strcmp(name, "assert") == 0 || strcmp(name, "args") == 0) return true;
    if (strcmp(name, "min") == 0 || strcmp(name, "max") == 0) return true;
    
    // String builtins
    if (strcmp(name, "ord") == 0 || strcmp(name, "chr") == 0 || strcmp(name, "substring") == 0) return true;
    if (strcmp(name, "str_split") == 0 || strcmp(name, "str_find") == 0) return true;
    if (strcmp(name, "str_concat") == 0 || strcmp(name, "str_cmp") == 0) return true;
    if (strcmp(name, "char_at") == 0 || strcmp(name, "int_to_str") == 0) return true;
    if (strcmp(name, "str_substr") == 0 || strcmp(name, "str_to_int") == 0) return true;
    if (strcmp(name, "to_string") == 0) return true;
    if (strcmp(name, "format") == 0 || strcmp(name, "str_format") == 0) return true;
    
    // Math builtins
    if (strcmp(name, "abs") == 0 || strcmp(name, "sqrtf") == 0) return true;
    if (strcmp(name, "int_to_float") == 0 || strcmp(name, "float_to_int") == 0) return true;
    
    // File/IO builtins
    if (strcmp(name, "len") == 0 || strcmp(name, "syscall") == 0) return true;
    if (strcmp(name, "read_file") == 0 || strcmp(name, "write_file") == 0) return true;
    if (strcmp(name, "append_file") == 0 || strcmp(name, "file_exists") == 0) return true;
    if (strcmp(name, "file_size") == 0 || strcmp(name, "read_stdin_line") == 0) return true;
    if (strcmp(name, "mkdir") == 0 || strcmp(name, "rmdir") == 0) return true;
    if (strcmp(name, "rename_file") == 0) return true;
    
    // OS builtins
    if (strcmp(name, "getcwd") == 0 || strcmp(name, "getpid") == 0 || strcmp(name, "exit") == 0) return true;
    if (strcmp(name, "getenv_wyn") == 0 || strcmp(name, "setenv_wyn") == 0) return true;
    if (strcmp(name, "exec_wyn") == 0 || strcmp(name, "exit_wyn") == 0) return true;
    if (strcmp(name, "args_wyn") == 0 || strcmp(name, "cwd_wyn") == 0 || strcmp(name, "chdir_wyn") == 0) return true;
    
    // JSON builtins
    if (strcmp(name, "parse_json") == 0 || strcmp(name, "stringify_json") == 0) return true;
    if (strcmp(name, "get_string_json") == 0 || strcmp(name, "get_int_json") == 0 || strcmp(name, "get_bool_json") == 0) return true;
    
    // Config builtins
    if (strcmp(name, "yaml_parse") == 0 || strcmp(name, "toml_parse") == 0) return true;
    
    // Net builtins
    if (strcmp(name, "http_get_wyn") == 0 || strcmp(name, "http_post_wyn") == 0) return true;
    if (strcmp(name, "tcp_connect_wyn") == 0 || strcmp(name, "tcp_send_wyn") == 0) return true;
    if (strcmp(name, "tcp_recv_wyn") == 0 || strcmp(name, "tcp_close_wyn") == 0) return true;
    if (strcmp(name, "delete_file") == 0 || strcmp(name, "file_size") == 0) return true;
    if (strcmp(name, "mkdir_wyn") == 0 || strcmp(name, "rmdir_wyn") == 0) return true;
    if (strcmp(name, "is_dir_wyn") == 0 || strcmp(name, "is_file_wyn") == 0) return true;
    if (strcmp(name, "hostname_wyn") == 0 || strcmp(name, "getpid_wyn") == 0) return true;
    if (strcmp(name, "http_put_wyn") == 0 || strcmp(name, "http_delete_wyn") == 0) return true;
    if (strcmp(name, "http_head_wyn") == 0 || strcmp(name, "http_get_headers_wyn") == 0) return true;
    if (strcmp(name, "udp_send_wyn") == 0 || strcmp(name, "udp_recv_wyn") == 0) return true;
    if (strcmp(name, "resolve_host_wyn") == 0 || strcmp(name, "get_local_ip_wyn") == 0) return true;
    if (strcmp(name, "now_unix") == 0 || strcmp(name, "now_millis") == 0 || strcmp(name, "now_micros") == 0) return true;
    if (strcmp(name, "sleep_seconds") == 0) return true;
    if (strcmp(name, "format_timestamp") == 0 || strcmp(name, "format_iso8601") == 0 || strcmp(name, "parse_timestamp") == 0) return true;
    
    // Log builtins
    if (strcmp(name, "log_info") == 0 || strcmp(name, "log_warn") == 0) return true;
    if (strcmp(name, "log_error") == 0 || strcmp(name, "log_debug") == 0) return true;
    if (strcmp(name, "log_with_level") == 0) return true;
    
    // Encoding builtins
    if (strcmp(name, "base64_encode") == 0 || strcmp(name, "base64_decode") == 0) return true;
    if (strcmp(name, "hex_encode") == 0 || strcmp(name, "hex_decode") == 0) return true;
    if (strcmp(name, "url_encode") == 0 || strcmp(name, "url_decode") == 0) return true;
    
    // Hash builtins
    if (strcmp(name, "sha256_hash") == 0 || strcmp(name, "md5_hash") == 0) return true;
    if (strcmp(name, "sha1_hash") == 0) return true;
    if (strcmp(name, "hashmap_new") == 0 || strcmp(name, "hashmap_put") == 0) return true;
    if (strcmp(name, "hashmap_get") == 0 || strcmp(name, "hashmap_contains") == 0) return true;
    if (strcmp(name, "hashmap_remove") == 0 || strcmp(name, "hashmap_size") == 0 || strcmp(name, "hashmap_clear") == 0) return true;
    if (strcmp(name, "hashmap_keys") == 0 || strcmp(name, "hashmap_values") == 0) return true;
    if (strcmp(name, "set_new") == 0 || strcmp(name, "set_add") == 0) return true;
    if (strcmp(name, "set_contains") == 0 || strcmp(name, "set_remove") == 0) return true;
    if (strcmp(name, "set_size") == 0 || strcmp(name, "set_clear") == 0) return true;
    if (strcmp(name, "encrypt_aes256") == 0 || strcmp(name, "decrypt_aes256") == 0) return true;
    if (strcmp(name, "generate_random_bytes") == 0 || strcmp(name, "hmac_sha256") == 0) return true;
    if (strcmp(name, "verify_signature") == 0) return true;
    if (strcmp(name, "http_server_create") == 0 || strcmp(name, "http_server_accept") == 0) return true;
    if (strcmp(name, "http_server_read_request") == 0 || strcmp(name, "http_parse_method") == 0) return true;
    if (strcmp(name, "http_parse_path") == 0 || strcmp(name, "http_parse_body") == 0) return true;
    if (strcmp(name, "http_server_send_response") == 0 || strcmp(name, "http_server_send_json") == 0) return true;
    if (strcmp(name, "http_server_close_client") == 0 || strcmp(name, "http_server_close") == 0) return true;
    
    // Time builtins
    if (strcmp(name, "time_now") == 0 || strcmp(name, "sleep_ms") == 0 || strcmp(name, "sleep") == 0) return true;
    if (strcmp(name, "clock") == 0 || strcmp(name, "random") == 0) return true;
    
    // Task management builtins
    if (strcmp(name, "task_await") == 0 || strcmp(name, "task_join_all") == 0) return true;
    
    // GPU builtins
    if (strcmp(name, "gpu_device_count") == 0) return true;
    
    return false;
}

static Type* tc1_infer_binary(TypeChecker1* tc, Type* left, Type* right, TokenKind op) {
    if (op == TOK_DOTDOT || op == TOK_DOTDOTEQ) {
        Type* t = new_type(TYPE_ARRAY);
        t->inner = left;
        return t;
    }
    if (op == TOK_IN) return new_type(TYPE_BOOL);
    if (op == TOK_PLUS && left->kind == TYPE_STR && right->kind == TYPE_STR) return new_type(TYPE_STR);
    if (op == TOK_EQEQ || op == TOK_NOTEQ || op == TOK_LT || op == TOK_GT || op == TOK_LTEQ || op == TOK_GTEQ) 
        return new_type(TYPE_BOOL);
    if (op == TOK_AND || op == TOK_OR) return new_type(TYPE_BOOL);
    return left->kind != TYPE_ANY ? left : right;
}

static Type* tc1_check_expr(TypeChecker1* tc, Expr* e);

static Type* tc1_check_expr(TypeChecker1* tc, Expr* e) {
    if (!e) return NULL;
    
    switch (e->kind) {
        case EXPR_INT: return new_type(TYPE_INT);
        case EXPR_FLOAT: return new_type(TYPE_FLOAT);
        case EXPR_STRING: return new_type(TYPE_STR);
        case EXPR_BOOL: return new_type(TYPE_BOOL);
        
        case EXPR_IDENT: {
            if (strcmp(e->ident, "_") == 0) return new_type(TYPE_ANY);
            Symbol* s = tc1_lookup(tc, e->ident);
            if (!s) {
                if (tc_is_enum_variant((TypeChecker*)tc, e->ident)) return new_type(TYPE_INT);
                if (tc_lookup_fn((TypeChecker*)tc, e->ident)) return new_type(TYPE_FUNCTION);
                if (tc1_is_builtin(e->ident)) return new_type(TYPE_FUNCTION);
                
                // Check if this identifier might be part of a module path
                // Look for any imported modules that start with this identifier
                for (int i = 0; i < ((TypeChecker*)tc)->module->import_count; i++) {
                    const char* import_path = ((TypeChecker*)tc)->module->imports[i].path;
                    if (strncmp(import_path, e->ident, strlen(e->ident)) == 0 &&
                        (import_path[strlen(e->ident)] == '.' || import_path[strlen(e->ident)] == '\0')) {
                        // This might be part of a module path, return a special type
                        return new_type(TYPE_ANY);
                    }
                }
                
                tc1_error(tc, e->line, e->column, "undefined variable '%s'", e->ident);
                tc1_suggest(tc, "check spelling or declare the variable");
                return new_type(TYPE_ANY);
            }
            return s->type;
        }
        
        case EXPR_BINARY: {
            Type* left = tc1_check_expr(tc, e->binary.left);
            Type* right = tc1_check_expr(tc, e->binary.right);
            TokenKind op = e->binary.op;
            
            Type* result = tc1_infer_binary(tc, left, right, op);
            
            if (op == TOK_AND || op == TOK_OR) {
                if (left->kind != TYPE_BOOL) {
                    tc1_error(tc, e->line, e->column, "left operand must be bool, got %s", type_name(left));
                    tc1_suggest(tc, "use comparison operators like ==, !=, <, >");
                }
                if (right->kind != TYPE_BOOL) {
                    tc1_error(tc, e->line, e->column, "right operand must be bool, got %s", type_name(right));
                }
            } else if (!types_equal(left, right) && left->kind != TYPE_ANY && right->kind != TYPE_ANY) {
                tc1_error(tc, e->line, e->column, "type mismatch: %s vs %s", type_name(left), type_name(right));
                if (left->kind == TYPE_INT && right->kind == TYPE_FLOAT)
                    tc1_suggest(tc, "convert int to float with .to_float()");
                else if (left->kind == TYPE_FLOAT && right->kind == TYPE_INT)
                    tc1_suggest(tc, "convert int to float with .to_float()");
            }
            return result;
        }
        
        case EXPR_UNARY: {
            Type* operand = tc1_check_expr(tc, e->unary.operand);
            if (e->unary.op == TOK_NOT || e->unary.op == TOK_BANG) {
                if (operand->kind != TYPE_BOOL && operand->kind != TYPE_ANY) {
                    tc1_error(tc, e->line, e->column, "not operator requires bool, got %s", type_name(operand));
                }
                return new_type(TYPE_BOOL);
            }
            return operand;
        }
        
        case EXPR_ADDR: {
            Type* operand = tc1_check_expr(tc, e->unary.operand);
            Type* ptr = new_type(TYPE_POINTER);
            ptr->inner = operand;
            return ptr;
        }
        
        case EXPR_DEREF: {
            Type* operand = tc1_check_expr(tc, e->unary.operand);
            if (operand && operand->kind == TYPE_POINTER && operand->inner) return operand->inner;
            if (operand->kind != TYPE_POINTER && operand->kind != TYPE_ANY) {
                tc1_error(tc, e->line, e->column, "cannot dereference non-pointer type %s", type_name(operand));
            }
            return new_type(TYPE_ANY);
        }
        
        case EXPR_CALL: {
            if (e->call.func->kind == EXPR_FIELD && e->call.func->field.object->kind == EXPR_IDENT) {
                const char* module = e->call.func->field.object->ident;
                const char* function = e->call.func->field.field;
                const char* builtin_name = map_module_function(module, function);
                if (builtin_name) {
                    for (int i = 0; i < e->call.arg_count; i++) tc1_check_expr(tc, e->call.args[i]);
                    return get_builtin_return_type(builtin_name);
                }
            }
            
            Type* func_type = tc1_check_expr(tc, e->call.func);
            (void)func_type;
            for (int i = 0; i < e->call.arg_count; i++) tc1_check_expr(tc, e->call.args[i]);
            
            if (e->call.func->kind == EXPR_IDENT) {
                FnDef* fn = tc_lookup_fn((TypeChecker*)tc, e->call.func->ident);
                if (fn) {
                    // Count required parameters (those without defaults)
                    int required_params = 0;
                    for (int i = 0; i < fn->param_count; i++) {
                        if (!fn->params[i].default_val) required_params++;
                    }
                    
                    if (e->call.arg_count < required_params || e->call.arg_count > fn->param_count) {
                        if (required_params == fn->param_count) {
                            tc1_error(tc, e->line, e->column, "function '%s' expects %d arguments, got %d",
                                      e->call.func->ident, fn->param_count, e->call.arg_count);
                        } else {
                            tc1_error(tc, e->line, e->column, "function '%s' expects %d-%d arguments, got %d",
                                      e->call.func->ident, required_params, fn->param_count, e->call.arg_count);
                        }
                    }
                    if (fn->return_type) {
                        if (e->call.type_arg_count > 0 && fn->generic_count > 0) {
                            Type* ret = fn->return_type;
                            if (ret->kind == TYPE_NAMED) {
                                for (int i = 0; i < fn->generic_count && i < e->call.type_arg_count; i++) {
                                    if (strcmp(ret->name, fn->generics[i]) == 0) return e->call.type_args[i];
                                }
                            }
                            return ret;
                        }
                        return fn->return_type;
                    }
                } else if (tc1_is_builtin(e->call.func->ident)) {
                    // Handle builtin functions
                    return get_builtin_return_type(e->call.func->ident);
                }
            }
            
            if (e->call.func->kind == EXPR_FIELD) {
                const char* method_name = e->call.func->field.field;
                // String methods
                if (strcmp(method_name, "len") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "upper") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "lower") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "trim") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "contains") == 0) return new_type(TYPE_BOOL);
                if (strcmp(method_name, "starts_with") == 0) return new_type(TYPE_BOOL);
                if (strcmp(method_name, "ends_with") == 0) return new_type(TYPE_BOOL);
                if (strcmp(method_name, "split") == 0) return new_type(TYPE_ARRAY);
                if (strcmp(method_name, "replace") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "index_of") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "find") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "substring") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "char_at") == 0) return new_type(TYPE_STR);
                // Int methods
                if (strcmp(method_name, "abs") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "to_str") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "to_int") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "to_float") == 0) return new_type(TYPE_FLOAT);
                // Float methods
                if (strcmp(method_name, "floor") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "ceil") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "round") == 0) return new_type(TYPE_INT);
                // Array methods
                if (strcmp(method_name, "get") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "get_str") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "reverse") == 0) return new_type(TYPE_ARRAY);
                if (strcmp(method_name, "append") == 0) return new_type(TYPE_ARRAY);
                if (strcmp(method_name, "contains") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "prepend") == 0) return new_type(TYPE_ARRAY);
                if (strcmp(method_name, "filter") == 0) return new_type(TYPE_ARRAY);
                if (strcmp(method_name, "map") == 0) return new_type(TYPE_ARRAY);
                if (strcmp(method_name, "join") == 0) return new_type(TYPE_STR);
                
                // Look up extension methods
                Type* obj_type = tc1_check_expr(tc, e->call.func->field.object);
                const char* obj_type_name = type_name(obj_type);
                if (obj_type_name) {
                    for (int i = 0; i < tc->module->function_count; i++) {
                        FnDef* fn = &tc->module->functions[i];
                        if (fn && fn->is_extension && fn->extension_type && fn->extension_method &&
                            strcmp(fn->extension_type, obj_type_name) == 0 &&
                            strcmp(fn->extension_method, method_name) == 0) {
                            return fn->return_type ? fn->return_type : new_type(TYPE_ANY);
                        }
                    }
                }
                
                for (int i = 0; i < tc->module->struct_count; i++) {
                    StructDef* s = &tc->module->structs[i];
                    for (int j = 0; j < s->method_count; j++) {
                        if (strcmp(s->methods[j].name, method_name) == 0) {
                            return s->methods[j].return_type;
                        }
                    }
                }
            }
            return new_type(TYPE_ANY);
        }
        
        case EXPR_INDEX: {
            Type* obj = tc1_check_expr(tc, e->index.object);
            Type* idx = tc1_check_expr(tc, e->index.index);
            if (idx->kind != TYPE_INT && idx->kind != TYPE_ANY) {
                tc1_error(tc, e->line, e->column, "index must be int, got %s", type_name(idx));
            }
            if (obj->kind == TYPE_ARRAY) return obj->inner ? obj->inner : new_type(TYPE_ANY);
            if (obj->kind == TYPE_STR) return new_type(TYPE_STR);
            if (obj->kind != TYPE_ANY) {
                tc1_error(tc, e->line, e->column, "cannot index type %s", type_name(obj));
            }
            return new_type(TYPE_ANY);
        }
        
        case EXPR_SLICE: {
            Type* obj = tc1_check_expr(tc, e->slice.object);
            if (e->slice.start) tc1_check_expr(tc, e->slice.start);
            if (e->slice.end) tc1_check_expr(tc, e->slice.end);
            if (e->slice.step) tc1_check_expr(tc, e->slice.step);
            return obj;
        }
        
        case EXPR_FIELD: {
            // FIXED: Check if object is a VARIABLE first (method call)
            if (e->field.object->kind == EXPR_IDENT) {
                Symbol* sym = tc_lookup((TypeChecker*)tc, e->field.object->ident);
                if (sym) {
                    // It's a variable - this is a method call
                    return new_type(TYPE_FUNCTION);
                }
                
                // Not a variable - check for module
                if (tc_is_module_imported((TypeChecker*)tc, e->field.object->ident)) {
                    // This is a module namespace, return function type
                    return new_type(TYPE_FUNCTION);
                }
            }
            Type* obj = tc1_check_expr(tc, e->field.object);
            if (obj->kind == TYPE_NAMED) {
                StructDef* s = tc_lookup_struct((TypeChecker*)tc, obj->name);
                if (s) {
                    for (int i = 0; i < s->field_count; i++) {
                        if (strcmp(s->fields[i].name, e->field.field) == 0) return s->fields[i].type;
                    }
                    // Could be a method - don't error, let EXPR_CALL handle it
                }
            }
            return new_type(TYPE_ANY);
        }
        
        case EXPR_ARRAY: {
            Type* elem_type = NULL;
            for (int i = 0; i < e->array.count; i++) {
                Type* t = tc1_check_expr(tc, e->array.elements[i]);
                if (!elem_type) elem_type = t;
                else if (!types_equal(elem_type, t) && t->kind != TYPE_ANY) {
                    tc1_error(tc, e->line, e->column, "array elements must have same type");
                }
            }
            Type* arr = new_type(TYPE_ARRAY);
            arr->inner = elem_type ? elem_type : new_type(TYPE_ANY);
            return arr;
        }
        
        case EXPR_STRUCT: {
            StructDef* s = tc_lookup_struct((TypeChecker*)tc, e->struct_lit.name);
            if (!s) {
                tc1_error(tc, e->line, e->column, "unknown struct '%s'", e->struct_lit.name);
                return new_type(TYPE_ANY);
            }
            for (int i = 0; i < e->struct_lit.count; i++) tc1_check_expr(tc, e->struct_lit.values[i]);
            
            if (e->struct_lit.type_arg_count > 0) {
                Type* t = new_type(TYPE_GENERIC);
                strcpy(t->name, e->struct_lit.name);
                t->params = malloc(sizeof(Type*) * e->struct_lit.type_arg_count);
                t->param_count = e->struct_lit.type_arg_count;
                for (int i = 0; i < e->struct_lit.type_arg_count; i++) t->params[i] = e->struct_lit.type_args[i];
                return t;
            }
            Type* t = new_type(TYPE_NAMED);
            strcpy(t->name, e->struct_lit.name);
            return t;
        }
        
        case EXPR_MAP: {
            for (int i = 0; i < e->map.count; i++) tc1_check_expr(tc, e->map.values[i]);
            return new_type(TYPE_MAP);
        }
        
        case EXPR_IF: {
            Type* cond = tc1_check_expr(tc, e->if_expr.cond);
            if (cond->kind != TYPE_BOOL && cond->kind != TYPE_ANY) {
                tc1_error(tc, e->line, e->column, "condition must be bool, got %s", type_name(cond));
            }
            Type* then_t = tc1_check_expr(tc, e->if_expr.then_expr);
            Type* else_t = tc1_check_expr(tc, e->if_expr.else_expr);
            if (!types_equal(then_t, else_t) && then_t->kind != TYPE_ANY && else_t->kind != TYPE_ANY) {
                tc1_error(tc, e->line, e->column, "if branches have different types: %s vs %s",
                          type_name(then_t), type_name(else_t));
            }
            return then_t;
        }
        
        case EXPR_SOME: {
            Type* inner = tc1_check_expr(tc, e->some.value);
            Type* opt = new_type(TYPE_OPTIONAL);
            opt->inner = inner;
            return opt;
        }
        
        case EXPR_NONE: return new_type(TYPE_OPTIONAL);
        
        case EXPR_OK: {
            Type* inner = tc1_check_expr(tc, e->some.value);
            Type* res = new_type(TYPE_RESULT);
            res->inner = inner;
            return res;
        }
        
        case EXPR_ERR: {
            Type* inner = tc1_check_expr(tc, e->some.value);
            Type* res = new_type(TYPE_RESULT);
            res->value_type = inner;
            return res;
        }
        
        case EXPR_DEFAULT: {
            tc1_check_expr(tc, e->default_expr.value);
            return tc1_check_expr(tc, e->default_expr.default_val);
        }
        
        case EXPR_MATCH: {
            Type* match_type = tc1_check_expr(tc, e->match_expr.value);
            Type* result_type = NULL;
            for (int i = 0; i < e->match_expr.arm_count; i++) {
                Expr* pat = e->match_expr.patterns[i];
                bool is_binding_pattern = (pat->kind == EXPR_SOME || pat->kind == EXPR_OK || pat->kind == EXPR_ERR) &&
                                         pat->some.value && pat->some.value->kind == EXPR_IDENT;
                if (!is_binding_pattern) tc1_check_expr(tc, pat);
                
                tc1_enter_scope(tc);
                if (e->match_expr.bindings[i][0] != '\0') {
                    Type* bind_type = new_type(TYPE_INT);
                    if (match_type && match_type->kind == TYPE_OPTIONAL && match_type->inner) {
                        bind_type = match_type->inner;
                    } else if (match_type && match_type->kind == TYPE_RESULT) {
                        if (pat->kind == EXPR_ERR) bind_type = new_type(TYPE_STR);
                        else if (match_type->inner) bind_type = match_type->inner;
                    }
                    e->match_expr.binding_types[i] = bind_type;
                    tc1_define(tc, e->match_expr.bindings[i], bind_type, false);
                }
                Type* arm_type = tc1_check_expr(tc, e->match_expr.arms[i]);
                if (result_type == NULL) result_type = arm_type;
                tc1_exit_scope(tc);
            }
            return result_type ? result_type : new_type(TYPE_ANY);
        }
        
        default: return new_type(TYPE_ANY);
    }
}

static void tc1_check_stmt(TypeChecker1* tc, Stmt* s);

static void tc1_check_stmt(TypeChecker1* tc, Stmt* s) {
    if (!s) return;
    
    switch (s->kind) {
        case STMT_LET: {
            Type* init_type = s->let.value ? tc1_check_expr(tc, s->let.value) : NULL;
            Type* decl_type = s->let.type;
            
            if (tc->enable_inference && !decl_type && init_type) decl_type = init_type;
            
            if (decl_type && init_type && !types_equal(decl_type, init_type) && 
                init_type->kind != TYPE_ANY && decl_type->kind != TYPE_ANY) {
                tc1_error(tc, s->line, s->column, "type mismatch: expected %s, got %s",
                          type_name(decl_type), type_name(init_type));
                if (decl_type->kind == TYPE_INT && init_type->kind == TYPE_FLOAT)
                    tc1_suggest(tc, "use .to_int() to convert float to int");
            }
            
            tc1_define(tc, s->let.name, decl_type ? decl_type : init_type, s->let.is_mut);
            break;
        }
        
        case STMT_ASSIGN: {
            Type* target = tc1_check_expr(tc, s->assign.target);
            Type* value = tc1_check_expr(tc, s->assign.value);
            
            if (s->assign.target->kind == EXPR_IDENT) {
                Symbol* sym = tc1_lookup(tc, s->assign.target->ident);
                if (sym && !sym->is_mut) {
                    tc1_error(tc, s->line, s->column, "cannot assign to immutable variable '%s'", 
                              s->assign.target->ident);
                    tc1_suggest(tc, "declare with 'let mut' to make it mutable");
                }
            }
            
            // Special case: array += item (append operation)
            if (s->assign.op == TOK_PLUSEQ && target->kind == TYPE_ARRAY) {
                // For arrays, += means append, so value type should match element type
                // We allow this without strict type checking for now
                break;
            }
            
            if (!types_equal(target, value) && target->kind != TYPE_ANY && value->kind != TYPE_ANY) {
                tc1_error(tc, s->line, s->column, "type mismatch in assignment: %s vs %s",
                          type_name(target), type_name(value));
            }
            break;
        }
        
        case STMT_EXPR:
            tc1_check_expr(tc, s->expr.expr);
            break;
        
        case STMT_RETURN: {
            Type* ret_type = s->ret.value ? tc1_check_expr(tc, s->ret.value) : new_type(TYPE_VOID);
            if (tc->current_return_type && !types_equal(tc->current_return_type, ret_type) &&
                ret_type->kind != TYPE_ANY && tc->current_return_type->kind != TYPE_ANY) {
                tc1_error(tc, s->line, s->column, "return type mismatch: expected %s, got %s",
                          type_name(tc->current_return_type), type_name(ret_type));
            }
            break;
        }
        
        case STMT_IF: {
            Type* cond = tc1_check_expr(tc, s->if_stmt.cond);
            if (cond->kind != TYPE_BOOL && cond->kind != TYPE_ANY) {
                tc1_error(tc, s->line, s->column, "condition must be bool, got %s", type_name(cond));
            }
            tc1_enter_scope(tc);
            for (int i = 0; i < s->if_stmt.then_count; i++) tc1_check_stmt(tc, s->if_stmt.then_block[i]);
            tc1_exit_scope(tc);
            if (s->if_stmt.else_block) {
                tc1_enter_scope(tc);
                for (int i = 0; i < s->if_stmt.else_count; i++) tc1_check_stmt(tc, s->if_stmt.else_block[i]);
                tc1_exit_scope(tc);
            }
            break;
        }
        
        case STMT_WHILE: {
            Type* cond = tc1_check_expr(tc, s->while_stmt.cond);
            if (cond->kind != TYPE_BOOL && cond->kind != TYPE_ANY) {
                tc1_error(tc, s->line, s->column, "condition must be bool, got %s", type_name(cond));
            }
            tc1_enter_scope(tc);
            for (int i = 0; i < s->while_stmt.body_count; i++) tc1_check_stmt(tc, s->while_stmt.body[i]);
            tc1_exit_scope(tc);
            break;
        }
        
        case STMT_FOR: {
            Type* iter = tc1_check_expr(tc, s->for_stmt.iter);
            tc1_enter_scope(tc);
            Type* var_type = new_type(TYPE_INT);
            if (iter && iter->kind == TYPE_ARRAY && iter->inner) {
                var_type = iter->inner;
            } else if (iter && iter->kind == TYPE_STR) {
                // String iteration produces single-character strings
                var_type = new_type(TYPE_STR);
            }
            tc1_define(tc, s->for_stmt.var, var_type, false);
            for (int i = 0; i < s->for_stmt.body_count; i++) tc1_check_stmt(tc, s->for_stmt.body[i]);
            tc1_exit_scope(tc);
            break;
        }
        
        case STMT_BLOCK:
            tc1_enter_scope(tc);
            for (int i = 0; i < s->block.count; i++) tc1_check_stmt(tc, s->block.stmts[i]);
            tc1_exit_scope(tc);
            break;
        
        case STMT_DEFER:
            tc1_check_expr(tc, s->defer.expr);
            break;
        
        case STMT_SPAWN:
            tc1_enter_scope(tc);
            for (int i = 0; i < s->spawn.body_count; i++) tc1_check_stmt(tc, s->spawn.body[i]);
            tc1_exit_scope(tc);
            break;
        
        case STMT_MATCH: {
            Type* match_type = tc1_check_expr(tc, s->match_stmt.value);
            for (int i = 0; i < s->match_stmt.arm_count; i++) {
                Expr* pat = s->match_stmt.patterns[i];
                bool is_binding_pattern = (pat->kind == EXPR_SOME || pat->kind == EXPR_OK || pat->kind == EXPR_ERR) &&
                                          pat->some.value && pat->some.value->kind == EXPR_IDENT;
                if (!is_binding_pattern) tc1_check_expr(tc, pat);
                
                tc1_enter_scope(tc);
                if (s->match_stmt.bindings[i][0] != '\0') {
                    Type* bind_type = new_type(TYPE_INT);
                    if (match_type && match_type->kind == TYPE_OPTIONAL && match_type->inner) {
                        bind_type = match_type->inner;
                    } else if (match_type && match_type->kind == TYPE_RESULT) {
                        if (pat->kind == EXPR_ERR) bind_type = new_type(TYPE_STR);
                        else if (match_type->inner) bind_type = match_type->inner;
                    }
                    s->match_stmt.binding_types[i] = bind_type;
                    tc1_define(tc, s->match_stmt.bindings[i], bind_type, false);
                }
                for (int j = 0; j < s->match_stmt.arm_counts[i]; j++) {
                    tc1_check_stmt(tc, s->match_stmt.arms[i][j]);
                }
                tc1_exit_scope(tc);
            }
            break;
        }
        
        case STMT_BREAK:
        case STMT_CONTINUE:
            break;
        
        default: break;
    }
}

static void tc1_check_function(TypeChecker1* tc, FnDef* fn, Type* self_type) {
    tc1_enter_scope(tc);
    
    for (int i = 0; i < fn->param_count; i++) {
        Type* param_type = fn->params[i].type;
        if (strcmp(fn->params[i].name, "self") == 0 && self_type) param_type = self_type;
        tc1_define(tc, fn->params[i].name, param_type, false);
    }
    
    tc->current_return_type = fn->return_type;
    for (int i = 0; i < fn->body_count; i++) tc1_check_stmt(tc, fn->body[i]);
    tc->current_return_type = NULL;
    tc1_exit_scope(tc);
}

static bool typecheck_module_stage1(TypeChecker1* tc) {
    for (int i = 0; i < tc->module->function_count; i++) {
        FnDef* fn = &tc->module->functions[i];
        Type* self_type = NULL;
        
        // For extension methods, create self_type
        if (fn->is_extension) {
            if (strcmp(fn->extension_type, "str") == 0) {
                self_type = new_type(TYPE_STR);
            } else if (strcmp(fn->extension_type, "int") == 0) {
                self_type = new_type(TYPE_INT);
            } else if (strcmp(fn->extension_type, "bool") == 0) {
                self_type = new_type(TYPE_BOOL);
            } else if (strcmp(fn->extension_type, "array") == 0) {
                self_type = new_type(TYPE_ARRAY);
            } else {
                // Named type (struct)
                self_type = new_type(TYPE_NAMED);
                strcpy(self_type->name, fn->extension_type);
            }
        }
        
        tc1_check_function(tc, fn, self_type);
    }
    
    for (int i = 0; i < tc->module->struct_count; i++) {
        StructDef* s = &tc->module->structs[i];
        Type* self_type = new_type(TYPE_NAMED);
        strcpy(self_type->name, s->name);
        for (int j = 0; j < s->method_count; j++) {
            tc1_check_function(tc, &s->methods[j], self_type);
        }
    }
    
    for (int i = 0; i < tc->module->test_count; i++) {
        tc1_enter_scope(tc);
        for (int j = 0; j < tc->module->tests[i].body_count; j++) {
            tc1_check_stmt(tc, tc->module->tests[i].body[j]);
        }
        tc1_exit_scope(tc);
    }
    
    return !tc->had_error;
}

// ============================================================================
// Stage 1 Optimizer - Constant Folding
// ============================================================================

typedef struct {
    Module* module;
    bool had_change;
} Optimizer;

static Optimizer* opt_new(Module* m) {
    Optimizer* opt = calloc(1, sizeof(Optimizer));
    opt->module = m;
    return opt;
}

static Expr* fold_binary(Expr* e) {
    if (e->binary.left->kind != EXPR_INT || e->binary.right->kind != EXPR_INT) return e;
    
    int64_t left = e->binary.left->int_val;
    int64_t right = e->binary.right->int_val;
    int64_t result = 0;
    
    switch (e->binary.op) {
        case TOK_PLUS: result = left + right; break;
        case TOK_MINUS: result = left - right; break;
        case TOK_STAR: result = left * right; break;
        case TOK_SLASH: if (right != 0) result = left / right; else return e; break;
        case TOK_PERCENT: if (right != 0) result = left % right; else return e; break;
        default: return e;
    }
    
    Expr* folded = calloc(1, sizeof(Expr));
    folded->kind = EXPR_INT;
    folded->int_val = result;
    folded->line = e->line;
    folded->column = e->column;
    return folded;
}

static Expr* optimize_expr(Optimizer* opt, Expr* e);

static Expr* optimize_expr(Optimizer* opt, Expr* e) {
    if (!e) return e;
    
    switch (e->kind) {
        case EXPR_BINARY:
            e->binary.left = optimize_expr(opt, e->binary.left);
            e->binary.right = optimize_expr(opt, e->binary.right);
            return fold_binary(e);
        
        case EXPR_CALL:
            // Optimize math function calls
            if (e->call.func->kind == EXPR_FIELD && 
                e->call.func->field.object->kind == EXPR_IDENT &&
                strcmp(e->call.func->field.object->ident, "math") == 0) {
                // Mark for potential inlining (future optimization)
                opt->had_change = true;
            }
            // Optimize arguments
            for (int i = 0; i < e->call.arg_count; i++) {
                e->call.args[i] = optimize_expr(opt, e->call.args[i]);
            }
            return e;
        
        default:
            return e;
    }
}

static void optimize_stmt(Optimizer* opt, Stmt* s) {
    if (!s) return;
    
    switch (s->kind) {
        case STMT_LET:
            if (s->let.value) s->let.value = optimize_expr(opt, s->let.value);
            break;
        case STMT_EXPR:
            s->expr.expr = optimize_expr(opt, s->expr.expr);
            break;
        default: break;
    }
}

static void optimize_module(Optimizer* opt) {
    for (int i = 0; i < opt->module->function_count; i++) {
        FnDef* fn = &opt->module->functions[i];
        for (int j = 0; j < fn->body_count; j++) {
            optimize_stmt(opt, fn->body[j]);
        }
    }
}

// ============================================================================
// Stage 1 Optimizer - Dead Code Elimination
// ============================================================================

static bool stmt_has_return(Stmt* s) {
    if (!s) return false;
    if (s->kind == STMT_RETURN) return true;
    if (s->kind == STMT_IF) {
        bool then_returns = false, else_returns = false;
        for (int i = 0; i < s->if_stmt.then_count; i++) {
            if (stmt_has_return(s->if_stmt.then_block[i])) { then_returns = true; break; }
        }
        if (s->if_stmt.else_block) {
            for (int i = 0; i < s->if_stmt.else_count; i++) {
                if (stmt_has_return(s->if_stmt.else_block[i])) { else_returns = true; break; }
            }
        }
        return then_returns && else_returns;
    }
    return false;
}

static bool block_ends_with_return(Stmt** stmts, int count) {
    return count > 0 && stmts[count - 1]->kind == STMT_RETURN;
}

static bool block_ends_with_break_or_continue(Stmt** stmts, int count) {
    return count > 0 && (stmts[count - 1]->kind == STMT_BREAK || stmts[count - 1]->kind == STMT_CONTINUE);
}

static int eliminate_dead_code(Stmt** stmts, int count) {
    int new_count = 0;
    bool found_return = false;
    
    for (int i = 0; i < count; i++) {
        if (found_return) break; // Skip everything after return
        stmts[new_count++] = stmts[i];
        if (stmt_has_return(stmts[i])) found_return = true;
    }
    
    return new_count;
}

static void optimize_dead_code(Module* m) {
    for (int i = 0; i < m->function_count; i++) {
        FnDef* fn = &m->functions[i];
        fn->body_count = eliminate_dead_code(fn->body, fn->body_count);
    }
}

// ============================================================================
// Stage 1 Optimizer - Bounds Check Elimination
// ============================================================================

static bool is_non_negative_expr(Expr* e) {
    if (!e) return false;
    if (e->kind == EXPR_INT && e->int_val >= 0) return true;
    // Assume all identifiers are non-negative (typically loop counters)
    if (e->kind == EXPR_IDENT) return true;
    // Binary expressions with non-negative operands
    if (e->kind == EXPR_BINARY) {
        if (e->binary.op == TOK_PLUS || e->binary.op == TOK_STAR) {
            return is_non_negative_expr(e->binary.left) && is_non_negative_expr(e->binary.right);
        }
    }
    return false;
}

static void mark_safe_index_expr(Expr* e) {
    if (!e) return;
    
    switch (e->kind) {
        case EXPR_INDEX:
            if (is_non_negative_expr(e->index.index)) {
                e->index.safe = true;
            }
            mark_safe_index_expr(e->index.object);
            mark_safe_index_expr(e->index.index);
            break;
        case EXPR_BINARY:
            mark_safe_index_expr(e->binary.left);
            mark_safe_index_expr(e->binary.right);
            break;
        case EXPR_CALL:
            for (int i = 0; i < e->call.arg_count; i++) {
                mark_safe_index_expr(e->call.args[i]);
            }
            break;
        case EXPR_ARRAY:
            for (int i = 0; i < e->array.count; i++) {
                mark_safe_index_expr(e->array.elements[i]);
            }
            break;
        default: break;
    }
}

static void mark_safe_index_stmt(Stmt* s) {
    if (!s) return;
    
    switch (s->kind) {
        case STMT_LET:
            if (s->let.value) mark_safe_index_expr(s->let.value);
            break;
        case STMT_ASSIGN:
            mark_safe_index_expr(s->assign.value);
            break;
        case STMT_EXPR:
            mark_safe_index_expr(s->expr.expr);
            break;
        case STMT_RETURN:
            if (s->ret.value) mark_safe_index_expr(s->ret.value);
            break;
        case STMT_WHILE:
            for (int i = 0; i < s->while_stmt.body_count; i++) {
                mark_safe_index_stmt(s->while_stmt.body[i]);
            }
            break;
        case STMT_FOR:
            for (int i = 0; i < s->for_stmt.body_count; i++) {
                mark_safe_index_stmt(s->for_stmt.body[i]);
            }
            break;
        default: break;
    }
}

static void mark_all_safe_expr(Expr* e);
static void mark_all_safe_stmt(Stmt* s);

static void mark_all_safe_expr(Expr* e) {
    if (!e) return;
    
    switch (e->kind) {
        case EXPR_INDEX:
            e->index.safe = true;
            mark_all_safe_expr(e->index.object);
            mark_all_safe_expr(e->index.index);
            break;
        case EXPR_BINARY:
            mark_all_safe_expr(e->binary.left);
            mark_all_safe_expr(e->binary.right);
            break;
        case EXPR_CALL:
            for (int i = 0; i < e->call.arg_count; i++) {
                mark_all_safe_expr(e->call.args[i]);
            }
            break;
        default: break;
    }
}

static void mark_all_safe_stmt(Stmt* s) {
    if (!s) return;
    
    switch (s->kind) {
        case STMT_LET:
            if (s->let.value) mark_all_safe_expr(s->let.value);
            break;
        case STMT_ASSIGN:
            mark_all_safe_expr(s->assign.value);
            break;
        case STMT_EXPR:
            mark_all_safe_expr(s->expr.expr);
            break;
        case STMT_RETURN:
            if (s->ret.value) mark_all_safe_expr(s->ret.value);
            break;
        case STMT_WHILE:
            for (int i = 0; i < s->while_stmt.body_count; i++) {
                mark_all_safe_stmt(s->while_stmt.body[i]);
            }
            break;
        case STMT_FOR:
            for (int i = 0; i < s->for_stmt.body_count; i++) {
                mark_all_safe_stmt(s->for_stmt.body[i]);
            }
            break;
        default: break;
    }
}

static void optimize_bounds_checks(Module* m, bool disable_all) {
    for (int i = 0; i < m->function_count; i++) {
        FnDef* fn = &m->functions[i];
        for (int j = 0; j < fn->body_count; j++) {
            mark_safe_index_stmt(fn->body[j]);
        }
    }
    
    // If --no-bounds-check, mark ALL indices as safe
    if (disable_all) {
        for (int i = 0; i < m->function_count; i++) {
            FnDef* fn = &m->functions[i];
            for (int j = 0; j < fn->body_count; j++) {
                mark_all_safe_stmt(fn->body[j]);
            }
        }
    }
}

// ============================================================================
// Function Inlining
// ============================================================================

static int count_stmts(Stmt** body, int count) {
    int total = 0;
    for (int i = 0; i < count; i++) {
        total++;
        if (body[i]->kind == STMT_IF) {
            total += count_stmts(body[i]->if_stmt.then_block, body[i]->if_stmt.then_count);
            total += count_stmts(body[i]->if_stmt.else_block, body[i]->if_stmt.else_count);
        } else if (body[i]->kind == STMT_WHILE) {
            total += count_stmts(body[i]->while_stmt.body, body[i]->while_stmt.body_count);
        } else if (body[i]->kind == STMT_FOR) {
            total += count_stmts(body[i]->for_stmt.body, body[i]->for_stmt.body_count);
        }
    }
    return total;
}

static bool has_recursion(FnDef* fn, const char* fn_name);
static bool expr_calls_fn(Expr* e, const char* fn_name) {
    if (!e) return false;
    if (e->kind == EXPR_CALL && e->call.func->kind == EXPR_IDENT && 
        strcmp(e->call.func->ident, fn_name) == 0) return true;
    
    switch (e->kind) {
        case EXPR_BINARY: return expr_calls_fn(e->binary.left, fn_name) || expr_calls_fn(e->binary.right, fn_name);
        case EXPR_UNARY: return expr_calls_fn(e->unary.operand, fn_name);
        case EXPR_CALL:
            for (int i = 0; i < e->call.arg_count; i++)
                if (expr_calls_fn(e->call.args[i], fn_name)) return true;
            return false;
        case EXPR_INDEX: return expr_calls_fn(e->index.object, fn_name) || expr_calls_fn(e->index.index, fn_name);
        case EXPR_FIELD: return expr_calls_fn(e->field.object, fn_name);
        case EXPR_IF: return expr_calls_fn(e->if_expr.cond, fn_name) || expr_calls_fn(e->if_expr.then_expr, fn_name) || expr_calls_fn(e->if_expr.else_expr, fn_name);
        default: return false;
    }
}

static bool stmt_calls_fn(Stmt* s, const char* fn_name) {
    if (!s) return false;
    switch (s->kind) {
        case STMT_LET: return expr_calls_fn(s->let.value, fn_name);
        case STMT_ASSIGN: return expr_calls_fn(s->assign.value, fn_name);
        case STMT_EXPR: return expr_calls_fn(s->expr.expr, fn_name);
        case STMT_RETURN: return expr_calls_fn(s->ret.value, fn_name);
        case STMT_IF:
            for (int i = 0; i < s->if_stmt.then_count; i++)
                if (stmt_calls_fn(s->if_stmt.then_block[i], fn_name)) return true;
            for (int i = 0; i < s->if_stmt.else_count; i++)
                if (stmt_calls_fn(s->if_stmt.else_block[i], fn_name)) return true;
            return expr_calls_fn(s->if_stmt.cond, fn_name);
        case STMT_WHILE:
            for (int i = 0; i < s->while_stmt.body_count; i++)
                if (stmt_calls_fn(s->while_stmt.body[i], fn_name)) return true;
            return expr_calls_fn(s->while_stmt.cond, fn_name);
        case STMT_FOR:
            for (int i = 0; i < s->for_stmt.body_count; i++)
                if (stmt_calls_fn(s->for_stmt.body[i], fn_name)) return true;
            return expr_calls_fn(s->for_stmt.iter, fn_name);
        default: return false;
    }
}

static bool has_recursion(FnDef* fn, const char* fn_name) {
    for (int i = 0; i < fn->body_count; i++)
        if (stmt_calls_fn(fn->body[i], fn_name)) return true;
    return false;
}

static bool should_inline(FnDef* fn) {
    if (fn->body_count == 0) return false;
    if (count_stmts(fn->body, fn->body_count) >= 10) return false;
    if (has_recursion(fn, fn->name)) return false;
    
    for (int i = 0; i < fn->body_count; i++) {
        if (fn->body[i]->kind == STMT_SPAWN || fn->body[i]->kind == STMT_DEFER) return false;
    }
    return true;
}

static Expr* clone_expr(Expr* e);

static Expr* clone_expr(Expr* e) {
    if (!e) return NULL;
    Expr* c = malloc(sizeof(Expr));
    memcpy(c, e, sizeof(Expr));
    
    switch (e->kind) {
        case EXPR_BINARY:
            c->binary.left = clone_expr(e->binary.left);
            c->binary.right = clone_expr(e->binary.right);
            break;
        case EXPR_UNARY:
            c->unary.operand = clone_expr(e->unary.operand);
            break;
        case EXPR_CALL:
            c->call.func = clone_expr(e->call.func);
            c->call.args = malloc(sizeof(Expr*) * e->call.arg_count);
            for (int i = 0; i < e->call.arg_count; i++)
                c->call.args[i] = clone_expr(e->call.args[i]);
            break;
        case EXPR_INDEX:
            c->index.object = clone_expr(e->index.object);
            c->index.index = clone_expr(e->index.index);
            break;
        case EXPR_FIELD:
            c->field.object = clone_expr(e->field.object);
            break;
        default: break;
    }
    return c;
}

static Expr* substitute_params(Expr* e, FnDef* fn, Expr** args) {
    if (!e) return NULL;
    
    if (e->kind == EXPR_IDENT) {
        for (int i = 0; i < fn->param_count; i++) {
            if (strcmp(e->ident, fn->params[i].name) == 0) {
                return clone_expr(args[i]);
            }
        }
    }
    
    switch (e->kind) {
        case EXPR_BINARY:
            e->binary.left = substitute_params(e->binary.left, fn, args);
            e->binary.right = substitute_params(e->binary.right, fn, args);
            break;
        case EXPR_UNARY:
            e->unary.operand = substitute_params(e->unary.operand, fn, args);
            break;
        case EXPR_CALL:
            e->call.func = substitute_params(e->call.func, fn, args);
            for (int i = 0; i < e->call.arg_count; i++)
                e->call.args[i] = substitute_params(e->call.args[i], fn, args);
            break;
        case EXPR_INDEX:
            e->index.object = substitute_params(e->index.object, fn, args);
            e->index.index = substitute_params(e->index.index, fn, args);
            break;
        default: break;
    }
    return e;
}

static Expr* inline_function_call(Expr* call, FnDef* fn) {
    if (fn->body_count == 1 && fn->body[0]->kind == STMT_RETURN) {
        Expr* result = clone_expr(fn->body[0]->ret.value);
        return substitute_params(result, fn, call->call.args);
    }
    return call;
}

static void inline_expr(Expr* e, Module* m) {
    if (!e) return;
    
    if (e->kind == EXPR_CALL && e->call.func->kind == EXPR_IDENT) {
        for (int i = 0; i < m->function_count; i++) {
            FnDef* fn = &m->functions[i];
            if (strcmp(fn->name, e->call.func->ident) == 0 && should_inline(fn)) {
                Expr* inlined = inline_function_call(e, fn);
                if (inlined != e) {
                    memcpy(e, inlined, sizeof(Expr));
                    free(inlined);
                    return;
                }
            }
        }
    }
    
    switch (e->kind) {
        case EXPR_BINARY:
            inline_expr(e->binary.left, m);
            inline_expr(e->binary.right, m);
            break;
        case EXPR_UNARY:
            inline_expr(e->unary.operand, m);
            break;
