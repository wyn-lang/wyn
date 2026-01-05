#ifndef WYN_TYPES_H
#define WYN_TYPES_H

#include "common.h"

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_VOID,
    TYPE_STRUCT,
    TYPE_ENUM,
    TYPE_FUNCTION,
} TypeKind;

typedef struct Type Type;

typedef struct {
    Token name;
    Type** field_types;
    Token* field_names;
    int field_count;
} StructType;

typedef struct {
    Type** param_types;
    int param_count;
    Type* return_type;
} FunctionType;

struct Type {
    TypeKind kind;
    Token name;
    union {
        StructType struct_type;
        FunctionType fn_type;
    };
};

typedef struct {
    Token name;
    Type* type;
    bool is_mutable;
} Symbol;

typedef struct SymbolTable {
    Symbol* symbols;
    int count;
    int capacity;
    struct SymbolTable* parent;
} SymbolTable;

#endif
