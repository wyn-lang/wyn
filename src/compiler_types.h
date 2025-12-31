#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H

#include <stdbool.h>

#define MAX_IDENT_LEN 128

// Type kinds
#define TYPE_INT 1
#define TYPE_FLOAT 2
#define TYPE_STR 3
#define TYPE_BOOL 4
#define TYPE_ARRAY 5
#define TYPE_NAMED 6
#define TYPE_GENERIC 7
#define TYPE_RESULT 8
#define TYPE_OPTIONAL 9
#define TYPE_ANY 10

// Token types
#define TOK_PLUS 1
#define TOK_MINUS 2
#define TOK_STAR 3
#define TOK_SLASH 4
#define TOK_PERCENT 5
#define TOK_LT 6
#define TOK_GT 7
#define TOK_LTEQ 8
#define TOK_GTEQ 9
#define TOK_EQEQ 10
#define TOK_NOTEQ 11
#define TOK_BANG 12
#define TOK_TILDE 13
#define TOK_PIPE 14
#define TOK_AMPERSAND 15
#define TOK_CARET 16
#define TOK_LTLT 17
#define TOK_GTGT 18
#define TOK_STARSTAR 19
#define TOK_IN 20
#define TOK_DOTDOT 21
#define TOK_DOTDOTEQ 22
#define TOK_EQ 23
#define TOK_PLUSEQ 24
#define TOK_MINUSEQ 25
#define TOK_STAREQ 26
#define TOK_SLASHEQ 27

// Expression kinds
#define EXPR_INT 1
#define EXPR_FLOAT 2
#define EXPR_STRING 3
#define EXPR_BOOL 4
#define EXPR_IDENT 5
#define EXPR_BINARY 6
#define EXPR_UNARY 7
#define EXPR_CALL 8
#define EXPR_ARRAY 9
#define EXPR_INDEX 10
#define EXPR_STRUCT 11
#define EXPR_FIELD 12
#define EXPR_SLICE 13
#define EXPR_AWAIT 14
#define EXPR_TERNARY 15
#define EXPR_COMPREHENSION 16
#define EXPR_LAMBDA 17
#define EXPR_MATCH 18
#define EXPR_UNWRAP 19
#define EXPR_OK 20
#define EXPR_ERR 21
#define EXPR_SOME 22
#define EXPR_NONE 23

// Statement kinds
#define STMT_LET 1
#define STMT_ASSIGN 2
#define STMT_EXPR 3
#define STMT_RETURN 4
#define STMT_IF 5
#define STMT_WHILE 6
#define STMT_FOR 7
#define STMT_BREAK 8
#define STMT_CONTINUE 9
#define STMT_BLOCK 10
#define STMT_MATCH 11
#define STMT_SPAWN 12

// Forward declarations - actual definitions in compiler.c
typedef struct Type Type;
typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct FnDef FnDef;
typedef struct Module Module;
typedef struct StructDef StructDef;
typedef struct StringTable StringTable;

// String table for deduplication
struct StringTable {
    char strings[1024][256];
    int count;
};

// Module structure (simplified)
struct Module {
    struct FnDef* functions;
    int function_count;
    struct StructDef* structs;
    int struct_count;
    struct Stmt** top_level_stmts;
    int top_level_count;
    int enum_count;  // Placeholder for enum support
    void* enums;     // Placeholder
};

// Function definition
struct FnDef {
    char name[MAX_IDENT_LEN];
    struct {
        char name[MAX_IDENT_LEN];
        struct Type* type;
        struct Expr* default_val;
    } params[32];
    int param_count;
    struct Type* return_type;
    struct Stmt** body;
    int body_count;
    bool is_pub;
    bool is_async;
    int generic_count;
    bool is_extension;
    char extension_method[MAX_IDENT_LEN];
    char extension_type[MAX_IDENT_LEN];
};

// Struct definition
struct StructDef {
    char name[MAX_IDENT_LEN];
    struct {
        char name[MAX_IDENT_LEN];
        struct Type* type;
    } fields[32];
    int field_count;
};

// Minimal struct definitions needed for codegen
struct Type {
    int kind;
    struct Type* inner;  // For arrays, optionals, etc.
    char name[MAX_IDENT_LEN];  // For named types
};

struct Expr {
    int kind;
    union {
        long long int_val;
        double float_val;
        bool bool_val;
        char* str_val;
        char* ident;
        struct {
            struct Expr* left;
            struct Expr* right;
            int op;
        } binary;
        struct {
            struct Expr* operand;
            int op;
        } unary;
        struct {
            struct Expr* func;
            struct Expr** args;
            int arg_count;
            struct Type** type_args;
            int type_arg_count;
        } call;
        struct {
            struct Expr** elements;
            int count;
        } array;
        struct {
            struct Expr* object;
            struct Expr* index;
        } index;
        struct {
            char* name;
            char** fields;
            struct Expr** values;
            int count;
        } struct_lit;
        struct {
            struct Expr* object;
            char* field;
        } field;
        struct {
            struct Expr* object;
            struct Expr* start;
            struct Expr* end;
        } slice;
        struct {
            struct Expr* future;
        } await_expr;
        struct {
            struct Expr* cond;
            struct Expr* true_val;
            struct Expr* false_val;
        } ternary;
        struct {
            struct Expr* value;
        } some;
    };
};

struct Stmt {
    int kind;
    union {
        struct {
            char* name;
            struct Type* type;
            struct Expr* value;
        } let;
        struct {
            struct Expr* target;
            struct Expr* value;
            int op;
        } assign;
        struct {
            struct Expr* expr;
        } expr;
        struct {
            struct Expr* value;
        } ret;
        struct {
            struct Expr* cond;
            struct Stmt** then_block;
            int then_count;
            struct Stmt** else_block;
            int else_count;
        } if_stmt;
        struct {
            struct Expr* cond;
            struct Stmt** body;
            int body_count;
        } while_stmt;
        struct {
            char* var;
            struct Expr* iter;
            struct Stmt** body;
            int body_count;
        } for_stmt;
        struct {
            struct Stmt** stmts;
            int count;
        } block;
        struct {
            struct Expr* value;
            struct Expr** patterns;
            struct Stmt*** arms;
            int* arm_counts;
            int arm_count;
            char bindings[32][MAX_IDENT_LEN];
        } match_stmt;
        struct {
            struct Stmt** body;
            int body_count;
            char** captured_vars;
            struct Type** captured_types;
            int captured_count;
        } spawn;
    };
};

#endif // COMPILER_TYPES_H