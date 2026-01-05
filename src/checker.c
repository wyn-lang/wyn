#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "ast.h"
#include "types.h"

static SymbolTable* global_scope = NULL;
static Type* builtin_int = NULL;
static Type* builtin_float = NULL;
static Type* builtin_string = NULL;
static Type* builtin_bool = NULL;
static Type* builtin_void = NULL;
static bool had_error = false;

static Type* make_type(TypeKind kind) {
    Type* t = calloc(1, sizeof(Type));
    t->kind = kind;
    return t;
}

static void add_symbol(SymbolTable* scope, Token name, Type* type, bool is_mutable) {
    if (scope->count >= scope->capacity) {
        scope->capacity *= 2;
        scope->symbols = realloc(scope->symbols, scope->capacity * sizeof(Symbol));
    }
    scope->symbols[scope->count].name = name;
    scope->symbols[scope->count].type = type;
    scope->symbols[scope->count].is_mutable = is_mutable;
    scope->count++;
}

void init_checker() {
    global_scope = calloc(1, sizeof(SymbolTable));
    global_scope->capacity = 128;
    global_scope->symbols = calloc(128, sizeof(Symbol));
    had_error = false;
    
    builtin_int = make_type(TYPE_INT);
    builtin_float = make_type(TYPE_FLOAT);
    builtin_string = make_type(TYPE_STRING);
    builtin_bool = make_type(TYPE_BOOL);
    builtin_void = make_type(TYPE_VOID);
    
    // Add built-in functions
    const char* stdlib_funcs[] = {
        "print", "print_float", "print_str", "print_bool", "input",
        "str_len", "str_eq", "str_concat", "str_upper", "str_lower",
        "abs_val", "min", "max", "pow_int", "clamp", "sign", "gcd", "lcm", "is_even", "is_odd",
        "arr_sum", "arr_max", "arr_min", "arr_contains", "arr_find", "arr_reverse",
        "file_read", "file_write", "file_exists",
        "random_int", "seed_random", "time_now", "assert_eq"
    };
    
    for (int i = 0; i < 33; i++) {
        Token tok = {TOKEN_IDENT, stdlib_funcs[i], (int)strlen(stdlib_funcs[i]), 0};
        add_symbol(global_scope, tok, builtin_int, false);
    }
}

static Symbol* find_symbol(SymbolTable* scope, Token name) {
    for (int i = 0; i < scope->count; i++) {
        if (scope->symbols[i].name.length == name.length &&
            memcmp(scope->symbols[i].name.start, name.start, name.length) == 0) {
            return &scope->symbols[i];
        }
    }
    if (scope->parent) return find_symbol(scope->parent, name);
    return NULL;
}

Type* check_expr(Expr* expr, SymbolTable* scope) {
    if (!expr) return NULL;
    
    switch (expr->type) {
        case EXPR_INT:
            return builtin_int;
        case EXPR_FLOAT:
            return builtin_float;
        case EXPR_STRING:
            return builtin_string;
        case EXPR_BOOL:
            return builtin_bool;
        case EXPR_IDENT: {
            Symbol* sym = find_symbol(scope, expr->token);
            if (!sym) {
                fprintf(stderr, "Error at line %d: Undefined variable '%.*s'\n", 
                        expr->token.line, expr->token.length, expr->token.start);
                had_error = true;
                return NULL;
            }
            return sym->type;
        }
        case EXPR_BINARY: {
            Type* left = check_expr(expr->binary.left, scope);
            Type* right = check_expr(expr->binary.right, scope);
            if (!left || !right) return NULL;
            if (left->kind != right->kind) {
                fprintf(stderr, "Error at line %d: Type mismatch in binary expression\n", 
                        expr->binary.op.line);
                had_error = true;
                return NULL;
            }
            return left;
        }
        case EXPR_CALL: {
            Type* callee_type = check_expr(expr->call.callee, scope);
            
            // Check argument count if function type is known
            if (callee_type && callee_type->kind == TYPE_FUNCTION) {
                if (expr->call.arg_count != callee_type->fn_type.param_count) {
                    fprintf(stderr, "Error: Function expects %d arguments, got %d\n",
                            callee_type->fn_type.param_count, expr->call.arg_count);
                    had_error = true;
                }
            }
            
            // Check all arguments
            for (int i = 0; i < expr->call.arg_count; i++) {
                check_expr(expr->call.args[i], scope);
            }
            
            if (callee_type && callee_type->kind == TYPE_FUNCTION) {
                return callee_type->fn_type.return_type;
            }
            return builtin_int;
        }
        case EXPR_METHOD_CALL:
            check_expr(expr->method_call.object, scope);
            for (int i = 0; i < expr->method_call.arg_count; i++) {
                check_expr(expr->method_call.args[i], scope);
            }
            return builtin_int;
        case EXPR_ARRAY:
            return builtin_int;
        case EXPR_INDEX: {
            check_expr(expr->index.array, scope);
            Type* idx_type = check_expr(expr->index.index, scope);
            if (idx_type && idx_type->kind != TYPE_INT) {
                fprintf(stderr, "Error: Array index must be int\n");
                return NULL;
            }
            return builtin_int;
        }
        case EXPR_ASSIGN: {
            Symbol* sym = find_symbol(scope, expr->assign.name);
            if (!sym) {
                fprintf(stderr, "Error: Undefined variable '%.*s'\n",
                        expr->assign.name.length, expr->assign.name.start);
                return NULL;
            }
            Type* val_type = check_expr(expr->assign.value, scope);
            if (val_type && val_type->kind != sym->type->kind) {
                fprintf(stderr, "Error: Type mismatch in assignment\n");
                return NULL;
            }
            return sym->type;
        }
        default:
            return builtin_int;
    }
}

void check_stmt(Stmt* stmt, SymbolTable* scope) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case STMT_VAR: {
            Type* init_type = check_expr(stmt->var.init, scope);
            if (init_type) {
                add_symbol(scope, stmt->var.name, init_type, !stmt->var.is_const);
            }
            break;
        }
        case STMT_EXPR:
            check_expr(stmt->expr, scope);
            break;
        case STMT_RETURN:
            check_expr(stmt->ret.value, scope);
            break;
        case STMT_BLOCK:
            for (int i = 0; i < stmt->block.count; i++) {
                check_stmt(stmt->block.stmts[i], scope);
            }
            break;
        case STMT_IF:
            check_expr(stmt->if_stmt.condition, scope);
            check_stmt(stmt->if_stmt.then_branch, scope);
            if (stmt->if_stmt.else_branch) {
                check_stmt(stmt->if_stmt.else_branch, scope);
            }
            break;
        case STMT_WHILE:
            check_expr(stmt->while_stmt.condition, scope);
            check_stmt(stmt->while_stmt.body, scope);
            break;
        case STMT_FOR:
            if (stmt->for_stmt.init) {
                check_stmt(stmt->for_stmt.init, scope);
            }
            check_expr(stmt->for_stmt.condition, scope);
            check_expr(stmt->for_stmt.increment, scope);
            check_stmt(stmt->for_stmt.body, scope);
            break;
        default:
            break;
    }
}

void check_program(Program* prog) {
    // First pass: register functions with their signatures
    for (int i = 0; i < prog->count; i++) {
        if (prog->stmts[i]->type == STMT_FN) {
            FnStmt* fn = &prog->stmts[i]->fn;
            
            // Create function type
            Type* fn_type = make_type(TYPE_FUNCTION);
            fn_type->fn_type.param_count = fn->param_count;
            fn_type->fn_type.param_types = malloc(sizeof(Type*) * fn->param_count);
            for (int j = 0; j < fn->param_count; j++) {
                fn_type->fn_type.param_types[j] = builtin_int; // For now, all params are int
            }
            fn_type->fn_type.return_type = builtin_int;
            
            add_symbol(global_scope, fn->name, fn_type, false);
        } else if (prog->stmts[i]->type == STMT_ENUM) {
            EnumStmt* enum_decl = &prog->stmts[i]->enum_decl;
            for (int j = 0; j < enum_decl->variant_count; j++) {
                add_symbol(global_scope, enum_decl->variants[j], builtin_int, false);
            }
        }
    }
    
    // Second pass: check function bodies
    for (int i = 0; i < prog->count; i++) {
        if (prog->stmts[i]->type == STMT_FN) {
            SymbolTable local_scope;
            local_scope.parent = global_scope;
            local_scope.capacity = 32;
            local_scope.symbols = calloc(32, sizeof(Symbol));
            local_scope.count = 0;
            
            FnStmt* fn = &prog->stmts[i]->fn;
            for (int j = 0; j < fn->param_count; j++) {
                add_symbol(&local_scope, fn->params[j], builtin_int, true);
            }
            
            check_stmt(fn->body, &local_scope);
            free(local_scope.symbols);
        } else {
            check_stmt(prog->stmts[i], global_scope);
        }
    }
}

SymbolTable* get_global_scope() {
    return global_scope;
}

bool checker_had_error() {
    return had_error;
}
