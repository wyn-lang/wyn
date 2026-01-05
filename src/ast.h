#ifndef WYN_AST_H
#define WYN_AST_H

#include "common.h"

typedef enum {
    EXPR_INT,
    EXPR_FLOAT,
    EXPR_STRING,
    EXPR_IDENT,
    EXPR_BINARY,
    EXPR_CALL,
    EXPR_METHOD_CALL,
    EXPR_ARRAY,
    EXPR_INDEX,
    EXPR_ASSIGN,
    EXPR_STRUCT_INIT,
    EXPR_FIELD_ACCESS,
    EXPR_BOOL,
    EXPR_UNARY,
    EXPR_MATCH,
    EXPR_SOME,
    EXPR_NONE,
    EXPR_OK,
    EXPR_ERR,
} ExprType;

typedef struct Expr Expr;

typedef struct {
    Expr* left;
    Token op;
    Expr* right;
} BinaryExpr;

typedef struct {
    Expr* callee;
    Expr** args;
    int arg_count;
} CallExpr;

typedef struct {
    Expr* object;
    Token method;
    Expr** args;
    int arg_count;
} MethodCallExpr;

typedef struct {
    Expr** elements;
    int count;
} ArrayExpr;

typedef struct {
    Expr* array;
    Expr* index;
} IndexExpr;

typedef struct {
    Token name;
    Expr* value;
} AssignExpr;

typedef struct {
    Token type_name;
    Token* field_names;
    Expr** field_values;
    int field_count;
} StructInitExpr;

typedef struct {
    Expr* object;
    Token field;
} FieldAccessExpr;

typedef struct {
    Token op;
    Expr* operand;
} UnaryExpr;

typedef struct {
    Token pattern;
    Expr* result;
} MatchArm;

typedef struct {
    Expr* value;
    MatchArm* arms;
    int arm_count;
} MatchExpr;

typedef struct {
    Expr* value;
} OptionExpr;

struct Expr {
    ExprType type;
    Token token;
    union {
        BinaryExpr binary;
        CallExpr call;
        MethodCallExpr method_call;
        ArrayExpr array;
        IndexExpr index;
        AssignExpr assign;
        StructInitExpr struct_init;
        FieldAccessExpr field_access;
        UnaryExpr unary;
        MatchExpr match;
        OptionExpr option;
    };
};

typedef enum {
    STMT_EXPR,
    STMT_VAR,
    STMT_RETURN,
    STMT_BLOCK,
    STMT_FN,
    STMT_STRUCT,
    STMT_IMPL,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_ENUM,
    STMT_TYPE_ALIAS,
    STMT_IMPORT,
} StmtType;

typedef struct Stmt Stmt;

typedef struct {
    Token name;
    Expr* type;
    Expr* init;
    bool is_const;
} VarStmt;

typedef struct {
    Expr* value;
} ReturnStmt;

typedef struct {
    Stmt** stmts;
    int count;
} BlockStmt;

typedef struct {
    Token name;
    Token* params;
    Expr** param_types;
    bool* param_mutable;
    int param_count;
    Token* type_params;
    int type_param_count;
    Expr* return_type;
    Stmt* body;
    bool is_public;
} FnStmt;

typedef struct {
    Token name;
    Token* fields;
    Expr** field_types;
    int field_count;
} StructStmt;

typedef struct {
    Token type_name;
    FnStmt** methods;
    int method_count;
} ImplStmt;

typedef struct {
    Expr* condition;
    Stmt* then_branch;
    Stmt* else_branch;
} IfStmt;

typedef struct {
    Expr* condition;
    Stmt* body;
} WhileStmt;

typedef struct {
    Stmt* init;
    Expr* condition;
    Expr* increment;
    Stmt* body;
} ForStmt;

typedef struct {
    Token name;
    Token* variants;
    int variant_count;
} EnumStmt;

typedef struct {
    Token name;
    Token target;
} TypeAliasStmt;

typedef struct {
    Token module;
    Token* items;
    int item_count;
} ImportStmt;

struct Stmt {
    StmtType type;
    union {
        Expr* expr;
        VarStmt var;
        ReturnStmt ret;
        BlockStmt block;
        FnStmt fn;
        StructStmt struct_decl;
        ImplStmt impl;
        IfStmt if_stmt;
        WhileStmt while_stmt;
        ForStmt for_stmt;
        EnumStmt enum_decl;
        TypeAliasStmt type_alias;
        ImportStmt import;
    };
};

typedef struct {
    Stmt** stmts;
    int count;
} Program;

#endif
