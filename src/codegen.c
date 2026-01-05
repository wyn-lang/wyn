#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "common.h"
#include "ast.h"

static FILE* out = NULL;

void init_codegen(FILE* output) {
    out = output;
}

static void emit(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);
}

void codegen_expr(Expr* expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case EXPR_INT:
            emit("%.*s", expr->token.length, expr->token.start);
            break;
        case EXPR_FLOAT:
            emit("%.*s", expr->token.length, expr->token.start);
            break;
        case EXPR_STRING:
            emit("\"%.*s\"", expr->token.length - 2, expr->token.start + 1);
            break;
        case EXPR_IDENT:
            emit("%.*s", expr->token.length, expr->token.start);
            break;
        case EXPR_BOOL:
            emit("%.*s", expr->token.length, expr->token.start);
            break;
        case EXPR_UNARY:
            if (expr->unary.op.type == TOKEN_NOT) {
                emit("!");
            } else {
                emit("%.*s", expr->unary.op.length, expr->unary.op.start);
            }
            codegen_expr(expr->unary.operand);
            break;
        case EXPR_BINARY:
            emit("(");
            codegen_expr(expr->binary.left);
            if (expr->binary.op.type == TOKEN_AND) {
                emit(" && ");
            } else if (expr->binary.op.type == TOKEN_OR) {
                emit(" || ");
            } else {
                emit(" %.*s ", expr->binary.op.length, expr->binary.op.start);
            }
            codegen_expr(expr->binary.right);
            emit(")");
            break;
        case EXPR_CALL:
            codegen_expr(expr->call.callee);
            emit("(");
            for (int i = 0; i < expr->call.arg_count; i++) {
                if (i > 0) emit(", ");
                codegen_expr(expr->call.args[i]);
            }
            emit(")");
            break;
        case EXPR_METHOD_CALL:
            codegen_expr(expr->method_call.object);
            emit(".%.*s(", expr->method_call.method.length, expr->method_call.method.start);
            for (int i = 0; i < expr->method_call.arg_count; i++) {
                if (i > 0) emit(", ");
                codegen_expr(expr->method_call.args[i]);
            }
            emit(")");
            break;
        case EXPR_ARRAY:
            emit("{");
            for (int i = 0; i < expr->array.count; i++) {
                if (i > 0) emit(", ");
                codegen_expr(expr->array.elements[i]);
            }
            emit("}");
            break;
        case EXPR_INDEX:
            codegen_expr(expr->index.array);
            emit("[");
            codegen_expr(expr->index.index);
            emit("]");
            break;
        case EXPR_ASSIGN:
            emit("%.*s = ", expr->assign.name.length, expr->assign.name.start);
            codegen_expr(expr->assign.value);
            break;
        case EXPR_FIELD_ACCESS:
            codegen_expr(expr->field_access.object);
            emit(".%.*s", expr->field_access.field.length, expr->field_access.field.start);
            break;
        case EXPR_MATCH: {
            emit("({ int __match_result; switch (");
            codegen_expr(expr->match.value);
            emit(") {\n");
            for (int i = 0; i < expr->match.arm_count; i++) {
                emit("        case %.*s: __match_result = ", 
                     expr->match.arms[i].pattern.length,
                     expr->match.arms[i].pattern.start);
                codegen_expr(expr->match.arms[i].result);
                emit("; break;\n");
            }
            emit("    } __match_result; })");
            break;
        }
        case EXPR_SOME:
            emit("some(");
            codegen_expr(expr->option.value);
            emit(")");
            break;
        case EXPR_NONE:
            emit("none()");
            break;
        case EXPR_OK:
            emit("ok(");
            codegen_expr(expr->option.value);
            emit(")");
            break;
        case EXPR_ERR:
            emit("err(");
            codegen_expr(expr->option.value);
            emit(")");
            break;
        default:
            break;
    }
}

void codegen_c_header() {
    emit("#include <stdio.h>\n");
    emit("#include <stdlib.h>\n");
    emit("#include <stdint.h>\n");
    emit("#include <stdbool.h>\n");
    emit("#include <string.h>\n");
    emit("#include <math.h>\n");
    emit("#include <time.h>\n");
    emit("#include <ctype.h>\n\n");
    
    emit("void print(int x) { printf(\"%%d\\n\", x); }\n");
    emit("void print_float(double x) { printf(\"%%f\\n\", x); }\n");
    emit("void print_str(const char* s) { printf(\"%%s\\n\", s); }\n");
    emit("void print_bool(bool b) { printf(\"%%s\\n\", b ? \"true\" : \"false\"); }\n");
    emit("int input() { int x; scanf(\"%%d\", &x); return x; }\n");
    emit("int str_len(const char* s) { return strlen(s); }\n");
    emit("int str_eq(const char* a, const char* b) { return strcmp(a, b) == 0; }\n");
    emit("char* str_concat(const char* a, const char* b) { char* r = malloc(strlen(a) + strlen(b) + 1); strcpy(r, a); strcat(r, b); return r; }\n");
    emit("char* str_upper(const char* s) { char* r = malloc(strlen(s) + 1); for(int i = 0; s[i]; i++) r[i] = toupper(s[i]); r[strlen(s)] = 0; return r; }\n");
    emit("char* str_lower(const char* s) { char* r = malloc(strlen(s) + 1); for(int i = 0; s[i]; i++) r[i] = tolower(s[i]); r[strlen(s)] = 0; return r; }\n");
    emit("int abs_val(int x) { return x < 0 ? -x : x; }\n");
    emit("int min(int a, int b) { return a < b ? a : b; }\n");
    emit("int max(int a, int b) { return a > b ? a : b; }\n");
    emit("int pow_int(int base, int exp) { int r = 1; for(int i = 0; i < exp; i++) r *= base; return r; }\n");
    emit("int clamp(int x, int min_val, int max_val) { return x < min_val ? min_val : (x > max_val ? max_val : x); }\n");
    emit("int sign(int x) { return x < 0 ? -1 : (x > 0 ? 1 : 0); }\n");
    emit("int gcd(int a, int b) { while(b) { int t = b; b = a %% b; a = t; } return a; }\n");
    emit("int lcm(int a, int b) { return a * b / gcd(a, b); }\n");
    emit("int is_even(int x) { return x %% 2 == 0; }\n");
    emit("int is_odd(int x) { return x %% 2 != 0; }\n");
    emit("char* file_read(const char* path) { FILE* f = fopen(path, \"r\"); if(!f) return NULL; fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET); char* buf = malloc(sz + 1); fread(buf, 1, sz, f); buf[sz] = 0; fclose(f); return buf; }\n");
    emit("int file_write(const char* path, const char* data) { FILE* f = fopen(path, \"w\"); if(!f) return 0; fputs(data, f); fclose(f); return 1; }\n");
    emit("int file_exists(const char* path) { FILE* f = fopen(path, \"r\"); if(f) { fclose(f); return 1; } return 0; }\n");
    emit("int arr_sum(int* arr, int len) { int s = 0; for(int i = 0; i < len; i++) s += arr[i]; return s; }\n");
    emit("int arr_max(int* arr, int len) { int m = arr[0]; for(int i = 1; i < len; i++) if(arr[i] > m) m = arr[i]; return m; }\n");
    emit("int arr_min(int* arr, int len) { int m = arr[0]; for(int i = 1; i < len; i++) if(arr[i] < m) m = arr[i]; return m; }\n");
    emit("int arr_contains(int* arr, int len, int val) { for(int i = 0; i < len; i++) if(arr[i] == val) return 1; return 0; }\n");
    emit("int arr_find(int* arr, int len, int val) { for(int i = 0; i < len; i++) if(arr[i] == val) return i; return -1; }\n");
    emit("void arr_reverse(int* arr, int len) { for(int i = 0; i < len/2; i++) { int t = arr[i]; arr[i] = arr[len-1-i]; arr[len-1-i] = t; } }\n");
    emit("int random_int(int max) { return rand() %% max; }\n");
    emit("void seed_random(int seed) { srand(seed); }\n");
    emit("int time_now() { return (int)time(NULL); }\n");
    emit("void assert_eq(int a, int b) { if (a != b) { printf(\"Assertion failed: %%d != %%d\\n\", a, b); exit(1); } }\n\n");
}

void codegen_stmt(Stmt* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case STMT_EXPR:
            codegen_expr(stmt->expr);
            emit(";\n");
            break;
        case STMT_VAR: {
            // Determine C type based on initializer
            const char* c_type = "int";
            if (stmt->var.init) {
                if (stmt->var.init->type == EXPR_STRING) {
                    c_type = "const char*";
                } else if (stmt->var.init->type == EXPR_FLOAT) {
                    c_type = "double";
                } else if (stmt->var.init->type == EXPR_BOOL) {
                    c_type = "bool";
                } else if (stmt->var.init->type == EXPR_ARRAY) {
                    c_type = "int";
                }
            }
            
            if (stmt->var.init && stmt->var.init->type == EXPR_ARRAY) {
                if (stmt->var.is_const) {
                    emit("const %s %.*s[] = ", c_type, stmt->var.name.length, stmt->var.name.start);
                } else {
                    emit("%s %.*s[] = ", c_type, stmt->var.name.length, stmt->var.name.start);
                }
            } else {
                if (stmt->var.is_const) {
                    emit("const %s %.*s = ", c_type, stmt->var.name.length, stmt->var.name.start);
                } else {
                    emit("%s %.*s = ", c_type, stmt->var.name.length, stmt->var.name.start);
                }
            }
            codegen_expr(stmt->var.init);
            emit(";\n");
            break;
        }
        case STMT_RETURN:
            emit("return ");
            codegen_expr(stmt->ret.value);
            emit(";\n");
            break;
        case STMT_BREAK:
            emit("break;\n");
            break;
        case STMT_CONTINUE:
            emit("continue;\n");
            break;
        case STMT_BLOCK:
            for (int i = 0; i < stmt->block.count; i++) {
                emit("    ");
                codegen_stmt(stmt->block.stmts[i]);
            }
            break;
        case STMT_FN:
            emit("int %.*s(", stmt->fn.name.length, stmt->fn.name.start);
            for (int i = 0; i < stmt->fn.param_count; i++) {
                if (i > 0) emit(", ");
                emit("int %.*s", stmt->fn.params[i].length, stmt->fn.params[i].start);
            }
            emit(") {\n");
            codegen_stmt(stmt->fn.body);
            emit("}\n\n");
            break;
        case STMT_STRUCT:
            emit("typedef struct {\n");
            for (int i = 0; i < stmt->struct_decl.field_count; i++) {
                emit("    int %.*s;\n", 
                     stmt->struct_decl.fields[i].length,
                     stmt->struct_decl.fields[i].start);
            }
            emit("} %.*s;\n\n", 
                 stmt->struct_decl.name.length,
                 stmt->struct_decl.name.start);
            break;
        case STMT_ENUM:
            emit("typedef enum {\n");
            for (int i = 0; i < stmt->enum_decl.variant_count; i++) {
                emit("    %.*s", 
                     stmt->enum_decl.variants[i].length,
                     stmt->enum_decl.variants[i].start);
                if (i < stmt->enum_decl.variant_count - 1) {
                    emit(",");
                }
                emit("\n");
            }
            emit("} %.*s;\n\n", 
                 stmt->enum_decl.name.length,
                 stmt->enum_decl.name.start);
            break;
        case STMT_TYPE_ALIAS:
            emit("typedef %.*s %.*s;\n\n",
                 stmt->type_alias.target.length,
                 stmt->type_alias.target.start,
                 stmt->type_alias.name.length,
                 stmt->type_alias.name.start);
            break;
        case STMT_IMPL:
            for (int i = 0; i < stmt->impl.method_count; i++) {
                FnStmt* method = stmt->impl.methods[i];
                emit("int %.*s_%.*s(", 
                     stmt->impl.type_name.length, stmt->impl.type_name.start,
                     method->name.length, method->name.start);
                for (int j = 0; j < method->param_count; j++) {
                    if (j > 0) emit(", ");
                    emit("int %.*s", method->params[j].length, method->params[j].start);
                }
                emit(") {\n");
                codegen_stmt(method->body);
                emit("}\n\n");
            }
            break;
        case STMT_IF:
            emit("if (");
            codegen_expr(stmt->if_stmt.condition);
            emit(") {\n");
            codegen_stmt(stmt->if_stmt.then_branch);
            emit("    }");
            if (stmt->if_stmt.else_branch) {
                emit(" else ");
                if (stmt->if_stmt.else_branch->type == STMT_IF) {
                    codegen_stmt(stmt->if_stmt.else_branch);
                } else {
                    emit("{\n");
                    codegen_stmt(stmt->if_stmt.else_branch);
                    emit("    }\n");
                }
            } else {
                emit("\n");
            }
            break;
        case STMT_WHILE:
            emit("while (");
            codegen_expr(stmt->while_stmt.condition);
            emit(") {\n");
            codegen_stmt(stmt->while_stmt.body);
            emit("    }\n");
            break;
        case STMT_FOR:
            emit("for (");
            if (stmt->for_stmt.init) {
                emit("int %.*s = ", stmt->for_stmt.init->var.name.length, stmt->for_stmt.init->var.name.start);
                codegen_expr(stmt->for_stmt.init->var.init);
            }
            emit("; ");
            codegen_expr(stmt->for_stmt.condition);
            emit("; ");
            codegen_expr(stmt->for_stmt.increment);
            emit(") {\n");
            codegen_stmt(stmt->for_stmt.body);
            emit("    }\n");
            break;
        default:
            break;
    }
}

void codegen_program(Program* prog) {
    bool has_main = false;
    
    for (int i = 0; i < prog->count; i++) {
        if (prog->stmts[i]->type == STMT_FN) {
            if (prog->stmts[i]->fn.name.length == 4 &&
                memcmp(prog->stmts[i]->fn.name.start, "main", 4) == 0) {
                has_main = true;
            }
        }
    }
    
    if (!has_main && prog->count == 1 && prog->stmts[0]->type == STMT_EXPR) {
        emit("int main() {\n");
        emit("    return ");
        codegen_expr(prog->stmts[0]->expr);
        emit(";\n");
        emit("}\n");
    } else {
        for (int i = 0; i < prog->count; i++) {
            codegen_stmt(prog->stmts[i]);
        }
    }
}
