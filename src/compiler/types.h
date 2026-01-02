#ifndef WYN_TYPES_H
#define WYN_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_TOKEN_LEN 256
#define MAX_IDENT_LEN 128
#define MAX_STRING_LEN 4096
#define MAX_PARAMS 32
#define MAX_FIELDS 64
#define MAX_VARIANTS 128
#define MAX_GENERICS 8

// Forward declarations
typedef struct Token Token;
typedef struct Lexer Lexer;
typedef struct Type Type;
typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct FnDef FnDef;
typedef struct StructDef StructDef;
typedef struct EnumDef EnumDef;
typedef struct Module Module;
typedef struct Parser Parser;
typedef struct TypeChecker TypeChecker;

// Token types
typedef enum {
    TOK_EOF, TOK_NEWLINE, TOK_IDENT, TOK_INT, TOK_FLOAT,
    TOK_STRING, TOK_RAW_STRING, TOK_MULTILINE_STRING,
    TOK_FN, TOK_STRUCT, TOK_ENUM, TOK_VAR, TOK_MUT, TOK_CONST, TOK_PUB,
    TOK_IF, TOK_ELSE, TOK_MATCH, TOK_FOR, TOK_WHILE, TOK_IN,
    TOK_BREAK, TOK_CONTINUE, TOK_RETURN, TOK_IMPORT, TOK_FROM, TOK_AS,
    TOK_TRUE, TOK_FALSE, TOK_NONE, TOK_SOME, TOK_OK, TOK_ERR, TOK_SELF,
    TOK_SPAWN, TOK_AND, TOK_OR, TOK_NOT, TOK_TEST, TOK_ASSERT,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT, TOK_STARSTAR,
    TOK_EQEQ, TOK_NOTEQ, TOK_LT, TOK_GT, TOK_LTEQ, TOK_GTEQ,
    TOK_EQ, TOK_PLUSEQ, TOK_MINUSEQ, TOK_STAREQ, TOK_SLASHEQ, TOK_PERCENTEQ,
    TOK_DOTDOT, TOK_DOTDOTEQ, TOK_DOTDOTDOT, TOK_ARROW, TOK_FATARROW,
    TOK_QUESTION, TOK_QUESTIONDOT, TOK_QUESTIONQUESTION, TOK_BANG, TOK_AT,
    TOK_PIPE, TOK_PIPEGT, TOK_AMPERSAND, TOK_CARET, TOK_TILDE, TOK_LTLT, TOK_GTGT,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_LBRACKET, TOK_RBRACKET,
    TOK_COMMA, TOK_COLON, TOK_SEMICOLON, TOK_DOT, TOK_ERROR
} TokenKind;

// Type kinds
typedef enum {
    TYPE_INT, TYPE_FLOAT, TYPE_STR, TYPE_BOOL, TYPE_BYTE, TYPE_ANY, TYPE_VOID,
    TYPE_ARRAY, TYPE_OPTIONAL, TYPE_RESULT, TYPE_FUNCTION, TYPE_STRUCT,
    TYPE_ENUM, TYPE_NAMED, TYPE_GENERIC, TYPE_TRAIT
} TypeKind;

// Expression kinds
typedef enum {
    EXPR_INT, EXPR_FLOAT, EXPR_STRING, EXPR_BOOL, EXPR_IDENT, EXPR_BINARY,
    EXPR_UNARY, EXPR_CALL, EXPR_INDEX, EXPR_FIELD, EXPR_ARRAY, EXPR_STRUCT_INIT,
    EXPR_LAMBDA, EXPR_MATCH, EXPR_IF, EXPR_BLOCK, EXPR_RANGE, EXPR_SLICE,
    EXPR_TERNARY, EXPR_SPAWN, EXPR_COMPREHENSION, EXPR_CAST
} ExprKind;

// Statement kinds
typedef enum {
    STMT_EXPR, STMT_VAR, STMT_ASSIGN, STMT_RETURN, STMT_IF, STMT_WHILE,
    STMT_FOR, STMT_MATCH, STMT_BREAK, STMT_CONTINUE, STMT_BLOCK, STMT_DEFER
} StmtKind;

// Token structure
struct Token {
    TokenKind kind;
    char ident[MAX_IDENT_LEN];
    int64_t int_val;
    double float_val;
    char str_val[MAX_STRING_LEN];
    int line, column;
};

// Lexer structure
struct Lexer {
    const char* source;
    const char* filename;
    size_t pos;
    size_t len;
    int line, column;
    Token current, peek;
    bool has_peek;
};

#endif // WYN_TYPES_H
