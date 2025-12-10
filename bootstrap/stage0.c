/*
 * Wyn Language Bootstrap Compiler (Stage 0)
 * 
 * This is a minimal C compiler that can compile a subset of Wyn
 * sufficient to build the self-hosting compiler.
 * 
 * Supported subset:
 * - Basic types: int, str, bool
 * - Functions with parameters and return types
 * - Structs and enums
 * - Control flow: if/else, match, for, while
 * - Basic expressions and operators
 * - Simple generics (monomorphization)
 * 
 * Output: x86_64 assembly (AT&T syntax) -> assembled with system assembler
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include "windows_compat.h"

#define MAX_TOKEN_LEN 256
#define MAX_IDENT_LEN 128
#define MAX_STRING_LEN 4096
#define MAX_PARAMS 32
#define MAX_THREADS 1024

// Thread pool for spawn
typedef struct {
    pthread_t threads[MAX_THREADS];
    int count;
} ThreadPool;

static ThreadPool g_thread_pool = {0};
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

// ============================================================================
// Token Types
// ============================================================================

typedef enum {
    TOK_EOF,
    TOK_NEWLINE,
    TOK_IDENT,
    TOK_INT,
    TOK_FLOAT,
    TOK_STRING,
    TOK_RAW_STRING,
    TOK_MULTILINE_STRING,
    TOK_BYTE,
    
    // Keywords
    TOK_FN,
    TOK_STRUCT,
    TOK_ENUM,
    TOK_INTERFACE,
    TOK_IMPL,
    TOK_LET,
    TOK_MUT,
    TOK_CONST,
    TOK_PUB,
    TOK_IF,
    TOK_ELSE,
    TOK_THEN,
    TOK_MATCH,
    TOK_FOR,
    TOK_WHILE,
    TOK_IN,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_RETURN,
    TOK_IMPORT,
    TOK_FROM,
    TOK_AS,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NONE,
    TOK_SOME,
    TOK_OK,
    TOK_ERR,
    TOK_SELF,
    TOK_SELF_TYPE,
    TOK_TRY,
    TOK_CATCH,
    TOK_DEFER,
    TOK_SPAWN,
    TOK_PARALLEL,
    TOK_ASYNC,
    TOK_AWAIT,
    TOK_YIELD,
    TOK_AND,
    TOK_OR,
    TOK_NOT,
    TOK_TEST,
    TOK_BENCH,
    TOK_ASSERT,
    
    // Operators
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_STARSTAR,
    TOK_EQEQ,
    TOK_NOTEQ,
    TOK_LT,
    TOK_GT,
    TOK_LTEQ,
    TOK_GTEQ,
    TOK_EQ,
    TOK_PLUSEQ,
    TOK_MINUSEQ,
    TOK_STAREQ,
    TOK_SLASHEQ,
    TOK_PERCENTEQ,
    TOK_DOTDOT,
    TOK_DOTDOTEQ,
    TOK_DOTDOTDOT,
    TOK_ARROW,
    TOK_FATARROW,
    TOK_QUESTION,
    TOK_QUESTIONDOT,
    TOK_QUESTIONQUESTION,
    TOK_BANG,
    TOK_AT,
    TOK_PIPE,
    TOK_AMPERSAND,
    TOK_CARET,
    TOK_TILDE,
    TOK_LTLT,
    TOK_GTGT,
    
    // Delimiters
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COMMA,
    TOK_COLON,
    TOK_SEMICOLON,
    TOK_DOT,
    
    TOK_ERROR
} TokenKind;

struct Token {
    TokenKind kind;
    int line;
    int column;
    union {
        int64_t int_val;
        double float_val;
        char str_val[MAX_STRING_LEN];
        char ident[MAX_IDENT_LEN];
    };
};

// ============================================================================
// Lexer
// ============================================================================

struct Lexer {
    const char* source;
    const char* filename;
    size_t pos;
    size_t len;
    int line;
    int column;
    Token current;
    Token peek;
    bool has_peek;
};

static const char* keywords[] = {
    "fn", "struct", "enum", "interface", "impl", "let", "mut", "const", "pub",
    "if", "else", "then", "match", "for", "while", "in",
    "break", "continue", "return", "import", "from", "as",
    "true", "false", "none", "some", "ok", "err", "self", "Self",
    "try", "catch", "defer", "spawn", "parallel", "async", "await", "yield",
    "and", "or", "not", "test", "bench", "assert"
};

static const TokenKind keyword_tokens[] = {
    TOK_FN, TOK_STRUCT, TOK_ENUM, TOK_INTERFACE, TOK_IMPL, TOK_LET, TOK_MUT, TOK_CONST, TOK_PUB,
    TOK_IF, TOK_ELSE, TOK_THEN, TOK_MATCH, TOK_FOR, TOK_WHILE, TOK_IN,
    TOK_BREAK, TOK_CONTINUE, TOK_RETURN, TOK_IMPORT, TOK_FROM, TOK_AS,
    TOK_TRUE, TOK_FALSE, TOK_NONE, TOK_SOME, TOK_OK, TOK_ERR, TOK_SELF, TOK_SELF_TYPE,
    TOK_TRY, TOK_CATCH, TOK_DEFER, TOK_SPAWN, TOK_PARALLEL, TOK_ASYNC, TOK_AWAIT, TOK_YIELD,
    TOK_AND, TOK_OR, TOK_NOT, TOK_TEST, TOK_BENCH, TOK_ASSERT
};

#define NUM_KEYWORDS (sizeof(keywords) / sizeof(keywords[0]))

Lexer* lexer_new(const char* source, const char* filename) {
    Lexer* l = malloc(sizeof(Lexer));
    l->source = source;
    l->filename = filename;
    l->pos = 0;
    l->len = strlen(source);
    l->line = 1;
    l->column = 1;
    l->has_peek = false;
    return l;
}

void lexer_free(Lexer* l) {
    free(l);
}

static char lexer_char(Lexer* l) {
    if (l->pos >= l->len) return '\0';
    return l->source[l->pos];
}

static char lexer_peek_char(Lexer* l, int offset) {
    if (l->pos + offset >= l->len) return '\0';
    return l->source[l->pos + offset];
}

static void lexer_advance(Lexer* l) {
    if (l->pos < l->len) {
        if (l->source[l->pos] == '\n') {
            l->line++;
            l->column = 1;
        } else {
            l->column++;
        }
        l->pos++;
    }
}

static void lexer_skip_whitespace(Lexer* l) {
    while (l->pos < l->len) {
        char c = lexer_char(l);
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer_advance(l);
        } else if (c == '#' || (c == '/' && lexer_peek_char(l, 1) == '/')) {
            // Single-line comment (# or //)
            while (l->pos < l->len && lexer_char(l) != '\n') {
                lexer_advance(l);
            }
        } else if (c == '/' && lexer_peek_char(l, 1) == '*') {
            // Multi-line comment
            lexer_advance(l);
            lexer_advance(l);
            while (l->pos < l->len) {
                if (lexer_char(l) == '*' && lexer_peek_char(l, 1) == '/') {
                    lexer_advance(l);
                    lexer_advance(l);
                    break;
                }
                lexer_advance(l);
            }
        } else {
            break;
        }
    }
}

static Token lexer_make_token(Lexer* l, TokenKind kind) {
    Token t;
    t.kind = kind;
    t.line = l->line;
    t.column = l->column;
    return t;
}

static Token lexer_scan_ident(Lexer* l) {
    Token t = lexer_make_token(l, TOK_IDENT);
    int i = 0;
    
    while (l->pos < l->len && (isalnum(lexer_char(l)) || lexer_char(l) == '_')) {
        if (i < MAX_IDENT_LEN - 1) {
            t.ident[i++] = lexer_char(l);
        }
        lexer_advance(l);
    }
    t.ident[i] = '\0';
    
    // Check for keywords
    for (size_t k = 0; k < NUM_KEYWORDS; k++) {
        if (strcmp(t.ident, keywords[k]) == 0) {
            t.kind = keyword_tokens[k];
            break;
        }
    }
    
    return t;
}

static Token lexer_scan_number(Lexer* l) {
    Token t = lexer_make_token(l, TOK_INT);
    char buf[64];
    int i = 0;
    bool is_float = false;
    
    // Check for hex (0x) or binary (0b)
    if (lexer_char(l) == '0') {
        char next = lexer_peek_char(l, 1);
        if (next == 'x' || next == 'X') {
            // Hex literal
            t.kind = TOK_BYTE;
            lexer_advance(l); lexer_advance(l); // skip 0x
            while (l->pos < l->len && (isxdigit(lexer_char(l)) || lexer_char(l) == '_')) {
                if (lexer_char(l) != '_' && i < 63) {
                    buf[i++] = lexer_char(l);
                }
                lexer_advance(l);
            }
            buf[i] = '\0';
            t.int_val = strtoll(buf, NULL, 16);
            return t;
        } else if (next == 'b' || next == 'B') {
            // Binary literal
            t.kind = TOK_BYTE;
            lexer_advance(l); lexer_advance(l); // skip 0b
            while (l->pos < l->len && (lexer_char(l) == '0' || lexer_char(l) == '1' || lexer_char(l) == '_')) {
                if (lexer_char(l) != '_' && i < 63) {
                    buf[i++] = lexer_char(l);
                }
                lexer_advance(l);
            }
            buf[i] = '\0';
            t.int_val = strtoll(buf, NULL, 2);
            return t;
        }
    }
    
    while (l->pos < l->len && (isdigit(lexer_char(l)) || lexer_char(l) == '_')) {
        if (lexer_char(l) != '_' && i < 63) {
            buf[i++] = lexer_char(l);
        }
        lexer_advance(l);
    }
    
    if (lexer_char(l) == '.' && isdigit(lexer_peek_char(l, 1))) {
        is_float = true;
        buf[i++] = '.';
        lexer_advance(l);
        while (l->pos < l->len && (isdigit(lexer_char(l)) || lexer_char(l) == '_')) {
            if (lexer_char(l) != '_' && i < 63) {
                buf[i++] = lexer_char(l);
            }
            lexer_advance(l);
        }
    }
    
    buf[i] = '\0';
    
    if (is_float) {
        t.kind = TOK_FLOAT;
        t.float_val = strtod(buf, NULL);
    } else {
        t.int_val = strtoll(buf, NULL, 10);
    }
    
    return t;
}

static Token lexer_scan_string(Lexer* l) {
    Token t = lexer_make_token(l, TOK_STRING);
    
    // Check for multi-line string """
    if (lexer_peek_char(l, 1) == '"' && lexer_peek_char(l, 2) == '"') {
        t.kind = TOK_MULTILINE_STRING;
        lexer_advance(l); lexer_advance(l); lexer_advance(l); // skip """
        int i = 0;
        while (l->pos < l->len) {
            if (lexer_char(l) == '"' && lexer_peek_char(l, 1) == '"' && lexer_peek_char(l, 2) == '"') {
                lexer_advance(l); lexer_advance(l); lexer_advance(l);
                break;
            }
            if (i < MAX_STRING_LEN - 1) {
                t.str_val[i++] = lexer_char(l);
            }
            lexer_advance(l);
        }
        t.str_val[i] = '\0';
        return t;
    }
    
    lexer_advance(l); // skip opening quote
    int i = 0;
    
    while (l->pos < l->len && lexer_char(l) != '"') {
        if (lexer_char(l) == '\\') {
            lexer_advance(l);
            char c = lexer_char(l);
            switch (c) {
                case 'n': t.str_val[i++] = '\n'; break;
                case 't': t.str_val[i++] = '\t'; break;
                case 'r': t.str_val[i++] = '\r'; break;
                case '\\': t.str_val[i++] = '\\'; break;
                case '"': t.str_val[i++] = '"'; break;
                case '{': t.str_val[i++] = '{'; break;  // escape for interpolation
                default: t.str_val[i++] = c; break;
            }
        } else {
            if (i < MAX_STRING_LEN - 1) {
                t.str_val[i++] = lexer_char(l);
            }
        }
        lexer_advance(l);
    }
    t.str_val[i] = '\0';
    lexer_advance(l); // skip closing quote
    
    return t;
}

static Token lexer_scan_raw_string(Lexer* l) {
    Token t = lexer_make_token(l, TOK_RAW_STRING);
    lexer_advance(l); // skip 'r'
    lexer_advance(l); // skip opening quote
    int i = 0;
    
    while (l->pos < l->len && lexer_char(l) != '"') {
        if (i < MAX_STRING_LEN - 1) {
            t.str_val[i++] = lexer_char(l);
        }
        lexer_advance(l);
    }
    t.str_val[i] = '\0';
    lexer_advance(l); // skip closing quote
    
    return t;
}

Token lexer_next(Lexer* l) {
    if (l->has_peek) {
        l->has_peek = false;
        l->current = l->peek;
        return l->current;
    }
    
    lexer_skip_whitespace(l);
    
    if (l->pos >= l->len) {
        l->current = lexer_make_token(l, TOK_EOF);
        return l->current;
    }
    
    char c = lexer_char(l);
    
    // Newline
    if (c == '\n') {
        Token t = lexer_make_token(l, TOK_NEWLINE);
        lexer_advance(l);
        l->current = t;
        return t;
    }
    
    // Identifiers and keywords (and raw strings r"...")
    if (isalpha(c) || c == '_') {
        // Check for raw string r"..."
        if (c == 'r' && lexer_peek_char(l, 1) == '"') {
            l->current = lexer_scan_raw_string(l);
            return l->current;
        }
        l->current = lexer_scan_ident(l);
        return l->current;
    }
    
    // Numbers
    if (isdigit(c)) {
        l->current = lexer_scan_number(l);
        return l->current;
    }
    
    // Strings
    if (c == '"') {
        l->current = lexer_scan_string(l);
        return l->current;
    }
    
    // Operators and delimiters
    Token t = lexer_make_token(l, TOK_ERROR);
    
    switch (c) {
        case '+':
            lexer_advance(l);
            if (lexer_char(l) == '=') { lexer_advance(l); t.kind = TOK_PLUSEQ; }
            else { t.kind = TOK_PLUS; }
            break;
        case '-':
            lexer_advance(l);
            if (lexer_char(l) == '>') { lexer_advance(l); t.kind = TOK_ARROW; }
            else if (lexer_char(l) == '=') { lexer_advance(l); t.kind = TOK_MINUSEQ; }
            else { t.kind = TOK_MINUS; }
            break;
        case '*':
            lexer_advance(l);
            if (lexer_char(l) == '*') { lexer_advance(l); t.kind = TOK_STARSTAR; }
            else if (lexer_char(l) == '=') { lexer_advance(l); t.kind = TOK_STAREQ; }
            else { t.kind = TOK_STAR; }
            break;
        case '/':
            lexer_advance(l);
            if (lexer_char(l) == '=') { lexer_advance(l); t.kind = TOK_SLASHEQ; }
            else { t.kind = TOK_SLASH; }
            break;
        case '%':
            lexer_advance(l);
            if (lexer_char(l) == '=') { lexer_advance(l); t.kind = TOK_PERCENTEQ; }
            else { t.kind = TOK_PERCENT; }
            break;
        case '=':
            lexer_advance(l);
            if (lexer_char(l) == '=') { lexer_advance(l); t.kind = TOK_EQEQ; }
            else if (lexer_char(l) == '>') { lexer_advance(l); t.kind = TOK_FATARROW; }
            else { t.kind = TOK_EQ; }
            break;
        case '!':
            lexer_advance(l);
            if (lexer_char(l) == '=') { lexer_advance(l); t.kind = TOK_NOTEQ; }
            else { t.kind = TOK_BANG; }
            break;
        case '<':
            lexer_advance(l);
            if (lexer_char(l) == '=') { lexer_advance(l); t.kind = TOK_LTEQ; }
            else if (lexer_char(l) == '<') { lexer_advance(l); t.kind = TOK_LTLT; }
            else { t.kind = TOK_LT; }
            break;
        case '>':
            lexer_advance(l);
            if (lexer_char(l) == '=') { lexer_advance(l); t.kind = TOK_GTEQ; }
            else if (lexer_char(l) == '>') { lexer_advance(l); t.kind = TOK_GTGT; }
            else { t.kind = TOK_GT; }
            break;
        case '&': lexer_advance(l); t.kind = TOK_AMPERSAND; break;
        case '|': lexer_advance(l); t.kind = TOK_PIPE; break;
        case '^': lexer_advance(l); t.kind = TOK_CARET; break;
        case '~': lexer_advance(l); t.kind = TOK_TILDE; break;
        case '.':
            lexer_advance(l);
            if (lexer_char(l) == '.') {
                lexer_advance(l);
                if (lexer_char(l) == '.') { lexer_advance(l); t.kind = TOK_DOTDOTDOT; }
                else if (lexer_char(l) == '=') { lexer_advance(l); t.kind = TOK_DOTDOTEQ; }
                else { t.kind = TOK_DOTDOT; }
            } else { t.kind = TOK_DOT; }
            break;
        case '?':
            lexer_advance(l);
            if (lexer_char(l) == '?') { lexer_advance(l); t.kind = TOK_QUESTIONQUESTION; }
            else if (lexer_char(l) == '.') { lexer_advance(l); t.kind = TOK_QUESTIONDOT; }
            else { t.kind = TOK_QUESTION; }
            break;
        case '@': lexer_advance(l); t.kind = TOK_AT; break;
        case '(': lexer_advance(l); t.kind = TOK_LPAREN; break;
        case ')': lexer_advance(l); t.kind = TOK_RPAREN; break;
        case '{': lexer_advance(l); t.kind = TOK_LBRACE; break;
        case '}': lexer_advance(l); t.kind = TOK_RBRACE; break;
        case '[': lexer_advance(l); t.kind = TOK_LBRACKET; break;
        case ']': lexer_advance(l); t.kind = TOK_RBRACKET; break;
        case ',': lexer_advance(l); t.kind = TOK_COMMA; break;
        case ':': lexer_advance(l); t.kind = TOK_COLON; break;
        case ';': lexer_advance(l); t.kind = TOK_SEMICOLON; break;
        default:
            lexer_advance(l);
            t.kind = TOK_ERROR;
            break;
    }
    
    l->current = t;
    return t;
}

Token lexer_peek(Lexer* l) {
    if (l->has_peek) {
        return l->peek;
    }
    Token current = l->current;
    l->peek = lexer_next(l);
    l->current = current;
    l->has_peek = true;
    return l->peek;
}

// ============================================================================
// AST Node Types
// ============================================================================

typedef enum {
    TYPE_INT, TYPE_FLOAT, TYPE_STR, TYPE_BOOL, TYPE_BYTE, TYPE_ANY, TYPE_VOID,
    TYPE_ARRAY, TYPE_MAP, TYPE_TUPLE, TYPE_OPTIONAL, TYPE_RESULT,
    TYPE_FUNCTION, TYPE_NAMED, TYPE_GENERIC, TYPE_POINTER
} TypeKind;

struct Type {
    TypeKind kind;
    char name[MAX_IDENT_LEN];
    Type* inner;          // for array, optional
    Type* key_type;       // for map
    Type* value_type;     // for map, result
    Type** params;        // for tuple, function params
    int param_count;
    Type* return_type;    // for function
};

typedef enum {
    EXPR_INT, EXPR_FLOAT, EXPR_STRING, EXPR_BOOL, EXPR_IDENT,
    EXPR_BINARY, EXPR_UNARY, EXPR_CALL, EXPR_INDEX, EXPR_FIELD,
    EXPR_ARRAY, EXPR_MAP, EXPR_TUPLE, EXPR_LAMBDA, EXPR_STRUCT,
    EXPR_IF, EXPR_MATCH, EXPR_SOME, EXPR_NONE, EXPR_OK, EXPR_ERR,
    EXPR_UNWRAP, EXPR_TRY, EXPR_DEFAULT, EXPR_SLICE, EXPR_ADDR, EXPR_DEREF, EXPR_AWAIT
} ExprKind;

struct Expr {
    ExprKind kind;
    int line, column;
    union {
        int64_t int_val;
        double float_val;
        char str_val[MAX_STRING_LEN];
        bool bool_val;
        char ident[MAX_IDENT_LEN];
        struct { Expr* left; Expr* right; TokenKind op; } binary;
        struct { Expr* operand; TokenKind op; } unary;
        struct { Expr* func; Expr** args; int arg_count; Type** type_args; int type_arg_count; } call;
        struct { Expr* object; Expr* index; bool safe; } index;
        struct { Expr* object; char field[MAX_IDENT_LEN]; } field;
        struct { Expr** elements; int count; } array;
        struct { char** keys; Expr** values; int count; } map;
        struct { char name[MAX_IDENT_LEN]; char fields[MAX_FIELDS][MAX_IDENT_LEN]; Expr* values[MAX_FIELDS]; int count; Type** type_args; int type_arg_count; } struct_lit;
        struct { Expr* cond; Expr* then_expr; Expr* else_expr; } if_expr;
        struct { Expr* value; Expr** patterns; Expr** arms; int arm_count; char bindings[64][MAX_IDENT_LEN]; Type** binding_types; } match_expr;
        struct { Expr* value; } some;  // also used for ok/err
        struct { Expr* value; Expr* default_val; } default_expr;
        struct { Expr* object; Expr* start; Expr* end; Expr* step; } slice;
        struct { char params[8][MAX_IDENT_LEN]; Type* param_types[8]; int param_count; Type* return_type; Stmt** body; int body_count; int id; } lambda;
        struct { Expr* future; } await_expr;
    };
};

typedef enum {
    STMT_LET, STMT_ASSIGN, STMT_EXPR, STMT_RETURN, STMT_IF, STMT_WHILE,
    STMT_FOR, STMT_MATCH, STMT_BLOCK, STMT_BREAK, STMT_CONTINUE, STMT_DEFER, STMT_SPAWN
} StmtKind;

struct Stmt {
    StmtKind kind;
    int line, column;
    union {
        struct { char name[MAX_IDENT_LEN]; Type* type; Expr* value; bool is_mut; bool is_pub; } let;
        struct { Expr* target; Expr* value; TokenKind op; } assign;
        struct { Expr* expr; } expr;
        struct { Expr* value; } ret;
        struct { Expr* cond; Stmt** then_block; int then_count; Stmt** else_block; int else_count; } if_stmt;
        struct { Expr* cond; Stmt** body; int body_count; } while_stmt;
        struct { char var[MAX_IDENT_LEN]; Expr* iter; Stmt** body; int body_count; bool is_parallel; } for_stmt;
        struct { Expr* value; Expr** patterns; Stmt*** arms; int* arm_counts; int arm_count; char bindings[64][MAX_IDENT_LEN]; Type** binding_types; } match_stmt;
        struct { Stmt** stmts; int count; } block;
        struct { Expr* expr; } defer;
        struct { Stmt** body; int body_count; char** captured_vars; int captured_count; } spawn;
    };
};

typedef struct {
    char name[MAX_IDENT_LEN];
    Type* type;
    Expr* default_val;
    bool is_variadic;
} Param;

struct FnDef {
    char name[MAX_IDENT_LEN];
    Param params[MAX_PARAMS];
    int param_count;
    Type* return_type;
    Stmt** body;
    int body_count;
    bool is_pub;
    bool is_async;
    char generics[MAX_GENERICS][MAX_IDENT_LEN];
    int generic_count;
};

typedef struct {
    char name[MAX_IDENT_LEN];
    Type* type;
    bool is_pub;
} Field;

struct StructDef {
    char name[MAX_IDENT_LEN];
    Field fields[MAX_FIELDS];
    int field_count;
    FnDef* methods;
    int method_count;
    bool is_pub;
    char generics[MAX_GENERICS][MAX_IDENT_LEN];
    int generic_count;
};

typedef struct {
    char name[MAX_IDENT_LEN];
    Type** types;
    int type_count;
    int64_t value;
    bool has_value;
} Variant;

struct EnumDef {
    char name[MAX_IDENT_LEN];
    Variant variants[MAX_VARIANTS];
    int variant_count;
    bool is_pub;
    char generics[MAX_GENERICS][MAX_IDENT_LEN];
    int generic_count;
};

typedef struct {
    char name[MAX_IDENT_LEN];
    FnDef* methods;
    int method_count;
    bool is_pub;
} InterfaceDef;

typedef struct {
    char interface_name[MAX_IDENT_LEN];
    char type_name[MAX_IDENT_LEN];
    FnDef* methods;
    int method_count;
} ImplDef;

typedef struct {
    char path[MAX_STRING_LEN];
    char alias[MAX_IDENT_LEN];
    char** imports;
    int import_count;
} Import;

typedef struct {
    char name[MAX_IDENT_LEN];
    Stmt** body;
    int body_count;
} TestDef;

struct Module {
    char name[MAX_IDENT_LEN];
    Import* imports;
    int import_count;
    FnDef* functions;
    int function_count;
    StructDef* structs;
    int struct_count;
    EnumDef* enums;
    int enum_count;
    InterfaceDef* interfaces;
    int interface_count;
    ImplDef* impls;
    int impl_count;
    TestDef* tests;
    int test_count;
};

// ============================================================================
// Parser
// ============================================================================

typedef struct {
    Lexer* lexer;
    Token current;
    const char* filename;
    bool had_error;
    int lambda_count;
    bool newline_before;  // True if newline was skipped before current token
} Parser;

static Parser* parser_new(Lexer* lexer, const char* filename) {
    Parser* p = malloc(sizeof(Parser));
    p->lexer = lexer;
    p->filename = filename;
    p->had_error = false;
    p->lambda_count = 0;
    p->newline_before = false;
    p->current = lexer_next(lexer);
    return p;
}

static void parser_free(Parser* p) { free(p); }

// Helper: Map module.function() to builtin function name
// Returns the builtin name if it's a module call, NULL otherwise
static const char* map_module_function(const char* module, const char* function) {
    // Builtin mappings - but type checker enforces imports are required
    if (strcmp(module, "string") == 0) {
        if (strcmp(function, "split") == 0) return "str_split";
        if (strcmp(function, "find") == 0) return "str_find";
        if (strcmp(function, "concat") == 0) return "str_concat";
        if (strcmp(function, "cmp") == 0) return "str_cmp";
        if (strcmp(function, "substring") == 0) return "substring";
        if (strcmp(function, "char_at") == 0) return "char_at";
        if (strcmp(function, "chr") == 0) return "chr";
        if (strcmp(function, "ord") == 0) return "ord";
        if (strcmp(function, "int_to_str") == 0) return "int_to_str";
        if (strcmp(function, "from_int") == 0) return "int_to_str";
    } else if (strcmp(module, "time") == 0) {
        if (strcmp(function, "sleep_ms") == 0) return "sleep_ms";
        if (strcmp(function, "now") == 0) return "time_now";
        if (strcmp(function, "clock") == 0) return "clock";
    } else if (strcmp(module, "os") == 0) {
        if (strcmp(function, "getenv") == 0) return "getenv";
        if (strcmp(function, "getpid") == 0) return "getpid";
        if (strcmp(function, "getcwd") == 0) return "getcwd";
        if (strcmp(function, "exit") == 0) return "exit";
        if (strcmp(function, "system") == 0) return "system";
        if (strcmp(function, "chdir") == 0) return "chdir";
        if (strcmp(function, "args") == 0) return "args";
        if (strcmp(function, "args") == 0) return "args";
    } else if (strcmp(module, "io") == 0) {
        if (strcmp(function, "print") == 0) return "print";
        if (strcmp(function, "println") == 0) return "println";
        if (strcmp(function, "print_str") == 0) return "print_str";
        if (strcmp(function, "print_int") == 0) return "print_int";
        if (strcmp(function, "print_float") == 0) return "print_float";
        if (strcmp(function, "print_newline") == 0) return "print_newline";
        if (strcmp(function, "read_line") == 0) return "read_line";
        if (strcmp(function, "input") == 0) return "input";
        // File operations
        if (strcmp(function, "read_file") == 0) return "read_file";
        if (strcmp(function, "write_file") == 0) return "write_file";
        if (strcmp(function, "append_file") == 0) return "append_file";
        if (strcmp(function, "delete_file") == 0) return "delete_file";
        if (strcmp(function, "file_exists") == 0) return "file_exists";
        if (strcmp(function, "file_size") == 0) return "file_size";
        if (strcmp(function, "is_dir") == 0) return "is_dir";
        if (strcmp(function, "list_dir") == 0) return "list_dir";
        if (strcmp(function, "mkdir") == 0) return "mkdir";
        if (strcmp(function, "rmdir") == 0) return "rmdir";
    } else if (strcmp(module, "fs") == 0) {
        if (strcmp(function, "read_file") == 0) return "read_file";
        if (strcmp(function, "write_file") == 0) return "write_file";
        if (strcmp(function, "append_file") == 0) return "append_file";
        if (strcmp(function, "delete_file") == 0) return "delete_file";
        if (strcmp(function, "file_exists") == 0) return "file_exists";
        if (strcmp(function, "file_size") == 0) return "file_size";
        if (strcmp(function, "is_dir") == 0) return "is_dir";
        if (strcmp(function, "list_dir") == 0) return "list_dir";
        if (strcmp(function, "mkdir") == 0) return "mkdir";
        if (strcmp(function, "rmdir") == 0) return "rmdir";
    } else if (strcmp(module, "math") == 0) {
        if (strcmp(function, "abs") == 0) return "abs";
        if (strcmp(function, "min") == 0) return "min";
        if (strcmp(function, "max") == 0) return "max";
        if (strcmp(function, "pow") == 0) return "pow";
        if (strcmp(function, "sqrt") == 0) return "sqrt";
        if (strcmp(function, "gcd") == 0) return "gcd";
        if (strcmp(function, "lcm") == 0) return "lcm";
        if (strcmp(function, "factorial") == 0) return "factorial";
        if (strcmp(function, "sin") == 0) return "sinf";
        if (strcmp(function, "cos") == 0) return "cosf";
        if (strcmp(function, "tan") == 0) return "tanf";
        if (strcmp(function, "asin") == 0) return "asinf";
        if (strcmp(function, "acos") == 0) return "acosf";
        if (strcmp(function, "atan") == 0) return "atanf";
        if (strcmp(function, "atan2") == 0) return "atan2f";
        if (strcmp(function, "floor") == 0) return "floorf";
        if (strcmp(function, "ceil") == 0) return "ceilf";
        if (strcmp(function, "round") == 0) return "roundf";
        if (strcmp(function, "log") == 0) return "logf";
        if (strcmp(function, "log10") == 0) return "log10f";
        if (strcmp(function, "exp") == 0) return "expf";
    } else if (strcmp(module, "test") == 0) {
        if (strcmp(function, "assert") == 0) return "assert";
    } else if (strcmp(module, "gui") == 0) {
        if (strcmp(function, "create_window") == 0) return "gui_create_window";
        if (strcmp(function, "show_window") == 0) return "gui_show_window";
        if (strcmp(function, "set_title") == 0) return "gui_set_title";
        if (strcmp(function, "run") == 0) return "gui_run";
        if (strcmp(function, "poll_events") == 0) return "gui_poll_events";
        if (strcmp(function, "close_window") == 0) return "gui_close_window";
        if (strcmp(function, "draw_rect") == 0) return "gui_draw_rect";
        if (strcmp(function, "draw_text") == 0) return "gui_draw_text";
    } else if (strcmp(module, "mobile") == 0) {
        if (strcmp(function, "init") == 0) return "mobile_init";
        if (strcmp(function, "run") == 0) return "mobile_run";
        if (strcmp(function, "create_view") == 0) return "mobile_create_view";
        if (strcmp(function, "add_subview") == 0) return "mobile_add_subview";
        if (strcmp(function, "add_to_root") == 0) return "mobile_add_to_root";
        if (strcmp(function, "create_button") == 0) return "mobile_create_button";
        if (strcmp(function, "create_label") == 0) return "mobile_create_label";
        if (strcmp(function, "create_textfield") == 0) return "mobile_create_textfield";
        if (strcmp(function, "set_text") == 0) return "mobile_set_text";
        if (strcmp(function, "get_text") == 0) return "mobile_get_text";
        if (strcmp(function, "set_frame") == 0) return "mobile_set_frame";
        if (strcmp(function, "set_background_color") == 0) return "mobile_set_background_color";
        if (strcmp(function, "set_text_color") == 0) return "mobile_set_text_color";
        if (strcmp(function, "set_font_size") == 0) return "mobile_set_font_size";
        if (strcmp(function, "on_tap") == 0) return "mobile_on_tap";
        if (strcmp(function, "on_text_change") == 0) return "mobile_on_text_change";
        if (strcmp(function, "show_alert") == 0) return "mobile_show_alert";
        if (strcmp(function, "hide_view") == 0) return "mobile_hide_view";
        if (strcmp(function, "show_view") == 0) return "mobile_show_view";
        if (strcmp(function, "remove_view") == 0) return "mobile_remove_view";
    } else if (strcmp(module, "gpu") == 0) {
        if (strcmp(function, "device_count") == 0) return "__wyn_gpu_device_count";
        if (strcmp(function, "device_name") == 0) return "__wyn_gpu_device_name";
        if (strcmp(function, "malloc") == 0) return "__wyn_gpu_malloc";
        if (strcmp(function, "free") == 0) return "__wyn_gpu_free";
        if (strcmp(function, "sync") == 0) return "__wyn_gpu_sync";
    }
    return NULL;
}

// Helper: Check if a call expression is calling a specific builtin
// Handles both direct calls (func()) and module calls (module.func())
static bool is_builtin_call(Expr* call_func, const char* builtin_name) {
    if (call_func->kind == EXPR_IDENT) {
        return strcmp(call_func->ident, builtin_name) == 0;
    }
    if (call_func->kind == EXPR_FIELD && call_func->field.object->kind == EXPR_IDENT) {
        const char* module = call_func->field.object->ident;
        const char* function = call_func->field.field;
        const char* mapped = map_module_function(module, function);
        return mapped && strcmp(mapped, builtin_name) == 0;
    }
    return false;
}

static void parser_error(Parser* p, const char* fmt, ...) {
    p->had_error = true;
    fprintf(stderr, "%s:%d:%d: error: ", p->filename, p->current.line, p->current.column);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

static Token parser_advance(Parser* p) {
    Token prev = p->current;
    p->current = lexer_next(p->lexer);
    // Track and skip newlines
    p->newline_before = false;
    while (p->current.kind == TOK_NEWLINE) {
        p->newline_before = true;
        p->current = lexer_next(p->lexer);
    }
    return prev;
}

static bool parser_check(Parser* p, TokenKind kind) {
    return p->current.kind == kind;
}

static bool parser_match(Parser* p, TokenKind kind) {
    if (parser_check(p, kind)) {
        parser_advance(p);
        return true;
    }
    return false;
}

static Token parser_expect(Parser* p, TokenKind kind, const char* msg) {
    if (p->current.kind == kind) {
        return parser_advance(p);
    }
    parser_error(p, "expected %s", msg);
    return p->current;
}

// Forward declarations
static Expr* parse_expr(Parser* p);
static Stmt* parse_stmt(Parser* p);
static Type* parse_type(Parser* p);

// Memory allocation helpers
static Expr* new_expr(ExprKind kind, int line, int col) {
    Expr* e = calloc(1, sizeof(Expr));
    e->kind = kind;
    e->line = line;
    e->column = col;
    return e;
}

static Stmt* new_stmt(StmtKind kind, int line, int col) {
    Stmt* s = calloc(1, sizeof(Stmt));
    s->kind = kind;
    s->line = line;
    s->column = col;
    return s;
}

static Type* new_type(TypeKind kind) {
    Type* t = calloc(1, sizeof(Type));
    t->kind = kind;
    return t;
}

// Type parsing
static Type* parse_type(Parser* p) {
    Type* t = NULL;
    
    // Pointer type: *Type
    if (parser_match(p, TOK_STAR)) {
        t = new_type(TYPE_POINTER);
        t->inner = parse_type(p);
        return t;
    }
    
    // Optional type: ?Type
    if (parser_match(p, TOK_QUESTION)) {
        t = new_type(TYPE_OPTIONAL);
        t->inner = parse_type(p);
        return t;
    }
    
    // Array type: [Type]
    if (parser_match(p, TOK_LBRACKET)) {
        t = new_type(TYPE_ARRAY);
        t->inner = parse_type(p);
        parser_expect(p, TOK_RBRACKET, "]");
        return t;
    }
    
    // Map type: {KeyType: ValueType}
    if (parser_match(p, TOK_LBRACE)) {
        t = new_type(TYPE_MAP);
        t->key_type = parse_type(p);
        parser_expect(p, TOK_COLON, ":");
        t->value_type = parse_type(p);
        parser_expect(p, TOK_RBRACE, "}");
        return t;
    }
    
    // Tuple type: (Type, Type, ...)
    if (parser_match(p, TOK_LPAREN)) {
        t = new_type(TYPE_TUPLE);
        t->params = malloc(sizeof(Type*) * 16);
        t->param_count = 0;
        if (!parser_check(p, TOK_RPAREN)) {
            do {
                t->params[t->param_count++] = parse_type(p);
            } while (parser_match(p, TOK_COMMA));
        }
        parser_expect(p, TOK_RPAREN, ")");
        return t;
    }
    
    // Function type: fn(Type, Type) -> Type
    if (parser_match(p, TOK_FN)) {
        t = new_type(TYPE_FUNCTION);
        parser_expect(p, TOK_LPAREN, "(");
        t->params = malloc(sizeof(Type*) * MAX_PARAMS);
        t->param_count = 0;
        if (!parser_check(p, TOK_RPAREN)) {
            do {
                t->params[t->param_count++] = parse_type(p);
            } while (parser_match(p, TOK_COMMA));
        }
        parser_expect(p, TOK_RPAREN, ")");
        if (parser_match(p, TOK_ARROW)) {
            t->return_type = parse_type(p);
        }
        return t;
    }
    
    // Named type (including built-ins and generics)
    if (parser_check(p, TOK_IDENT)) {
        Token name = parser_advance(p);
        
        // Check built-in types
        if (strcmp(name.ident, "int") == 0) return new_type(TYPE_INT);
        if (strcmp(name.ident, "float") == 0) return new_type(TYPE_FLOAT);
        if (strcmp(name.ident, "str") == 0) return new_type(TYPE_STR);
        if (strcmp(name.ident, "bool") == 0) return new_type(TYPE_BOOL);
        if (strcmp(name.ident, "byte") == 0) return new_type(TYPE_BYTE);
        if (strcmp(name.ident, "any") == 0) return new_type(TYPE_ANY);
        
        // Handle Result[T, E] specially
        if (strcmp(name.ident, "Result") == 0 && parser_match(p, TOK_LBRACKET)) {
            t = new_type(TYPE_RESULT);
            t->inner = parse_type(p);  // ok type
            parser_expect(p, TOK_COMMA, ",");
            t->value_type = parse_type(p);  // error type
            parser_expect(p, TOK_RBRACKET, "]");
            return t;
        }
        
        t = new_type(TYPE_NAMED);
        strcpy(t->name, name.ident);
        
        // Generic parameters: Type[T, U]
        if (parser_match(p, TOK_LBRACKET)) {
            t->kind = TYPE_GENERIC;
            t->params = malloc(sizeof(Type*) * MAX_GENERICS);
            t->param_count = 0;
            do {
                t->params[t->param_count++] = parse_type(p);
            } while (parser_match(p, TOK_COMMA));
            parser_expect(p, TOK_RBRACKET, "]");
        }
        return t;
    }
    
    parser_error(p, "expected type");
    return new_type(TYPE_ANY);
}

// Helper to check if string has interpolation {expr}
static bool has_interpolation(const char* s) {
    for (int i = 0; s[i]; i++) {
        if (s[i] == '{' && s[i+1] && s[i+1] != '{') return true;
    }
    return false;
}

// Parse interpolated string into concatenation expression
static Expr* parse_interpolated_string(Parser* p, const char* str, int line, int col) {
    Expr* result = NULL;
    char buf[MAX_STRING_LEN];
    int bi = 0;
    
    for (int i = 0; str[i]; i++) {
        if (str[i] == '{' && str[i+1] && str[i+1] != '{') {
            // Flush literal part
            if (bi > 0) {
                buf[bi] = '\0';
                Expr* lit = new_expr(EXPR_STRING, line, col);
                strcpy(lit->str_val, buf);
                if (!result) result = lit;
                else {
                    Expr* cat = new_expr(EXPR_BINARY, line, col);
                    cat->binary.op = TOK_PLUS;
                    cat->binary.left = result;
                    cat->binary.right = lit;
                    result = cat;
                }
                bi = 0;
            }
            // Extract variable name
            i++;
            char var[256];
            int vi = 0;
            while (str[i] && str[i] != '}') {
                var[vi++] = str[i++];
            }
            var[vi] = '\0';
            // Create to_string(var) call
            Expr* varexpr = new_expr(EXPR_IDENT, line, col);
            strcpy(varexpr->ident, var);
            Expr* call = new_expr(EXPR_CALL, line, col);
            Expr* fn = new_expr(EXPR_IDENT, line, col);
            strcpy(fn->ident, "to_string");
            call->call.func = fn;
            call->call.args = malloc(sizeof(Expr*));
            call->call.args[0] = varexpr;
            call->call.arg_count = 1;
            if (!result) result = call;
            else {
                Expr* cat = new_expr(EXPR_BINARY, line, col);
                cat->binary.op = TOK_PLUS;
                cat->binary.left = result;
                cat->binary.right = call;
                result = cat;
            }
        } else {
            buf[bi++] = str[i];
        }
    }
    // Flush remaining literal
    if (bi > 0) {
        buf[bi] = '\0';
        Expr* lit = new_expr(EXPR_STRING, line, col);
        strcpy(lit->str_val, buf);
        if (!result) result = lit;
        else {
            Expr* cat = new_expr(EXPR_BINARY, line, col);
            cat->binary.op = TOK_PLUS;
            cat->binary.left = result;
            cat->binary.right = lit;
            result = cat;
        }
    }
    return result ? result : new_expr(EXPR_STRING, line, col);
}

// Expression parsing - precedence climbing
static Expr* parse_primary(Parser* p) {
    int line = p->current.line, col = p->current.column;
    
    // Literals
    if (parser_check(p, TOK_INT)) {
        Expr* e = new_expr(EXPR_INT, line, col);
        e->int_val = p->current.int_val;
        parser_advance(p);
        return e;
    }
    if (parser_check(p, TOK_FLOAT)) {
        Expr* e = new_expr(EXPR_FLOAT, line, col);
        e->float_val = p->current.float_val;
        parser_advance(p);
        return e;
    }
    if (parser_check(p, TOK_STRING) || parser_check(p, TOK_RAW_STRING) || parser_check(p, TOK_MULTILINE_STRING)) {
        char str[MAX_STRING_LEN];
        strcpy(str, p->current.str_val);
        parser_advance(p);
        if (has_interpolation(str)) {
            return parse_interpolated_string(p, str, line, col);
        }
        Expr* e = new_expr(EXPR_STRING, line, col);
        strcpy(e->str_val, str);
        return e;
    }
    if (parser_match(p, TOK_TRUE)) {
        Expr* e = new_expr(EXPR_BOOL, line, col);
        e->bool_val = true;
        return e;
    }
    if (parser_match(p, TOK_FALSE)) {
        Expr* e = new_expr(EXPR_BOOL, line, col);
        e->bool_val = false;
        return e;
    }
    if (parser_match(p, TOK_NONE)) {
        return new_expr(EXPR_NONE, line, col);
    }
    if (parser_match(p, TOK_SOME)) {
        Expr* e = new_expr(EXPR_SOME, line, col);
        parser_expect(p, TOK_LPAREN, "(");
        e->some.value = parse_expr(p);
        parser_expect(p, TOK_RPAREN, ")");
        return e;
    }
    if (parser_match(p, TOK_OK)) {
        Expr* e = new_expr(EXPR_OK, line, col);
        parser_expect(p, TOK_LPAREN, "(");
        e->some.value = parse_expr(p);
        parser_expect(p, TOK_RPAREN, ")");
        return e;
    }
    if (parser_match(p, TOK_ERR)) {
        Expr* e = new_expr(EXPR_ERR, line, col);
        parser_expect(p, TOK_LPAREN, "(");
        e->some.value = parse_expr(p);
        parser_expect(p, TOK_RPAREN, ")");
        return e;
    }
    
    // Identifier (including built-in functions like assert)
    if (parser_check(p, TOK_IDENT) || parser_check(p, TOK_SELF) || 
        parser_check(p, TOK_ASSERT)) {
        char name[MAX_IDENT_LEN];
        if (parser_check(p, TOK_ASSERT)) {
            strcpy(name, "assert");
        } else {
            strcpy(name, p->current.ident);
        }
        parser_advance(p);
        
        // Parse optional type arguments: Name[T, U]
        Type** type_args = NULL;
        int type_arg_count = 0;
        if (parser_check(p, TOK_LBRACKET) && name[0] >= 'A' && name[0] <= 'Z') {
            parser_advance(p);
            type_args = malloc(sizeof(Type*) * MAX_GENERICS);
            while (!parser_check(p, TOK_RBRACKET) && !parser_check(p, TOK_EOF)) {
                type_args[type_arg_count++] = parse_type(p);
                if (!parser_match(p, TOK_COMMA)) break;
            }
            parser_expect(p, TOK_RBRACKET, "]");
        }
        
        // Check for struct literal: Name { field: value, ... } or Name[T] { ... }
        // Only if name starts with uppercase (struct naming convention)
        // AND the next tokens look like a struct literal (ident: or })
        if (parser_check(p, TOK_LBRACE) && name[0] >= 'A' && name[0] <= 'Z') {
            // Save lexer state to peek ahead
            int saved_pos = p->lexer->pos;
            int saved_line = p->lexer->line;
            int saved_col = p->lexer->column;
            Token saved_current = p->current;
            
            parser_advance(p);  // consume {
            Token next = p->current;
            bool is_struct_lit = (next.kind == TOK_RBRACE) ||  // empty struct
                                 (next.kind == TOK_IDENT);
            if (is_struct_lit && next.kind == TOK_IDENT) {
                parser_advance(p);  // consume ident
                // Field shorthand: {field} or {field, ...} or {field: value}
                is_struct_lit = (p->current.kind == TOK_COLON || 
                                 p->current.kind == TOK_COMMA ||
                                 p->current.kind == TOK_RBRACE);
            }
            
            // Restore lexer state
            p->lexer->pos = saved_pos;
            p->lexer->line = saved_line;
            p->lexer->column = saved_col;
            p->current = saved_current;
            
            if (is_struct_lit) {
                parser_advance(p);  // consume { again
                Expr* e = new_expr(EXPR_STRUCT, line, col);
                strcpy(e->struct_lit.name, name);
                e->struct_lit.count = 0;
                e->struct_lit.type_args = type_args;
                e->struct_lit.type_arg_count = type_arg_count;
                if (!parser_check(p, TOK_RBRACE)) {
                    do {
                        Token fld = parser_expect(p, TOK_IDENT, "field name");
                        strcpy(e->struct_lit.fields[e->struct_lit.count], fld.ident);
                        if (parser_match(p, TOK_COLON)) {
                            e->struct_lit.values[e->struct_lit.count] = parse_expr(p);
                        } else {
                            // Field shorthand: {field} means {field: field}
                            Expr* ident = new_expr(EXPR_IDENT, fld.line, fld.column);
                            strcpy(ident->ident, fld.ident);
                            e->struct_lit.values[e->struct_lit.count] = ident;
                        }
                        e->struct_lit.count++;
                    } while (parser_match(p, TOK_COMMA) && !parser_check(p, TOK_RBRACE));
                }
                parser_expect(p, TOK_RBRACE, "}");
                return e;
            }
        }
        
        Expr* e = new_expr(EXPR_IDENT, line, col);
        strcpy(e->ident, name);
        return e;
    }
    
    // Parenthesized expr or tuple or unit type
    if (parser_match(p, TOK_LPAREN)) {
        // Check for unit type ()
        if (parser_check(p, TOK_RPAREN)) {
            parser_advance(p);
            // Return empty tuple as unit type
            Expr* e = new_expr(EXPR_TUPLE, line, col);
            e->array.elements = NULL;
            e->array.count = 0;
            return e;
        }
        
        Expr* first = parse_expr(p);
        if (parser_match(p, TOK_COMMA)) {
            // Tuple
            Expr* e = new_expr(EXPR_TUPLE, line, col);
            e->array.elements = malloc(sizeof(Expr*) * 16);
            e->array.elements[0] = first;
            e->array.count = 1;
            do {
                e->array.elements[e->array.count++] = parse_expr(p);
            } while (parser_match(p, TOK_COMMA));
            parser_expect(p, TOK_RPAREN, ")");
            return e;
        }
        parser_expect(p, TOK_RPAREN, ")");
        return first;
    }
    
    // Array literal
    if (parser_match(p, TOK_LBRACKET)) {
        Expr* e = new_expr(EXPR_ARRAY, line, col);
        e->array.elements = malloc(sizeof(Expr*) * 256);
        e->array.count = 0;
        if (!parser_check(p, TOK_RBRACKET)) {
            do {
                e->array.elements[e->array.count++] = parse_expr(p);
            } while (parser_match(p, TOK_COMMA));
        }
        parser_expect(p, TOK_RBRACKET, "]");
        return e;
    }
    
    // Map literal
    if (parser_match(p, TOK_LBRACE)) {
        Expr* e = new_expr(EXPR_MAP, line, col);
        e->map.keys = malloc(sizeof(char*) * 64);
        e->map.values = malloc(sizeof(Expr*) * 64);
        e->map.count = 0;
        if (!parser_check(p, TOK_RBRACE)) {
            do {
                if (parser_check(p, TOK_STRING)) {
                    e->map.keys[e->map.count] = strdup(p->current.str_val);
                    parser_advance(p);
                } else if (parser_check(p, TOK_IDENT)) {
                    e->map.keys[e->map.count] = strdup(p->current.ident);
                    parser_advance(p);
                }
                parser_expect(p, TOK_COLON, ":");
                e->map.values[e->map.count] = parse_expr(p);
                e->map.count++;
            } while (parser_match(p, TOK_COMMA));
        }
        parser_expect(p, TOK_RBRACE, "}");
        return e;
    }
    
    // If expression (ternary)
    if (parser_match(p, TOK_IF)) {
        Expr* e = new_expr(EXPR_IF, line, col);
        e->if_expr.cond = parse_expr(p);
        parser_expect(p, TOK_THEN, "then");
        e->if_expr.then_expr = parse_expr(p);
        parser_expect(p, TOK_ELSE, "else");
        e->if_expr.else_expr = parse_expr(p);
        return e;
    }
    
    // Match expression
    if (parser_match(p, TOK_MATCH)) {
        Expr* e = new_expr(EXPR_MATCH, line, col);
        e->match_expr.value = parse_expr(p);
        e->match_expr.patterns = malloc(sizeof(Expr*) * 64);
        e->match_expr.arms = malloc(sizeof(Expr*) * 64);
        e->match_expr.binding_types = malloc(sizeof(Type*) * 64);
        e->match_expr.arm_count = 0;
        parser_expect(p, TOK_LBRACE, "{");
        while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
            Expr* pattern = parse_expr(p);
            e->match_expr.patterns[e->match_expr.arm_count] = pattern;
            e->match_expr.bindings[e->match_expr.arm_count][0] = '\0';
            e->match_expr.binding_types[e->match_expr.arm_count] = NULL;
            if ((pattern->kind == EXPR_SOME || pattern->kind == EXPR_OK || pattern->kind == EXPR_ERR) &&
                pattern->some.value && pattern->some.value->kind == EXPR_IDENT) {
                strcpy(e->match_expr.bindings[e->match_expr.arm_count], pattern->some.value->ident);
            }
            parser_expect(p, TOK_FATARROW, "=>");
            e->match_expr.arms[e->match_expr.arm_count] = parse_expr(p);
            e->match_expr.arm_count++;
        }
        parser_expect(p, TOK_RBRACE, "}");
        return e;
    }
    
    // Lambda: fn(params) { body } or fn(params) -> Type { body }
    if (parser_match(p, TOK_FN)) {
        Expr* e = new_expr(EXPR_LAMBDA, line, col);
        e->lambda.param_count = 0;
        e->lambda.return_type = NULL;
        e->lambda.id = p->lambda_count++;
        
        parser_expect(p, TOK_LPAREN, "(");
        while (!parser_check(p, TOK_RPAREN) && !parser_check(p, TOK_EOF)) {
            if (e->lambda.param_count > 0) parser_expect(p, TOK_COMMA, ",");
            Token name = parser_expect(p, TOK_IDENT, "parameter name");
            strcpy(e->lambda.params[e->lambda.param_count], name.ident);
            parser_expect(p, TOK_COLON, ":");
            e->lambda.param_types[e->lambda.param_count] = parse_type(p);
            e->lambda.param_count++;
        }
        parser_expect(p, TOK_RPAREN, ")");
        
        if (parser_match(p, TOK_ARROW)) {
            e->lambda.return_type = parse_type(p);
        }
        
        parser_expect(p, TOK_LBRACE, "{");
        e->lambda.body = malloc(sizeof(Stmt*) * 256);
        e->lambda.body_count = 0;
        while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
            e->lambda.body[e->lambda.body_count++] = parse_stmt(p);
        }
        parser_expect(p, TOK_RBRACE, "}");
        return e;
    }
    
    parser_error(p, "expected expression, got token %d", p->current.kind);
    parser_advance(p);  // Skip the bad token to avoid infinite loop
    return new_expr(EXPR_INT, line, col);
}

static Expr* parse_postfix(Parser* p) {
    Expr* e = parse_primary(p);
    
    while (true) {
        int line = p->current.line, col = p->current.column;
        
        // Generic function call: func[T](args) - detect by checking if next is type-like
        if (parser_check(p, TOK_LBRACKET) && e->kind == EXPR_IDENT) {
            // Peek ahead: if next token after [ is a type keyword or uppercase ident, it's generic
            parser_advance(p);  // consume [
            
            bool is_type = parser_check(p, TOK_IDENT) && 
                ((p->current.ident[0] >= 'A' && p->current.ident[0] <= 'Z') ||
                 strcmp(p->current.ident, "int") == 0 ||
                 strcmp(p->current.ident, "str") == 0 ||
                 strcmp(p->current.ident, "bool") == 0 ||
                 strcmp(p->current.ident, "float") == 0 ||
                 strcmp(p->current.ident, "byte") == 0);
            
            if (is_type) {
                Type** type_args = malloc(sizeof(Type*) * MAX_GENERICS);
                int type_arg_count = 0;
                
                while (!parser_check(p, TOK_RBRACKET) && !parser_check(p, TOK_EOF)) {
                    type_args[type_arg_count++] = parse_type(p);
                    if (!parser_match(p, TOK_COMMA)) break;
                }
                parser_expect(p, TOK_RBRACKET, "]");
                
                if (parser_match(p, TOK_LPAREN)) {
                    // Generic function call
                    Expr* call = new_expr(EXPR_CALL, line, col);
                    call->call.func = e;
                    call->call.args = malloc(sizeof(Expr*) * MAX_PARAMS);
                    call->call.arg_count = 0;
                    call->call.type_args = type_args;
                    call->call.type_arg_count = type_arg_count;
                    if (!parser_check(p, TOK_RPAREN)) {
                        do {
                            call->call.args[call->call.arg_count++] = parse_expr(p);
                        } while (parser_match(p, TOK_COMMA));
                    }
                    parser_expect(p, TOK_RPAREN, ")");
                    e = call;
                    continue;
                }
                // Not followed by (, treat as error or continue
                free(type_args);
            } else {
                // Not a type, parse as index/slice - put back the [
                // We already consumed [, so continue with index parsing
                Expr* start = NULL;
                Expr* end = NULL;
                Expr* step = NULL;
                bool is_slice = false;
                
                if (parser_check(p, TOK_COLON)) {
                    is_slice = true;
                } else if (!parser_check(p, TOK_RBRACKET)) {
                    start = parse_expr(p);
                }
                
                if (parser_match(p, TOK_COLON)) {
                    is_slice = true;
                    if (!parser_check(p, TOK_RBRACKET) && !parser_check(p, TOK_COLON)) {
                        end = parse_expr(p);
                    }
                    if (parser_match(p, TOK_COLON)) {
                        if (!parser_check(p, TOK_RBRACKET)) {
                            step = parse_expr(p);
                        }
                    }
                }
                parser_expect(p, TOK_RBRACKET, "]");
                
                if (is_slice) {
                    Expr* slice = new_expr(EXPR_SLICE, line, col);
                    slice->slice.object = e;
                    slice->slice.start = start;
                    slice->slice.end = end;
                    slice->slice.step = step;
                    e = slice;
                } else {
                    Expr* idx = new_expr(EXPR_INDEX, line, col);
                    idx->index.object = e;
                    idx->index.index = start;
                    idx->index.safe = false;
                    e = idx;
                }
                continue;
            }
        }
        
        // Function call: expr(args)
        if (parser_match(p, TOK_LPAREN)) {
            Expr* call = new_expr(EXPR_CALL, line, col);
            call->call.func = e;
            call->call.args = malloc(sizeof(Expr*) * MAX_PARAMS);
            call->call.arg_count = 0;
            call->call.type_args = NULL;
            call->call.type_arg_count = 0;
            if (!parser_check(p, TOK_RPAREN)) {
                do {
                    call->call.args[call->call.arg_count++] = parse_expr(p);
                } while (parser_match(p, TOK_COMMA));
            }
            parser_expect(p, TOK_RPAREN, ")");
            e = call;
            continue;
        }
        
        // Index or Slice: expr[index] or expr[start:end] or expr[start:end:step]
        if (parser_match(p, TOK_LBRACKET)) {
            Expr* start = NULL;
            Expr* end = NULL;
            Expr* step = NULL;
            bool is_slice = false;
            
            // Check for [:...] (empty start)
            if (parser_check(p, TOK_COLON)) {
                is_slice = true;
            } else if (!parser_check(p, TOK_RBRACKET)) {
                start = parse_expr(p);
            }
            
            // Check for slice syntax
            if (parser_match(p, TOK_COLON)) {
                is_slice = true;
                // Parse end if present
                if (!parser_check(p, TOK_COLON) && !parser_check(p, TOK_RBRACKET)) {
                    end = parse_expr(p);
                }
                // Check for step
                if (parser_match(p, TOK_COLON)) {
                    if (!parser_check(p, TOK_RBRACKET)) {
                        step = parse_expr(p);
                    }
                }
            }
            
            parser_expect(p, TOK_RBRACKET, "]");
            
            if (is_slice) {
                Expr* sl = new_expr(EXPR_SLICE, line, col);
                sl->slice.object = e;
                sl->slice.start = start;
                sl->slice.end = end;
                sl->slice.step = step;
                e = sl;
            } else {
                Expr* idx = new_expr(EXPR_INDEX, line, col);
                idx->index.object = e;
                idx->index.index = start;
                idx->index.safe = false;
                e = idx;
            }
            continue;
        }
        
        // Field access: expr.field
        if (parser_match(p, TOK_DOT)) {
            Expr* fld = new_expr(EXPR_FIELD, line, col);
            fld->field.object = e;
            Token name = parser_expect(p, TOK_IDENT, "field name");
            strcpy(fld->field.field, name.ident);
            e = fld;
            continue;
        }
        
        // Optional chaining: expr?.field
        if (parser_match(p, TOK_QUESTIONDOT)) {
            Expr* fld = new_expr(EXPR_FIELD, line, col);
            fld->field.object = e;
            Token name = parser_expect(p, TOK_IDENT, "field name");
            strcpy(fld->field.field, name.ident);
            e = fld;
            continue;
        }
        
        // Unwrap: expr!
        if (parser_match(p, TOK_BANG)) {
            Expr* unwrap = new_expr(EXPR_UNWRAP, line, col);
            unwrap->unary.operand = e;
            unwrap->unary.op = TOK_BANG;
            e = unwrap;
            continue;
        }
        
        // Try: expr?
        if (parser_match(p, TOK_QUESTION)) {
            Expr* try_e = new_expr(EXPR_TRY, line, col);
            try_e->unary.operand = e;
            try_e->unary.op = TOK_QUESTION;
            e = try_e;
            continue;
        }
        
        break;
    }
    return e;
}

static Expr* parse_unary(Parser* p) {
    int line = p->current.line, col = p->current.column;
    
    // Await: await expr
    if (parser_match(p, TOK_AWAIT)) {
        Expr* e = new_expr(EXPR_AWAIT, line, col);
        e->await_expr.future = parse_unary(p);
        return e;
    }
    
    // Address-of: &expr
    if (parser_match(p, TOK_AMPERSAND)) {
        Expr* e = new_expr(EXPR_ADDR, line, col);
        e->unary.operand = parse_unary(p);
        return e;
    }
    
    // Dereference: *expr
    if (parser_match(p, TOK_STAR)) {
        Expr* e = new_expr(EXPR_DEREF, line, col);
        e->unary.operand = parse_unary(p);
        return e;
    }
    
    if (parser_check(p, TOK_MINUS) || parser_check(p, TOK_NOT) || 
        parser_check(p, TOK_BANG) || parser_check(p, TOK_TILDE)) {
        TokenKind op = p->current.kind;
        parser_advance(p);
        Expr* e = new_expr(EXPR_UNARY, line, col);
        e->unary.op = op;
        e->unary.operand = parse_unary(p);
        return e;
    }
    return parse_postfix(p);
}

static int get_precedence(TokenKind op) {
    switch (op) {
        case TOK_OR: return 1;
        case TOK_AND: return 2;
        case TOK_PIPE: return 3;        // bitwise or
        case TOK_CARET: return 4;       // bitwise xor
        case TOK_AMPERSAND: return 5;   // bitwise and
        case TOK_EQEQ: case TOK_NOTEQ: case TOK_IN: return 6;  // in is comparison-level
        case TOK_LT: case TOK_GT: case TOK_LTEQ: case TOK_GTEQ: return 7;
        case TOK_LTLT: case TOK_GTGT: return 8;  // shifts
        case TOK_DOTDOT: case TOK_DOTDOTEQ: return 9;  // range operators
        case TOK_PLUS: case TOK_MINUS: return 10;
        case TOK_STAR: case TOK_SLASH: case TOK_PERCENT: return 11;
        case TOK_STARSTAR: return 12;
        case TOK_QUESTIONQUESTION: return 0;
        default: return -1;
    }
}

static Expr* parse_binary(Parser* p, int min_prec) {
    Expr* left = parse_unary(p);
    
    while (true) {
        // Stop at newline for operators that could start a new statement
        // This prevents *p on a new line from being parsed as multiplication
        if (p->newline_before && (p->current.kind == TOK_STAR || 
                                   p->current.kind == TOK_MINUS ||
                                   p->current.kind == TOK_AMPERSAND)) {
            break;
        }
        
        int prec = get_precedence(p->current.kind);
        if (prec < min_prec) break;
        
        int line = p->current.line, col = p->current.column;
        TokenKind op = p->current.kind;
        parser_advance(p);
        
        Expr* right = parse_binary(p, prec + 1);
        
        if (op == TOK_QUESTIONQUESTION) {
            Expr* e = new_expr(EXPR_DEFAULT, line, col);
            e->default_expr.value = left;
            e->default_expr.default_val = right;
            left = e;
        } else {
            Expr* e = new_expr(EXPR_BINARY, line, col);
            e->binary.left = left;
            e->binary.right = right;
            e->binary.op = op;
            left = e;
        }
    }
    return left;
}

static Expr* parse_expr(Parser* p) {
    return parse_binary(p, 0);
}

// Statement parsing
static Stmt** parse_block(Parser* p, int* count) {
    Stmt** stmts = malloc(sizeof(Stmt*) * 256);
    *count = 0;
    parser_expect(p, TOK_LBRACE, "{");
    while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
        stmts[(*count)++] = parse_stmt(p);
    }
    parser_expect(p, TOK_RBRACE, "}");
    return stmts;
}

static Stmt* parse_stmt(Parser* p) {
    int line = p->current.line, col = p->current.column;
    
    // Variable declaration: let (mutable) or const (immutable)
    if (parser_check(p, TOK_LET) || parser_check(p, TOK_CONST) ||
        (parser_check(p, TOK_PUB) && (lexer_peek(p->lexer).kind == TOK_LET || lexer_peek(p->lexer).kind == TOK_CONST))) {
        Stmt* s = new_stmt(STMT_LET, line, col);
        s->let.is_pub = parser_match(p, TOK_PUB);
        
        // let = mutable, const = immutable
        if (parser_match(p, TOK_LET)) {
            s->let.is_mut = true;
        } else if (parser_match(p, TOK_CONST)) {
            s->let.is_mut = false;
        }
        
        Token name = parser_expect(p, TOK_IDENT, "variable name");
        strcpy(s->let.name, name.ident);
        
        if (parser_match(p, TOK_COLON)) {
            s->let.type = parse_type(p);
        }
        if (parser_match(p, TOK_EQ)) {
            s->let.value = parse_expr(p);
        }
        return s;
    }
    
    // Return statement
    if (parser_match(p, TOK_RETURN)) {
        Stmt* s = new_stmt(STMT_RETURN, line, col);
        if (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_NEWLINE) && !parser_check(p, TOK_EOF)) {
            s->ret.value = parse_expr(p);
        }
        return s;
    }
    
    // Break/Continue
    if (parser_match(p, TOK_BREAK)) {
        return new_stmt(STMT_BREAK, line, col);
    }
    if (parser_match(p, TOK_CONTINUE)) {
        return new_stmt(STMT_CONTINUE, line, col);
    }
    
    // Defer
    if (parser_match(p, TOK_DEFER)) {
        Stmt* s = new_stmt(STMT_DEFER, line, col);
        s->defer.expr = parse_expr(p);
        return s;
    }
    
    // Spawn
    if (parser_match(p, TOK_SPAWN)) {
        Stmt* s = new_stmt(STMT_SPAWN, line, col);
        s->spawn.body = parse_block(p, &s->spawn.body_count);
        s->spawn.captured_vars = NULL;
        s->spawn.captured_count = 0;
        return s;
    }
    
    // If statement
    if (parser_match(p, TOK_IF)) {
        Stmt* s = new_stmt(STMT_IF, line, col);
        s->if_stmt.cond = parse_expr(p);
        s->if_stmt.then_block = parse_block(p, &s->if_stmt.then_count);
        if (parser_match(p, TOK_ELSE)) {
            if (parser_check(p, TOK_IF)) {
                // else if - wrap in block
                s->if_stmt.else_block = malloc(sizeof(Stmt*));
                s->if_stmt.else_block[0] = parse_stmt(p);
                s->if_stmt.else_count = 1;
            } else {
                s->if_stmt.else_block = parse_block(p, &s->if_stmt.else_count);
            }
        }
        return s;
    }
    
    // While statement
    if (parser_match(p, TOK_WHILE)) {
        Stmt* s = new_stmt(STMT_WHILE, line, col);
        s->while_stmt.cond = parse_expr(p);
        s->while_stmt.body = parse_block(p, &s->while_stmt.body_count);
        return s;
    }
    
    // For statement (including parallel for)
    if (parser_check(p, TOK_FOR) || parser_check(p, TOK_PARALLEL)) {
        Stmt* s = new_stmt(STMT_FOR, line, col);
        s->for_stmt.is_parallel = parser_match(p, TOK_PARALLEL);
        parser_expect(p, TOK_FOR, "for");
        Token var = parser_expect(p, TOK_IDENT, "variable name");
        strcpy(s->for_stmt.var, var.ident);
        parser_expect(p, TOK_IN, "in");
        s->for_stmt.iter = parse_expr(p);
        s->for_stmt.body = parse_block(p, &s->for_stmt.body_count);
        return s;
    }
    
    // Match statement
    if (parser_match(p, TOK_MATCH)) {
        Stmt* s = new_stmt(STMT_MATCH, line, col);
        s->match_stmt.value = parse_expr(p);
        s->match_stmt.patterns = malloc(sizeof(Expr*) * 64);
        s->match_stmt.arms = malloc(sizeof(Stmt**) * 64);
        s->match_stmt.arm_counts = malloc(sizeof(int) * 64);
        s->match_stmt.binding_types = malloc(sizeof(Type*) * 64);
        s->match_stmt.arm_count = 0;
        parser_expect(p, TOK_LBRACE, "{");
        while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
            // Parse pattern (expression or _ for wildcard)
            Expr* pattern = parse_expr(p);
            s->match_stmt.patterns[s->match_stmt.arm_count] = pattern;
            // Extract binding variable from patterns like some(x), ok(v), err(e)
            s->match_stmt.bindings[s->match_stmt.arm_count][0] = '\0';
            s->match_stmt.binding_types[s->match_stmt.arm_count] = NULL;
            if ((pattern->kind == EXPR_SOME || pattern->kind == EXPR_OK || pattern->kind == EXPR_ERR) &&
                pattern->some.value && pattern->some.value->kind == EXPR_IDENT) {
                strcpy(s->match_stmt.bindings[s->match_stmt.arm_count], pattern->some.value->ident);
            }
            parser_expect(p, TOK_FATARROW, "=>");
            // Parse arm body - either block or single statement
            if (parser_check(p, TOK_LBRACE)) {
                s->match_stmt.arms[s->match_stmt.arm_count] = parse_block(p, &s->match_stmt.arm_counts[s->match_stmt.arm_count]);
            } else {
                s->match_stmt.arms[s->match_stmt.arm_count] = malloc(sizeof(Stmt*));
                s->match_stmt.arms[s->match_stmt.arm_count][0] = parse_stmt(p);
                s->match_stmt.arm_counts[s->match_stmt.arm_count] = 1;
            }
            s->match_stmt.arm_count++;
        }
        parser_expect(p, TOK_RBRACE, "}");
        return s;
    }
    
    // Block
    if (parser_check(p, TOK_LBRACE)) {
        Stmt* s = new_stmt(STMT_BLOCK, line, col);
        s->block.stmts = parse_block(p, &s->block.count);
        return s;
    }
    
    // Special handling for pointer dereference assignment: *ident = value
    if (parser_check(p, TOK_STAR)) {
        parser_advance(p);  // consume *
        Expr* target = new_expr(EXPR_DEREF, line, col);
        // Parse just the identifier or parenthesized expression
        if (parser_check(p, TOK_IDENT)) {
            Expr* ident = new_expr(EXPR_IDENT, p->current.line, p->current.column);
            strcpy(ident->ident, p->current.ident);
            parser_advance(p);
            target->unary.operand = ident;
        } else if (parser_check(p, TOK_LPAREN)) {
            parser_advance(p);
            target->unary.operand = parse_expr(p);
            parser_expect(p, TOK_RPAREN, ")");
        } else {
            target->unary.operand = parse_primary(p);
        }
        
        if (parser_check(p, TOK_EQ)) {
            Stmt* s = new_stmt(STMT_ASSIGN, line, col);
            s->assign.target = target;
            s->assign.op = TOK_EQ;
            parser_advance(p);  // consume =
            s->assign.value = parse_expr(p);
            return s;
        }
        // Not an assignment, wrap in expression statement
        Stmt* s = new_stmt(STMT_EXPR, line, col);
        s->expr.expr = target;
        return s;
    }
    
    // Expression statement (or assignment)
    Expr* expr = parse_expr(p);
    
    // Check for assignment
    if (parser_check(p, TOK_EQ) || parser_check(p, TOK_PLUSEQ) || parser_check(p, TOK_MINUSEQ) ||
        parser_check(p, TOK_STAREQ) || parser_check(p, TOK_SLASHEQ) || parser_check(p, TOK_PERCENTEQ)) {
        Stmt* s = new_stmt(STMT_ASSIGN, line, col);
        s->assign.target = expr;
        s->assign.op = p->current.kind;
        parser_advance(p);
        s->assign.value = parse_expr(p);
        return s;
    }
    
    Stmt* s = new_stmt(STMT_EXPR, line, col);
    s->expr.expr = expr;
    return s;
}

// Top-level parsing
static FnDef* parse_function(Parser* p, bool is_pub, bool is_async) {
    FnDef* fn = calloc(1, sizeof(FnDef));
    fn->is_pub = is_pub;
    fn->is_async = is_async;
    
    parser_expect(p, TOK_FN, "fn");
    Token name = parser_expect(p, TOK_IDENT, "function name");
    strcpy(fn->name, name.ident);
    
    // Generic parameters
    if (parser_match(p, TOK_LBRACKET)) {
        while (!parser_check(p, TOK_RBRACKET) && !parser_check(p, TOK_EOF)) {
            Token g = parser_expect(p, TOK_IDENT, "generic parameter");
            strcpy(fn->generics[fn->generic_count++], g.ident);
            if (!parser_match(p, TOK_COMMA)) break;
        }
        parser_expect(p, TOK_RBRACKET, "]");
    }
    
    // Parameters
    parser_expect(p, TOK_LPAREN, "(");
    if (!parser_check(p, TOK_RPAREN)) {
        do {
            // All parameters are mutable by default
            // Handle 'self' as a special parameter
            if (parser_check(p, TOK_SELF)) {
                parser_advance(p);
                strcpy(fn->params[fn->param_count].name, "self");
            } else {
                Token pname = parser_expect(p, TOK_IDENT, "parameter name");
                strcpy(fn->params[fn->param_count].name, pname.ident);
            }
            if (parser_match(p, TOK_COLON)) {
                // Check for variadic: ...Type
                if (parser_match(p, TOK_DOTDOTDOT)) {
                    fn->params[fn->param_count].is_variadic = true;
                    Type* inner = parse_type(p);
                    Type* arr = new_type(TYPE_ARRAY);
                    arr->inner = inner;
                    fn->params[fn->param_count].type = arr;
                } else {
                    fn->params[fn->param_count].type = parse_type(p);
                }
            }
            if (parser_match(p, TOK_EQ)) {
                fn->params[fn->param_count].default_val = parse_expr(p);
            }
            fn->param_count++;
        } while (parser_match(p, TOK_COMMA));
    }
    parser_expect(p, TOK_RPAREN, ")");
    
    // Return type
    if (parser_match(p, TOK_ARROW)) {
        fn->return_type = parse_type(p);
    }
    
    // Body
    fn->body = parse_block(p, &fn->body_count);
    return fn;
}

static StructDef* parse_struct(Parser* p, bool is_pub) {
    StructDef* s = calloc(1, sizeof(StructDef));
    s->is_pub = is_pub;
    
    parser_expect(p, TOK_STRUCT, "struct");
    Token name = parser_expect(p, TOK_IDENT, "struct name");
    strcpy(s->name, name.ident);
    
    // Generic parameters
    if (parser_match(p, TOK_LBRACKET)) {
        while (!parser_check(p, TOK_RBRACKET) && !parser_check(p, TOK_EOF)) {
            Token g = parser_expect(p, TOK_IDENT, "generic parameter");
            strcpy(s->generics[s->generic_count++], g.ident);
            if (!parser_match(p, TOK_COMMA)) break;
        }
        parser_expect(p, TOK_RBRACKET, "]");
    }
    
    parser_expect(p, TOK_LBRACE, "{");
    s->methods = malloc(sizeof(FnDef) * 64);
    
    while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
        while (parser_match(p, TOK_NEWLINE));
        if (parser_check(p, TOK_RBRACE)) break;
        
        bool field_pub = parser_match(p, TOK_PUB);
        
        if (parser_check(p, TOK_FN)) {
            FnDef* method = parse_function(p, field_pub, false);
            s->methods[s->method_count++] = *method;
            free(method);
        } else if (parser_check(p, TOK_IDENT)) {
            Token fname = parser_advance(p);
            strcpy(s->fields[s->field_count].name, fname.ident);
            s->fields[s->field_count].is_pub = field_pub;
            if (parser_match(p, TOK_COLON)) {
                s->fields[s->field_count].type = parse_type(p);
            }
            s->field_count++;
            parser_match(p, TOK_COMMA);
            while (parser_match(p, TOK_NEWLINE));
        }
    }
    parser_expect(p, TOK_RBRACE, "}");
    return s;
}

static EnumDef* parse_enum(Parser* p, bool is_pub) {
    EnumDef* e = calloc(1, sizeof(EnumDef));
    e->is_pub = is_pub;
    
    parser_expect(p, TOK_ENUM, "enum");
    Token name = parser_expect(p, TOK_IDENT, "enum name");
    strcpy(e->name, name.ident);
    
    // Generic parameters
    if (parser_match(p, TOK_LBRACKET)) {
        while (!parser_check(p, TOK_RBRACKET) && !parser_check(p, TOK_EOF)) {
            Token g = parser_expect(p, TOK_IDENT, "generic parameter");
            strcpy(e->generics[e->generic_count++], g.ident);
            if (!parser_match(p, TOK_COMMA)) break;
        }
        parser_expect(p, TOK_RBRACKET, "]");
    }
    
    parser_expect(p, TOK_LBRACE, "{");
    while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
        // Skip newlines
        while (parser_match(p, TOK_NEWLINE));
        if (parser_check(p, TOK_RBRACE)) break;
        
        Token vname = parser_expect(p, TOK_IDENT, "variant name");
        strcpy(e->variants[e->variant_count].name, vname.ident);
        
        // Associated types
        if (parser_match(p, TOK_LPAREN)) {
            e->variants[e->variant_count].types = malloc(sizeof(Type*) * 8);
            do {
                e->variants[e->variant_count].types[e->variants[e->variant_count].type_count++] = parse_type(p);
            } while (parser_match(p, TOK_COMMA));
            parser_expect(p, TOK_RPAREN, ")");
        }
        
        // Explicit value
        if (parser_match(p, TOK_EQ)) {
            e->variants[e->variant_count].has_value = true;
            e->variants[e->variant_count].value = p->current.int_val;
            parser_advance(p);
        }
        e->variant_count++;
        
        // Skip comma and newlines
        parser_match(p, TOK_COMMA);
        while (parser_match(p, TOK_NEWLINE));
    }
    parser_expect(p, TOK_RBRACE, "}");
    return e;
}

// Parse method signature (for interfaces - no body)
static FnDef* parse_method_sig(Parser* p) {
    FnDef* fn = calloc(1, sizeof(FnDef));
    
    parser_expect(p, TOK_FN, "fn");
    Token name = parser_expect(p, TOK_IDENT, "method name");
    strcpy(fn->name, name.ident);
    
    parser_expect(p, TOK_LPAREN, "(");
    if (!parser_check(p, TOK_RPAREN)) {
        do {
            // All parameters are mutable by default
            if (parser_check(p, TOK_SELF)) {
                parser_advance(p);
                strcpy(fn->params[fn->param_count].name, "self");
            } else {
                Token pname = parser_expect(p, TOK_IDENT, "parameter name");
                strcpy(fn->params[fn->param_count].name, pname.ident);
            }
            if (parser_match(p, TOK_COLON)) {
                fn->params[fn->param_count].type = parse_type(p);
            }
            fn->param_count++;
        } while (parser_match(p, TOK_COMMA));
    }
    parser_expect(p, TOK_RPAREN, ")");
    
    if (parser_match(p, TOK_ARROW)) {
        fn->return_type = parse_type(p);
    }
    
    return fn;
}

static InterfaceDef* parse_interface(Parser* p, bool is_pub) {
    InterfaceDef* i = calloc(1, sizeof(InterfaceDef));
    i->is_pub = is_pub;
    
    parser_expect(p, TOK_INTERFACE, "interface");
    Token name = parser_expect(p, TOK_IDENT, "interface name");
    strcpy(i->name, name.ident);
    
    parser_expect(p, TOK_LBRACE, "{");
    i->methods = malloc(sizeof(FnDef) * 32);
    while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
        FnDef* method = parse_method_sig(p);
        i->methods[i->method_count++] = *method;
        free(method);
    }
    parser_expect(p, TOK_RBRACE, "}");
    return i;
}

static TestDef* parse_test(Parser* p) {
    TestDef* t = calloc(1, sizeof(TestDef));
    parser_expect(p, TOK_TEST, "test");
    Token name = parser_expect(p, TOK_STRING, "test name");
    strcpy(t->name, name.str_val);
    t->body = parse_block(p, &t->body_count);
    return t;
}

static Import* parse_import(Parser* p) {
    Import* imp = calloc(1, sizeof(Import));
    parser_expect(p, TOK_IMPORT, "import");
    
    // Selective: import { a, b } from path
    if (parser_match(p, TOK_LBRACE)) {
        imp->imports = malloc(sizeof(char*) * 32);
        do {
            Token name = parser_expect(p, TOK_IDENT, "import name");
            imp->imports[imp->import_count++] = strdup(name.ident);
        } while (parser_match(p, TOK_COMMA));
        parser_expect(p, TOK_RBRACE, "}");
        parser_expect(p, TOK_FROM, "from");
    }
    
    // Path (identifier or ./path or url)
    if (parser_check(p, TOK_IDENT)) {
        strcpy(imp->path, p->current.ident);
        parser_advance(p);
        // Handle std/module or module/submodule syntax
        while (parser_check(p, TOK_SLASH)) {
            strcat(imp->path, "/");
            parser_advance(p);
            if (parser_check(p, TOK_IDENT)) {
                strcat(imp->path, p->current.ident);
                parser_advance(p);
            }
        }
    } else if (parser_check(p, TOK_DOT)) {
        // Relative path ./something
        int i = 0;
        while (parser_check(p, TOK_DOT) || parser_check(p, TOK_SLASH) || parser_check(p, TOK_IDENT)) {
            if (parser_check(p, TOK_DOT)) { imp->path[i++] = '.'; parser_advance(p); }
            else if (parser_check(p, TOK_SLASH)) { imp->path[i++] = '/'; parser_advance(p); }
            else { 
                strcat(imp->path, p->current.ident); 
                i += strlen(p->current.ident);
                parser_advance(p); 
            }
        }
    }
    
    // Alias: import x as y
    if (parser_match(p, TOK_AS)) {
        Token alias = parser_expect(p, TOK_IDENT, "alias");
        strcpy(imp->alias, alias.ident);
    }
    
    return imp;
}

static Module* parse_module(Parser* p) {
    Module* m = calloc(1, sizeof(Module));
    m->imports = malloc(sizeof(Import) * 64);
    m->functions = malloc(sizeof(FnDef) * 256);
    m->structs = malloc(sizeof(StructDef) * 64);
    m->enums = malloc(sizeof(EnumDef) * 64);
    m->interfaces = malloc(sizeof(InterfaceDef) * 32);
    m->tests = malloc(sizeof(TestDef) * 128);
    
    while (!parser_check(p, TOK_EOF)) {
        bool is_pub = parser_match(p, TOK_PUB);
        bool is_async = parser_match(p, TOK_ASYNC);
        
        if (parser_check(p, TOK_IMPORT)) {
            m->imports[m->import_count++] = *parse_import(p);
        } else if (parser_check(p, TOK_FN)) {
            m->functions[m->function_count++] = *parse_function(p, is_pub, is_async);
        } else if (parser_check(p, TOK_STRUCT)) {
            m->structs[m->struct_count++] = *parse_struct(p, is_pub);
        } else if (parser_check(p, TOK_ENUM)) {
            m->enums[m->enum_count++] = *parse_enum(p, is_pub);
        } else if (parser_check(p, TOK_INTERFACE)) {
            m->interfaces[m->interface_count++] = *parse_interface(p, is_pub);
        } else if (parser_check(p, TOK_TEST)) {
            m->tests[m->test_count++] = *parse_test(p);
        } else if (parser_check(p, TOK_BENCH)) {
            // Skip bench for now
            while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) parser_advance(p);
            if (parser_check(p, TOK_RBRACE)) parser_advance(p);
        } else {
            parser_advance(p); // Skip unknown
        }
    }
    return m;
}

// AST printing for debugging
static void print_indent(int depth) { for (int i = 0; i < depth; i++) printf("  "); }

static void print_type(Type* t) {
    if (!t) { printf("?"); return; }
    switch (t->kind) {
        case TYPE_INT: printf("int"); break;
        case TYPE_FLOAT: printf("float"); break;
        case TYPE_STR: printf("str"); break;
        case TYPE_BOOL: printf("bool"); break;
        case TYPE_BYTE: printf("byte"); break;
        case TYPE_ANY: printf("any"); break;
        case TYPE_VOID: printf("void"); break;
        case TYPE_ARRAY: printf("["); print_type(t->inner); printf("]"); break;
        case TYPE_OPTIONAL: printf("?"); print_type(t->inner); break;
        case TYPE_NAMED: printf("%s", t->name); break;
        case TYPE_GENERIC: 
            printf("%s[", t->name);
            for (int i = 0; i < t->param_count; i++) {
                if (i > 0) printf(", ");
                print_type(t->params[i]);
            }
            printf("]");
            break;
        default: printf("???"); break;
    }
}

static void print_module(Module* m) {
    printf("\n=== AST ===\n");
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
    bool had_error;
} TypeChecker;

static TypeChecker* typechecker_new(Module* m, const char* filename) {
    TypeChecker* tc = calloc(1, sizeof(TypeChecker));
    tc->module = m;
    tc->filename = filename;
    return tc;
}

static void tc_error(TypeChecker* tc, int line, int col, const char* fmt, ...) {
    fprintf(stderr, "%s:%d:%d: type error: ", tc->filename, line, col);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
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
                if (strcmp(e->ident, "print") == 0 || strcmp(e->ident, "println") == 0 || strcmp(e->ident, "print_str") == 0 ||
                    strcmp(e->ident, "assert") == 0 || strcmp(e->ident, "args") == 0 || 
                    strcmp(e->ident, "syscall") == 0 || strcmp(e->ident, "len") == 0 ||
                    strcmp(e->ident, "ord") == 0 || strcmp(e->ident, "chr") == 0 ||
                    strcmp(e->ident, "min") == 0 || strcmp(e->ident, "max") == 0 ||
                    strcmp(e->ident, "substring") == 0 || strcmp(e->ident, "str_split") == 0 ||
                    strcmp(e->ident, "str_find") == 0 || strcmp(e->ident, "str_concat") == 0 ||
                    strcmp(e->ident, "str_cmp") == 0 || strcmp(e->ident, "char_at") == 0 ||
                    strcmp(e->ident, "int_to_str") == 0 || strcmp(e->ident, "str_substr") == 0 ||
                    strcmp(e->ident, "str_to_int") == 0 || strcmp(e->ident, "abs") == 0 ||
                    strcmp(e->ident, "sqrtf") == 0 || strcmp(e->ident, "int_to_float") == 0 ||
                    strcmp(e->ident, "float_to_int") == 0 ||
                    strcmp(e->ident, "read_file") == 0 || strcmp(e->ident, "write_file") == 0 ||
                    strcmp(e->ident, "append_file") == 0 || strcmp(e->ident, "file_exists") == 0 ||
                    strcmp(e->ident, "file_size") == 0 || strcmp(e->ident, "read_stdin_line") == 0 ||
                    strcmp(e->ident, "mkdir") == 0 || strcmp(e->ident, "rmdir") == 0 ||
                    strcmp(e->ident, "rename_file") == 0 || strcmp(e->ident, "getcwd") == 0 ||
                    strcmp(e->ident, "getpid") == 0 || strcmp(e->ident, "time_now") == 0 ||
                    strcmp(e->ident, "sleep_ms") == 0 || strcmp(e->ident, "sleep") == 0 ||
                    strcmp(e->ident, "clock") == 0 || strcmp(e->ident, "random") == 0 || strcmp(e->ident, "exit") == 0 ||
                    strcmp(e->ident, "gpu_device_count") == 0) {
                    return new_type(TYPE_FUNCTION);
                }
                tc_error(tc, e->line, e->column, "undefined variable '%s'", e->ident);
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
                    // For now, return ANY - could be improved with a lookup table
                    return new_type(TYPE_ANY);
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
                // Check for primitive type methods first
                if (strcmp(method_name, "len") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "abs") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "to_str") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "to_int") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "to_float") == 0) return new_type(TYPE_FLOAT);
                if (strcmp(method_name, "contains") == 0) return new_type(TYPE_BOOL);
                if (strcmp(method_name, "index_of") == 0) return new_type(TYPE_INT);
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
            // Check if this is a module.function reference
            if (e->field.object->kind == EXPR_IDENT) {
                const char* potential_module = e->field.object->ident;
                if (tc_is_module_imported(tc, potential_module)) {
                    // This is a module namespace, return function type
                    return new_type(TYPE_FUNCTION);
                }
            }
            Type* obj = tc_check_expr(tc, e->field.object);
            if (obj->kind == TYPE_NAMED) {
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
            tc_check_expr(tc, e->match_expr.value);
            Type* result_type = NULL;
            for (int i = 0; i < e->match_expr.arm_count; i++) {
                tc_check_expr(tc, e->match_expr.patterns[i]);
                Type* arm_type = tc_check_expr(tc, e->match_expr.arms[i]);
                if (result_type == NULL) {
                    result_type = arm_type;
                }
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
            
            if (decl_type && init_type && !types_equal(decl_type, init_type) && 
                init_type->kind != TYPE_ANY && decl_type->kind != TYPE_ANY) {
                tc_error(tc, s->line, s->column, "type mismatch in let: expected %s, got %s",
                         type_name(decl_type), type_name(init_type));
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
            if (cond->kind != TYPE_BOOL) tc_error(tc, s->line, s->column, "condition must be bool");
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
            if (cond->kind != TYPE_BOOL) tc_error(tc, s->line, s->column, "condition must be bool");
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
            if (iter && iter->kind == TYPE_ARRAY && iter->inner) var_type = iter->inner;
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
    tc_exit_scope(tc);
}

static bool typecheck_module(TypeChecker* tc) {
    // Check all functions
    for (int i = 0; i < tc->module->function_count; i++) {
        tc_check_function(tc, &tc->module->functions[i]);
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
    if (strcmp(name, "print") == 0 || strcmp(name, "println") == 0 || strcmp(name, "print_str") == 0 || strcmp(name, "assert") == 0 || strcmp(name, "args") == 0) return true;
    if (strcmp(name, "min") == 0 || strcmp(name, "max") == 0) return true;
    
    // String builtins
    if (strcmp(name, "ord") == 0 || strcmp(name, "chr") == 0 || strcmp(name, "substring") == 0) return true;
    if (strcmp(name, "str_split") == 0 || strcmp(name, "str_find") == 0) return true;
    if (strcmp(name, "str_concat") == 0 || strcmp(name, "str_cmp") == 0) return true;
    if (strcmp(name, "char_at") == 0 || strcmp(name, "int_to_str") == 0) return true;
    if (strcmp(name, "str_substr") == 0 || strcmp(name, "str_to_int") == 0) return true;
    
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
    
    // Time builtins
    if (strcmp(name, "time_now") == 0 || strcmp(name, "sleep_ms") == 0 || strcmp(name, "sleep") == 0) return true;
    if (strcmp(name, "clock") == 0 || strcmp(name, "random") == 0) return true;
    
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
                    return new_type(TYPE_ANY);
                }
            }
            
            Type* func_type = tc1_check_expr(tc, e->call.func);
            (void)func_type;
            for (int i = 0; i < e->call.arg_count; i++) tc1_check_expr(tc, e->call.args[i]);
            
            if (e->call.func->kind == EXPR_IDENT) {
                FnDef* fn = tc_lookup_fn((TypeChecker*)tc, e->call.func->ident);
                if (fn) {
                    if (e->call.arg_count != fn->param_count) {
                        tc1_error(tc, e->line, e->column, "function '%s' expects %d arguments, got %d",
                                  e->call.func->ident, fn->param_count, e->call.arg_count);
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
                }
            }
            
            if (e->call.func->kind == EXPR_FIELD) {
                const char* method_name = e->call.func->field.field;
                if (strcmp(method_name, "len") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "abs") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "to_str") == 0) return new_type(TYPE_STR);
                if (strcmp(method_name, "to_int") == 0) return new_type(TYPE_INT);
                if (strcmp(method_name, "to_float") == 0) return new_type(TYPE_FLOAT);
                if (strcmp(method_name, "contains") == 0) return new_type(TYPE_BOOL);
                if (strcmp(method_name, "index_of") == 0) return new_type(TYPE_INT);
                
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
            // Check if this is a module.function reference
            if (e->field.object->kind == EXPR_IDENT) {
                const char* potential_module = e->field.object->ident;
                if (tc_is_module_imported((TypeChecker*)tc, potential_module)) {
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
            tc1_check_expr(tc, e->match_expr.value);
            Type* result_type = NULL;
            for (int i = 0; i < e->match_expr.arm_count; i++) {
                tc1_check_expr(tc, e->match_expr.patterns[i]);
                Type* arm_type = tc1_check_expr(tc, e->match_expr.arms[i]);
                if (result_type == NULL) result_type = arm_type;
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
            if (iter && iter->kind == TYPE_ARRAY && iter->inner) var_type = iter->inner;
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
        tc1_check_function(tc, &tc->module->functions[i], NULL);
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
static Stmt* clone_stmt(Stmt* s);

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

static Stmt* clone_stmt(Stmt* s) {
    if (!s) return NULL;
    Stmt* c = malloc(sizeof(Stmt));
    memcpy(c, s, sizeof(Stmt));
    
    switch (s->kind) {
        case STMT_LET:
            c->let.value = clone_expr(s->let.value);
            break;
        case STMT_ASSIGN:
            c->assign.target = clone_expr(s->assign.target);
            c->assign.value = clone_expr(s->assign.value);
            break;
        case STMT_EXPR:
            c->expr.expr = clone_expr(s->expr.expr);
            break;
        case STMT_RETURN:
            c->ret.value = clone_expr(s->ret.value);
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
        case EXPR_CALL:
            for (int i = 0; i < e->call.arg_count; i++)
                inline_expr(e->call.args[i], m);
            break;
        case EXPR_INDEX:
            inline_expr(e->index.object, m);
            inline_expr(e->index.index, m);
            break;
        default: break;
    }
}

static void inline_stmt(Stmt* s, Module* m) {
    if (!s) return;
    
    switch (s->kind) {
        case STMT_LET: inline_expr(s->let.value, m); break;
        case STMT_ASSIGN: inline_expr(s->assign.value, m); break;
        case STMT_EXPR: inline_expr(s->expr.expr, m); break;
        case STMT_RETURN: inline_expr(s->ret.value, m); break;
        case STMT_IF:
            inline_expr(s->if_stmt.cond, m);
            for (int i = 0; i < s->if_stmt.then_count; i++)
                inline_stmt(s->if_stmt.then_block[i], m);
            for (int i = 0; i < s->if_stmt.else_count; i++)
                inline_stmt(s->if_stmt.else_block[i], m);
            break;
        case STMT_WHILE:
            inline_expr(s->while_stmt.cond, m);
            for (int i = 0; i < s->while_stmt.body_count; i++)
                inline_stmt(s->while_stmt.body[i], m);
            break;
        case STMT_FOR:
            inline_expr(s->for_stmt.iter, m);
            for (int i = 0; i < s->for_stmt.body_count; i++)
                inline_stmt(s->for_stmt.body[i], m);
            break;
        default: break;
    }
}

static void inline_pass(Module* m) {
    for (int i = 0; i < m->function_count; i++) {
        FnDef* fn = &m->functions[i];
        for (int j = 0; j < fn->body_count; j++) {
            inline_stmt(fn->body[j], m);
        }
    }
}

// ============================================================================
// Stage 1 Optimizer - Loop Unrolling
// ============================================================================

static bool can_unroll_loop(Stmt* s) {
    if (s->kind != STMT_FOR) return false;
    if (s->for_stmt.body_count > 5) return false; // Only unroll small loops
    
    // Check if iterator is a simple range
    Expr* iter = s->for_stmt.iter;
    if (iter->kind == EXPR_BINARY && 
        (iter->binary.op == TOK_DOTDOT || iter->binary.op == TOK_DOTDOTEQ)) {
        // Check if range is small constant
        if (iter->binary.left->kind == EXPR_INT && iter->binary.right->kind == EXPR_INT) {
            int start = iter->binary.left->int_val;
            int end = iter->binary.right->int_val;
            int count = end - start;
            return count > 0 && count <= 4; // Unroll up to 4 iterations
        }
    }
    return false;
}

static void optimize_loop_unrolling(Module* m) {
    // Mark loops that can be unrolled
    // Actual unrolling happens in codegen
    // For now, just a placeholder for future optimization
    (void)m;
}

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
            s->spawn.captured_count = used_count;
            for (int k = 0; k < used_count; k++) {
                s->spawn.captured_vars[k] = strdup(used_vars[k]);
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
// Code Generator (ARM64 for Apple Silicon)
// ============================================================================

typedef enum { ARCH_ARM64, ARCH_X86_64 } Arch;
typedef enum { OS_MACOS, OS_LINUX } TargetOS;

typedef struct {
    FILE* out;
    Module* module;
    Arch arch;
    TargetOS os;
    int label_count;
    int stack_offset;
    struct { char name[MAX_IDENT_LEN]; int offset; Type* type; bool is_shared; } locals[256];
    int local_count;
    struct { char* str; int label; } strings[256];
    int string_count;
    struct { double val; int label; } floats[256];
    int float_count;
    int loop_start_label;  // For continue
    int loop_end_label;    // For break
    TypeChecker* tc;       // For type checking during codegen
    Expr* defers[64];      // Deferred expressions (LIFO)
    int defer_count;
    Expr* lambdas[64];     // Collected lambdas to emit
    int lambda_count;
    int spawn_count;       // Track spawned threads
    bool in_thread_context;  // True when generating code inside a thread function
} CodeGen;

static CodeGen* codegen_new(FILE* out, Module* m, Arch arch, TargetOS os, TypeChecker* tc) {
    CodeGen* cg = calloc(1, sizeof(CodeGen));
    cg->out = out;
    cg->module = m;
    cg->arch = arch;
    cg->os = os;
    cg->tc = tc;
    return cg;
}

static int cg_new_label(CodeGen* cg) { return cg->label_count++; }

// Returns "_" for macOS, "" for Linux (symbol prefix)
static const char* cg_sym_prefix(CodeGen* cg) {
    return cg->os == OS_MACOS ? "_" : "";
}

static void cg_emit(CodeGen* cg, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(cg->out, fmt, args);
    va_end(args);
    fprintf(cg->out, "\n");
}

// Escape string for assembly .asciz directive
static void escape_string_for_asm(const char* src, char* dst, size_t dst_size) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j < dst_size - 2; i++) {
        char c = src[i];
        if (c == '\n') { dst[j++] = '\\'; dst[j++] = 'n'; }
        else if (c == '\r') { dst[j++] = '\\'; dst[j++] = 'r'; }
        else if (c == '\t') { dst[j++] = '\\'; dst[j++] = 't'; }
        else if (c == '"') { dst[j++] = '\\'; dst[j++] = '"'; }
        else if (c == '\\') { dst[j++] = '\\'; dst[j++] = '\\'; }
        else dst[j++] = c;
    }
    dst[j] = '\0';
}

// Emit adrp instruction with correct syntax for target OS
static void cg_emit_adrp(CodeGen* cg, const char* reg, const char* label) {
    if (cg->os == OS_MACOS) {
        cg_emit(cg, "    adrp %s, %s@PAGE", reg, label);
    } else {
        cg_emit(cg, "    adrp %s, %s", reg, label);
    }
}

// Emit add instruction for page offset with correct syntax
static void cg_emit_add_pageoff(CodeGen* cg, const char* dst, const char* src, const char* label) {
    if (cg->os == OS_MACOS) {
        cg_emit(cg, "    add %s, %s, %s@PAGEOFF", dst, src, label);
    } else {
        cg_emit(cg, "    add %s, %s, :lo12:%s", dst, src, label);
    }
}

// Emit ldr instruction with page offset for floats
static void cg_emit_ldr_pageoff(CodeGen* cg, const char* reg, const char* base, const char* label) {
    if (cg->os == OS_MACOS) {
        cg_emit(cg, "    ldr %s, [%s, %s@PAGEOFF]", reg, base, label);
    } else {
        cg_emit(cg, "    ldr %s, [%s, :lo12:%s]", reg, base, label);
    }
}

// Emit str with large offset support (>4095 uses add+str)
static void cg_emit_str_offset(CodeGen* cg, const char* reg, int offset) {
    if (offset <= 4095) {
        cg_emit(cg, "    str %s, [x29, #%d]", reg, offset);
    } else {
        cg_emit(cg, "    mov x9, #%d", offset);
        cg_emit(cg, "    str %s, [x29, x9]", reg);
    }
}

// Emit ldr with large offset support
static void cg_emit_ldr_offset(CodeGen* cg, const char* reg, int offset) {
    if (offset <= 4095) {
        cg_emit(cg, "    ldr %s, [x29, #%d]", reg, offset);
    } else {
        cg_emit(cg, "    mov x9, #%d", offset);
        cg_emit(cg, "    ldr %s, [x29, x9]", reg);
    }
}

// Emit add with large offset support
static void cg_emit_add_offset(CodeGen* cg, const char* dst, int offset) {
    if (offset <= 4095) {
        cg_emit(cg, "    add %s, x29, #%d", dst, offset);
    } else {
        cg_emit(cg, "    mov x9, #%d", offset);
        cg_emit(cg, "    add %s, x29, x9", dst);
    }
}

static int cg_local_offset(CodeGen* cg, const char* name) {
    // Search from end to find most recent variable with this name
    for (int i = cg->local_count - 1; i >= 0; i--) {
        if (strcmp(cg->locals[i].name, name) == 0) return cg->locals[i].offset;
    }
    return 0;
}

static StructDef* cg_lookup_struct(CodeGen* cg, const char* name) {
    for (int i = 0; i < cg->module->struct_count; i++) {
        if (strcmp(cg->module->structs[i].name, name) == 0) return &cg->module->structs[i];
    }
    return NULL;
}

// Get struct size in bytes (0 if not a struct type)
static int cg_struct_size(CodeGen* cg, Type* t) {
    if (!t || t->kind != TYPE_NAMED) return 0;
    StructDef* s = cg_lookup_struct(cg, t->name);
    return s ? s->field_count * 8 : 0;
}

static int cg_struct_field_offset(StructDef* s, const char* field) {
    for (int i = 0; i < s->field_count; i++) {
        if (strcmp(s->fields[i].name, field) == 0) return i * 8;
    }
    return -1;
}

static Type* cg_struct_field_type(StructDef* s, const char* field) {
    for (int i = 0; i < s->field_count; i++) {
        if (strcmp(s->fields[i].name, field) == 0) return s->fields[i].type;
    }
    return NULL;
}

static int64_t cg_lookup_enum_variant(CodeGen* cg, const char* name, bool* found) {
    *found = false;
    for (int i = 0; i < cg->module->enum_count; i++) {
        for (int j = 0; j < cg->module->enums[i].variant_count; j++) {
            if (strcmp(cg->module->enums[i].variants[j].name, name) == 0) {
                *found = true;
                return cg->module->enums[i].variants[j].has_value ? 
                       cg->module->enums[i].variants[j].value : j;
            }
        }
    }
    return 0;
}

static void cg_add_local(CodeGen* cg, const char* name, Type* type) {
    // Reuse existing slot if variable with same name exists (for loop variables)
    // But don't reuse internal variables like __pad, __arr, __idx, __end
    if (name[0] != '_' || name[1] != '_') {
        for (int i = cg->local_count - 1; i >= 0; i--) {
            if (strcmp(cg->locals[i].name, name) == 0) {
                // Update type if provided, keep existing offset
                if (type) cg->locals[i].type = type;
                return;
            }
        }
    }
    cg->stack_offset += 8;
    strcpy(cg->locals[cg->local_count].name, name);
    cg->locals[cg->local_count].offset = cg->stack_offset;
    cg->locals[cg->local_count].type = type;
    cg->locals[cg->local_count].is_shared = false;
    cg->local_count++;
}

static bool cg_is_shared(CodeGen* cg, const char* name) {
    for (int i = cg->local_count - 1; i >= 0; i--) {
        if (strcmp(cg->locals[i].name, name) == 0) return cg->locals[i].is_shared;
    }
    return false;
}

// Atomic load for shared variables
static void cg_atomic_load(CodeGen* cg, const char* name, int offset) {
    if (cg->arch == ARCH_ARM64) {
        // Use mutex-based load for reliability
        if (cg->in_thread_context) {
            cg_emit_ldr_offset(cg, "x0", 16 + offset);
        } else {
            cg_emit_add_offset(cg, "x0", 16 + offset);
        }
        cg_emit(cg, "    bl ___wyn_atomic_load");
    } else {
        // x86_64: Use mutex-based load
        if (cg->in_thread_context) {
            cg_emit(cg, "    movq -%d(%%rbp), %%rdi", offset);
        } else {
            cg_emit(cg, "    leaq -%d(%%rbp), %%rdi", offset);
        }
        cg_emit(cg, "    call ___wyn_atomic_load");
    }
}

// Atomic store for shared variables
static void cg_atomic_store(CodeGen* cg, const char* name, int offset) {
    if (cg->arch == ARCH_ARM64) {
        if (cg->in_thread_context) {
            cg_emit_ldr_offset(cg, "x9", 16 + offset);
            cg_emit(cg, "    stlr x0, [x9]");
        } else {
            cg_emit_add_offset(cg, "x9", 16 + offset);
            cg_emit(cg, "    stlr x0, [x9]");
        }
    } else {
        if (cg->in_thread_context) {
            cg_emit(cg, "    movq -%d(%%rbp), %%rcx", offset);
            cg_emit(cg, "    lock xchgq %%rax, (%%rcx)");
        } else {
            cg_emit(cg, "    lock xchgq %%rax, -%d(%%rbp)", offset);
        }
    }
}

// Atomic compare-and-swap for read-modify-write operations
static void cg_atomic_rmw_start(CodeGen* cg, const char* name, int offset) {
    // Load address into a register for CAS loop
    if (cg->arch == ARCH_ARM64) {
        if (cg->in_thread_context) {
            cg_emit_ldr_offset(cg, "x19", 16 + offset);  // x19 = address
        } else {
            cg_emit_add_offset(cg, "x19", 16 + offset);
        }
        cg_emit(cg, "    ldar x0, [x19]");  // Load current value
    } else {
        if (cg->in_thread_context) {
            cg_emit(cg, "    movq -%d(%%rbp), %%r12", offset);  // r12 = address
            cg_emit(cg, "    movq (%%r12), %%rax");  // Load current value
        } else {
            cg_emit(cg, "    leaq -%d(%%rbp), %%r12", offset);  // r12 = address
            cg_emit(cg, "    movq (%%r12), %%rax");
        }
    }
}

static void cg_atomic_rmw_end(CodeGen* cg, const char* name, int offset) {
    // Complete CAS loop: retry if value changed
    int retry_lbl = cg_new_label(cg);
    int done_lbl = cg_new_label(cg);
    
    if (cg->arch == ARCH_ARM64) {
        cg_emit(cg, "L_cas_%d:", retry_lbl);
        cg_emit(cg, "    mov x1, x0");  // x1 = old value
        cg_emit(cg, "    mov x2, x20");  // x20 = new value (computed)
        cg_emit(cg, "    ldxr x3, [x19]");
        cg_emit(cg, "    cmp x3, x1");
        cg_emit(cg, "    b.ne L_cas_fail_%d", retry_lbl);
        cg_emit(cg, "    stxr w4, x2, [x19]");
        cg_emit(cg, "    cbnz w4, L_cas_%d", retry_lbl);
        cg_emit(cg, "    b L_cas_done_%d", done_lbl);
        cg_emit(cg, "L_cas_fail_%d:", retry_lbl);
        cg_emit(cg, "    clrex");
        cg_emit(cg, "    mov x0, x3");  // Reload and retry
        cg_emit(cg, "    b L_cas_%d", retry_lbl);
        cg_emit(cg, "L_cas_done_%d:", done_lbl);
        cg_emit(cg, "    mov x0, x2");  // Result in x0
    } else {
        cg_emit(cg, "L_cas_%d:", retry_lbl);
        cg_emit(cg, "    movq %%rax, %%rcx");  // rcx = old value
        cg_emit(cg, "    movq %%r13, %%rdx");  // r13 = new value (computed)
        cg_emit(cg, "    lock cmpxchgq %%rdx, (%%r12)");
        cg_emit(cg, "    jne L_cas_%d", retry_lbl);
        cg_emit(cg, "    movq %%rdx, %%rax");  // Result in rax
    }
}

static Type* cg_local_type(CodeGen* cg, const char* name) {
    for (int i = cg->local_count - 1; i >= 0; i--) {
        if (strcmp(cg->locals[i].name, name) == 0) return cg->locals[i].type;
    }
    return NULL;
}

static FnDef* cg_lookup_fn(CodeGen* cg, const char* name) {
    for (int i = 0; i < cg->module->function_count; i++) {
        if (strcmp(cg->module->functions[i].name, name) == 0) return &cg->module->functions[i];
    }
    return NULL;
}

// Forward declarations for variable capture
static void find_used_vars_stmt(Stmt* s, char vars[][MAX_IDENT_LEN], int* count, int max);

// Mark variables as shared if used in spawn blocks
static void mark_shared_vars(CodeGen* cg, Stmt** body, int body_count) {
    char used_vars[64][MAX_IDENT_LEN];
    int used_count = 0;
    
    // Find all variables used in spawn body
    for (int i = 0; i < body_count; i++) {
        find_used_vars_stmt(body[i], used_vars, &used_count, 64);
    }
    
    // Mark them as shared in locals
    for (int i = 0; i < used_count; i++) {
        for (int j = 0; j < cg->local_count; j++) {
            if (strcmp(cg->locals[j].name, used_vars[i]) == 0) {
                cg->locals[j].is_shared = true;
            }
        }
    }
}

// Infer expression type for codegen (simplified)
static bool cg_is_float_expr(CodeGen* cg, Expr* e) {
    if (!e) return false;
    switch (e->kind) {
        case EXPR_FLOAT: return true;
        case EXPR_INT: return false;
        case EXPR_IDENT: {
            Type* t = cg_local_type(cg, e->ident);
            return t && t->kind == TYPE_FLOAT;
        }
        case EXPR_BINARY:
            return cg_is_float_expr(cg, e->binary.left) || cg_is_float_expr(cg, e->binary.right);
        case EXPR_CALL:
            // Check for float conversion builtins
            if (e->call.func->kind == EXPR_IDENT) {
                if (strcmp(e->call.func->ident, "int_to_float") == 0) return true;
                if (strcmp(e->call.func->ident, "float_to_int") == 0) return false;
            }
            // Check for .to_float() method
            if (e->call.func->kind == EXPR_FIELD) {
                const char* method = e->call.func->field.field;
                if (strcmp(method, "to_float") == 0) return true;
            }
            return false;
        default: return false;
    }
}

static void cg_expr(CodeGen* cg, Expr* e);

static void cg_expr(CodeGen* cg, Expr* e) {
    if (!e) return;
    
    if (cg->arch == ARCH_X86_64) {
        // x86_64 code generation
        switch (e->kind) {
            case EXPR_INT:
                cg_emit(cg, "    movq $%lld, %%rax", (long long)e->int_val);
                break;
            case EXPR_BOOL:
                cg_emit(cg, "    movq $%d, %%rax", e->bool_val ? 1 : 0);
                break;
            case EXPR_STRING: {
                int lbl = cg->label_count++;
                cg->strings[cg->string_count].str = e->str_val;
                cg->strings[cg->string_count].label = lbl;
                cg->string_count++;
                cg_emit(cg, "    leaq L_.str%d(%%rip), %%rax", lbl);
                break;
            }
            case EXPR_IDENT: {
                bool is_enum = false;
                int64_t enum_val = cg_lookup_enum_variant(cg, e->ident, &is_enum);
                if (is_enum) {
                    cg_emit(cg, "    movq $%lld, %%rax", (long long)enum_val);
                } else {
                    int off = cg_local_offset(cg, e->ident);
                    if (off) {
                        if (cg_is_shared(cg, e->ident)) {
                            cg_atomic_load(cg, e->ident, off);
                        } else {
                            cg_emit(cg, "    movq -%d(%%rbp), %%rax", off);
                        }
                    }
                }
                break;
            }
            case EXPR_BINARY: {
                // Check for string concatenation
                if (e->binary.op == TOK_PLUS) {
                    bool left_is_str = (e->binary.left->kind == EXPR_STRING);
                    bool right_is_str = (e->binary.right->kind == EXPR_STRING);
                    // Check if variables are string type
                    if (e->binary.left->kind == EXPR_IDENT) {
                        Type* t = cg_local_type(cg, e->binary.left->ident);
                        if (t && t->kind == TYPE_STR) left_is_str = true;
                    }
                    if (e->binary.right->kind == EXPR_IDENT) {
                        Type* t = cg_local_type(cg, e->binary.right->ident);
                        if (t && t->kind == TYPE_STR) right_is_str = true;
                    }
                    // Check for chained concat: (str + str) + str
                    if (e->binary.left->kind == EXPR_BINARY && e->binary.left->binary.op == TOK_PLUS) {
                        Expr* l = e->binary.left;
                        while (l->kind == EXPR_BINARY && l->binary.op == TOK_PLUS) {
                            if (l->binary.right->kind == EXPR_STRING) { left_is_str = true; break; }
                            if (l->binary.left->kind == EXPR_STRING) { left_is_str = true; break; }
                            l = l->binary.left;
                        }
                    }
                    
                    if (left_is_str || right_is_str) {
                        // Allocate a fixed buffer on stack for result (256 bytes should be enough)
                        int buf_off = cg->stack_offset + 8;
                        cg->stack_offset += 256;
                        
                        // Evaluate left string
                        cg_expr(cg, e->binary.left);
                        cg_emit(cg, "    pushq %%rax");  // save left ptr
                        
                        // Evaluate right string
                        cg_expr(cg, e->binary.right);
                        cg_emit(cg, "    pushq %%rax");  // save right ptr
                        
                        // Copy left string to buffer
                        cg_emit(cg, "    movq 8(%%rsp), %%rsi");  // left ptr
                        cg_emit(cg, "    leaq -%d(%%rbp), %%rdi", buf_off);  // dest buffer
                        int copy1 = cg_new_label(cg);
                        cg_emit(cg, "L%d:", copy1);
                        cg_emit(cg, "    lodsb");
                        cg_emit(cg, "    stosb");
                        cg_emit(cg, "    testb %%al, %%al");
                        cg_emit(cg, "    jnz L%d", copy1);
                        cg_emit(cg, "    decq %%rdi");  // back up over null terminator
                        
                        // Copy right string (including null terminator)
                        cg_emit(cg, "    popq %%rsi");  // right ptr
                        int copy2 = cg_new_label(cg);
                        cg_emit(cg, "L%d:", copy2);
                        cg_emit(cg, "    lodsb");
                        cg_emit(cg, "    stosb");
                        cg_emit(cg, "    testb %%al, %%al");
                        cg_emit(cg, "    jnz L%d", copy2);
                        
                        cg_emit(cg, "    addq $8, %%rsp");  // clean up left ptr
                        cg_emit(cg, "    leaq -%d(%%rbp), %%rax", buf_off);  // return buffer ptr
                        break;
                    }
                }
                // Check for string comparison
                if ((e->binary.op == TOK_EQEQ || e->binary.op == TOK_NOTEQ)) {
                    if (e->binary.left->kind == EXPR_STRING && e->binary.right->kind == EXPR_STRING) {
                        int lbl1 = cg->label_count++, lbl2 = cg->label_count++, loop_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                        cg->strings[cg->string_count].str = e->binary.left->str_val;
                        cg->strings[cg->string_count].label = lbl1;
                        cg->string_count++;
                        cg->strings[cg->string_count].str = e->binary.right->str_val;
                        cg->strings[cg->string_count].label = lbl2;
                        cg->string_count++;
                        cg_emit(cg, "    leaq L_.str%d(%%rip), %%rsi", lbl1);
                        cg_emit(cg, "    leaq L_.str%d(%%rip), %%rdi", lbl2);
                        cg_emit(cg, "L%d:", loop_lbl);
                        cg_emit(cg, "    movb (%%rsi), %%al");
                        cg_emit(cg, "    movb (%%rdi), %%cl");
                        cg_emit(cg, "    cmpb %%cl, %%al");
                        cg_emit(cg, "    jne L%d", end_lbl);
                        cg_emit(cg, "    testb %%al, %%al");
                        cg_emit(cg, "    jz L%d", end_lbl);
                        cg_emit(cg, "    incq %%rsi");
                        cg_emit(cg, "    incq %%rdi");
                        cg_emit(cg, "    jmp L%d", loop_lbl);
                        cg_emit(cg, "L%d:", end_lbl);
                        cg_emit(cg, "    set%s %%al", e->binary.op == TOK_EQEQ ? "e" : "ne");
                        cg_emit(cg, "    movzbq %%al, %%rax");
                        break;
                    }
                }
                
                // Handle 'in' operator: elem in array
                if (e->binary.op == TOK_IN) {
                    // Evaluate element to search for
                    cg_expr(cg, e->binary.left);
                    cg_emit(cg, "    pushq %%rax");  // save element
                    // Evaluate array
                    cg_expr(cg, e->binary.right);
                    cg_emit(cg, "    movq %%rax, %%rsi");  // array ptr
                    cg_emit(cg, "    popq %%rdx");  // element to find
                    cg_emit(cg, "    movq (%%rsi), %%rcx");  // array length
                    cg_emit(cg, "    addq $8, %%rsi");  // skip length field
                    cg_emit(cg, "    xorq %%rax, %%rax");  // result = false
                    int loop_lbl = cg_new_label(cg), found_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                    cg_emit(cg, "L%d:", loop_lbl);
                    cg_emit(cg, "    testq %%rcx, %%rcx");
                    cg_emit(cg, "    jz L%d", end_lbl);
                    cg_emit(cg, "    cmpq %%rdx, (%%rsi)");
                    cg_emit(cg, "    je L%d", found_lbl);
                    cg_emit(cg, "    addq $8, %%rsi");
                    cg_emit(cg, "    decq %%rcx");
                    cg_emit(cg, "    jmp L%d", loop_lbl);
                    cg_emit(cg, "L%d:", found_lbl);
                    cg_emit(cg, "    movq $1, %%rax");
                    cg_emit(cg, "L%d:", end_lbl);
                    break;
                }
                
                // Check if this is a float operation
                bool is_float = cg_is_float_expr(cg, e);
                
                cg_expr(cg, e->binary.right);
                cg_emit(cg, "    pushq %%rax");
                cg_expr(cg, e->binary.left);
                cg_emit(cg, "    popq %%rcx");
                
                if (is_float) {
                    // Float arithmetic: convert bit patterns to xmm0/xmm1
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_emit(cg, "    movq %%rcx, %%xmm1");
                    switch (e->binary.op) {
                        case TOK_PLUS:  cg_emit(cg, "    addsd %%xmm1, %%xmm0"); break;
                        case TOK_MINUS: cg_emit(cg, "    subsd %%xmm1, %%xmm0"); break;
                        case TOK_STAR:  cg_emit(cg, "    mulsd %%xmm1, %%xmm0"); break;
                        case TOK_SLASH: cg_emit(cg, "    divsd %%xmm1, %%xmm0"); break;
                        case TOK_LT: case TOK_GT: case TOK_LTEQ: case TOK_GTEQ:
                        case TOK_EQEQ: case TOK_NOTEQ:
                            cg_emit(cg, "    ucomisd %%xmm1, %%xmm0");
                            cg_emit(cg, "    set%s %%al",
                                e->binary.op == TOK_EQEQ ? "e" : e->binary.op == TOK_NOTEQ ? "ne" :
                                e->binary.op == TOK_LT ? "b" : e->binary.op == TOK_GT ? "a" :
                                e->binary.op == TOK_LTEQ ? "be" : "ae");
                            cg_emit(cg, "    movzbq %%al, %%rax");
                            break;
                        default:
                            goto x86_int_arith;
                    }
                    // For arithmetic ops, move result back to rax
                    if (e->binary.op == TOK_PLUS || e->binary.op == TOK_MINUS ||
                        e->binary.op == TOK_STAR || e->binary.op == TOK_SLASH) {
                        cg_emit(cg, "    movq %%xmm0, %%rax");
                    }
                    break;
                }
                
                x86_int_arith:
                switch (e->binary.op) {
                    case TOK_PLUS:  cg_emit(cg, "    addq %%rcx, %%rax"); break;
                    case TOK_MINUS: cg_emit(cg, "    subq %%rcx, %%rax"); break;
                    case TOK_STAR:  cg_emit(cg, "    imulq %%rcx, %%rax"); break;
                    case TOK_SLASH: cg_emit(cg, "    cqo"); cg_emit(cg, "    idivq %%rcx"); break;
                    case TOK_PERCENT: cg_emit(cg, "    cqo"); cg_emit(cg, "    idivq %%rcx"); cg_emit(cg, "    movq %%rdx, %%rax"); break;
                    case TOK_EQEQ: case TOK_NOTEQ: case TOK_LT: case TOK_GT: case TOK_LTEQ: case TOK_GTEQ:
                        cg_emit(cg, "    cmpq %%rcx, %%rax");
                        cg_emit(cg, "    set%s %%al", 
                            e->binary.op == TOK_EQEQ ? "e" : e->binary.op == TOK_NOTEQ ? "ne" :
                            e->binary.op == TOK_LT ? "l" : e->binary.op == TOK_GT ? "g" :
                            e->binary.op == TOK_LTEQ ? "le" : "ge");
                        cg_emit(cg, "    movzbq %%al, %%rax");
                        break;
                    case TOK_AND:
                        cg_emit(cg, "    testq %%rax, %%rax");
                        cg_emit(cg, "    setne %%al");
                        cg_emit(cg, "    testq %%rcx, %%rcx");
                        cg_emit(cg, "    setne %%cl");
                        cg_emit(cg, "    andb %%cl, %%al");
                        cg_emit(cg, "    movzbq %%al, %%rax");
                        break;
                    case TOK_OR:
                        cg_emit(cg, "    orq %%rcx, %%rax");
                        cg_emit(cg, "    testq %%rax, %%rax");
                        cg_emit(cg, "    setne %%al");
                        cg_emit(cg, "    movzbq %%al, %%rax");
                        break;
                    case TOK_PIPE: cg_emit(cg, "    orq %%rcx, %%rax"); break;
                    case TOK_AMPERSAND: cg_emit(cg, "    andq %%rcx, %%rax"); break;
                    case TOK_CARET: cg_emit(cg, "    xorq %%rcx, %%rax"); break;
                    case TOK_LTLT: cg_emit(cg, "    shlq %%cl, %%rax"); break;
                    case TOK_GTGT: cg_emit(cg, "    sarq %%cl, %%rax"); break;
                    case TOK_STARSTAR: {
                        // Power: loop-based implementation
                        int loop_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                        cg_emit(cg, "    movq %%rax, %%rsi");  // base
                        cg_emit(cg, "    movq %%rcx, %%rdx");  // exp
                        cg_emit(cg, "    movq $1, %%rax");     // result
                        cg_emit(cg, "L%d:", loop_lbl);
                        cg_emit(cg, "    testq %%rdx, %%rdx");
                        cg_emit(cg, "    jz L%d", end_lbl);
                        cg_emit(cg, "    imulq %%rsi, %%rax");
                        cg_emit(cg, "    decq %%rdx");
                        cg_emit(cg, "    jmp L%d", loop_lbl);
                        cg_emit(cg, "L%d:", end_lbl);
                        break;
                    }
                    default: cg_emit(cg, "    # unsupported op");
                }
                break;
            }
            case EXPR_UNARY:
                cg_expr(cg, e->unary.operand);
                if (e->unary.op == TOK_MINUS) cg_emit(cg, "    negq %%rax");
                else if (e->unary.op == TOK_TILDE) cg_emit(cg, "    notq %%rax");
                else if (e->unary.op == TOK_NOT) {
                    cg_emit(cg, "    testq %%rax, %%rax");
                    cg_emit(cg, "    sete %%al");
                    cg_emit(cg, "    movzbq %%al, %%rax");
                }
                break;
            case EXPR_ADDR:
                // Address-of: get address of variable
                if (e->unary.operand->kind == EXPR_IDENT) {
                    int off = cg_local_offset(cg, e->unary.operand->ident);
                    if (off) cg_emit(cg, "    leaq -%d(%%rbp), %%rax", off);
                } else {
                    cg_expr(cg, e->unary.operand);  // For other expressions, just evaluate
                }
                break;
            case EXPR_DEREF:
                // Dereference: load value at pointer
                cg_expr(cg, e->unary.operand);
                cg_emit(cg, "    movq (%%rax), %%rax");
                break;
            case EXPR_UNWRAP:
                // Unwrap optional: get value (second word)
                cg_expr(cg, e->unary.operand);
                cg_emit(cg, "    movq 8(%%rax), %%rax");
                break;
            case EXPR_CALL: {
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "len") == 0) {
                    // Check type to determine array vs string
                    Expr* arg = e->call.args[0];
                    Type* arg_type = NULL;
                    if (arg->kind == EXPR_IDENT) {
                        arg_type = cg_local_type(cg, arg->ident);
                    } else if (arg->kind == EXPR_ARRAY) {
                        arg_type = new_type(TYPE_ARRAY);
                    }
                    bool is_array = (arg_type && arg_type->kind == TYPE_ARRAY);
                    if (is_array) {
                        // Array - read length from first element
                        cg_expr(cg, arg);
                        cg_emit(cg, "    movq (%%rax), %%rax");
                    } else {
                        // String - count chars until null terminator
                        cg_expr(cg, arg);
                        int loop_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                        cg_emit(cg, "    movq %%rax, %%rsi");
                        cg_emit(cg, "    xorq %%rax, %%rax");
                        cg_emit(cg, "L%d:", loop_lbl);
                        cg_emit(cg, "    cmpb $0, (%%rsi)");
                        cg_emit(cg, "    je L%d", end_lbl);
                        cg_emit(cg, "    incq %%rax");
                        cg_emit(cg, "    incq %%rsi");
                        cg_emit(cg, "    jmp L%d", loop_lbl);
                        cg_emit(cg, "L%d:", end_lbl);
                    }
                    break;
                }
                // Handle built-in abs()
                if (is_builtin_call(e->call.func, "abs")) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rcx");
                    cg_emit(cg, "    negq %%rax");
                    cg_emit(cg, "    cmovlq %%rcx, %%rax");
                    break;
                }
                // Handle built-in min()
                if (is_builtin_call(e->call.func, "min")) {
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    popq %%rcx");
                    cg_emit(cg, "    cmpq %%rcx, %%rax");
                    cg_emit(cg, "    cmovgq %%rcx, %%rax");
                    break;
                }
                // Handle built-in max()
                if (is_builtin_call(e->call.func, "max")) {
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    popq %%rcx");
                    cg_emit(cg, "    cmpq %%rcx, %%rax");
                    cg_emit(cg, "    cmovlq %%rcx, %%rax");
                    break;
                }
                // Handle built-in int_to_str()
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "int_to_str") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdx");
                    cg_emit(cg, "    leaq L_.itsbuf(%%rip), %%rdi");
                    cg_emit(cg, "    leaq L_.itsfmt(%%rip), %%rsi");
                    cg_emit(cg, "    xorl %%eax, %%eax");
                    cg_emit(cg, "    callq %ssprintf", cg_sym_prefix(cg));
                    cg_emit(cg, "    leaq L_.itsbuf(%%rip), %%rax");
                    break;
                }
                // Handle built-in str_to_int()
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "str_to_int") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    xorl %%esi, %%esi");
                    cg_emit(cg, "    movl $10, %%edx");
                    cg_emit(cg, "    callq %sstrtol", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in ord() - get ASCII code of first char
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "ord") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movzbl (%%rax), %%eax");  // Load first byte, zero-extend
                    break;
                }
                // Handle built-in chr() - create single-char string
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "chr") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    leaq L_.chrbuf(%%rip), %%rcx");
                    cg_emit(cg, "    movb %%al, (%%rcx)");
                    cg_emit(cg, "    movb $0, 1(%%rcx)");
                    cg_emit(cg, "    movq %%rcx, %%rax");
                    break;
                }
                // Handle built-in int_to_float() - convert int to float
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "int_to_float") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    cvtsi2sdq %%rax, %%xmm0");  // Convert int to double
                    cg_emit(cg, "    movq %%xmm0, %%rax");       // Move to rax for return
                    break;
                }
                // Handle built-in float_to_int() - convert float to int
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "float_to_int") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");       // Move to xmm0
                    cg_emit(cg, "    cvttsd2siq %%xmm0, %%rax"); // Convert to int (truncate)
                    break;
                }
                // Handle built-in read_file() - reads entire file to string
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "read_file") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %s_wyn_read_file", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in write_file(path, content) - writes string to file
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "write_file") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    movq %%rax, %%rsi");
                    cg_emit(cg, "    popq %%rdi");
                    cg_emit(cg, "    callq %s_wyn_write_file", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in read_line() - reads a line from stdin
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "read_line") == 0) {
                    cg_emit(cg, "    callq %s_wyn_read_line", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in file_exists(path) - check if file exists
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "file_exists") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %s_wyn_file_exists", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in delete_file(path) - delete a file
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "delete_file") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %s_wyn_delete_file", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in mkdir(path) - create directory
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "mkdir") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %s_wyn_mkdir", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in rmdir(path) - remove directory
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "rmdir") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %s_wyn_rmdir", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in append_file(path, content) - append to file
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "append_file") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    movq %%rax, %%rsi");
                    cg_emit(cg, "    popq %%rdi");
                    cg_emit(cg, "    callq %s_wyn_append_file", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in file_size(path) -> int
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "file_size") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %s_wyn_file_size", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in is_dir(path) -> bool
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "is_dir") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %s_wyn_is_dir", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in time_now() - returns Unix timestamp
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "time_now") == 0) {
                    cg_emit(cg, "    xorl %%edi, %%edi");
                    cg_emit(cg, "    callq %stime", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in sleep_ms(ms) - sleep for milliseconds
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "sleep_ms") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    imulq $1000, %%rax, %%rdi");
                    cg_emit(cg, "    callq %susleep", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in tcp_connect(host, port) - connect to TCP socket
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "tcp_connect") == 0) {
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    popq %%rsi");
                    cg_emit(cg, "    callq %s_wyn_tcp_connect", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in tcp_close(sock) - close TCP socket
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "tcp_close") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movl %%eax, %%edi");
                    cg_emit(cg, "    callq %sclose", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in char_at(s, i) - get character at index
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "char_at") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    movq %%rax, %%rsi");
                    cg_emit(cg, "    popq %%rdi");
                    cg_emit(cg, "    callq %s_wyn_char_at", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in clock() - returns CPU time in microseconds
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "clock") == 0) {
                    cg_emit(cg, "    callq %sclock", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in random() - returns random integer
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "random") == 0) {
                    cg_emit(cg, "    callq %srand", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in sleep(seconds) - pause execution
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "sleep") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movl %%eax, %%edi");
                    cg_emit(cg, "    callq %ssleep", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in getenv(name) - get environment variable
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "getenv") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %sgetenv", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in args() - get command line arguments as [str]
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "args") == 0) {
                    // Allocate array on stack: [len, capacity, elem0, elem1, ...]
                    int arr_off = cg->stack_offset + 8;
                    cg->stack_offset += 16 + 8 * 64;  // len + capacity + max 64 args
                    // Load argc and argv from globals
                    cg_emit(cg, "    movq L_.wyn_argc(%%rip), %%r8");  // r8 = argc
                    cg_emit(cg, "    movq L_.wyn_argv(%%rip), %%r9");  // r9 = argv
                    cg_emit(cg, "    leaq -%d(%%rbp), %%r10", arr_off);  // r10 = array base
                    cg_emit(cg, "    movq %%r8, (%%r10)");  // store length
                    cg_emit(cg, "    movq %%r8, 8(%%r10)");  // store capacity
                    cg_emit(cg, "    xorq %%r11, %%r11");  // i = 0
                    int loop_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                    cg_emit(cg, "L%d:", loop_lbl);
                    cg_emit(cg, "    cmpq %%r8, %%r11");
                    cg_emit(cg, "    jge L%d", end_lbl);
                    cg_emit(cg, "    movq (%%r9,%%r11,8), %%r12");  // argv[i]
                    cg_emit(cg, "    leaq 2(%%r11), %%r13");  // +2 for len and capacity
                    cg_emit(cg, "    movq %%r12, (%%r10,%%r13,8)");  // arr[i+2]
                    cg_emit(cg, "    incq %%r11");
                    cg_emit(cg, "    jmp L%d", loop_lbl);
                    cg_emit(cg, "L%d:", end_lbl);
                    cg_emit(cg, "    leaq -%d(%%rbp), %%rax", arr_off);  // return array ptr
                    break;
                }
                // Handle built-in setenv(name, value) - set environment variable
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "setenv") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    movq %%rax, %%rsi");
                    cg_emit(cg, "    popq %%rdi");
                    cg_emit(cg, "    xorq %%rdx, %%rdx");  // overwrite=0
                    cg_emit(cg, "    callq %ssetenv", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in environ() - get all environment variables
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "environ") == 0) {
                    cg_emit(cg, "    leaq environ(%%rip), %%rax");
                    break;
                }
                // Handle built-in system(cmd) - run shell command
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "system") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %ssystem", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in str_find(haystack, needle) - returns index or -1
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "str_find") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    movq %%rax, %%rsi");
                    cg_emit(cg, "    popq %%rdi");
                    cg_emit(cg, "    pushq %%rdi");  // save haystack for offset calc
                    cg_emit(cg, "    callq %sstrstr", cg_sym_prefix(cg));
                    cg_emit(cg, "    popq %%rdi");
                    cg_emit(cg, "    testq %%rax, %%rax");
                    int found_lbl = cg_new_label(cg);
                    cg_emit(cg, "    jnz L%d", found_lbl);
                    cg_emit(cg, "    movq $-1, %%rax");
                    int end_lbl = cg_new_label(cg);
                    cg_emit(cg, "    jmp L%d", end_lbl);
                    cg_emit(cg, "L%d:", found_lbl);
                    cg_emit(cg, "    subq %%rdi, %%rax");  // offset = result - haystack
                    cg_emit(cg, "L%d:", end_lbl);
                    break;
                }
                // Handle built-in str_cmp(a, b) - returns 0 if equal, <0 or >0 otherwise
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "str_cmp") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    movq %%rax, %%rsi");
                    cg_emit(cg, "    popq %%rdi");
                    cg_emit(cg, "    callq %sstrcmp", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in str_concat(a, b) -> str
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "str_concat") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_emit(cg, "    movq 8(%%rsp), %%rdi");
                    cg_emit(cg, "    callq %sstrlen", cg_sym_prefix(cg));
                    cg_emit(cg, "    pushq %%rax");
                    cg_emit(cg, "    movq 8(%%rsp), %%rdi");
                    cg_emit(cg, "    callq %sstrlen", cg_sym_prefix(cg));
                    cg_emit(cg, "    popq %%rdi");
                    cg_emit(cg, "    addq %%rdi, %%rax");
                    cg_emit(cg, "    incq %%rax");
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %smalloc", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    movq 16(%%rsp), %%rsi");
                    cg_emit(cg, "    callq %sstrcpy", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    movq (%%rsp), %%rsi");
                    cg_emit(cg, "    callq %sstrcat", cg_sym_prefix(cg));
                    cg_emit(cg, "    addq $24, %%rsp");
                    break;
                }
                // Handle built-in str_substr(s, start, len) -> str
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "str_substr") == 0) {
                    cg_expr(cg, e->call.args[2]);
                    cg_emit(cg, "    incq %%rax");
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %smalloc", cg_sym_prefix(cg));
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    popq %%rsi");
                    cg_emit(cg, "    addq %%rax, %%rsi");
                    cg_expr(cg, e->call.args[2]);
                    cg_emit(cg, "    movq %%rax, %%rdx");
                    cg_emit(cg, "    movq (%%rsp), %%rdi");
                    cg_emit(cg, "    callq %smemcpy", cg_sym_prefix(cg));
                    cg_expr(cg, e->call.args[2]);
                    cg_emit(cg, "    movq (%%rsp), %%rdi");
                    cg_emit(cg, "    addq %%rax, %%rdi");
                    cg_emit(cg, "    movb $0, (%%rdi)");
                    cg_emit(cg, "    popq %%rax");
                    break;
                }
                // Handle built-in str_split(s, delim) -> [str]
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "str_split") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %sstrlen", cg_sym_prefix(cg));
                    cg_emit(cg, "    incq %%rax");
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %smalloc", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%rax, %%r12");
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rsi");
                    cg_emit(cg, "    movq %%r12, %%rdi");
                    cg_emit(cg, "    callq %sstrcpy", cg_sym_prefix(cg));
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_emit(cg, "    movq $136, %%rdi");
                    cg_emit(cg, "    callq %smalloc", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq $0, (%%rax)");
                    cg_emit(cg, "    movq $16, 8(%%rax)");
                    cg_emit(cg, "    pushq %%rax");
                    int loop_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                    cg_emit(cg, "L%d:", loop_lbl);
                    cg_emit(cg, "    movq %%r12, %%rdi");
                    cg_emit(cg, "    movq 8(%%rsp), %%rsi");
                    cg_emit(cg, "    callq %sstrstr", cg_sym_prefix(cg));
                    cg_emit(cg, "    testq %%rax, %%rax");
                    cg_emit(cg, "    jz L%d", end_lbl);
                    cg_emit(cg, "    movq %%rax, %%r13");
                    cg_emit(cg, "    movb $0, (%%r13)");
                    cg_emit(cg, "    movq (%%rsp), %%rdi");
                    cg_emit(cg, "    movq (%%rdi), %%rsi");
                    cg_emit(cg, "    addq $2, %%rsi");
                    cg_emit(cg, "    movq %%r12, (%%rdi,%%rsi,8)");
                    cg_emit(cg, "    incq (%%rdi)");
                    cg_emit(cg, "    movq 8(%%rsp), %%rdi");
                    cg_emit(cg, "    callq %sstrlen", cg_sym_prefix(cg));
                    cg_emit(cg, "    leaq 1(%%r13,%%rax), %%r12");
                    cg_emit(cg, "    jmp L%d", loop_lbl);
                    cg_emit(cg, "L%d:", end_lbl);
                    cg_emit(cg, "    movq (%%rsp), %%rdi");
                    cg_emit(cg, "    movq (%%rdi), %%rsi");
                    cg_emit(cg, "    addq $2, %%rsi");
                    cg_emit(cg, "    movq %%r12, (%%rdi,%%rsi,8)");
                    cg_emit(cg, "    incq (%%rdi)");
                    cg_emit(cg, "    popq %%rax");
                    cg_emit(cg, "    addq $8, %%rsp");
                    break;
                }
                // Handle built-in tcp_send(fd, data) - returns bytes sent
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "tcp_send") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    movq %%rax, %%rsi");  // buf
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    pushq %%rsi");
                    // Get string length
                    cg_emit(cg, "    xorl %%eax, %%eax");
                    cg_emit(cg, "    movq $-1, %%rcx");
                    cg_emit(cg, "    repnz scasb");
                    cg_emit(cg, "    notq %%rcx");
                    cg_emit(cg, "    decq %%rcx");
                    cg_emit(cg, "    movq %%rcx, %%rdx");  // len
                    cg_emit(cg, "    popq %%rsi");
                    cg_emit(cg, "    popq %%rdi");  // fd
                    cg_emit(cg, "    xorl %%ecx, %%ecx");  // flags = 0
                    cg_emit(cg, "    callq %ssend", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in tcp_recv(fd, maxlen) - returns string
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "tcp_recv") == 0) {
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    pushq %%rax");  // maxlen
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    pushq %%rax");  // fd
                    cg_emit(cg, "    movq 8(%%rsp), %%rdi");  // maxlen for malloc
                    cg_emit(cg, "    addq $1, %%rdi");  // +1 for null
                    cg_emit(cg, "    callq %smalloc", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%rax, %%r12");  // save buf ptr
                    cg_emit(cg, "    popq %%rdi");  // fd
                    cg_emit(cg, "    movq %%r12, %%rsi");  // buf
                    cg_emit(cg, "    popq %%rdx");  // maxlen
                    cg_emit(cg, "    xorl %%ecx, %%ecx");  // flags = 0
                    cg_emit(cg, "    callq %srecv", cg_sym_prefix(cg));
                    cg_emit(cg, "    movb $0, (%%r12, %%rax)");  // null terminate
                    cg_emit(cg, "    movq %%r12, %%rax");  // return buf
                    break;
                }
                // Handle built-in tcp_listen(port) - returns listening fd
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "tcp_listen") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %s_wyn_tcp_listen", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in tcp_accept(fd) - returns client fd
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "tcp_accept") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movl %%eax, %%edi");
                    cg_emit(cg, "    xorl %%esi, %%esi");  // NULL
                    cg_emit(cg, "    xorl %%edx, %%edx");  // NULL
                    cg_emit(cg, "    callq %saccept", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in getcwd() - returns current directory
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "getcwd") == 0) {
                    cg_emit(cg, "    movq $1024, %%rdi");
                    cg_emit(cg, "    callq %smalloc", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    movq $1024, %%rsi");
                    cg_emit(cg, "    callq %sgetcwd", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in chdir(path) - change directory, returns 0 on success
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "chdir") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %schdir", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in sqrt(float) -> float
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "sqrtf") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_emit(cg, "    sqrtsd %%xmm0, %%xmm0");
                    cg_emit(cg, "    movq %%xmm0, %%rax");
                    break;
                }
                // Handle built-in sin(float) -> float
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "sinf") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_emit(cg, "    callq %ssin", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%xmm0, %%rax");
                    break;
                }
                // Handle built-in cos(float) -> float
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "cosf") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_emit(cg, "    callq %scos", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%xmm0, %%rax");
                    break;
                }
                // Handle built-in tan(float) -> float
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "tanf") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_emit(cg, "    callq %stan", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%xmm0, %%rax");
                    break;
                }
                // Handle built-in log(float) -> float (natural log)
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "logf") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_emit(cg, "    callq %slog", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%xmm0, %%rax");
                    break;
                }
                // Handle built-in exp(float) -> float
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "expf") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_emit(cg, "    callq %sexp", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%xmm0, %%rax");
                    break;
                }
                // Handle built-in powf(base, exp) -> float
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "powf") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    movq %%rax, %%xmm1");
                    cg_emit(cg, "    callq %spow", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%xmm0, %%rax");
                    break;
                }
                // Handle built-in floor(float) -> float
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "floorf") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_emit(cg, "    roundsd $1, %%xmm0, %%xmm0");
                    cg_emit(cg, "    movq %%xmm0, %%rax");
                    break;
                }
                // Handle built-in ceil(float) -> float
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "ceilf") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_emit(cg, "    roundsd $2, %%xmm0, %%xmm0");
                    cg_emit(cg, "    movq %%xmm0, %%rax");
                    break;
                }
                // Handle built-in asin(float) -> float
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "asinf") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_emit(cg, "    callq %sasin", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%xmm0, %%rax");
                    break;
                }
                // Handle built-in acos(float) -> float
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "acosf") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_emit(cg, "    callq %sacos", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%xmm0, %%rax");
                    break;
                }
                // Handle built-in atan(float) -> float
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "atanf") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_emit(cg, "    callq %satan", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%xmm0, %%rax");
                    break;
                }
                // Handle built-in atan2(y, x) -> float
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "atan2f") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    movq %%rax, %%xmm1");
                    cg_emit(cg, "    callq %satan2", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%xmm0, %%rax");
                    break;
                }
                // Handle built-in log10(float) -> float
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "log10f") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%xmm0");
                    cg_emit(cg, "    callq %slog10", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%xmm0, %%rax");
                    break;
                }
                // Handle built-in getpid() -> int
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "getpid") == 0) {
                    cg_emit(cg, "    callq %sgetpid", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in rename_file(old, new) -> int (0 on success)
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "rename_file") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    movq %%rax, %%rsi");
                    cg_emit(cg, "    popq %%rdi");
                    cg_emit(cg, "    callq %srename", cg_sym_prefix(cg));
                    break;
                }
                // Handle built-in to_string(val) - convert to string for interpolation
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "to_string") == 0) {
                    Expr* arg = e->call.args[0];
                    Type* t = cg_local_type(cg, arg->kind == EXPR_IDENT ? arg->ident : NULL);
                    cg_expr(cg, arg);
                    // If it's a string type, just return it
                    if (t && t->kind == TYPE_STR) {
                        break;  // already a string
                    }
                    // Allocate buffer and convert int to string
                    cg_emit(cg, "    pushq %%rax");  // save value
                    cg_emit(cg, "    movq $24, %%rdi");
                    cg_emit(cg, "    callq %smalloc", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%rax, %%rdi");  // buffer
                    cg_emit(cg, "    leaq L_.itsfmt(%%rip), %%rsi");  // format
                    cg_emit(cg, "    popq %%rdx");  // value
                    cg_emit(cg, "    xorl %%eax, %%eax");
                    cg_emit(cg, "    pushq %%rdi");  // save buffer ptr
                    cg_emit(cg, "    callq %ssprintf", cg_sym_prefix(cg));
                    cg_emit(cg, "    popq %%rax");  // return buffer ptr
                    break;
                }
                // Handle built-in substring(str, start, len) -> str
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "substring") == 0) {
                    // Evaluate len
                    cg_expr(cg, e->call.args[2]);
                    cg_emit(cg, "    pushq %%rax");  // save len
                    // Evaluate start
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    pushq %%rax");  // save start
                    // Evaluate str
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    pushq %%rax");  // save str
                    // Allocate buffer: malloc(len + 1)
                    cg_emit(cg, "    movq 16(%%rsp), %%rdi");  // len
                    cg_emit(cg, "    addq $1, %%rdi");
                    cg_emit(cg, "    callq %smalloc", cg_sym_prefix(cg));
                    cg_emit(cg, "    movq %%rax, %%rdi");  // dest
                    cg_emit(cg, "    popq %%rsi");  // str
                    cg_emit(cg, "    popq %%rcx");  // start
                    cg_emit(cg, "    addq %%rcx, %%rsi");  // str + start
                    cg_emit(cg, "    popq %%rdx");  // len
                    cg_emit(cg, "    pushq %%rdi");  // save dest
                    cg_emit(cg, "    callq %smemcpy", cg_sym_prefix(cg));
                    cg_emit(cg, "    popq %%rax");  // return dest
                    break;
                }
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "assert") == 0) {
                    int ok_lbl = cg_new_label(cg);
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    testq %%rax, %%rax");
                    cg_emit(cg, "    jnz L%d", ok_lbl);
                    cg_emit(cg, "    movq $1, %%rdi");
                    cg_emit(cg, "    callq %sexit", cg_sym_prefix(cg));
                    cg_emit(cg, "L%d:", ok_lbl);
                    break;
                }
                // Handle built-in exit()
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "exit") == 0) {
                    if (e->call.arg_count > 0) {
                        cg_expr(cg, e->call.args[0]);
                        cg_emit(cg, "    movq %%rax, %%rdi");
                    } else {
                        cg_emit(cg, "    xorq %%rdi, %%rdi");
                    }
                    cg_emit(cg, "    callq %sexit", cg_sym_prefix(cg));
                    break;
                }
                // Built-in methods on primitive types (desugar to free functions)
                if (e->call.func->kind == EXPR_FIELD) {
                    const char* method = e->call.func->field.field;
                    Expr* obj = e->call.func->field.object;
                    
                    // .len() -> len(obj)
                    if (strcmp(method, "len") == 0 && e->call.arg_count == 0) {
                        cg_expr(cg, obj);
                        int loop_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                        cg_emit(cg, "    movq %%rax, %%rsi");
                        cg_emit(cg, "    xorq %%rax, %%rax");
                        cg_emit(cg, "L%d:", loop_lbl);
                        cg_emit(cg, "    movb (%%rsi,%%rax), %%cl");
                        cg_emit(cg, "    testb %%cl, %%cl");
                        cg_emit(cg, "    jz L%d", end_lbl);
                        cg_emit(cg, "    incq %%rax");
                        cg_emit(cg, "    jmp L%d", loop_lbl);
                        cg_emit(cg, "L%d:", end_lbl);
                        break;
                    }
                    // .abs() -> abs(obj)
                    if (strcmp(method, "abs") == 0 && e->call.arg_count == 0) {
                        cg_expr(cg, obj);
                        cg_emit(cg, "    movq %%rax, %%rcx");
                        cg_emit(cg, "    negq %%rax");
                        cg_emit(cg, "    cmovlq %%rcx, %%rax");
                        break;
                    }
                    // .to_str() -> int_to_str(obj)
                    if (strcmp(method, "to_str") == 0 && e->call.arg_count == 0) {
                        cg_expr(cg, obj);
                        cg_emit(cg, "    movq %%rax, %%rdx");
                        cg_emit(cg, "    leaq L_.itsbuf(%%rip), %%rdi");
                        cg_emit(cg, "    leaq L_.itsfmt(%%rip), %%rsi");
                        cg_emit(cg, "    xorl %%eax, %%eax");
                        cg_emit(cg, "    callq %ssprintf", cg_sym_prefix(cg));
                        cg_emit(cg, "    leaq L_.itsbuf(%%rip), %%rax");
                        break;
                    }
                    // .to_int() -> str_to_int(obj)
                    if (strcmp(method, "to_int") == 0 && e->call.arg_count == 0) {
                        cg_expr(cg, obj);
                        cg_emit(cg, "    movq %%rax, %%rdi");
                        cg_emit(cg, "    xorl %%esi, %%esi");
                        cg_emit(cg, "    movl $10, %%edx");
                        cg_emit(cg, "    callq %sstrtol", cg_sym_prefix(cg));
                        break;
                    }
                    // .to_float() -> int_to_float(obj)
                    if (strcmp(method, "to_float") == 0 && e->call.arg_count == 0) {
                        cg_expr(cg, obj);
                        cg_emit(cg, "    cvtsi2sdq %%rax, %%xmm0");
                        cg_emit(cg, "    movq %%xmm0, %%rax");
                        break;
                    }
                    // .contains(substr) -> str_find(obj, substr) != -1
                    if (strcmp(method, "contains") == 0 && e->call.arg_count == 1) {
                        cg_expr(cg, obj);
                        cg_emit(cg, "    pushq %%rax");
                        cg_expr(cg, e->call.args[0]);
                        cg_emit(cg, "    movq %%rax, %%rsi");
                        cg_emit(cg, "    popq %%rdi");
                        cg_emit(cg, "    callq %s_wyn_str_find", cg_sym_prefix(cg));
                        cg_emit(cg, "    cmpq $-1, %%rax");
                        cg_emit(cg, "    setne %%al");
                        cg_emit(cg, "    movzbq %%al, %%rax");
                        break;
                    }
                    // .index_of(substr) -> str_find(obj, substr)
                    if (strcmp(method, "index_of") == 0 && e->call.arg_count == 1) {
                        cg_expr(cg, obj);
                        cg_emit(cg, "    pushq %%rax");
                        cg_expr(cg, e->call.args[0]);
                        cg_emit(cg, "    movq %%rax, %%rsi");
                        cg_emit(cg, "    popq %%rdi");
                        cg_emit(cg, "    callq %s_wyn_str_find", cg_sym_prefix(cg));
                        break;
                    }
                    // .append(item) -> __wyn_array_append(obj, item)
                    if (strcmp(method, "append") == 0 && e->call.arg_count == 1) {
                        cg_expr(cg, obj);
                        cg_emit(cg, "    pushq %%rax");
                        cg_expr(cg, e->call.args[0]);
                        cg_emit(cg, "    movq %%rax, %%rsi");
                        cg_emit(cg, "    popq %%rdi");
                        cg_emit(cg, "    callq __wyn_array_append");
                        break;
                    }
                }
                // Method call: obj.method(args)
                if (e->call.func->kind == EXPR_FIELD) {
                    const char* regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                    // Push args in reverse
                    for (int i = e->call.arg_count - 1; i >= 0; i--) {
                        cg_expr(cg, e->call.args[i]);
                        cg_emit(cg, "    pushq %%rax");
                    }
                    // Push self (object)
                    cg_expr(cg, e->call.func->field.object);
                    cg_emit(cg, "    pushq %%rax");
                    // Pop into registers (self first)
                    for (int i = 0; i <= e->call.arg_count && i < 6; i++) {
                        cg_emit(cg, "    popq %%%s", regs[i]);
                    }
                    // Find struct by looking for method
                    const char* struct_name = NULL;
                    for (int i = 0; i < cg->module->struct_count; i++) {
                        StructDef* s = &cg->module->structs[i];
                        for (int j = 0; j < s->method_count; j++) {
                            if (strcmp(s->methods[j].name, e->call.func->field.field) == 0) {
                                struct_name = s->name;
                                break;
                            }
                        }
                        if (struct_name) break;
                    }
                    char mangled[256];
                    snprintf(mangled, 256, "_%s_%s", struct_name ? struct_name : "STRUCT", e->call.func->field.field);
                    cg_emit(cg, "    xorl %%eax, %%eax");
                    cg_emit(cg, "    callq %s", mangled);
                    break;
                }
                // Look up function for default parameters
                FnDef* fn = NULL;
                int total_args = e->call.arg_count;
                const char* func_name = NULL;
                
                if (e->call.func->kind == EXPR_IDENT) {
                    func_name = e->call.func->ident;
                    fn = cg_lookup_fn(cg, func_name);
                    if (fn) total_args = fn->param_count;
                } else if (e->call.func->kind == EXPR_FIELD && 
                           e->call.func->field.object->kind == EXPR_IDENT) {
                    // Check if this is a module.function() call
                    const char* module = e->call.func->field.object->ident;
                    const char* function = e->call.func->field.field;
                    func_name = map_module_function(module, function);
                    if (func_name) {
                        // Treat as builtin function call
                        fn = cg_lookup_fn(cg, func_name);
                        if (fn) total_args = fn->param_count;
                    }
                }
                
                // x86_64 SysV ABI: rdi, rsi, rdx, rcx, r8, r9
                const char* regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                // Push default values for missing args (in reverse order)
                if (fn && e->call.arg_count < fn->param_count) {
                    for (int i = fn->param_count - 1; i >= e->call.arg_count; i--) {
                        if (fn->params[i].default_val) {
                            cg_expr(cg, fn->params[i].default_val);
                            cg_emit(cg, "    pushq %%rax");
                        } else {
                            cg_emit(cg, "    pushq $0");  // Zero for missing
                        }
                    }
                }
                // Push all provided args
                for (int i = e->call.arg_count - 1; i >= 0; i--) {
                    cg_expr(cg, e->call.args[i]);
                    cg_emit(cg, "    pushq %%rax");
                }
                // Pop into registers
                for (int i = 0; i < total_args && i < 6; i++) {
                    cg_emit(cg, "    popq %%%s", regs[i]);
                }
                if (func_name) {
                    cg_emit(cg, "    xorl %%eax, %%eax");  // Clear AL for varargs
                    cg_emit(cg, "    callq %s%s", cg_sym_prefix(cg), func_name);
                } else if (e->call.func->kind == EXPR_IDENT) {
                    cg_emit(cg, "    xorl %%eax, %%eax");  // Clear AL for varargs
                    // Check if it's a local variable (function pointer) or global function
                    if (fn) {
                        // Direct call to known function
                        cg_emit(cg, "    callq %s%s", cg_sym_prefix(cg), e->call.func->ident);
                    } else {
                        // Indirect call through function pointer variable
                        int off = cg_local_offset(cg, e->call.func->ident);
                        if (off > 0) {
                            cg_emit(cg, "    callq *-%d(%%rbp)", off);
                        } else {
                            // Fallback to direct call
                            cg_emit(cg, "    callq %s%s", cg_sym_prefix(cg), e->call.func->ident);
                        }
                    }
                }
                break;
            }
            case EXPR_ARRAY: {
                int count = e->array.count;
                int base_off = cg->stack_offset + 8;
                cg->stack_offset += (count + 2) * 8;  // +2 for length and capacity
                cg_emit(cg, "    movq $%d, -%d(%%rbp)", count, base_off);  // length
                cg_emit(cg, "    movq $%d, -%d(%%rbp)", count, base_off + 8);  // capacity = length
                for (int i = 0; i < count; i++) {
                    cg_expr(cg, e->array.elements[i]);
                    cg_emit(cg, "    movq %%rax, -%d(%%rbp)", base_off + (i + 2) * 8);
                }
                cg_emit(cg, "    leaq -%d(%%rbp), %%rax", base_off);
                break;
            }
            case EXPR_INDEX: {
                cg_expr(cg, e->index.index);
                cg_emit(cg, "    pushq %%rax");
                cg_expr(cg, e->index.object);
                cg_emit(cg, "    popq %%rcx");
                if (!e->index.safe) {
                    // Handle negative index: if idx < 0, idx += len
                    cg_emit(cg, "    testq %%rcx, %%rcx");
                    cg_emit(cg, "    jns 1f");
                    cg_emit(cg, "    addq (%%rax), %%rcx");  // rcx += len
                    cg_emit(cg, "1:");
                }
                cg_emit(cg, "    addq $2, %%rcx");  // +2 to skip length and capacity
                cg_emit(cg, "    movq (%%rax,%%rcx,8), %%rax");
                break;
            }
            case EXPR_SLICE: {
                // Check if slicing a string (literal or variable)
                bool is_string_slice = (e->slice.object->kind == EXPR_STRING) ||
                    (e->slice.object->kind == EXPR_IDENT);  // Assume ident is string for now
                
                if (is_string_slice) {
                    // String slicing - copy bytes to stack buffer
                    int buf_off = cg->stack_offset + 8;
                    cg->stack_offset += 256;  // 256 byte buffer
                    
                    // Get string pointer
                    cg_expr(cg, e->slice.object);
                    cg_emit(cg, "    pushq %%rax");  // Save string ptr
                    
                    // Get string length
                    cg_emit(cg, "    movq %%rax, %%rsi");
                    cg_emit(cg, "    xorq %%r8, %%r8");
                    int len_loop = cg_new_label(cg), len_end = cg_new_label(cg);
                    cg_emit(cg, "L%d:", len_loop);
                    cg_emit(cg, "    cmpb $0, (%%rsi,%%r8)");
                    cg_emit(cg, "    je L%d", len_end);
                    cg_emit(cg, "    incq %%r8");
                    cg_emit(cg, "    jmp L%d", len_loop);
                    cg_emit(cg, "L%d:", len_end);  // r8 = len
                    cg_emit(cg, "    pushq %%r8");  // Save len
                    
                    // Compute start
                    if (e->slice.start) {
                        cg_expr(cg, e->slice.start);
                        cg_emit(cg, "    movq %%rax, %%r9");
                        cg_emit(cg, "    movq (%%rsp), %%r8");  // Restore len
                        cg_emit(cg, "    testq %%r9, %%r9");
                        cg_emit(cg, "    jns 1f");
                        cg_emit(cg, "    addq %%r8, %%r9");
                        cg_emit(cg, "1:");
                    } else {
                        cg_emit(cg, "    xorq %%r9, %%r9");
                    }
                    cg_emit(cg, "    pushq %%r9");  // Save start
                    
                    // Compute end
                    if (e->slice.end) {
                        cg_expr(cg, e->slice.end);
                        cg_emit(cg, "    movq %%rax, %%r10");
                        cg_emit(cg, "    movq 8(%%rsp), %%r8");  // Restore len
                        cg_emit(cg, "    testq %%r10, %%r10");
                        cg_emit(cg, "    jns 2f");
                        cg_emit(cg, "    addq %%r8, %%r10");
                        cg_emit(cg, "2:");
                    } else {
                        cg_emit(cg, "    movq 8(%%rsp), %%r10");  // end = len
                    }
                    
                    // Copy bytes: for i = start; i < end; i++
                    cg_emit(cg, "    popq %%r9");   // Restore start
                    cg_emit(cg, "    addq $8, %%rsp");  // Pop len
                    cg_emit(cg, "    popq %%rsi");  // Restore string ptr
                    cg_emit(cg, "    leaq -%d(%%rbp), %%rdi", buf_off);  // dest buffer
                    cg_emit(cg, "    movq %%r9, %%rcx");  // i = start
                    int copy_loop = cg_new_label(cg), copy_end = cg_new_label(cg);
                    cg_emit(cg, "L%d:", copy_loop);
                    cg_emit(cg, "    cmpq %%r10, %%rcx");
                    cg_emit(cg, "    jge L%d", copy_end);
                    cg_emit(cg, "    movb (%%rsi,%%rcx), %%al");
                    cg_emit(cg, "    movq %%rcx, %%rdx");
                    cg_emit(cg, "    subq %%r9, %%rdx");  // dest_idx = i - start
                    cg_emit(cg, "    movb %%al, (%%rdi,%%rdx)");
                    cg_emit(cg, "    incq %%rcx");
                    cg_emit(cg, "    jmp L%d", copy_loop);
                    cg_emit(cg, "L%d:", copy_end);
                    // Null terminate
                    cg_emit(cg, "    movq %%r10, %%rdx");
                    cg_emit(cg, "    subq %%r9, %%rdx");
                    cg_emit(cg, "    movb $0, (%%rdi,%%rdx)");
                    cg_emit(cg, "    leaq -%d(%%rbp), %%rax", buf_off);
                    break;
                }
                
                // Array slicing (original code)
                cg_expr(cg, e->slice.object);
                cg_emit(cg, "    pushq %%rax");  // Save array ptr
                
                // Get array length
                cg_emit(cg, "    movq (%%rax), %%r8");  // r8 = len
                
                // Compute start (default 0)
                if (e->slice.start) {
                    cg_expr(cg, e->slice.start);
                    cg_emit(cg, "    movq %%rax, %%r9");  // r9 = start
                    // Handle negative index
                    cg_emit(cg, "    testq %%r9, %%r9");
                    cg_emit(cg, "    jns 1f");
                    cg_emit(cg, "    addq %%r8, %%r9");  // start += len
                    cg_emit(cg, "1:");
                } else {
                    cg_emit(cg, "    xorq %%r9, %%r9");  // start = 0
                }
                cg_emit(cg, "    pushq %%r9");  // Save start
                
                // Compute end (default len)
                if (e->slice.end) {
                    cg_expr(cg, e->slice.end);
                    cg_emit(cg, "    movq %%rax, %%r10");  // r10 = end
                    // Handle negative index
                    cg_emit(cg, "    popq %%r9");  // Restore start
                    cg_emit(cg, "    movq (%%rsp), %%rax");  // Get array ptr
                    cg_emit(cg, "    movq (%%rax), %%r8");  // r8 = len
                    cg_emit(cg, "    testq %%r10, %%r10");
                    cg_emit(cg, "    jns 2f");
                    cg_emit(cg, "    addq %%r8, %%r10");  // end += len
                    cg_emit(cg, "2:");
                } else {
                    cg_emit(cg, "    popq %%r9");  // Restore start
                    cg_emit(cg, "    movq (%%rsp), %%rax");  // Get array ptr
                    cg_emit(cg, "    movq (%%rax), %%r10");  // end = len
                }
                
                // Compute new length: end - start
                cg_emit(cg, "    movq %%r10, %%r11");
                cg_emit(cg, "    subq %%r9, %%r11");  // r11 = new_len
                
                // Allocate new array on stack (64 elements max)
                int base_off = cg->stack_offset + 8;
                cg->stack_offset += 64 * 8;
                cg_emit(cg, "    movq %%r11, -%d(%%rbp)", base_off);  // Store length
                
                // Copy elements: for i = 0; i < new_len; i++
                cg_emit(cg, "    xorq %%rcx, %%rcx");  // i = 0
                cg_emit(cg, "    popq %%rax");  // Get array ptr
                cg_emit(cg, "3:");
                cg_emit(cg, "    cmpq %%r11, %%rcx");
                cg_emit(cg, "    jge 4f");
                // src_idx = start + i
                cg_emit(cg, "    movq %%r9, %%rdx");
                cg_emit(cg, "    addq %%rcx, %%rdx");
                cg_emit(cg, "    addq $1, %%rdx");  // Skip length field
                cg_emit(cg, "    movq (%%rax,%%rdx,8), %%rdi");  // Load element
                // Store at new_arr[i+1]
                cg_emit(cg, "    movq %%rcx, %%rdx");
                cg_emit(cg, "    addq $1, %%rdx");
                cg_emit(cg, "    negq %%rdx");
                cg_emit(cg, "    movq %%rdi, -%d(%%rbp,%%rdx,8)", base_off);
                cg_emit(cg, "    incq %%rcx");
                cg_emit(cg, "    jmp 3b");
                cg_emit(cg, "4:");
                cg_emit(cg, "    leaq -%d(%%rbp), %%rax", base_off);
                break;
            }
            case EXPR_STRUCT: {
                StructDef* s = cg_lookup_struct(cg, e->struct_lit.name);
                int size = s ? s->field_count * 8 : e->struct_lit.count * 8;
                int base_off = cg->stack_offset + 8;
                cg->stack_offset += size;
                for (int i = 0; i < e->struct_lit.count; i++) {
                    int foff = s ? cg_struct_field_offset(s, e->struct_lit.fields[i]) : i * 8;
                    if (foff < 0) foff = i * 8;
                    cg_expr(cg, e->struct_lit.values[i]);
                    cg_emit(cg, "    movq %%rax, -%d(%%rbp)", base_off + foff);
                }
                cg_emit(cg, "    leaq -%d(%%rbp), %%rax", base_off);
                break;
            }
            case EXPR_SOME: {
                // Optional with value: store [1, value] on stack
                int base_off = cg->stack_offset + 8;
                cg->stack_offset += 16;
                cg_emit(cg, "    movq $1, -%d(%%rbp)", base_off);  // has_value = 1
                cg_expr(cg, e->some.value);
                cg_emit(cg, "    movq %%rax, -%d(%%rbp)", base_off + 8);  // value
                cg_emit(cg, "    leaq -%d(%%rbp), %%rax", base_off);
                break;
            }
            case EXPR_NONE: {
                // Optional without value: store [0, 0] on stack
                int base_off = cg->stack_offset + 8;
                cg->stack_offset += 16;
                cg_emit(cg, "    movq $0, -%d(%%rbp)", base_off);  // has_value = 0
                cg_emit(cg, "    movq $0, -%d(%%rbp)", base_off + 8);  // value = 0
                cg_emit(cg, "    leaq -%d(%%rbp), %%rax", base_off);
                break;
            }
            case EXPR_OK: {
                // Result ok: store [1, value] on stack (tag=1 means ok)
                int base_off = cg->stack_offset + 8;
                cg->stack_offset += 16;
                cg_emit(cg, "    movq $1, -%d(%%rbp)", base_off);  // is_ok = 1
                cg_expr(cg, e->some.value);
                cg_emit(cg, "    movq %%rax, -%d(%%rbp)", base_off + 8);  // value
                cg_emit(cg, "    leaq -%d(%%rbp), %%rax", base_off);
                break;
            }
            case EXPR_ERR: {
                // Result err: store [0, error] on stack (tag=0 means err)
                int base_off = cg->stack_offset + 8;
                cg->stack_offset += 16;
                cg_emit(cg, "    movq $0, -%d(%%rbp)", base_off);  // is_ok = 0
                cg_expr(cg, e->some.value);
                cg_emit(cg, "    movq %%rax, -%d(%%rbp)", base_off + 8);  // error
                cg_emit(cg, "    leaq -%d(%%rbp), %%rax", base_off);
                break;
            }
            case EXPR_FIELD: {
                cg_expr(cg, e->field.object);
                // Look through structs to find field offset
                for (int i = 0; i < cg->module->struct_count; i++) {
                    int off = cg_struct_field_offset(&cg->module->structs[i], e->field.field);
                    if (off >= 0) {
                        cg_emit(cg, "    movq %d(%%rax), %%rax", off);
                        break;
                    }
                }
                break;
            }
            case EXPR_DEFAULT: {
                // value ?? default - if value has_value, use it, else use default
                int use_default = cg_new_label(cg), end_lbl = cg_new_label(cg);
                cg_expr(cg, e->default_expr.value);
                cg_emit(cg, "    movq (%%rax), %%rcx");  // has_value
                cg_emit(cg, "    testq %%rcx, %%rcx");
                cg_emit(cg, "    jz L%d", use_default);
                cg_emit(cg, "    movq 8(%%rax), %%rax");  // use value
                cg_emit(cg, "    jmp L%d", end_lbl);
                cg_emit(cg, "L%d:", use_default);
                cg_expr(cg, e->default_expr.default_val);
                cg_emit(cg, "L%d:", end_lbl);
                break;
            }
            case EXPR_IF: {
                // if cond then expr1 else expr2
                int else_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                cg_expr(cg, e->if_expr.cond);
                cg_emit(cg, "    testq %%rax, %%rax");
                cg_emit(cg, "    jz L%d", else_lbl);
                cg_expr(cg, e->if_expr.then_expr);
                cg_emit(cg, "    jmp L%d", end_lbl);
                cg_emit(cg, "L%d:", else_lbl);
                cg_expr(cg, e->if_expr.else_expr);
                cg_emit(cg, "L%d:", end_lbl);
                break;
            }
            case EXPR_MATCH: {
                int end_lbl = cg_new_label(cg);
                cg_expr(cg, e->match_expr.value);
                cg_emit(cg, "    pushq %%rax");
                for (int i = 0; i < e->match_expr.arm_count; i++) {
                    int next_lbl = cg_new_label(cg);
                    Expr* pat = e->match_expr.patterns[i];
                    if (pat->kind == EXPR_IDENT && strcmp(pat->ident, "_") == 0) {
                        cg_emit(cg, "    popq %%rax");
                        cg_expr(cg, e->match_expr.arms[i]);
                        cg_emit(cg, "    jmp L%d", end_lbl);
                    } else {
                        cg_expr(cg, pat);
                        cg_emit(cg, "    movq (%%rsp), %%rcx");
                        cg_emit(cg, "    cmpq %%rax, %%rcx");
                        cg_emit(cg, "    jne L%d", next_lbl);
                        cg_emit(cg, "    popq %%rax");
                        cg_expr(cg, e->match_expr.arms[i]);
                        cg_emit(cg, "    jmp L%d", end_lbl);
                        cg_emit(cg, "L%d:", next_lbl);
                    }
                }
                cg_emit(cg, "    popq %%rax");
                cg_emit(cg, "L%d:", end_lbl);
                break;
            }
            case EXPR_LAMBDA: {
                // Store lambda for later emission, return function pointer
                cg->lambdas[cg->lambda_count++] = e;
                cg_emit(cg, "    leaq _lambda%d(%%rip), %%rax", e->lambda.id);
                break;
            }
            default: cg_emit(cg, "    # unsupported expr");
        }
        return;
    }
    
    // ARM64 code generation
    switch (e->kind) {
        case EXPR_INT:
            if (e->int_val >= 0 && e->int_val < 65536) {
                cg_emit(cg, "    mov x0, #%lld", (long long)e->int_val);
            } else {
                cg_emit(cg, "    mov x0, #%lld", (long long)(e->int_val & 0xFFFF));
                if (e->int_val >= 65536) {
                    cg_emit(cg, "    movk x0, #%lld, lsl #16", (long long)((e->int_val >> 16) & 0xFFFF));
                }
                if (e->int_val >= 0x100000000LL) {
                    cg_emit(cg, "    movk x0, #%lld, lsl #32", (long long)((e->int_val >> 32) & 0xFFFF));
                }
            }
            break;
            
        case EXPR_FLOAT: {
            int lbl = cg->label_count++;
            cg->floats[cg->float_count].val = e->float_val;
            cg->floats[cg->float_count].label = lbl;
            cg->float_count++;
            char label[32];
            snprintf(label, sizeof(label), "L_.flt%d", lbl);
            cg_emit_adrp(cg, "x0", label);
            cg_emit_ldr_pageoff(cg, "d0", "x0", label);
            cg_emit(cg, "    fmov x0, d0");
            break;
        }
            
        case EXPR_BOOL:
            cg_emit(cg, "    mov x0, #%d", e->bool_val ? 1 : 0);
            break;
            
        case EXPR_STRING: {
            int lbl = cg->label_count++;
            cg->strings[cg->string_count].str = e->str_val;
            cg->strings[cg->string_count].label = lbl;
            cg->string_count++;
            char label[32];
            snprintf(label, sizeof(label), "L_.str%d", lbl);
            cg_emit_adrp(cg, "x0", label);
            cg_emit_add_pageoff(cg, "x0", "x0", label);
            break;
        }
        
        case EXPR_IDENT: {
            bool is_enum = false;
            int64_t enum_val = cg_lookup_enum_variant(cg, e->ident, &is_enum);
            if (is_enum) {
                cg_emit(cg, "    mov x0, #%lld", (long long)enum_val);
            } else {
                int off = cg_local_offset(cg, e->ident);
                if (off) {
                    if (cg_is_shared(cg, e->ident)) {
                        cg_atomic_load(cg, e->ident, off);
                    } else {
                        cg_emit_ldr_offset(cg, "x0", 16 + off);  // +16 for saved x29/x30
                    }
                }
            }
            break;
        }
        
        case EXPR_BINARY: {
            // Check for string concatenation
            if (e->binary.op == TOK_PLUS) {
                bool left_is_str = (e->binary.left->kind == EXPR_STRING);
                bool right_is_str = (e->binary.right->kind == EXPR_STRING);
                // Check if variables are string type
                if (e->binary.left->kind == EXPR_IDENT) {
                    Type* t = cg_local_type(cg, e->binary.left->ident);
                    if (t && t->kind == TYPE_STR) left_is_str = true;
                }
                if (e->binary.right->kind == EXPR_IDENT) {
                    Type* t = cg_local_type(cg, e->binary.right->ident);
                    if (t && t->kind == TYPE_STR) right_is_str = true;
                }
                // Check for chained concat: (str + str) + str
                if (e->binary.left->kind == EXPR_BINARY && e->binary.left->binary.op == TOK_PLUS) {
                    Expr* l = e->binary.left;
                    while (l->kind == EXPR_BINARY && l->binary.op == TOK_PLUS) {
                        if (l->binary.right->kind == EXPR_STRING) { left_is_str = true; break; }
                        if (l->binary.left->kind == EXPR_STRING) { left_is_str = true; break; }
                        l = l->binary.left;
                    }
                }
                
                if (left_is_str || right_is_str) {
                    // Allocate a fixed buffer on stack for result (256 bytes)
                    int buf_off = cg->stack_offset + 8;
                    cg->stack_offset += 256;
                    
                    // Evaluate left string
                    cg_expr(cg, e->binary.left);
                    cg_emit(cg, "    str x0, [sp, #-16]!");  // save left ptr
                    
                    // Evaluate right string
                    cg_expr(cg, e->binary.right);
                    cg_emit(cg, "    str x0, [sp, #-16]!");  // save right ptr
                    
                    // Copy left string to buffer
                    cg_emit(cg, "    ldr x1, [sp, #16]");  // left ptr
                    cg_emit_add_offset(cg, "x0", 16 + buf_off);  // dest buffer
                    int copy1 = cg_new_label(cg);
                    cg_emit(cg, "L%d:", copy1);
                    cg_emit(cg, "    ldrb w3, [x1], #1");
                    cg_emit(cg, "    strb w3, [x0], #1");
                    cg_emit(cg, "    cbnz w3, L%d", copy1);
                    cg_emit(cg, "    sub x0, x0, #1");  // back up over null terminator
                    
                    // Copy right string (including null terminator)
                    cg_emit(cg, "    ldr x1, [sp], #16");  // right ptr, pop
                    int copy2 = cg_new_label(cg);
                    cg_emit(cg, "L%d:", copy2);
                    cg_emit(cg, "    ldrb w3, [x1], #1");
                    cg_emit(cg, "    strb w3, [x0], #1");
                    cg_emit(cg, "    cbnz w3, L%d", copy2);
                    
                    cg_emit(cg, "    add sp, sp, #16");  // clean up left ptr
                    cg_emit_add_offset(cg, "x0", 16 + buf_off);  // return buffer ptr
                    break;
                }
            }
            // Check for string comparison
            if ((e->binary.op == TOK_EQEQ || e->binary.op == TOK_NOTEQ)) {
                bool left_is_str = (e->binary.left->kind == EXPR_STRING);
                bool right_is_str = (e->binary.right->kind == EXPR_STRING);
                if (e->binary.left->kind == EXPR_IDENT) {
                    Type* t = cg_local_type(cg, e->binary.left->ident);
                    if (t && t->kind == TYPE_STR) left_is_str = true;
                }
                if (e->binary.right->kind == EXPR_IDENT) {
                    Type* t = cg_local_type(cg, e->binary.right->ident);
                    if (t && t->kind == TYPE_STR) right_is_str = true;
                }
                // Check for known string-returning function calls
                if (e->binary.left->kind == EXPR_CALL && e->binary.left->call.func->kind == EXPR_IDENT) {
                    const char* fn = e->binary.left->call.func->ident;
                    if (strcmp(fn, "str_lower") == 0 || strcmp(fn, "str_upper") == 0 ||
                        strcmp(fn, "str_trim") == 0 || strcmp(fn, "read_file") == 0 ||
                        strcmp(fn, "to_string") == 0) left_is_str = true;
                }
                if (e->binary.right->kind == EXPR_CALL && e->binary.right->call.func->kind == EXPR_IDENT) {
                    const char* fn = e->binary.right->call.func->ident;
                    if (strcmp(fn, "str_lower") == 0 || strcmp(fn, "str_upper") == 0 ||
                        strcmp(fn, "str_trim") == 0 || strcmp(fn, "read_file") == 0 ||
                        strcmp(fn, "to_string") == 0) right_is_str = true;
                }
                
                if (left_is_str || right_is_str) {
                    // Evaluate left, save on stack
                    cg_expr(cg, e->binary.left);
                    cg_emit(cg, "    str x0, [sp, #-16]!");
                    // Evaluate right
                    cg_expr(cg, e->binary.right);
                    cg_emit(cg, "    mov x2, x0");  // right ptr in x2
                    cg_emit(cg, "    ldr x1, [sp], #16");  // left ptr in x1
                    // strcmp loop
                    int loop_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                    cg_emit(cg, "L%d:", loop_lbl);
                    cg_emit(cg, "    ldrb w3, [x1], #1");
                    cg_emit(cg, "    ldrb w4, [x2], #1");
                    cg_emit(cg, "    cmp w3, w4");
                    cg_emit(cg, "    b.ne L%d", end_lbl);
                    cg_emit(cg, "    cbz w3, L%d", end_lbl);
                    cg_emit(cg, "    b L%d", loop_lbl);
                    cg_emit(cg, "L%d:", end_lbl);
                    cg_emit(cg, "    cset x0, %s", e->binary.op == TOK_EQEQ ? "eq" : "ne");
                    break;
                }
            }
            
            // Handle 'in' operator: elem in array
            if (e->binary.op == TOK_IN) {
                // Evaluate element to search for
                cg_expr(cg, e->binary.left);
                cg_emit(cg, "    str x0, [sp, #-16]!");  // save element
                // Evaluate array
                cg_expr(cg, e->binary.right);
                cg_emit(cg, "    mov x2, x0");  // array ptr
                cg_emit(cg, "    ldr x1, [sp], #16");  // element to find
                cg_emit(cg, "    ldr x3, [x2]");  // array length
                cg_emit(cg, "    add x2, x2, #8");  // skip length field
                cg_emit(cg, "    mov x0, #0");  // result = false
                int loop_lbl = cg_new_label(cg), found_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                cg_emit(cg, "L%d:", loop_lbl);
                cg_emit(cg, "    cbz x3, L%d", end_lbl);
                cg_emit(cg, "    ldr x4, [x2], #8");
                cg_emit(cg, "    cmp x4, x1");
                cg_emit(cg, "    b.eq L%d", found_lbl);
                cg_emit(cg, "    sub x3, x3, #1");
                cg_emit(cg, "    b L%d", loop_lbl);
                cg_emit(cg, "L%d:", found_lbl);
                cg_emit(cg, "    mov x0, #1");
                cg_emit(cg, "L%d:", end_lbl);
                break;
            }
            
            // Check if this is a float operation
            bool is_float = cg_is_float_expr(cg, e);
            
            cg_expr(cg, e->binary.right);
            cg_emit(cg, "    str x0, [sp, #-16]!");
            cg_expr(cg, e->binary.left);
            cg_emit(cg, "    ldr x1, [sp], #16");
            
            if (is_float) {
                // Float arithmetic: convert bit patterns to d0/d1
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    fmov d1, x1");
                switch (e->binary.op) {
                    case TOK_PLUS:  cg_emit(cg, "    fadd d0, d0, d1"); break;
                    case TOK_MINUS: cg_emit(cg, "    fsub d0, d0, d1"); break;
                    case TOK_STAR:  cg_emit(cg, "    fmul d0, d0, d1"); break;
                    case TOK_SLASH: cg_emit(cg, "    fdiv d0, d0, d1"); break;
                    case TOK_LT:
                        cg_emit(cg, "    fcmp d0, d1");
                        cg_emit(cg, "    cset x0, lt");
                        break;
                    case TOK_GT:
                        cg_emit(cg, "    fcmp d0, d1");
                        cg_emit(cg, "    cset x0, gt");
                        break;
                    case TOK_LTEQ:
                        cg_emit(cg, "    fcmp d0, d1");
                        cg_emit(cg, "    cset x0, le");
                        break;
                    case TOK_GTEQ:
                        cg_emit(cg, "    fcmp d0, d1");
                        cg_emit(cg, "    cset x0, ge");
                        break;
                    case TOK_EQEQ:
                        cg_emit(cg, "    fcmp d0, d1");
                        cg_emit(cg, "    cset x0, eq");
                        break;
                    case TOK_NOTEQ:
                        cg_emit(cg, "    fcmp d0, d1");
                        cg_emit(cg, "    cset x0, ne");
                        break;
                    default:
                        // Fall through to integer for unsupported ops
                        goto int_arith;
                }
                // For arithmetic ops, move result back to x0
                if (e->binary.op == TOK_PLUS || e->binary.op == TOK_MINUS ||
                    e->binary.op == TOK_STAR || e->binary.op == TOK_SLASH) {
                    cg_emit(cg, "    fmov x0, d0");
                }
                break;
            }
            
            int_arith:
            switch (e->binary.op) {
                case TOK_PLUS:  cg_emit(cg, "    add x0, x0, x1"); break;
                case TOK_MINUS: cg_emit(cg, "    sub x0, x0, x1"); break;
                case TOK_STAR:  cg_emit(cg, "    mul x0, x0, x1"); break;
                case TOK_SLASH: cg_emit(cg, "    sdiv x0, x0, x1"); break;
                case TOK_PERCENT:
                    cg_emit(cg, "    sdiv x2, x0, x1");
                    cg_emit(cg, "    msub x0, x2, x1, x0");
                    break;
                case TOK_EQEQ:
                    cg_emit(cg, "    cmp x0, x1");
                    cg_emit(cg, "    cset x0, eq");
                    break;
                case TOK_NOTEQ:
                    cg_emit(cg, "    cmp x0, x1");
                    cg_emit(cg, "    cset x0, ne");
                    break;
                case TOK_LT:
                    cg_emit(cg, "    cmp x0, x1");
                    cg_emit(cg, "    cset x0, lt");
                    break;
                case TOK_GT:
                    cg_emit(cg, "    cmp x0, x1");
                    cg_emit(cg, "    cset x0, gt");
                    break;
                case TOK_LTEQ:
                    cg_emit(cg, "    cmp x0, x1");
                    cg_emit(cg, "    cset x0, le");
                    break;
                case TOK_GTEQ:
                    cg_emit(cg, "    cmp x0, x1");
                    cg_emit(cg, "    cset x0, ge");
                    break;
                case TOK_AND:
                    cg_emit(cg, "    and x0, x0, x1");
                    cg_emit(cg, "    cmp x0, #0");
                    cg_emit(cg, "    cset x0, ne");
                    break;
                case TOK_OR:
                    cg_emit(cg, "    orr x0, x0, x1");
                    cg_emit(cg, "    cmp x0, #0");
                    cg_emit(cg, "    cset x0, ne");
                    break;
                case TOK_PIPE: cg_emit(cg, "    orr x0, x0, x1"); break;
                case TOK_AMPERSAND: cg_emit(cg, "    and x0, x0, x1"); break;
                case TOK_CARET: cg_emit(cg, "    eor x0, x0, x1"); break;
                case TOK_LTLT: cg_emit(cg, "    lsl x0, x0, x1"); break;
                case TOK_GTGT: cg_emit(cg, "    asr x0, x0, x1"); break;
                case TOK_STARSTAR: {
                    // Power: loop-based implementation
                    int loop_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                    cg_emit(cg, "    mov x2, x0");   // base
                    cg_emit(cg, "    mov x3, x1");   // exp
                    cg_emit(cg, "    mov x0, #1");   // result
                    cg_emit(cg, "L%d:", loop_lbl);
                    cg_emit(cg, "    cbz x3, L%d", end_lbl);
                    cg_emit(cg, "    mul x0, x0, x2");
                    cg_emit(cg, "    sub x3, x3, #1");
                    cg_emit(cg, "    b L%d", loop_lbl);
                    cg_emit(cg, "L%d:", end_lbl);
                    break;
                }
                default:
                    cg_emit(cg, "    ; unsupported op");
            }
            break;
        }
        
        case EXPR_UNARY:
            cg_expr(cg, e->unary.operand);
            if (e->unary.op == TOK_MINUS) {
                cg_emit(cg, "    neg x0, x0");
            } else if (e->unary.op == TOK_TILDE) {
                cg_emit(cg, "    mvn x0, x0");
            } else if (e->unary.op == TOK_NOT) {
                cg_emit(cg, "    cmp x0, #0");
                cg_emit(cg, "    cset x0, eq");
            }
            break;
        
        case EXPR_ADDR:
            // Address-of: get address of variable
            if (e->unary.operand->kind == EXPR_IDENT) {
                int off = cg_local_offset(cg, e->unary.operand->ident);
                if (off) cg_emit_add_offset(cg, "x0", 16 + off);
            } else {
                cg_expr(cg, e->unary.operand);  // For other expressions, just evaluate
            }
            break;
        
        case EXPR_DEREF:
            // Dereference: load value at pointer
            cg_expr(cg, e->unary.operand);
            cg_emit(cg, "    ldr x0, [x0]");
            break;
        
        case EXPR_UNWRAP:
            // Unwrap optional: get value (second word)
            cg_expr(cg, e->unary.operand);
            cg_emit(cg, "    ldr x0, [x0, #8]");
            break;
            
        case EXPR_CALL: {
            // Handle built-in len()
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "len") == 0) {
                // Check type to determine array vs string
                Expr* arg = e->call.args[0];
                Type* arg_type = NULL;
                if (arg->kind == EXPR_IDENT) {
                    arg_type = cg_local_type(cg, arg->ident);
                } else if (arg->kind == EXPR_ARRAY) {
                    arg_type = new_type(TYPE_ARRAY);
                }
                bool is_array = (arg_type && arg_type->kind == TYPE_ARRAY);
                if (is_array) {
                    // Array - read length from first element
                    cg_expr(cg, arg);
                    cg_emit(cg, "    ldr x0, [x0]");
                } else {
                    // String - count chars until null terminator
                    cg_expr(cg, arg);
                    int loop_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                    cg_emit(cg, "    mov x1, x0");
                    cg_emit(cg, "    mov x0, #0");
                    cg_emit(cg, "L%d:", loop_lbl);
                    cg_emit(cg, "    ldrb w2, [x1], #1");
                    cg_emit(cg, "    cbz w2, L%d", end_lbl);
                    cg_emit(cg, "    add x0, x0, #1");
                    cg_emit(cg, "    b L%d", loop_lbl);
                    cg_emit(cg, "L%d:", end_lbl);
                }
                break;
            }
            // Handle built-in print() - type-aware dispatch with auto-newline
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "print") == 0 && e->call.arg_count > 0) {
                // Loop through all arguments
                for (int arg_idx = 0; arg_idx < e->call.arg_count; arg_idx++) {
                    Expr* arg = e->call.args[arg_idx];
                    Type* arg_type = NULL;
                    if (arg->kind == EXPR_IDENT) {
                        arg_type = cg_local_type(cg, arg->ident);
                    } else if (arg->kind == EXPR_STRING) {
                        arg_type = new_type(TYPE_STR);
                    } else if (arg->kind == EXPR_FLOAT) {
                        arg_type = new_type(TYPE_FLOAT);
                    } else if (arg->kind == EXPR_BINARY && arg->binary.op == TOK_PLUS) {
                        // String concatenation results in a string
                        if (arg->binary.left->kind == EXPR_STRING || arg->binary.right->kind == EXPR_STRING) {
                            arg_type = new_type(TYPE_STR);
                        } else if (arg->binary.left->kind == EXPR_IDENT) {
                            Type* left_type = cg_local_type(cg, arg->binary.left->ident);
                            if (left_type && left_type->kind == TYPE_STR) {
                                arg_type = new_type(TYPE_STR);
                            }
                        } else if (arg->binary.right->kind == EXPR_IDENT) {
                            Type* right_type = cg_local_type(cg, arg->binary.right->ident);
                            if (right_type && right_type->kind == TYPE_STR) {
                                arg_type = new_type(TYPE_STR);
                            }
                        }
                    } else if (arg->kind == EXPR_FIELD) {
                        // For field access, need to look up the field type
                        Type* obj_type = NULL;
                        if (arg->field.object->kind == EXPR_IDENT) {
                            obj_type = cg_local_type(cg, arg->field.object->ident);
                        }
                        if (obj_type && obj_type->kind == TYPE_NAMED) {
                            for (int i = 0; i < cg->module->struct_count; i++) {
                                if (strcmp(cg->module->structs[i].name, obj_type->name) == 0) {
                                    StructDef* s = &cg->module->structs[i];
                                    for (int j = 0; j < s->field_count; j++) {
                                        if (strcmp(s->fields[j].name, arg->field.field) == 0) {
                                            arg_type = s->fields[j].type;
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    cg_expr(cg, arg);
                    if (arg_type && arg_type->kind == TYPE_STR) {
                        cg_emit(cg, "    bl %sprint_str", cg_sym_prefix(cg));
                    } else if (arg_type && arg_type->kind == TYPE_FLOAT) {
                        cg_emit(cg, "    bl %sprint_float", cg_sym_prefix(cg));
                    } else {
                        cg_emit(cg, "    bl %sprint", cg_sym_prefix(cg));
                    }
                    // Add space between arguments (except after last one)
                    if (arg_idx < e->call.arg_count - 1) {
                        cg_emit(cg, "    mov w0, #32");  // space character
                        cg_emit(cg, "    bl %sputchar", cg_sym_prefix(cg));
                    }
                }
                // Auto-add newline
                cg_emit(cg, "    bl %sprint_newline", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in abs()
            if (is_builtin_call(e->call.func, "abs")) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    cmp x0, #0");
                cg_emit(cg, "    cneg x0, x0, lt");
                break;
            }
            // Handle built-in min()
            if (is_builtin_call(e->call.func, "min")) {
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    ldr x1, [sp], #16");
                cg_emit(cg, "    cmp x0, x1");
                cg_emit(cg, "    csel x0, x0, x1, lt");
                break;
            }
            // Handle built-in max()
            if (is_builtin_call(e->call.func, "max")) {
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    ldr x1, [sp], #16");
                cg_emit(cg, "    cmp x0, x1");
                cg_emit(cg, "    csel x0, x0, x1, gt");
                break;
            }
            // Handle built-in int_to_str()
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "int_to_str") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    mov x8, sp");
                cg_emit(cg, "    str x0, [x8]");
                cg_emit_adrp(cg, "x0", "L_.itsbuf");
                cg_emit_add_pageoff(cg, "x0", "x0", "L_.itsbuf");
                cg_emit_adrp(cg, "x1", "L_.itsfmt");
                cg_emit_add_pageoff(cg, "x1", "x1", "L_.itsfmt");
                cg_emit(cg, "    bl %ssprintf", cg_sym_prefix(cg));
                cg_emit_adrp(cg, "x0", "L_.itsbuf");
                cg_emit_add_pageoff(cg, "x0", "x0", "L_.itsbuf");
                break;
            }
            // Handle built-in str_to_int()
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "str_to_int") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    mov x1, #0");
                cg_emit(cg, "    mov x2, #10");
                cg_emit(cg, "    bl %sstrtol", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in ord() - get ASCII code of first char
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "ord") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    ldrb w0, [x0]");  // Load first byte
                break;
            }
            // Handle built-in chr() - create single-char string
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "chr") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit_adrp(cg, "x1", "L_.chrbuf");
                cg_emit_add_pageoff(cg, "x1", "x1", "L_.chrbuf");
                cg_emit(cg, "    strb w0, [x1]");
                cg_emit(cg, "    strb wzr, [x1, #1]");
                cg_emit(cg, "    mov x0, x1");
                break;
            }
            // Handle built-in int_to_float() - convert int to float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "int_to_float") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    scvtf d0, x0");  // Signed convert to float
                cg_emit(cg, "    fmov x0, d0");   // Move to x0 for return
                break;
            }
            // Handle built-in float_to_int() - convert float to int
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "float_to_int") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    fmov d0, x0");   // Move to d0
                cg_emit(cg, "    fcvtzs x0, d0"); // Convert to signed int (truncate)
                break;
            }
            // Handle built-in read_file() - reads entire file to string
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "read_file") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %s_wyn_read_file", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in write_file(path, content) - writes string to file
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "write_file") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    mov x1, x0");
                cg_emit(cg, "    ldr x0, [sp], #16");
                cg_emit(cg, "    bl %s_wyn_write_file", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in read_line() - reads a line from stdin
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "read_line") == 0) {
                cg_emit(cg, "    bl %s_wyn_read_line", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in file_exists(path) - check if file exists
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "file_exists") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %s_wyn_file_exists", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in delete_file(path) - delete a file
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "delete_file") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %s_wyn_delete_file", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in mkdir(path) - create directory
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "mkdir") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %s_wyn_mkdir", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in rmdir(path) - remove directory
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "rmdir") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %s_wyn_rmdir", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in append_file(path, content) - append to file
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "append_file") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    mov x1, x0");
                cg_emit(cg, "    ldr x0, [sp], #16");
                cg_emit(cg, "    bl %s_wyn_append_file", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in file_size(path) -> int
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "file_size") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %s_wyn_file_size", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in is_dir(path) -> bool
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "is_dir") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %s_wyn_is_dir", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in time_now() - returns Unix timestamp
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "time_now") == 0) {
                cg_emit(cg, "    mov x0, #0");
                cg_emit(cg, "    bl %stime", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in sleep_ms(ms) - sleep for milliseconds
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "sleep_ms") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    mov x1, #1000");
                cg_emit(cg, "    mul x0, x0, x1");
                cg_emit(cg, "    bl %susleep", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in tcp_connect(host, port) - connect to TCP socket
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "tcp_connect") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    mov x1, x0");
                cg_emit(cg, "    ldr x0, [sp], #16");
                cg_emit(cg, "    bl %s_wyn_tcp_connect", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in tcp_close(sock) - close TCP socket
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "tcp_close") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %sclose", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in char_at(s, i) - get character at index
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "char_at") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    mov x1, x0");
                cg_emit(cg, "    ldr x0, [sp], #16");
                cg_emit(cg, "    bl %s_wyn_char_at", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in clock() - returns CPU time in microseconds
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "clock") == 0) {
                cg_emit(cg, "    bl %sclock", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in random() - returns random integer
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "random") == 0) {
                cg_emit(cg, "    bl %srand", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in sleep(seconds) - pause execution
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "sleep") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %ssleep", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in getenv(name) - get environment variable
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "getenv") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %sgetenv", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in args() - get command line arguments as [str]
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "args") == 0) {
                // Allocate array on stack: [len, capacity, elem0, elem1, ...]
                int arr_off = cg->stack_offset + 8;
                cg->stack_offset += 16 + 8 * 64;  // len + capacity + max 64 args
                // Load argc and argv from globals
                cg_emit(cg, "    adrp x8, L_.wyn_argc@PAGE");
                cg_emit(cg, "    add x8, x8, L_.wyn_argc@PAGEOFF");
                cg_emit(cg, "    ldr x9, [x8]");  // x9 = argc
                cg_emit(cg, "    adrp x8, L_.wyn_argv@PAGE");
                cg_emit(cg, "    add x8, x8, L_.wyn_argv@PAGEOFF");
                cg_emit(cg, "    ldr x10, [x8]");  // x10 = argv
                cg_emit(cg, "    add x11, x29, #%d", 16 + arr_off);  // x11 = array base
                cg_emit(cg, "    str x9, [x11]");  // store length
                cg_emit(cg, "    str x9, [x11, #8]");  // store capacity
                cg_emit(cg, "    mov x12, #0");  // i = 0
                int loop_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                cg_emit(cg, "L%d:", loop_lbl);
                cg_emit(cg, "    cmp x12, x9");
                cg_emit(cg, "    b.ge L%d", end_lbl);
                cg_emit(cg, "    ldr x13, [x10, x12, lsl #3]");  // argv[i]
                cg_emit(cg, "    add x14, x12, #2");  // +2 for len and capacity
                cg_emit(cg, "    str x13, [x11, x14, lsl #3]");  // arr[i+2]
                cg_emit(cg, "    add x12, x12, #1");
                cg_emit(cg, "    b L%d", loop_lbl);
                cg_emit(cg, "L%d:", end_lbl);
                cg_emit_add_offset(cg, "x0", 16 + arr_off);  // return array ptr
                break;
            }
            // Handle built-in system(cmd) - run shell command
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "system") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %ssystem", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in syscall(num, arg1, arg2, arg3, arg4, arg5, arg6)
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "syscall") == 0) {
                // Push args in reverse
                for (int i = e->call.arg_count - 1; i >= 0; i--) {
                    cg_expr(cg, e->call.args[i]);
                    cg_emit(cg, "    str x0, [sp, #-16]!");
                }
                // Pop into registers
                for (int i = 0; i < e->call.arg_count && i < 8; i++) {
                    cg_emit(cg, "    ldr x%d, [sp], #16", i);
                }
                // Move syscall number to x16
                cg_emit(cg, "    mov x16, x0");
                // Shift args down
                for (int i = 0; i < e->call.arg_count - 1 && i < 7; i++) {
                    cg_emit(cg, "    mov x%d, x%d", i, i + 1);
                }
                // Make syscall
                cg_emit(cg, "    svc #0");
                break;
            }
            // Handle built-in str_find(haystack, needle) - returns index or -1
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "str_find") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    mov x1, x0");
                cg_emit(cg, "    ldr x0, [sp], #16");
                cg_emit(cg, "    str x0, [sp, #-16]!");  // save haystack
                cg_emit(cg, "    bl %sstrstr", cg_sym_prefix(cg));
                cg_emit(cg, "    ldr x1, [sp], #16");  // restore haystack
                int found_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                cg_emit(cg, "    cbnz x0, L%d", found_lbl);
                cg_emit(cg, "    mov x0, #-1");
                cg_emit(cg, "    b L%d", end_lbl);
                cg_emit(cg, "L%d:", found_lbl);
                cg_emit(cg, "    sub x0, x0, x1");  // offset = result - haystack
                cg_emit(cg, "L%d:", end_lbl);
                break;
            }
            // Handle built-in str_cmp(a, b) - returns 0 if equal
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "str_cmp") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    mov x1, x0");
                cg_emit(cg, "    ldr x0, [sp], #16");
                cg_emit(cg, "    bl %sstrcmp", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in str_concat(a, b) -> str
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "str_concat") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_emit(cg, "    ldr x0, [sp, #16]");
                cg_emit(cg, "    bl %sstrlen", cg_sym_prefix(cg));
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_emit(cg, "    ldr x0, [sp, #16]");
                cg_emit(cg, "    bl %sstrlen", cg_sym_prefix(cg));
                cg_emit(cg, "    ldr x1, [sp], #16");
                cg_emit(cg, "    add x0, x0, x1");
                cg_emit(cg, "    add x0, x0, #1");
                cg_emit(cg, "    bl %smalloc", cg_sym_prefix(cg));
                cg_emit(cg, "    mov x19, x0");
                cg_emit(cg, "    ldr x1, [sp, #16]");
                cg_emit(cg, "    bl %sstrcpy", cg_sym_prefix(cg));
                cg_emit(cg, "    mov x0, x19");
                cg_emit(cg, "    ldr x1, [sp], #16");
                cg_emit(cg, "    bl %sstrcat", cg_sym_prefix(cg));
                cg_emit(cg, "    add sp, sp, #16");
                break;
            }
            // Handle built-in str_substr(s, start, len) -> str
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "str_substr") == 0) {
                cg_expr(cg, e->call.args[2]);
                cg_emit(cg, "    add x0, x0, #1");
                cg_emit(cg, "    bl %smalloc", cg_sym_prefix(cg));
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    ldr x1, [sp], #16");
                cg_emit(cg, "    add x1, x1, x0");
                cg_expr(cg, e->call.args[2]);
                cg_emit(cg, "    mov x2, x0");
                cg_emit(cg, "    ldr x0, [sp]");
                cg_emit(cg, "    bl %smemcpy", cg_sym_prefix(cg));
                cg_expr(cg, e->call.args[2]);
                cg_emit(cg, "    ldr x1, [sp]");
                cg_emit(cg, "    add x1, x1, x0");
                cg_emit(cg, "    strb wzr, [x1]");
                cg_emit(cg, "    ldr x0, [sp], #16");
                break;
            }
            // Handle built-in str_split(s, delim) -> [str]
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "str_split") == 0) {
                cg_emit(cg, "    stp x19, x20, [sp, #-16]!");
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %sstrlen", cg_sym_prefix(cg));
                cg_emit(cg, "    add x0, x0, #1");
                cg_emit(cg, "    bl %smalloc", cg_sym_prefix(cg));
                cg_emit(cg, "    mov x19, x0");
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    mov x1, x0");
                cg_emit(cg, "    mov x0, x19");
                cg_emit(cg, "    bl %sstrcpy", cg_sym_prefix(cg));
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_emit(cg, "    mov x0, #136");
                cg_emit(cg, "    bl %smalloc", cg_sym_prefix(cg));
                cg_emit(cg, "    str xzr, [x0]");
                cg_emit(cg, "    mov x1, #16");
                cg_emit(cg, "    str x1, [x0, #8]");
                cg_emit(cg, "    str x0, [sp, #-16]!");
                int loop_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                cg_emit(cg, "L%d:", loop_lbl);
                cg_emit(cg, "    mov x0, x19");
                cg_emit(cg, "    ldr x1, [sp, #16]");
                cg_emit(cg, "    bl %sstrstr", cg_sym_prefix(cg));
                cg_emit(cg, "    cbz x0, L%d", end_lbl);
                cg_emit(cg, "    mov x20, x0");
                cg_emit(cg, "    strb wzr, [x20]");
                cg_emit(cg, "    ldr x1, [sp]");
                cg_emit(cg, "    ldr x2, [x1]");
                cg_emit(cg, "    add x3, x2, #2");
                cg_emit(cg, "    str x19, [x1, x3, lsl #3]");
                cg_emit(cg, "    add x2, x2, #1");
                cg_emit(cg, "    str x2, [x1]");
                cg_emit(cg, "    ldr x0, [sp, #16]");
                cg_emit(cg, "    bl %sstrlen", cg_sym_prefix(cg));
                cg_emit(cg, "    add x19, x20, x0");
                cg_emit(cg, "    b L%d", loop_lbl);
                cg_emit(cg, "L%d:", end_lbl);
                cg_emit(cg, "    ldr x1, [sp]");
                cg_emit(cg, "    ldr x2, [x1]");
                cg_emit(cg, "    add x3, x2, #2");
                cg_emit(cg, "    str x19, [x1, x3, lsl #3]");
                cg_emit(cg, "    add x2, x2, #1");
                cg_emit(cg, "    str x2, [x1]");
                cg_emit(cg, "    ldr x0, [sp], #16");
                cg_emit(cg, "    add sp, sp, #16");
                cg_emit(cg, "    ldp x19, x20, [sp], #16");
                break;
            }
            // Handle built-in tcp_send(fd, data) - returns bytes sent
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "tcp_send") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    str x0, [sp, #-16]!");  // save fd
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    mov x1, x0");  // buf
                // Get string length
                cg_emit(cg, "    mov x2, #0");
                int loop_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                cg_emit(cg, "L%d:", loop_lbl);
                cg_emit(cg, "    ldrb w3, [x1, x2]");
                cg_emit(cg, "    cbz w3, L%d", end_lbl);
                cg_emit(cg, "    add x2, x2, #1");
                cg_emit(cg, "    b L%d", loop_lbl);
                cg_emit(cg, "L%d:", end_lbl);
                cg_emit(cg, "    mov x1, x0");  // buf
                cg_emit(cg, "    ldr x0, [sp], #16");  // fd
                cg_emit(cg, "    mov x3, #0");  // flags
                cg_emit(cg, "    bl %ssend", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in tcp_recv(fd, maxlen) - returns string
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "tcp_recv") == 0) {
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    str x0, [sp, #-16]!");  // save maxlen
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    str x0, [sp, #-16]!");  // save fd
                cg_emit(cg, "    ldr x0, [sp, #16]");  // maxlen
                cg_emit(cg, "    add x0, x0, #1");  // +1 for null
                cg_emit(cg, "    bl %smalloc", cg_sym_prefix(cg));
                cg_emit(cg, "    mov x19, x0");  // save buf ptr
                cg_emit(cg, "    ldr x0, [sp], #16");  // fd
                cg_emit(cg, "    mov x1, x19");  // buf
                cg_emit(cg, "    ldr x2, [sp], #16");  // maxlen
                cg_emit(cg, "    mov x3, #0");  // flags
                cg_emit(cg, "    bl %srecv", cg_sym_prefix(cg));
                cg_emit(cg, "    strb wzr, [x19, x0]");  // null terminate
                cg_emit(cg, "    mov x0, x19");  // return buf
                break;
            }
            // Handle built-in tcp_listen(port) - returns listening fd
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "tcp_listen") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %s_wyn_tcp_listen", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in tcp_accept(fd) - returns client fd
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "tcp_accept") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    mov x1, #0");  // NULL
                cg_emit(cg, "    mov x2, #0");  // NULL
                cg_emit(cg, "    bl %saccept", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in getcwd() - returns current directory
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "getcwd") == 0) {
                cg_emit(cg, "    mov x0, #1024");
                cg_emit(cg, "    bl %smalloc", cg_sym_prefix(cg));
                cg_emit(cg, "    mov x1, #1024");
                cg_emit(cg, "    bl %sgetcwd", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in chdir(path) - change directory, returns 0 on success
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "chdir") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %schdir", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in sqrt(float) -> float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "sqrtf") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    fsqrt d0, d0");
                cg_emit(cg, "    fmov x0, d0");
                break;
            }
            // Handle built-in sin(float) -> float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "sinf") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    bl %ssin", cg_sym_prefix(cg));
                cg_emit(cg, "    fmov x0, d0");
                break;
            }
            // Handle built-in cos(float) -> float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "cosf") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    bl %scos", cg_sym_prefix(cg));
                cg_emit(cg, "    fmov x0, d0");
                break;
            }
            // Handle built-in tan(float) -> float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "tanf") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    bl %stan", cg_sym_prefix(cg));
                cg_emit(cg, "    fmov x0, d0");
                break;
            }
            // Handle built-in log(float) -> float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "logf") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    bl %slog", cg_sym_prefix(cg));
                cg_emit(cg, "    fmov x0, d0");
                break;
            }
            // Handle built-in exp(float) -> float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "expf") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    bl %sexp", cg_sym_prefix(cg));
                cg_emit(cg, "    fmov x0, d0");
                break;
            }
            // Handle built-in powf(base, exp) -> float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "powf") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    fmov d1, x0");
                cg_emit(cg, "    ldr x0, [sp], #16");
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    bl %spow", cg_sym_prefix(cg));
                cg_emit(cg, "    fmov x0, d0");
                break;
            }
            // Handle built-in floor(float) -> float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "floorf") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    frintm d0, d0");
                cg_emit(cg, "    fmov x0, d0");
                break;
            }
            // Handle built-in ceil(float) -> float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "ceilf") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    frintp d0, d0");
                cg_emit(cg, "    fmov x0, d0");
                break;
            }
            // Handle built-in asin(float) -> float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "asinf") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    bl %sasin", cg_sym_prefix(cg));
                cg_emit(cg, "    fmov x0, d0");
                break;
            }
            // Handle built-in acos(float) -> float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "acosf") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    bl %sacos", cg_sym_prefix(cg));
                cg_emit(cg, "    fmov x0, d0");
                break;
            }
            // Handle built-in atan(float) -> float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "atanf") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    bl %satan", cg_sym_prefix(cg));
                cg_emit(cg, "    fmov x0, d0");
                break;
            }
            // Handle built-in atan2(y, x) -> float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "atan2f") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    fmov d1, x0");
                cg_emit(cg, "    ldr x0, [sp], #16");
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    bl %satan2", cg_sym_prefix(cg));
                cg_emit(cg, "    fmov x0, d0");
                break;
            }
            // Handle built-in log10(float) -> float
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "log10f") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    fmov d0, x0");
                cg_emit(cg, "    bl %slog10", cg_sym_prefix(cg));
                cg_emit(cg, "    fmov x0, d0");
                break;
            }
            // Handle built-in getpid() -> int
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "getpid") == 0) {
                cg_emit(cg, "    bl %sgetpid", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in rename_file(old, new) -> int (0 on success)
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "rename_file") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    mov x1, x0");
                cg_emit(cg, "    ldr x0, [sp], #16");
                cg_emit(cg, "    bl %srename", cg_sym_prefix(cg));
                break;
            }
            // Handle built-in to_string(val) - convert to string for interpolation
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "to_string") == 0) {
                Expr* arg = e->call.args[0];
                Type* t = cg_local_type(cg, arg->kind == EXPR_IDENT ? arg->ident : NULL);
                cg_expr(cg, arg);
                if (t && t->kind == TYPE_STR) {
                    break;  // already a string
                }
                // Allocate buffer and convert int to string
                cg_emit(cg, "    str x0, [sp, #-16]!");  // save value
                cg_emit(cg, "    mov x0, #24");
                cg_emit(cg, "    bl %smalloc", cg_sym_prefix(cg));
                cg_emit(cg, "    str x0, [sp, #-16]!");  // save buffer ptr
                cg_emit(cg, "    ldr x2, [sp, #16]");  // value
                cg_emit_adrp(cg, "x1", "L_.itsfmt");
                cg_emit_add_pageoff(cg, "x1", "x1", "L_.itsfmt");
                cg_emit(cg, "    bl %ssprintf", cg_sym_prefix(cg));
                cg_emit(cg, "    ldr x0, [sp], #16");  // return buffer ptr
                cg_emit(cg, "    add sp, sp, #16");  // clean up value
                break;
            }
            // Handle built-in substring(str, start, len) -> str
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "substring") == 0) {
                // Call runtime function instead of inline code
                cg_expr(cg, e->call.args[2]);  // end
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[1]);  // start
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[0]);  // str
                cg_emit(cg, "    ldr x1, [sp], #16");  // start
                cg_emit(cg, "    ldr x2, [sp], #16");  // end
                cg_emit(cg, "    bl _substring");
                break;
            }
            // Handle built-in assert()
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "assert") == 0) {
                int ok_lbl = cg_new_label(cg);
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    cbnz x0, L%d", ok_lbl);
                cg_emit(cg, "    mov x0, #1");
                cg_emit(cg, "    bl %sexit", cg_sym_prefix(cg));
                cg_emit(cg, "L%d:", ok_lbl);
                break;
            }
            // Handle built-in exit()
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "exit") == 0) {
                if (e->call.arg_count > 0) {
                    cg_expr(cg, e->call.args[0]);
                } else {
                    cg_emit(cg, "    mov x0, #0");
                }
                cg_emit(cg, "    bl %sexit", cg_sym_prefix(cg));
                break;
            }
            // Built-in methods on primitive types (desugar to free functions)
            if (e->call.func->kind == EXPR_FIELD) {
                const char* method = e->call.func->field.field;
                Expr* obj = e->call.func->field.object;
                
                // .len() -> len(obj)
                if (strcmp(method, "len") == 0 && e->call.arg_count == 0) {
                    // Check if object is array type
                    Type* obj_type = NULL;
                    if (obj->kind == EXPR_IDENT) {
                        obj_type = cg_local_type(cg, obj->ident);
                    }
                    bool is_array = (obj_type && obj_type->kind == TYPE_ARRAY);
                    cg_expr(cg, obj);
                    if (is_array) {
                        // Array - read length from first element
                        cg_emit(cg, "    ldr x0, [x0]");
                    } else {
                        // String - count chars until null terminator
                        int loop_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                        cg_emit(cg, "    mov x1, x0");
                        cg_emit(cg, "    mov x0, #0");
                        cg_emit(cg, "L%d:", loop_lbl);
                        cg_emit(cg, "    ldrb w2, [x1], #1");
                        cg_emit(cg, "    cbz w2, L%d", end_lbl);
                        cg_emit(cg, "    add x0, x0, #1");
                        cg_emit(cg, "    b L%d", loop_lbl);
                        cg_emit(cg, "L%d:", end_lbl);
                    }
                    break;
                }
                // .abs() -> abs(obj)
                if (strcmp(method, "abs") == 0 && e->call.arg_count == 0) {
                    cg_expr(cg, obj);
                    cg_emit(cg, "    cmp x0, #0");
                    cg_emit(cg, "    cneg x0, x0, lt");
                    break;
                }
                // .to_str() -> int_to_str(obj)
                if (strcmp(method, "to_str") == 0 && e->call.arg_count == 0) {
                    cg_expr(cg, obj);
                    cg_emit(cg, "    mov x8, sp");
                    cg_emit(cg, "    str x0, [x8]");
                    cg_emit_adrp(cg, "x0", "L_.itsbuf");
                    cg_emit_add_pageoff(cg, "x0", "x0", "L_.itsbuf");
                    cg_emit_adrp(cg, "x1", "L_.itsfmt");
                    cg_emit_add_pageoff(cg, "x1", "x1", "L_.itsfmt");
                    cg_emit(cg, "    bl %ssprintf", cg_sym_prefix(cg));
                    cg_emit_adrp(cg, "x0", "L_.itsbuf");
                    cg_emit_add_pageoff(cg, "x0", "x0", "L_.itsbuf");
                    break;
                }
                // .to_int() -> str_to_int(obj)
                if (strcmp(method, "to_int") == 0 && e->call.arg_count == 0) {
                    cg_expr(cg, obj);
                    cg_emit(cg, "    mov x1, #0");
                    cg_emit(cg, "    mov x2, #10");
                    cg_emit(cg, "    bl %sstrtol", cg_sym_prefix(cg));
                    break;
                }
                // .to_float() -> int_to_float(obj)
                if (strcmp(method, "to_float") == 0 && e->call.arg_count == 0) {
                    cg_expr(cg, obj);
                    cg_emit(cg, "    scvtf d0, x0");
                    cg_emit(cg, "    fmov x0, d0");
                    break;
                }
                // .contains(substr) -> str_find(obj, substr) != -1
                if (strcmp(method, "contains") == 0 && e->call.arg_count == 1) {
                    cg_expr(cg, obj);
                    cg_emit(cg, "    str x0, [sp, #-16]!");
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    mov x1, x0");
                    cg_emit(cg, "    ldr x0, [sp], #16");
                    cg_emit(cg, "    str x0, [sp, #-16]!");
                    cg_emit(cg, "    bl %sstrstr", cg_sym_prefix(cg));
                    cg_emit(cg, "    ldr x1, [sp], #16");
                    cg_emit(cg, "    cbnz x0, 1f");
                    cg_emit(cg, "    mov x0, #0");
                    cg_emit(cg, "    b 2f");
                    cg_emit(cg, "1:  mov x0, #1");
                    cg_emit(cg, "2:");
                    break;
                }
                // .index_of(substr) -> str_find(obj, substr)
                if (strcmp(method, "index_of") == 0 && e->call.arg_count == 1) {
                    cg_expr(cg, obj);
                    cg_emit(cg, "    str x0, [sp, #-16]!");
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    mov x1, x0");
                    cg_emit(cg, "    ldr x0, [sp], #16");
                    cg_emit(cg, "    str x0, [sp, #-16]!");
                    cg_emit(cg, "    bl %sstrstr", cg_sym_prefix(cg));
                    cg_emit(cg, "    ldr x1, [sp], #16");
                    int found_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                    cg_emit(cg, "    cbnz x0, L%d", found_lbl);
                    cg_emit(cg, "    mov x0, #-1");
                    cg_emit(cg, "    b L%d", end_lbl);
                    cg_emit(cg, "L%d:", found_lbl);
                    cg_emit(cg, "    sub x0, x0, x1");
                    cg_emit(cg, "L%d:", end_lbl);
                    break;
                }
                // .append(item) -> __wyn_array_append(obj, item)
                if (strcmp(method, "append") == 0 && e->call.arg_count == 1) {
                    cg_expr(cg, obj);
                    cg_emit(cg, "    str x0, [sp, #-16]!");
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    mov x1, x0");
                    cg_emit(cg, "    ldr x0, [sp], #16");
                    cg_emit(cg, "    bl __wyn_array_append");
                    break;
                }
            }
            // Check for module.function() call first
            if (e->call.func->kind == EXPR_FIELD && 
                e->call.func->field.object->kind == EXPR_IDENT) {
                const char* module = e->call.func->field.object->ident;
                const char* function = e->call.func->field.field;
                const char* builtin_name = map_module_function(module, function);
                if (builtin_name) {
                    // It's a module.function() call - handle like a regular function call
                    // Push default values and args, then call the builtin
                    FnDef* fn = cg_lookup_fn(cg, builtin_name);
                    int total_args = e->call.arg_count;
                    if (fn) total_args = fn->param_count;
                    
                    // Push default values for missing args (in reverse order)
                    if (fn && e->call.arg_count < fn->param_count) {
                        for (int i = fn->param_count - 1; i >= e->call.arg_count; i--) {
                            if (fn->params[i].default_val) {
                                cg_expr(cg, fn->params[i].default_val);
                                cg_emit(cg, "    str x0, [sp, #-16]!");
                            } else {
                                cg_emit(cg, "    str xzr, [sp, #-16]!");
                            }
                        }
                    }
                    // Push all provided args
                    for (int i = e->call.arg_count - 1; i >= 0; i--) {
                        cg_expr(cg, e->call.args[i]);
                        cg_emit(cg, "    str x0, [sp, #-16]!");
                    }
                    // Pop into argument registers
                    for (int i = 0; i < total_args && i < 8; i++) {
                        cg_emit(cg, "    ldr x%d, [sp], #16", i);
                    }
                    cg_emit(cg, "    bl %s%s", cg_sym_prefix(cg), builtin_name);
                    break;
                }
            }
            // Method call: obj.method(args)
            if (e->call.func->kind == EXPR_FIELD) {
                // Push args in reverse
                for (int i = e->call.arg_count - 1; i >= 0; i--) {
                    cg_expr(cg, e->call.args[i]);
                    cg_emit(cg, "    str x0, [sp, #-16]!");
                }
                // Push self (object) - need address, not value
                Expr* obj = e->call.func->field.object;
                if (obj->kind == EXPR_IDENT) {
                    Type* obj_type = cg_local_type(cg, obj->ident);
                    int struct_size = cg_struct_size(cg, obj_type);
                    if (struct_size > 0) {
                        // Inline struct local - get address
                        int off = cg_local_offset(cg, obj->ident);
                        cg_emit_add_offset(cg, "x0", 16 + off);
                    } else {
                        cg_expr(cg, obj);
                    }
                } else {
                    cg_expr(cg, obj);
                }
                cg_emit(cg, "    str x0, [sp, #-16]!");
                // Pop into registers (self first)
                for (int i = 0; i <= e->call.arg_count && i < 8; i++) {
                    cg_emit(cg, "    ldr x%d, [sp], #16", i);
                }
                // Find struct by looking for method
                const char* struct_name = NULL;
                for (int i = 0; i < cg->module->struct_count; i++) {
                    StructDef* s = &cg->module->structs[i];
                    for (int j = 0; j < s->method_count; j++) {
                        if (strcmp(s->methods[j].name, e->call.func->field.field) == 0) {
                            struct_name = s->name;
                            break;
                        }
                    }
                    if (struct_name) break;
                }
                char mangled[256];
                snprintf(mangled, 256, "_%s_%s", struct_name ? struct_name : "STRUCT", e->call.func->field.field);
                cg_emit(cg, "    bl %s", mangled);
                break;
            }
            // Look up function for default parameters
            FnDef* fn = NULL;
            int total_args = e->call.arg_count;
            const char* func_name = NULL;
            bool is_fn_ptr = false;
            
            if (e->call.func->kind == EXPR_IDENT) {
                func_name = e->call.func->ident;
                // Check if it's a local variable (function pointer) first
                int off = cg_local_offset(cg, func_name);
                if (off > 0) {
                    is_fn_ptr = true;
                    fn = NULL;
                } else {
                    fn = cg_lookup_fn(cg, func_name);
                    if (fn) total_args = fn->param_count;
                }
            } else if (e->call.func->kind == EXPR_FIELD && 
                       e->call.func->field.object->kind == EXPR_IDENT) {
                // Check if this is a module.function() call
                const char* module = e->call.func->field.object->ident;
                const char* function = e->call.func->field.field;
                func_name = map_module_function(module, function);
                if (func_name) {
                    // Treat as builtin function call
                    fn = cg_lookup_fn(cg, func_name);
                    if (fn) total_args = fn->param_count;
                }
            }
            
            // Push default values for missing args (in reverse order)
            if (fn && e->call.arg_count < fn->param_count) {
                for (int i = fn->param_count - 1; i >= e->call.arg_count; i--) {
                    if (fn->params[i].default_val) {
                        cg_expr(cg, fn->params[i].default_val);
                        cg_emit(cg, "    str x0, [sp, #-16]!");
                    } else {
                        cg_emit(cg, "    str xzr, [sp, #-16]!");  // Zero for missing
                    }
                }
            }
            // Push all provided args to stack
            for (int i = e->call.arg_count - 1; i >= 0; i--) {
                Expr* arg = e->call.args[i];
                // Check if argument is a struct variable - pass address instead of value
                if (arg->kind == EXPR_IDENT) {
                    Type* arg_type = cg_local_type(cg, arg->ident);
                    int struct_size = cg_struct_size(cg, arg_type);
                    if (struct_size > 0) {
                        // Pass address of inline struct
                        int off = cg_local_offset(cg, arg->ident);
                        cg_emit_add_offset(cg, "x0", 16 + off);
                        cg_emit(cg, "    str x0, [sp, #-16]!");
                        continue;
                    }
                }
                cg_expr(cg, arg);
                cg_emit(cg, "    str x0, [sp, #-16]!");
            }
            // Pop into argument registers
            for (int i = 0; i < total_args && i < 8; i++) {
                cg_emit(cg, "    ldr x%d, [sp], #16", i);
            }
            if (is_fn_ptr) {
                // Indirect call through function pointer variable
                int off = cg_local_offset(cg, e->call.func->ident);
                cg_emit(cg, "    ldr x9, [x29, #%d]", 16 + off);
                cg_emit(cg, "    blr x9");
            } else if (func_name) {
                cg_emit(cg, "    bl %s%s", cg_sym_prefix(cg), func_name);
            } else if (e->call.func->kind == EXPR_IDENT) {
                // Check if it's a local variable (function pointer) or global function
                if (fn) {
                    // Direct call to known function
                    cg_emit(cg, "    bl %s%s", cg_sym_prefix(cg), e->call.func->ident);
                } else {
                    // Indirect call through function pointer variable
                    int off = cg_local_offset(cg, e->call.func->ident);
                    if (off > 0) {
                        cg_emit(cg, "    ldr x9, [x29, #%d]", 16 + off);
                        cg_emit(cg, "    blr x9");
                    } else {
                        // Fallback to direct call
                        cg_emit(cg, "    bl %s%s", cg_sym_prefix(cg), e->call.func->ident);
                    }
                }
            }
            break;
        }
        
        case EXPR_ARRAY: {
            // Allocate array in local variable area: [length, capacity, elem0, elem1, ...]
            int count = e->array.count;
            // Reserve space for length + capacity + elements
            int base_off = cg->stack_offset + 8;  // Start after current locals
            cg->stack_offset += (count + 2) * 8;  // +2 for length and capacity
            // Store length
            cg_emit(cg, "    mov x0, #%d", count);
            cg_emit_str_offset(cg, "x0", 16 + base_off);
            // Store capacity (same as length initially)
            cg_emit_str_offset(cg, "x0", 16 + base_off + 8);
            // Store elements
            for (int i = 0; i < count; i++) {
                cg_expr(cg, e->array.elements[i]);
                cg_emit_str_offset(cg, "x0", 16 + base_off + (i + 2) * 8);
            }
            // Return pointer to array
            cg_emit_add_offset(cg, "x0", 16 + base_off);
            break;
        }
        
        case EXPR_INDEX: {
            // arr[idx] - array is pointer, first element is length, second is capacity
            cg_expr(cg, e->index.index);
            cg_emit(cg, "    str x0, [sp, #-16]!");  // Save index
            cg_expr(cg, e->index.object);
            cg_emit(cg, "    ldr x1, [sp], #16");    // Restore index
            if (!e->index.safe) {
                // Handle negative index: if idx < 0, idx += len
                cg_emit(cg, "    cmp x1, #0");
                cg_emit(cg, "    b.ge 1f");
                cg_emit(cg, "    ldr x2, [x0]");         // x2 = len
                cg_emit(cg, "    add x1, x1, x2");       // idx += len
                cg_emit(cg, "1:");
            }
            cg_emit(cg, "    add x1, x1, #2");       // Skip length and capacity fields
            cg_emit(cg, "    ldr x0, [x0, x1, lsl #3]");  // Load arr[idx+2]
            break;
        }
        
        case EXPR_SLICE: {
            // Check if slicing a string
            Type* obj_type = NULL;
            if (e->slice.object->kind == EXPR_IDENT) {
                obj_type = cg_local_type(cg, e->slice.object->ident);
            } else if (e->slice.object->kind == EXPR_STRING) {
                obj_type = new_type(TYPE_STR);
            }
            
            bool is_string = (obj_type && obj_type->kind == TYPE_STR) || 
                            (e->slice.object->kind == EXPR_STRING);
            
            if (is_string) {
                // String slicing - allocate buffer and copy bytes
                int buf_off = cg->stack_offset + 8;
                cg->stack_offset += 256;  // 256 byte buffer
                
                // Get string pointer
                cg_expr(cg, e->slice.object);
                cg_emit(cg, "    str x0, [sp, #-16]!");  // Save string ptr
                
                // Get string length
                cg_expr(cg, e->slice.object);
                int len_loop = cg_new_label(cg), len_end = cg_new_label(cg);
                cg_emit(cg, "    mov x1, x0");
                cg_emit(cg, "    mov x8, #0");
                cg_emit(cg, "L%d:", len_loop);
                cg_emit(cg, "    ldrb w2, [x1], #1");
                cg_emit(cg, "    cbz w2, L%d", len_end);
                cg_emit(cg, "    add x8, x8, #1");
                cg_emit(cg, "    b L%d", len_loop);
                cg_emit(cg, "L%d:", len_end);  // x8 = len
                
                // Compute start
                if (e->slice.start) {
                    cg_expr(cg, e->slice.start);
                    cg_emit(cg, "    mov x9, x0");
                    cg_emit(cg, "    cmp x9, #0");
                    cg_emit(cg, "    b.ge 1f");
                    cg_emit(cg, "    add x9, x9, x8");
                    cg_emit(cg, "1:");
                } else {
                    cg_emit(cg, "    mov x9, #0");
                }
                
                // Compute end
                if (e->slice.end) {
                    cg_expr(cg, e->slice.end);
                    cg_emit(cg, "    mov x10, x0");
                    cg_emit(cg, "    cmp x10, #0");
                    cg_emit(cg, "    b.ge 2f");
                    cg_emit(cg, "    add x10, x10, x8");
                    cg_emit(cg, "2:");
                } else {
                    cg_emit(cg, "    mov x10, x8");
                }
                
                // Copy bytes
                cg_emit(cg, "    ldr x1, [sp], #16");  // Restore string ptr
                cg_emit_add_offset(cg, "x0", 16 + buf_off);  // dest buffer
                cg_emit(cg, "    add x1, x1, x9");  // src = str + start
                cg_emit(cg, "    sub x2, x10, x9");  // len = end - start
                cg_emit(cg, "    bl %smemcpy", cg_sym_prefix(cg));
                // Null terminate
                cg_emit(cg, "    sub x1, x10, x9");
                cg_emit_add_offset(cg, "x0", 16 + buf_off);
                cg_emit(cg, "    strb wzr, [x0, x1]");
                cg_emit_add_offset(cg, "x0", 16 + buf_off);
                break;
            }
            
            // Array slicing
            // Evaluate object (array pointer)
            cg_expr(cg, e->slice.object);
            cg_emit(cg, "    str x0, [sp, #-16]!");  // Save array ptr
            cg_emit(cg, "    ldr x8, [x0]");  // x8 = len
            
            // Compute start (default 0)
            if (e->slice.start) {
                cg_expr(cg, e->slice.start);
                cg_emit(cg, "    mov x9, x0");  // x9 = start
                // Handle negative index
                cg_emit(cg, "    cmp x9, #0");
                cg_emit(cg, "    b.ge 1f");
                cg_emit(cg, "    ldr x0, [sp]");
                cg_emit(cg, "    ldr x8, [x0]");
                cg_emit(cg, "    add x9, x9, x8");  // start += len
                cg_emit(cg, "1:");
            } else {
                cg_emit(cg, "    mov x9, #0");  // start = 0
            }
            cg_emit(cg, "    str x9, [sp, #-16]!");  // Save start
            
            // Compute end (default len)
            if (e->slice.end) {
                cg_expr(cg, e->slice.end);
                cg_emit(cg, "    mov x10, x0");  // x10 = end
                // Handle negative index
                cg_emit(cg, "    cmp x10, #0");
                cg_emit(cg, "    b.ge 2f");
                cg_emit(cg, "    ldr x0, [sp, #16]");  // Get array ptr
                cg_emit(cg, "    ldr x8, [x0]");
                cg_emit(cg, "    add x10, x10, x8");  // end += len
                cg_emit(cg, "2:");
            } else {
                cg_emit(cg, "    ldr x0, [sp, #16]");  // Get array ptr
                cg_emit(cg, "    ldr x10, [x0]");  // end = len
            }
            
            cg_emit(cg, "    ldr x9, [sp], #16");  // Restore start
            
            // Compute new length: end - start
            cg_emit(cg, "    sub x11, x10, x9");  // x11 = new_len
            
            // Allocate new array on stack (use smaller buffer: 64 elements max)
            int base_off = cg->stack_offset + 8;
            cg->stack_offset += 64 * 8;
            // Use mov for large immediates
            cg_emit(cg, "    mov x16, #%d", 16 + base_off);
            cg_emit(cg, "    add x16, x29, x16");  // x16 = base address
            cg_emit(cg, "    str x11, [x16]");  // Store length
            
            // Copy elements: for i = 0; i < new_len; i++
            cg_emit(cg, "    mov x12, #0");  // i = 0
            cg_emit(cg, "    ldr x0, [sp], #16");  // Get array ptr
            cg_emit(cg, "3:");
            cg_emit(cg, "    cmp x12, x11");
            cg_emit(cg, "    b.ge 4f");
            // src_idx = start + i + 1 (skip length)
            cg_emit(cg, "    add x13, x9, x12");
            cg_emit(cg, "    add x13, x13, #1");
            cg_emit(cg, "    ldr x14, [x0, x13, lsl #3]");  // Load element
            // Store at new_arr[i+1]
            cg_emit(cg, "    add x13, x12, #1");
            cg_emit(cg, "    str x14, [x16, x13, lsl #3]");
            cg_emit(cg, "    add x12, x12, #1");
            cg_emit(cg, "    b 3b");
            cg_emit(cg, "4:");
            cg_emit(cg, "    mov x0, x16");
            break;
        }
        
        case EXPR_STRUCT: {
            StructDef* s = cg_lookup_struct(cg, e->struct_lit.name);
            int size = s ? s->field_count * 8 : e->struct_lit.count * 8;
            int base_off = cg->stack_offset + 8;
            cg->stack_offset += size;
            for (int i = 0; i < e->struct_lit.count; i++) {
                int foff = s ? cg_struct_field_offset(s, e->struct_lit.fields[i]) : i * 8;
                if (foff < 0) foff = i * 8;
                cg_expr(cg, e->struct_lit.values[i]);
                cg_emit_str_offset(cg, "x0", 16 + base_off + foff);
            }
            cg_emit_add_offset(cg, "x0", 16 + base_off);
            break;
        }
        
        case EXPR_FIELD: {
            // Check if object is a local struct variable (stored inline)
            if (e->field.object->kind == EXPR_IDENT) {
                Type* obj_type = cg_local_type(cg, e->field.object->ident);
                int struct_size = cg_struct_size(cg, obj_type);
                if (struct_size > 0) {
                    // Local struct stored inline - check if field is also a struct
                    int base_off = cg_local_offset(cg, e->field.object->ident);
                    for (int i = 0; i < cg->module->struct_count; i++) {
                        int foff = cg_struct_field_offset(&cg->module->structs[i], e->field.field);
                        if (foff >= 0) {
                            // Check if this field is itself a struct type
                            Type* field_type = cg_struct_field_type(&cg->module->structs[i], e->field.field);
                            int field_struct_size = cg_struct_size(cg, field_type);
                            if (field_struct_size > 0) {
                                // Field is a struct - load the pointer stored at this offset
                                cg_emit_ldr_offset(cg, "x0", 16 + base_off + foff);
                            } else {
                                // Field is a primitive - load value
                                cg_emit_ldr_offset(cg, "x0", 16 + base_off + foff);
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            // Default: object is a pointer to struct (or expression returning pointer)
            cg_expr(cg, e->field.object);
            for (int i = 0; i < cg->module->struct_count; i++) {
                int off = cg_struct_field_offset(&cg->module->structs[i], e->field.field);
                if (off >= 0) {
                    // Check if this field is itself a struct type
                    Type* field_type = cg_struct_field_type(&cg->module->structs[i], e->field.field);
                    int field_struct_size = cg_struct_size(cg, field_type);
                    if (field_struct_size > 0) {
                        // Field is a struct - load the pointer, then add offset
                        cg_emit(cg, "    ldr x0, [x0, #%d]", off);
                    } else {
                        // Field is a primitive - load value
                        cg_emit(cg, "    ldr x0, [x0, #%d]", off);
                    }
                    break;
                }
            }
            break;
        }
        
        case EXPR_SOME: {
            // Optional with value: store [1, value] on stack
            int base_off = cg->stack_offset + 8;
            cg->stack_offset += 16;
            cg_emit(cg, "    mov x0, #1");
            cg_emit_str_offset(cg, "x0", 16 + base_off);  // has_value = 1
            cg_expr(cg, e->some.value);
            cg_emit_str_offset(cg, "x0", 16 + base_off + 8);  // value
            cg_emit_add_offset(cg, "x0", 16 + base_off);
            break;
        }
        
        case EXPR_NONE: {
            // Optional without value: store [0, 0] on stack
            int base_off = cg->stack_offset + 8;
            cg->stack_offset += 16;
            cg_emit(cg, "    mov x0, #0");
            cg_emit_str_offset(cg, "x0", 16 + base_off);  // has_value = 0
            cg_emit_str_offset(cg, "x0", 16 + base_off + 8);  // value = 0
            cg_emit_add_offset(cg, "x0", 16 + base_off);
            break;
        }
        
        case EXPR_OK: {
            // Result ok: store [1, value] on stack
            int base_off = cg->stack_offset + 8;
            cg->stack_offset += 16;
            cg_emit(cg, "    mov x0, #1");
            cg_emit_str_offset(cg, "x0", 16 + base_off);  // is_ok = 1
            cg_expr(cg, e->some.value);
            cg_emit_str_offset(cg, "x0", 16 + base_off + 8);  // value
            cg_emit_add_offset(cg, "x0", 16 + base_off);
            break;
        }
        
        case EXPR_ERR: {
            // Result err: store [0, error] on stack
            int base_off = cg->stack_offset + 8;
            cg->stack_offset += 16;
            cg_emit(cg, "    mov x0, #0");
            cg_emit_str_offset(cg, "x0", 16 + base_off);  // is_ok = 0
            cg_expr(cg, e->some.value);
            cg_emit_str_offset(cg, "x0", 16 + base_off + 8);  // error
            cg_emit_add_offset(cg, "x0", 16 + base_off);
            break;
        }
        
        case EXPR_DEFAULT: {
            // value ?? default - if value has_value, use it, else use default
            int use_default = cg_new_label(cg), end_lbl = cg_new_label(cg);
            cg_expr(cg, e->default_expr.value);
            cg_emit(cg, "    ldr x1, [x0]");  // has_value
            cg_emit(cg, "    cbz x1, L%d", use_default);
            cg_emit(cg, "    ldr x0, [x0, #8]");  // use value
            cg_emit(cg, "    b L%d", end_lbl);
            cg_emit(cg, "L%d:", use_default);
            cg_expr(cg, e->default_expr.default_val);
            cg_emit(cg, "L%d:", end_lbl);
            break;
        }
        
        case EXPR_IF: {
            // if cond then expr1 else expr2
            int else_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
            cg_expr(cg, e->if_expr.cond);
            cg_emit(cg, "    cbz x0, L%d", else_lbl);
            cg_expr(cg, e->if_expr.then_expr);
            cg_emit(cg, "    b L%d", end_lbl);
            cg_emit(cg, "L%d:", else_lbl);
            cg_expr(cg, e->if_expr.else_expr);
            cg_emit(cg, "L%d:", end_lbl);
            break;
        }
        
        case EXPR_MATCH: {
            int end_lbl = cg_new_label(cg);
            cg_expr(cg, e->match_expr.value);
            cg_emit(cg, "    str x0, [sp, #-16]!");
            for (int i = 0; i < e->match_expr.arm_count; i++) {
                int next_lbl = cg_new_label(cg);
                Expr* pat = e->match_expr.patterns[i];
                if (pat->kind == EXPR_IDENT && strcmp(pat->ident, "_") == 0) {
                    cg_emit(cg, "    ldr x0, [sp], #16");
                    cg_expr(cg, e->match_expr.arms[i]);
                    cg_emit(cg, "    b L%d", end_lbl);
                } else {
                    cg_expr(cg, pat);
                    cg_emit(cg, "    ldr x1, [sp]");
                    cg_emit(cg, "    cmp x0, x1");
                    cg_emit(cg, "    b.ne L%d", next_lbl);
                    cg_emit(cg, "    ldr x0, [sp], #16");
                    cg_expr(cg, e->match_expr.arms[i]);
                    cg_emit(cg, "    b L%d", end_lbl);
                    cg_emit(cg, "L%d:", next_lbl);
                }
            }
            cg_emit(cg, "    ldr x0, [sp], #16");
            cg_emit(cg, "L%d:", end_lbl);
            break;
        }
        
        case EXPR_LAMBDA: {
            // Store lambda for later emission, return function pointer
            cg->lambdas[cg->lambda_count++] = e;
            char label[64];
            snprintf(label, sizeof(label), "%s_lambda%d", cg_sym_prefix(cg), e->lambda.id);
            cg_emit_adrp(cg, "x0", label);
            cg_emit_add_pageoff(cg, "x0", "x0", label);
            break;
        }
        
        default:
            cg_emit(cg, "    ; unsupported expr");
    }
}

// Emit all deferred expressions in LIFO order
static void cg_emit_defers(CodeGen* cg) {
    for (int i = cg->defer_count - 1; i >= 0; i--) {
        cg_expr(cg, cg->defers[i]);
    }
}

static void cg_stmt(CodeGen* cg, Stmt* s);

static void cg_stmt(CodeGen* cg, Stmt* s) {
    if (!s) return;
    
    if (cg->arch == ARCH_X86_64) {
        switch (s->kind) {
            case STMT_LET:
                cg_add_local(cg, s->let.name, s->let.type);
                if (s->let.value) cg_expr(cg, s->let.value);
                else cg_emit(cg, "    xorq %%rax, %%rax");
                cg_emit(cg, "    movq %%rax, -%d(%%rbp)", cg_local_offset(cg, s->let.name));
                break;
            case STMT_ASSIGN:
                if (s->assign.op == TOK_EQ) {
                    cg_expr(cg, s->assign.value);
                } else {
                    // Compound assignment
                    if (s->assign.target->kind == EXPR_IDENT) {
                        int off = cg_local_offset(cg, s->assign.target->ident);
                        bool is_shared = cg_is_shared(cg, s->assign.target->ident);
                        
                        if (is_shared && (s->assign.op == TOK_PLUSEQ || s->assign.op == TOK_MINUSEQ)) {
                            // Use mutex-based atomic for 100% reliability
                            cg_expr(cg, s->assign.value);
                            cg_emit(cg, "    movq %%rax, %%rsi");  // value
                            if (s->assign.op == TOK_MINUSEQ) {
                                cg_emit(cg, "    negq %%rsi");
                            }
                            if (cg->in_thread_context) {
                                cg_emit(cg, "    movq -%d(%%rbp), %%rdi", off);  // ptr
                            } else {
                                cg_emit(cg, "    leaq -%d(%%rbp), %%rdi", off);  // ptr
                            }
                            cg_emit(cg, "    call ___wyn_atomic_add");
                            break;
                        } else if (is_shared) {
                            // For other ops on shared vars, use CAS loop
                            int retry_lbl = cg_new_label(cg);
                            if (cg->in_thread_context) {
                                cg_emit(cg, "    movq -%d(%%rbp), %%r12", off);
                            } else {
                                cg_emit(cg, "    leaq -%d(%%rbp), %%r12", off);
                            }
                            cg_emit(cg, "L_cas_%d:", retry_lbl);
                            cg_emit(cg, "    movq (%%r12), %%rax");
                            cg_emit(cg, "    pushq %%rax");
                            cg_expr(cg, s->assign.value);
                            cg_emit(cg, "    movq %%rax, %%rcx");
                            cg_emit(cg, "    popq %%rax");
                            switch (s->assign.op) {
                                case TOK_STAREQ: cg_emit(cg, "    imulq %%rcx, %%rax"); break;
                                case TOK_SLASHEQ: cg_emit(cg, "    cqo"); cg_emit(cg, "    idivq %%rcx"); break;
                                case TOK_PERCENTEQ: cg_emit(cg, "    cqo"); cg_emit(cg, "    idivq %%rcx"); cg_emit(cg, "    movq %%rdx, %%rax"); break;
                                default: break;
                            }
                            cg_emit(cg, "    movq %%rax, %%rdx");
                            cg_emit(cg, "    movq (%%r12), %%rax");
                            cg_emit(cg, "    lock cmpxchgq %%rdx, (%%r12)");
                            cg_emit(cg, "    jne L_cas_%d", retry_lbl);
                            cg_emit(cg, "    movq %%rdx, %%rax");
                            break;  // Skip normal store
                        } else {
                            // Non-shared: normal operation
                            if (off) {
                                cg_emit(cg, "    movq -%d(%%rbp), %%rax", off);
                            }
                        }
                    }
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, s->assign.value);
                    cg_emit(cg, "    movq %%rax, %%rcx");
                    cg_emit(cg, "    popq %%rax");
                    switch (s->assign.op) {
                        case TOK_PLUSEQ: cg_emit(cg, "    addq %%rcx, %%rax"); break;
                        case TOK_MINUSEQ: cg_emit(cg, "    subq %%rcx, %%rax"); break;
                        case TOK_STAREQ: cg_emit(cg, "    imulq %%rcx, %%rax"); break;
                        case TOK_SLASHEQ: cg_emit(cg, "    cqo"); cg_emit(cg, "    idivq %%rcx"); break;
                        case TOK_PERCENTEQ: cg_emit(cg, "    cqo"); cg_emit(cg, "    idivq %%rcx"); cg_emit(cg, "    movq %%rdx, %%rax"); break;
                        default: break;
                    }
                }
                if (s->assign.target->kind == EXPR_IDENT) {
                    int off = cg_local_offset(cg, s->assign.target->ident);
                    if (off) {
                        if (cg_is_shared(cg, s->assign.target->ident)) {
                            cg_atomic_store(cg, s->assign.target->ident, off);
                        } else {
                            cg_emit(cg, "    movq %%rax, -%d(%%rbp)", off);
                        }
                    }
                } else if (s->assign.target->kind == EXPR_DEREF) {
                    // *ptr = value: store through pointer
                    cg_emit(cg, "    pushq %%rax");  // save value
                    cg_expr(cg, s->assign.target->unary.operand);  // get pointer
                    cg_emit(cg, "    popq %%rcx");  // restore value
                    cg_emit(cg, "    movq %%rcx, (%%rax)");  // store through pointer
                } else if (s->assign.target->kind == EXPR_FIELD) {
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, s->assign.target->field.object);
                    cg_emit(cg, "    popq %%rcx");
                    for (int i = 0; i < cg->module->struct_count; i++) {
                        int off = cg_struct_field_offset(&cg->module->structs[i], s->assign.target->field.field);
                        if (off >= 0) {
                            cg_emit(cg, "    movq %%rcx, %d(%%rax)", off);
                            break;
                        }
                    }
                }
                break;
            case STMT_EXPR: {
                // Special handling for arr.append(x) - update the variable
                Expr* e = s->expr.expr;
                if (e->kind == EXPR_CALL && e->call.func->kind == EXPR_FIELD &&
                    strcmp(e->call.func->field.field, "append") == 0 &&
                    e->call.func->field.object->kind == EXPR_IDENT) {
                    // This is arr.append(x) where arr is a variable
                    const char* var_name = e->call.func->field.object->ident;
                    cg_expr(cg, e);  // Call append, result in rax
                    // Update the variable with the new pointer
                    int off = cg_local_offset(cg, var_name);
                    if (off >= 0) {
                        cg_emit(cg, "    movq %%rax, -%d(%%rbp)", off);
                    }
                } else {
                    cg_expr(cg, e);
                }
                break;
            }
            case STMT_RETURN:
                if (s->ret.value) cg_expr(cg, s->ret.value);
                if (cg->defer_count > 0) {
                    cg_emit(cg, "    pushq %%rax");  // Save return value
                    cg_emit_defers(cg);
                    cg_emit(cg, "    popq %%rax");   // Restore return value
                }
                cg_emit(cg, "    movq %%rbp, %%rsp");
                cg_emit(cg, "    popq %%rbp");
                cg_emit(cg, "    retq");
                break;
            case STMT_DEFER:
                if (cg->defer_count < 64) {
                    cg->defers[cg->defer_count++] = s->defer.expr;
                }
                break;
            case STMT_IF: {
                int else_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                cg_expr(cg, s->if_stmt.cond);
                cg_emit(cg, "    testq %%rax, %%rax");
                cg_emit(cg, "    je L%d", else_lbl);
                for (int i = 0; i < s->if_stmt.then_count; i++) cg_stmt(cg, s->if_stmt.then_block[i]);
                cg_emit(cg, "    jmp L%d", end_lbl);
                cg_emit(cg, "L%d:", else_lbl);
                for (int i = 0; i < s->if_stmt.else_count; i++) cg_stmt(cg, s->if_stmt.else_block[i]);
                cg_emit(cg, "L%d:", end_lbl);
                break;
            }
            case STMT_WHILE: {
                int start_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                int saved_start = cg->loop_start_label, saved_end = cg->loop_end_label;
                cg->loop_start_label = start_lbl;
                cg->loop_end_label = end_lbl;
                cg_emit(cg, "L%d:", start_lbl);
                cg_expr(cg, s->while_stmt.cond);
                cg_emit(cg, "    testq %%rax, %%rax");
                cg_emit(cg, "    je L%d", end_lbl);
                for (int i = 0; i < s->while_stmt.body_count; i++) cg_stmt(cg, s->while_stmt.body[i]);
                cg_emit(cg, "    jmp L%d", start_lbl);
                cg_emit(cg, "L%d:", end_lbl);
                cg->loop_start_label = saved_start;
                cg->loop_end_label = saved_end;
                break;
            }
            case STMT_FOR: {
                int start_lbl = cg_new_label(cg), cont_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                int saved_start = cg->loop_start_label, saved_end = cg->loop_end_label;
                cg->loop_start_label = cont_lbl;
                cg->loop_end_label = end_lbl;
                
                // Check if iterating over array or range
                bool is_array = s->for_stmt.iter && s->for_stmt.iter->kind != EXPR_BINARY;
                
                if (is_array) {
                    // Array iteration: for item in arr
                    cg_add_local(cg, "__arr", NULL);
                    int arr_off = cg->locals[cg->local_count - 1].offset;
                    cg_expr(cg, s->for_stmt.iter);
                    cg_emit(cg, "    movq %%rax, -%d(%%rbp)", arr_off);
                    
                    cg_add_local(cg, "__idx", NULL);
                    int idx_off = cg->locals[cg->local_count - 1].offset;
                    cg_emit(cg, "    movq $0, -%d(%%rbp)", idx_off);
                    
                    cg_add_local(cg, s->for_stmt.var, NULL);
                    int var_off = cg->locals[cg->local_count - 1].offset;
                    
                    cg_emit(cg, "L%d:", start_lbl);
                    cg_emit(cg, "    movq -%d(%%rbp), %%rax", arr_off);
                    cg_emit(cg, "    movq (%%rax), %%rcx");  // length
                    cg_emit(cg, "    movq -%d(%%rbp), %%rdx", idx_off);
                    cg_emit(cg, "    cmpq %%rcx, %%rdx");
                    cg_emit(cg, "    jge L%d", end_lbl);
                    
                    // Load element: arr[idx]
                    cg_emit(cg, "    movq -%d(%%rbp), %%rax", arr_off);
                    cg_emit(cg, "    movq -%d(%%rbp), %%rcx", idx_off);
                    cg_emit(cg, "    addq $1, %%rcx");
                    cg_emit(cg, "    movq (%%rax,%%rcx,8), %%rax");
                    cg_emit(cg, "    movq %%rax, -%d(%%rbp)", var_off);
                    
                    for (int i = 0; i < s->for_stmt.body_count; i++) cg_stmt(cg, s->for_stmt.body[i]);
                    cg_emit(cg, "L%d:", cont_lbl);
                    cg_emit(cg, "    incq -%d(%%rbp)", idx_off);
                    cg_emit(cg, "    jmp L%d", start_lbl);
                } else {
                    // Range iteration: for i in start..end or start..=end
                    cg_add_local(cg, s->for_stmt.var, NULL);
                    int var_off = cg->locals[cg->local_count - 1].offset;
                    cg_add_local(cg, "__end", NULL);
                    int end_off = cg->locals[cg->local_count - 1].offset;
                    bool inclusive = s->for_stmt.iter && s->for_stmt.iter->kind == EXPR_BINARY &&
                                     s->for_stmt.iter->binary.op == TOK_DOTDOTEQ;
                    if (s->for_stmt.iter && s->for_stmt.iter->kind == EXPR_BINARY) {
                        // Initialize start value
                        cg_expr(cg, s->for_stmt.iter->binary.left);
                        cg_emit(cg, "    movq %%rax, -%d(%%rbp)", var_off);
                        // Initialize end value
                        cg_expr(cg, s->for_stmt.iter->binary.right);
                        cg_emit(cg, "    movq %%rax, -%d(%%rbp)", end_off);
                    } else {
                        cg_emit(cg, "    movq $0, -%d(%%rbp)", var_off);
                        cg_emit(cg, "    movq $0, -%d(%%rbp)", end_off);
                    }
                    cg_emit(cg, "L%d:", start_lbl);
                    cg_emit(cg, "    movq -%d(%%rbp), %%rax", var_off);
                    cg_emit(cg, "    cmpq -%d(%%rbp), %%rax", end_off);
                    cg_emit(cg, "    %s L%d", inclusive ? "jg" : "jge", end_lbl);
                    for (int i = 0; i < s->for_stmt.body_count; i++) cg_stmt(cg, s->for_stmt.body[i]);
                    cg_emit(cg, "L%d:", cont_lbl);
                    cg_emit(cg, "    incq -%d(%%rbp)", var_off);
                    cg_emit(cg, "    jmp L%d", start_lbl);
                }
                
                cg_emit(cg, "L%d:", end_lbl);
                cg->loop_start_label = saved_start;
                cg->loop_end_label = saved_end;
                break;
            }
            case STMT_BREAK:
                cg_emit(cg, "    jmp L%d", cg->loop_end_label);
                break;
            case STMT_CONTINUE:
                cg_emit(cg, "    jmp L%d", cg->loop_start_label);
                break;
            case STMT_SPAWN:
                // Mark shared variables before generating thread function
                mark_shared_vars(cg, s->spawn.body, s->spawn.body_count);
                
                // Generate thread function and call __wyn_spawn
                {
                    int thread_fn_label = cg_new_label(cg);
                    int skip_label = cg_new_label(cg);
                    int cap_count = s->spawn.captured_count;
                    
                    // Skip thread function definition
                    cg_emit(cg, "    jmp L%d", skip_label);
                    
                    // Thread function - receives context in %rdi
                    cg_emit(cg, "L%d:", thread_fn_label);
                    cg_emit(cg, "    pushq %%rbp");
                    cg_emit(cg, "    movq %%rsp, %%rbp");
                    cg_emit(cg, "    subq $256, %%rsp");
                    
                    // Save context and setup locals with var addresses
                    int saved_offset = cg->stack_offset;
                    int saved_count = cg->local_count;
                    bool saved_in_thread = cg->in_thread_context;
                    // Save locals array (same type as cg->locals)
                    struct { char name[MAX_IDENT_LEN]; int offset; Type* type; bool is_shared; } saved_locals[256];
                    memcpy(saved_locals, cg->locals, sizeof(saved_locals));
                    cg->in_thread_context = true;  // Mark that we're in a thread
                    cg->stack_offset = 8;  // Reset for thread function
                    cg->local_count = 0;   // Reset locals for thread function
                    
                    // Store var addresses as locals (they'll be dereferenced on access)
                    for (int i = 0; i < cap_count; i++) {
                        cg_add_local(cg, s->spawn.captured_vars[i], NULL);
                        cg_emit(cg, "    movq %d(%%rdi), %%rax", i * 8);  // Load var address from context
                        cg_emit(cg, "    movq %%rax, -%d(%%rbp)", cg->locals[cg->local_count - 1].offset);
                        // Mark as shared so we use atomic ops
                        cg->locals[cg->local_count - 1].is_shared = true;
                    }
                    
                    // Execute spawn body
                    for (int i = 0; i < s->spawn.body_count; i++) {
                        cg_stmt(cg, s->spawn.body[i]);
                    }
                    
                    cg_emit(cg, "    xorq %%rax, %%rax");
                    cg_emit(cg, "    movq %%rbp, %%rsp");
                    cg_emit(cg, "    popq %%rbp");
                    cg_emit(cg, "    retq");
                    
                    // Restore state
                    cg->stack_offset = saved_offset;
                    cg->local_count = saved_count;
                    memcpy(cg->locals, saved_locals, sizeof(saved_locals));
                    cg->in_thread_context = saved_in_thread;
                    
                    // Back to main code - allocate context and store var addresses
                    cg_emit(cg, "L%d:", skip_label);
                    if (cap_count > 0) {
                        // malloc(cap_count * 8)
                        cg_emit(cg, "    movq $%d, %%rdi", cap_count * 8);
                        cg_emit(cg, "    call _malloc");
                        cg_emit(cg, "    movq %%rax, %%r12");  // Save context ptr
                        
                        // Store addresses of captured vars in context
                        for (int i = 0; i < cap_count; i++) {
                            int off = cg_local_offset(cg, s->spawn.captured_vars[i]);
                            if (off) {
                                cg_emit(cg, "    leaq -%d(%%rbp), %%rax", off);  // Get address
                                cg_emit(cg, "    movq %%rax, %d(%%r12)", i * 8);
                            }
                        }
                        
                        // Call __wyn_spawn with function and context
                        cg_emit(cg, "    leaq L%d(%%rip), %%rdi", thread_fn_label);
                        cg_emit(cg, "    movq %%r12, %%rsi");  // Context ptr
                        cg_emit(cg, "    call ___wyn_spawn");
                    } else {
                        cg_emit(cg, "    leaq L%d(%%rip), %%rdi", thread_fn_label);
                        cg_emit(cg, "    xorq %%rsi, %%rsi");
                        cg_emit(cg, "    call ___wyn_spawn");
                    }
                    cg->spawn_count++;
                }
                break;
            case STMT_MATCH: {
                int end_lbl = cg_new_label(cg);
                // Evaluate match value once and save
                cg_expr(cg, s->match_stmt.value);
                cg_emit(cg, "    pushq %%rax");
                for (int i = 0; i < s->match_stmt.arm_count; i++) {
                    int next_lbl = cg_new_label(cg);
                    Expr* pat = s->match_stmt.patterns[i];
                    // Check if pattern is wildcard (_)
                    bool is_wildcard = pat->kind == EXPR_IDENT && strcmp(pat->ident, "_") == 0;
                    
                    if (pat->kind == EXPR_SOME || pat->kind == EXPR_OK) {
                        // Match some(x) or ok(v): check has_value == 1
                        cg_emit(cg, "    movq (%%rsp), %%rax");  // Load optional/result ptr
                        cg_emit(cg, "    movq (%%rax), %%rcx");  // Load has_value/is_ok flag
                        cg_emit(cg, "    cmpq $1, %%rcx");
                        cg_emit(cg, "    jne L%d", next_lbl);
                        // Bind variable if present
                        if (s->match_stmt.bindings[i][0] != '\0') {
                            cg_emit(cg, "    movq (%%rsp), %%rax");
                            cg_emit(cg, "    movq 8(%%rax), %%rax");  // Load inner value
                            Type* bind_type = s->match_stmt.binding_types[i] ? s->match_stmt.binding_types[i] : new_type(TYPE_INT);
                            cg_add_local(cg, s->match_stmt.bindings[i], bind_type);
                            cg_emit(cg, "    movq %%rax, -%d(%%rbp)", cg->stack_offset);
                        }
                    } else if (pat->kind == EXPR_NONE) {
                        // Match none: check has_value == 0
                        cg_emit(cg, "    movq (%%rsp), %%rax");
                        cg_emit(cg, "    movq (%%rax), %%rcx");
                        cg_emit(cg, "    cmpq $0, %%rcx");
                        cg_emit(cg, "    jne L%d", next_lbl);
                    } else if (pat->kind == EXPR_ERR) {
                        // Match err(e): check is_ok == 0
                        cg_emit(cg, "    movq (%%rsp), %%rax");
                        cg_emit(cg, "    movq (%%rax), %%rcx");
                        cg_emit(cg, "    cmpq $0, %%rcx");
                        cg_emit(cg, "    jne L%d", next_lbl);
                        // Bind error variable if present
                        if (s->match_stmt.bindings[i][0] != '\0') {
                            cg_emit(cg, "    movq (%%rsp), %%rax");
                            cg_emit(cg, "    movq 8(%%rax), %%rax");  // Load error value
                            Type* bind_type = s->match_stmt.binding_types[i] ? s->match_stmt.binding_types[i] : new_type(TYPE_STR);
                            cg_add_local(cg, s->match_stmt.bindings[i], bind_type);
                            cg_emit(cg, "    movq %%rax, -%d(%%rbp)", cg->stack_offset);
                        }
                    } else if (!is_wildcard) {
                        cg_emit(cg, "    movq (%%rsp), %%rax");  // Load match value
                        cg_emit(cg, "    pushq %%rax");
                        cg_expr(cg, pat);
                        cg_emit(cg, "    popq %%rcx");
                        cg_emit(cg, "    cmpq %%rax, %%rcx");
                        cg_emit(cg, "    jne L%d", next_lbl);
                    }
                    // Execute arm body
                    for (int j = 0; j < s->match_stmt.arm_counts[i]; j++) {
                        cg_stmt(cg, s->match_stmt.arms[i][j]);
                    }
                    cg_emit(cg, "    jmp L%d", end_lbl);
                    cg_emit(cg, "L%d:", next_lbl);
                }
                cg_emit(cg, "L%d:", end_lbl);
                cg_emit(cg, "    addq $8, %%rsp");  // Pop match value
                break;
            }
            default: cg_emit(cg, "    # unsupported stmt");
        }
        return;
    }
    
    // ARM64
    switch (s->kind) {
        case STMT_LET: {
            int struct_size = cg_struct_size(cg, s->let.type);
            if (struct_size > 0) {
                // Struct type: allocate space and copy data
                for (int i = 0; i < struct_size / 8; i++) {
                    cg_add_local(cg, i == 0 ? s->let.name : "__pad", i == 0 ? s->let.type : NULL);
                }
                int base_off = cg->locals[cg->local_count - struct_size/8].offset;
                if (s->let.value) {
                    cg_expr(cg, s->let.value);  // x0 = pointer to struct data
                    // Copy struct fields
                    for (int i = 0; i < struct_size / 8; i++) {
                        cg_emit(cg, "    ldr x1, [x0, #%d]", i * 8);
                        cg_emit(cg, "    str x1, [x29, #%d]", 16 + base_off + i * 8);
                    }
                }
            } else {
                // Non-struct: just store the value
                // Infer type from value
                Type* var_type = s->let.type;
                if (!var_type && s->let.value) {
                    if (s->let.value->kind == EXPR_STRING) {
                        var_type = new_type(TYPE_STR);
                    } else if (s->let.value->kind == EXPR_CALL &&
                        s->let.value->call.func->kind == EXPR_IDENT) {
                        const char* func_name = s->let.value->call.func->ident;
                        if (strcmp(func_name, "args") == 0) {
                            var_type = new_type(TYPE_ARRAY);
                            var_type->inner = new_type(TYPE_STR);
                        } else if (strcmp(func_name, "substring") == 0) {
                            var_type = new_type(TYPE_STR);
                        }
                    }
                }
                cg_add_local(cg, s->let.name, var_type);
                if (s->let.value) {
                    cg_expr(cg, s->let.value);
                } else {
                    cg_emit(cg, "    mov x0, #0");
                }
                // Use cg_local_offset to get correct offset (handles slot reuse)
                cg_emit_str_offset(cg, "x0", 16 + cg_local_offset(cg, s->let.name));
            }
            break;
        }
            
        case STMT_ASSIGN: {
            if (s->assign.op == TOK_EQ) {
                cg_expr(cg, s->assign.value);
            } else {
                // Compound assignment
                if (s->assign.target->kind == EXPR_IDENT) {
                    int off = cg_local_offset(cg, s->assign.target->ident);
                    bool is_shared = cg_is_shared(cg, s->assign.target->ident);
                    
                    if (is_shared) {
                        // Use mutex-based atomic for reliability
                        cg_expr(cg, s->assign.value);
                        cg_emit(cg, "    mov x1, x0");  // value
                        if (s->assign.op == TOK_MINUSEQ) {
                            cg_emit(cg, "    neg x1, x1");
                        }
                        if (cg->in_thread_context) {
                            cg_emit_ldr_offset(cg, "x0", 16 + off);  // ptr
                        } else {
                            cg_emit_add_offset(cg, "x0", 16 + off);  // ptr
                        }
                        cg_emit(cg, "    bl ___wyn_atomic_add");
                        break;
                    } else {
                        // Non-shared: normal operation
                        if (off) {
                            cg_emit_ldr_offset(cg, "x0", 16 + off);
                        }
                    }
                }
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, s->assign.value);
                cg_emit(cg, "    mov x1, x0");
                cg_emit(cg, "    ldr x0, [sp], #16");
                switch (s->assign.op) {
                    case TOK_PLUSEQ: cg_emit(cg, "    add x0, x0, x1"); break;
                    case TOK_MINUSEQ: cg_emit(cg, "    sub x0, x0, x1"); break;
                    case TOK_STAREQ: cg_emit(cg, "    mul x0, x0, x1"); break;
                    case TOK_SLASHEQ: cg_emit(cg, "    sdiv x0, x0, x1"); break;
                    case TOK_PERCENTEQ: cg_emit(cg, "    sdiv x2, x0, x1"); cg_emit(cg, "    msub x0, x2, x1, x0"); break;
                    default: break;
                }
            }
            if (s->assign.target->kind == EXPR_IDENT) {
                int off = cg_local_offset(cg, s->assign.target->ident);
                if (off) {
                    if (cg_is_shared(cg, s->assign.target->ident)) {
                        cg_atomic_store(cg, s->assign.target->ident, off);
                    } else {
                        cg_emit_str_offset(cg, "x0", 16 + off);
                    }
                }
            } else if (s->assign.target->kind == EXPR_DEREF) {
                // *ptr = value: store through pointer
                cg_emit(cg, "    str x0, [sp, #-16]!");  // save value
                cg_expr(cg, s->assign.target->unary.operand);  // get pointer
                cg_emit(cg, "    ldr x1, [sp], #16");  // restore value
                cg_emit(cg, "    str x1, [x0]");  // store through pointer
            } else if (s->assign.target->kind == EXPR_FIELD) {
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, s->assign.target->field.object);
                cg_emit(cg, "    ldr x1, [sp], #16");
                for (int i = 0; i < cg->module->struct_count; i++) {
                    int off = cg_struct_field_offset(&cg->module->structs[i], s->assign.target->field.field);
                    if (off >= 0) {
                        cg_emit(cg, "    str x1, [x0, #%d]", off);
                        break;
                    }
                }
            }
            break;
        }
        
        case STMT_EXPR: {
            // Special handling for arr.append(x) - update the variable
            Expr* e = s->expr.expr;
            if (e->kind == EXPR_CALL && e->call.func->kind == EXPR_FIELD &&
                strcmp(e->call.func->field.field, "append") == 0 &&
                e->call.func->field.object->kind == EXPR_IDENT) {
                // This is arr.append(x) where arr is a variable
                const char* var_name = e->call.func->field.object->ident;
                cg_expr(cg, e);  // Call append, result in x0
                // Update the variable with the new pointer
                int off = cg_local_offset(cg, var_name);
                if (off >= 0) {
                    cg_emit_str_offset(cg, "x0", 16 + off);
                }
            } else {
                cg_expr(cg, e);
            }
            break;
        }
            
        case STMT_RETURN:
            if (s->ret.value) cg_expr(cg, s->ret.value);
            if (cg->defer_count > 0) {
                cg_emit(cg, "    str x0, [sp, #-16]!");  // Save return value
                cg_emit_defers(cg);
                cg_emit(cg, "    ldr x0, [sp], #16");    // Restore return value
            }
            cg_emit(cg, "    ldp x29, x30, [sp]");
            cg_emit(cg, "    add sp, sp, #4096");
            cg_emit(cg, "    ret");
            break;
        
        case STMT_DEFER:
            if (cg->defer_count < 64) {
                cg->defers[cg->defer_count++] = s->defer.expr;
            }
            break;
            
        case STMT_IF: {
            int else_lbl = cg_new_label(cg);
            int end_lbl = cg_new_label(cg);
            
            cg_expr(cg, s->if_stmt.cond);
            cg_emit(cg, "    cbz x0, L%d", else_lbl);
            
            for (int i = 0; i < s->if_stmt.then_count; i++) {
                cg_stmt(cg, s->if_stmt.then_block[i]);
            }
            cg_emit(cg, "    b L%d", end_lbl);
            
            cg_emit(cg, "L%d:", else_lbl);
            if (s->if_stmt.else_block) {
                for (int i = 0; i < s->if_stmt.else_count; i++) {
                    cg_stmt(cg, s->if_stmt.else_block[i]);
                }
            }
            cg_emit(cg, "L%d:", end_lbl);
            break;
        }
        
        case STMT_WHILE: {
            int start_lbl = cg_new_label(cg);
            int end_lbl = cg_new_label(cg);
            int saved_start = cg->loop_start_label, saved_end = cg->loop_end_label;
            cg->loop_start_label = start_lbl;
            cg->loop_end_label = end_lbl;
            
            cg_emit(cg, "L%d:", start_lbl);
            cg_expr(cg, s->while_stmt.cond);
            cg_emit(cg, "    cbz x0, L%d", end_lbl);
            
            for (int i = 0; i < s->while_stmt.body_count; i++) {
                cg_stmt(cg, s->while_stmt.body[i]);
            }
            cg_emit(cg, "    b L%d", start_lbl);
            cg_emit(cg, "L%d:", end_lbl);
            cg->loop_start_label = saved_start;
            cg->loop_end_label = saved_end;
            break;
        }
        
        case STMT_FOR: {
            int start_lbl = cg_new_label(cg);
            int cont_lbl = cg_new_label(cg);
            int end_lbl = cg_new_label(cg);
            int saved_start = cg->loop_start_label, saved_end = cg->loop_end_label;
            int saved_local_count = cg->local_count;
            cg->loop_start_label = cont_lbl;
            cg->loop_end_label = end_lbl;
            
            // Check if iterating over array or range
            bool is_array = s->for_stmt.iter && s->for_stmt.iter->kind != EXPR_BINARY;
            
            if (is_array) {
                // Array iteration: for item in arr
                cg_add_local(cg, "__arr", NULL);
                int arr_off = 16 + cg->locals[cg->local_count - 1].offset;
                cg_expr(cg, s->for_stmt.iter);
                cg_emit_str_offset(cg, "x0", arr_off);
                
                cg_add_local(cg, "__idx", NULL);
                int idx_off = 16 + cg->locals[cg->local_count - 1].offset;
                cg_emit(cg, "    mov x0, #0");
                cg_emit_str_offset(cg, "x0", idx_off);
                
                cg_add_local(cg, s->for_stmt.var, NULL);
                int var_off = 16 + cg->locals[cg->local_count - 1].offset;
                
                cg_emit(cg, "L%d:", start_lbl);
                cg_emit_ldr_offset(cg, "x0", arr_off);
                cg_emit(cg, "    ldr x1, [x0]");  // length
                cg_emit(cg, "    ldr x2, [x29, #%d]", idx_off);
                cg_emit(cg, "    cmp x2, x1");
                cg_emit(cg, "    b.ge L%d", end_lbl);
                
                // Load element: arr[idx]
                cg_emit_ldr_offset(cg, "x0", arr_off);
                cg_emit(cg, "    ldr x1, [x29, #%d]", idx_off);
                cg_emit(cg, "    add x1, x1, #1");
                cg_emit(cg, "    ldr x0, [x0, x1, lsl #3]");
                cg_emit_str_offset(cg, "x0", var_off);
                
                for (int i = 0; i < s->for_stmt.body_count; i++) cg_stmt(cg, s->for_stmt.body[i]);
                cg_emit(cg, "L%d:", cont_lbl);
                cg_emit_ldr_offset(cg, "x0", idx_off);
                cg_emit(cg, "    add x0, x0, #1");
                cg_emit_str_offset(cg, "x0", idx_off);
                cg_emit(cg, "    b L%d", start_lbl);
            } else {
                // Range iteration: for i in start..end or start..=end
                cg_add_local(cg, s->for_stmt.var, NULL);
                int var_off = 16 + cg->locals[cg->local_count - 1].offset;
                
                cg_add_local(cg, "__end", NULL);
                int end_off = 16 + cg->locals[cg->local_count - 1].offset;
                bool inclusive = s->for_stmt.iter && s->for_stmt.iter->kind == EXPR_BINARY &&
                                 s->for_stmt.iter->binary.op == TOK_DOTDOTEQ;
                if (s->for_stmt.iter && s->for_stmt.iter->kind == EXPR_BINARY) {
                    // Initialize start value
                    cg_expr(cg, s->for_stmt.iter->binary.left);
                    cg_emit_str_offset(cg, "x0", var_off);
                    // Initialize end value
                    cg_expr(cg, s->for_stmt.iter->binary.right);
                    cg_emit_str_offset(cg, "x0", end_off);
                } else {
                    cg_emit(cg, "    mov x0, #0");
                    cg_emit_str_offset(cg, "x0", var_off);
                    cg_emit_str_offset(cg, "x0", end_off);
                }
                
                cg_emit(cg, "L%d:", start_lbl);
                cg_emit_ldr_offset(cg, "x0", var_off);
                cg_emit(cg, "    ldr x1, [x29, #%d]", end_off);
                cg_emit(cg, "    cmp x0, x1");
                cg_emit(cg, "    b.%s L%d", inclusive ? "gt" : "ge", end_lbl);
                
                for (int i = 0; i < s->for_stmt.body_count; i++) {
                    cg_stmt(cg, s->for_stmt.body[i]);
                }
                
                cg_emit(cg, "L%d:", cont_lbl);
                cg_emit_ldr_offset(cg, "x0", var_off);
                cg_emit(cg, "    add x0, x0, #1");
                cg_emit_str_offset(cg, "x0", var_off);
                cg_emit(cg, "    b L%d", start_lbl);
            }
            
            cg_emit(cg, "L%d:", end_lbl);
            cg->loop_start_label = saved_start;
            cg->loop_end_label = saved_end;
            cg->local_count = saved_local_count;
            break;
        }
        
        case STMT_BREAK:
            cg_emit(cg, "    b L%d", cg->loop_end_label);
            break;
        
        case STMT_CONTINUE:
            cg_emit(cg, "    b L%d", cg->loop_start_label);
            break;
        
        case STMT_SPAWN:
            // Mark shared variables before generating thread function
            mark_shared_vars(cg, s->spawn.body, s->spawn.body_count);
            
            // Generate thread function and call __wyn_spawn
            {
                int thread_fn_label = cg_new_label(cg);
                int skip_label = cg_new_label(cg);
                int cap_count = s->spawn.captured_count;
                
                // Skip thread function definition
                cg_emit(cg, "    b L%d", skip_label);
                
                // Thread function - receives context in x0
                cg_emit(cg, "L%d:", thread_fn_label);
                cg_emit(cg, "    sub sp, sp, #4096");
                cg_emit(cg, "    stp x29, x30, [sp]");
                cg_emit(cg, "    mov x29, sp");
                
                // Save state and mark thread context
                int saved_offset = cg->stack_offset;
                int saved_count = cg->local_count;
                bool saved_in_thread = cg->in_thread_context;
                // Save locals array (same type as cg->locals)
                struct { char name[MAX_IDENT_LEN]; int offset; Type* type; bool is_shared; } saved_locals[256];
                memcpy(saved_locals, cg->locals, sizeof(saved_locals));
                cg->in_thread_context = true;
                cg->stack_offset = 8;  // Reset for thread function (start after saved fp/lr)
                cg->local_count = 0;   // Reset locals for thread function
                
                // Load captured var addresses from context into locals
                for (int i = 0; i < cap_count; i++) {
                    cg_add_local(cg, s->spawn.captured_vars[i], NULL);
                    cg_emit(cg, "    ldr x1, [x0, #%d]", i * 8);
                    cg_emit_str_offset(cg, "x1", 16 + cg->locals[cg->local_count - 1].offset);
                    cg->locals[cg->local_count - 1].is_shared = true;
                }
                
                // Execute spawn body
                for (int i = 0; i < s->spawn.body_count; i++) {
                    cg_stmt(cg, s->spawn.body[i]);
                }
                
                cg_emit(cg, "    mov x0, #0");
                cg_emit(cg, "    ldp x29, x30, [sp]");
                cg_emit(cg, "    add sp, sp, #4096");
                cg_emit(cg, "    ret");
                
                // Restore state
                cg->stack_offset = saved_offset;
                cg->local_count = saved_count;
                memcpy(cg->locals, saved_locals, sizeof(saved_locals));
                cg->in_thread_context = saved_in_thread;
                
                // Back to main code - allocate context and copy vars
                cg_emit(cg, "L%d:", skip_label);
                if (cap_count > 0) {
                    // malloc(cap_count * 8)
                    cg_emit(cg, "    mov x0, #%d", cap_count * 8);
                    cg_emit(cg, "    bl _malloc");
                    cg_emit(cg, "    mov x19, x0");  // Save context ptr
                    
                    // Copy captured var addresses to context
                    for (int i = 0; i < cap_count; i++) {
                        int off = cg_local_offset(cg, s->spawn.captured_vars[i]);
                        if (off) {
                            cg_emit_add_offset(cg, "x1", 16 + off);  // Get address
                            cg_emit(cg, "    str x1, [x19, #%d]", i * 8);
                        }
                    }
                    
                    // Call __wyn_spawn with function and context
                    cg_emit(cg, "    adrp x0, L%d@PAGE", thread_fn_label);
                    cg_emit(cg, "    add x0, x0, L%d@PAGEOFF", thread_fn_label);
                    cg_emit(cg, "    mov x1, x19");  // Context ptr
                    cg_emit(cg, "    bl ___wyn_spawn");
                } else {
                    cg_emit(cg, "    adrp x0, L%d@PAGE", thread_fn_label);
                    cg_emit(cg, "    add x0, x0, L%d@PAGEOFF", thread_fn_label);
                    cg_emit(cg, "    mov x1, #0");
                    cg_emit(cg, "    bl ___wyn_spawn");
                }
                cg->spawn_count++;
            }
            break;
        
        case STMT_MATCH: {
            int end_lbl = cg_new_label(cg);
            // Evaluate match value once and save
            cg_expr(cg, s->match_stmt.value);
            cg_emit(cg, "    str x0, [sp, #-16]!");
            for (int i = 0; i < s->match_stmt.arm_count; i++) {
                int next_lbl = cg_new_label(cg);
                Expr* pat = s->match_stmt.patterns[i];
                // Check if pattern is wildcard (_)
                bool is_wildcard = pat->kind == EXPR_IDENT && strcmp(pat->ident, "_") == 0;
                
                if (pat->kind == EXPR_SOME || pat->kind == EXPR_OK) {
                    // Match some(x) or ok(v): check has_value == 1
                    cg_emit(cg, "    ldr x0, [sp]");  // Load optional/result ptr
                    cg_emit(cg, "    ldr x1, [x0]");  // Load has_value/is_ok flag
                    cg_emit(cg, "    cmp x1, #1");
                    cg_emit(cg, "    b.ne L%d", next_lbl);
                    // Bind variable if present
                    if (s->match_stmt.bindings[i][0] != '\0') {
                        cg_emit(cg, "    ldr x0, [sp]");
                        cg_emit(cg, "    ldr x0, [x0, #8]");  // Load inner value
                        Type* bind_type = s->match_stmt.binding_types[i] ? s->match_stmt.binding_types[i] : new_type(TYPE_INT);
                        cg_add_local(cg, s->match_stmt.bindings[i], bind_type);
                        cg_emit_str_offset(cg, "x0", 16 + cg->stack_offset);
                    }
                } else if (pat->kind == EXPR_NONE) {
                    // Match none: check has_value == 0
                    cg_emit(cg, "    ldr x0, [sp]");
                    cg_emit(cg, "    ldr x1, [x0]");
                    cg_emit(cg, "    cmp x1, #0");
                    cg_emit(cg, "    b.ne L%d", next_lbl);
                } else if (pat->kind == EXPR_ERR) {
                    // Match err(e): check is_ok == 0
                    cg_emit(cg, "    ldr x0, [sp]");
                    cg_emit(cg, "    ldr x1, [x0]");
                    cg_emit(cg, "    cmp x1, #0");
                    cg_emit(cg, "    b.ne L%d", next_lbl);
                    // Bind error variable if present
                    if (s->match_stmt.bindings[i][0] != '\0') {
                        cg_emit(cg, "    ldr x0, [sp]");
                        cg_emit(cg, "    ldr x0, [x0, #8]");  // Load error value
                        Type* bind_type = s->match_stmt.binding_types[i] ? s->match_stmt.binding_types[i] : new_type(TYPE_STR);
                        cg_add_local(cg, s->match_stmt.bindings[i], bind_type);
                        cg_emit_str_offset(cg, "x0", 16 + cg->stack_offset);
                    }
                } else if (!is_wildcard) {
                    cg_emit(cg, "    ldr x0, [sp]");  // Load match value
                    cg_emit(cg, "    str x0, [sp, #-16]!");
                    cg_expr(cg, pat);
                    cg_emit(cg, "    ldr x1, [sp], #16");
                    cg_emit(cg, "    cmp x1, x0");
                    cg_emit(cg, "    b.ne L%d", next_lbl);
                }
                // Execute arm body
                for (int j = 0; j < s->match_stmt.arm_counts[i]; j++) {
                    cg_stmt(cg, s->match_stmt.arms[i][j]);
                }
                cg_emit(cg, "    b L%d", end_lbl);
                cg_emit(cg, "L%d:", next_lbl);
            }
            cg_emit(cg, "L%d:", end_lbl);
            cg_emit(cg, "    add sp, sp, #16");  // Pop match value
            break;
        }
        
        default:
            cg_emit(cg, "    ; unsupported stmt kind=%d", s->kind);
    }
}

static void cg_method(CodeGen* cg, const char* struct_name, FnDef* fn) {
    cg->stack_offset = 0;
    cg->local_count = 0;
    
    char mangled[256];
    snprintf(mangled, 256, "%s_%s", struct_name, fn->name);
    const char* pfx = cg_sym_prefix(cg);
    
    if (cg->arch == ARCH_X86_64) {
        cg_emit(cg, "    .globl %s%s", pfx, mangled);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s%s:", pfx, mangled);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    subq $256, %%rsp");
        
        // Save argc/argv for main function
        if (strcmp(fn->name, "main") == 0) {
            cg_emit(cg, "    leaq L_.wyn_argc(%%rip), %%r10");
            cg_emit(cg, "    movq %%rdi, (%%r10)");
            cg_emit(cg, "    leaq L_.wyn_argv(%%rip), %%r10");
            cg_emit(cg, "    movq %%rsi, (%%r10)");
        }
        
        const char* regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        for (int i = 0; i < fn->param_count && i < 6; i++) {
            cg_add_local(cg, fn->params[i].name, fn->params[i].type);
            cg_emit(cg, "    movq %%%s, -%d(%%rbp)", regs[i], cg->locals[cg->local_count - 1].offset);
        }
        
        for (int i = 0; i < fn->body_count; i++) cg_stmt(cg, fn->body[i]);
        
        cg_emit(cg, "    xorl %%eax, %%eax");
        cg_emit(cg, "    movq %%rbp, %%rsp");
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        return;
    }
    
    // ARM64 - use 2048 byte stack frame
    cg_emit(cg, "    .globl %s%s", pfx, mangled);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s%s:", pfx, mangled);
    cg_emit(cg, "    sub sp, sp, #4096");
    cg_emit(cg, "    stp x29, x30, [sp]");
    cg_emit(cg, "    mov x29, sp");
    
    for (int i = 0; i < fn->param_count && i < 8; i++) {
        // For struct parameters, store the pointer (not inline struct)
        Type* param_type = fn->params[i].type;
        int struct_size = cg_struct_size(cg, param_type);
        if (struct_size > 0) {
            cg_add_local(cg, fn->params[i].name, NULL);  // NULL type = pointer to struct
        } else {
            cg_add_local(cg, fn->params[i].name, param_type);
        }
        { int off = 16 + cg->locals[cg->local_count - 1].offset; if (off <= 4095) cg_emit(cg, "    str x%d, [x29, #%d]", i, off); else { cg_emit(cg, "    mov x9, #%d", off); cg_emit(cg, "    str x%d, [x29, x9]", i); } }
    }
    
    for (int i = 0; i < fn->body_count; i++) cg_stmt(cg, fn->body[i]);
    
    cg_emit(cg, "    mov x0, #0");
    cg_emit(cg, "    ldp x29, x30, [sp]");
    cg_emit(cg, "    add sp, sp, #4096");
    cg_emit(cg, "    ret");
}

static void cg_function(CodeGen* cg, FnDef* fn) {
    cg->stack_offset = 0;
    cg->local_count = 0;
    cg->defer_count = 0;  // Reset defers for each function
    const char* pfx = cg_sym_prefix(cg);
    
    if (cg->arch == ARCH_X86_64) {
        cg_emit(cg, "    .globl %s%s", pfx, fn->name);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s%s:", pfx, fn->name);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    subq $256, %%rsp");
        
        const char* regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        for (int i = 0; i < fn->param_count && i < 6; i++) {
            cg_add_local(cg, fn->params[i].name, fn->params[i].type);
            cg_emit(cg, "    movq %%%s, -%d(%%rbp)", regs[i], cg->locals[cg->local_count - 1].offset);
        }
        
        for (int i = 0; i < fn->body_count; i++) {
            // If this is the last statement and we have spawns, join first
            if (i == fn->body_count - 1 && cg->spawn_count > 0) {
                cg_emit(cg, "    call ___wyn_join_all");
                cg_emit(cg, "    mfence");
                cg->spawn_count = 0;
            }
            cg_stmt(cg, fn->body[i]);
        }
        
        // Join any remaining spawns
        if (cg->spawn_count > 0) {
            cg_emit(cg, "    call ___wyn_join_all");
            cg_emit(cg, "    mfence");
        }
        
        cg_emit_defers(cg);  // Execute defers at implicit return
        cg_emit(cg, "    xorl %%eax, %%eax");
        cg_emit(cg, "    movq %%rbp, %%rsp");
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        return;
    }
    
    // ARM64 - use 2048 byte stack frame for larger functions
    cg_emit(cg, "    .globl %s%s", pfx, fn->name);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s%s:", pfx, fn->name);
    cg_emit(cg, "    sub sp, sp, #4096");
    cg_emit(cg, "    stp x29, x30, [sp]");
    cg_emit(cg, "    mov x29, sp");
    
    // Save argc/argv for main function
    if (strcmp(fn->name, "main") == 0) {
        cg_emit(cg, "    adrp x8, L_.wyn_argc@PAGE");
        cg_emit(cg, "    add x8, x8, L_.wyn_argc@PAGEOFF");
        cg_emit(cg, "    str x0, [x8]");
        cg_emit(cg, "    adrp x8, L_.wyn_argv@PAGE");
        cg_emit(cg, "    add x8, x8, L_.wyn_argv@PAGEOFF");
        cg_emit(cg, "    str x1, [x8]");
    }
    
    for (int i = 0; i < fn->param_count && i < 8; i++) {
        // For struct parameters, store the pointer (not inline struct)
        Type* param_type = fn->params[i].type;
        int struct_size = cg_struct_size(cg, param_type);
        if (struct_size > 0) {
            cg_add_local(cg, fn->params[i].name, NULL);  // NULL type = pointer to struct
        } else {
            cg_add_local(cg, fn->params[i].name, param_type);
        }
        { int off = 16 + cg->locals[cg->local_count - 1].offset; if (off <= 4095) cg_emit(cg, "    str x%d, [x29, #%d]", i, off); else { cg_emit(cg, "    mov x9, #%d", off); cg_emit(cg, "    str x%d, [x29, x9]", i); } }
    }
    
    for (int i = 0; i < fn->body_count; i++) {
        // If this is the last statement and we have spawns, join first
        if (i == fn->body_count - 1 && cg->spawn_count > 0) {
            cg_emit(cg, "    bl ___wyn_join_all");
            cg_emit(cg, "    dmb sy");
            cg->spawn_count = 0;  // Mark as joined
        }
        cg_stmt(cg, fn->body[i]);
    }
    
    // Join any remaining spawns (if not already joined)
    if (cg->spawn_count > 0) {
        cg_emit(cg, "    bl ___wyn_join_all");
        cg_emit(cg, "    dmb sy");
    }
    
    cg_emit_defers(cg);  // Execute defers at implicit return
    cg_emit(cg, "    mov x0, #0");
    cg_emit(cg, "    ldp x29, x30, [sp]");
    cg_emit(cg, "    add sp, sp, #4096");
    cg_emit(cg, "    ret");
}

static void cg_emit_lambda_x86(CodeGen* cg, Expr* e) {
    const char* pfx = cg_sym_prefix(cg);
    cg_emit(cg, "    .p2align 4");
    cg_emit(cg, "%s_lambda%d:", pfx, e->lambda.id);
    cg_emit(cg, "    pushq %%rbp");
    cg_emit(cg, "    movq %%rsp, %%rbp");
    cg_emit(cg, "    subq $128, %%rsp");
    
    // Save parameters to locals
    const char* regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    int saved_local_count = cg->local_count;
    int saved_stack_offset = cg->stack_offset;
    cg->local_count = 0;
    cg->stack_offset = 0;
    
    for (int i = 0; i < e->lambda.param_count && i < 6; i++) {
        cg->stack_offset += 8;
        strcpy(cg->locals[cg->local_count].name, e->lambda.params[i]);
        cg->locals[cg->local_count].offset = cg->stack_offset;
        cg->locals[cg->local_count].type = e->lambda.param_types[i];
        cg->local_count++;
        cg_emit(cg, "    movq %%%s, -%d(%%rbp)", regs[i], cg->stack_offset);
    }
    
    for (int i = 0; i < e->lambda.body_count; i++) {
        cg_stmt(cg, e->lambda.body[i]);
    }
    
    cg_emit(cg, "    xorl %%eax, %%eax");
    cg_emit(cg, "    leave");
    cg_emit(cg, "    retq");
    
    cg->local_count = saved_local_count;
    cg->stack_offset = saved_stack_offset;
}

static void cg_emit_lambda_arm(CodeGen* cg, Expr* e) {
    const char* pfx = cg_sym_prefix(cg);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s_lambda%d:", pfx, e->lambda.id);
    cg_emit(cg, "    sub sp, sp, #4096");
    cg_emit(cg, "    stp x29, x30, [sp]");
    cg_emit(cg, "    mov x29, sp");
    
    // Save parameters to locals
    int saved_local_count = cg->local_count;
    int saved_stack_offset = cg->stack_offset;
    cg->local_count = 0;
    cg->stack_offset = 0;
    
    for (int i = 0; i < e->lambda.param_count && i < 8; i++) {
        cg->stack_offset += 8;
        strcpy(cg->locals[cg->local_count].name, e->lambda.params[i]);
        cg->locals[cg->local_count].offset = cg->stack_offset;
        cg->locals[cg->local_count].type = e->lambda.param_types[i];
        cg->local_count++;
        { int off = 16 + cg->stack_offset; if (off <= 4095) cg_emit(cg, "    str x%d, [x29, #%d]", i, off); else { cg_emit(cg, "    mov x9, #%d", off); cg_emit(cg, "    str x%d, [x29, x9]", i); } }
    }
    
    for (int i = 0; i < e->lambda.body_count; i++) {
        cg_stmt(cg, e->lambda.body[i]);
    }
    
    cg_emit(cg, "    mov x0, #0");
    cg_emit(cg, "    ldp x29, x30, [sp]");
    cg_emit(cg, "    add sp, sp, #4096");
    cg_emit(cg, "    ret");
    
    cg->local_count = saved_local_count;
    cg->stack_offset = saved_stack_offset;
}

static void cg_emit_lambdas(CodeGen* cg) {
    for (int i = 0; i < cg->lambda_count; i++) {
        if (cg->arch == ARCH_X86_64) {
            cg_emit_lambda_x86(cg, cg->lambdas[i]);
        } else {
            cg_emit_lambda_arm(cg, cg->lambdas[i]);
        }
        cg_emit(cg, "");
    }
}

static void codegen_module(CodeGen* cg) {
    if (cg->arch == ARCH_X86_64) {
        if (cg->os == OS_MACOS) {
            cg_emit(cg, "    .section __TEXT,__text,regular,pure_instructions");
            cg_emit(cg, "    .build_version macos, 14, 0");
        } else {
            cg_emit(cg, "    .text");
        }
        
        for (int i = 0; i < cg->module->function_count; i++) {
            cg_function(cg, &cg->module->functions[i]);
            cg_emit(cg, "");
        }
        
        // Emit struct methods
        for (int i = 0; i < cg->module->struct_count; i++) {
            StructDef* s = &cg->module->structs[i];
            for (int j = 0; j < s->method_count; j++) {
                cg_method(cg, s->name, &s->methods[j]);
                cg_emit(cg, "");
            }
        }
        
        // Emit lambdas
        cg_emit_lambdas(cg);
        
        const char* pfx = cg_sym_prefix(cg);
        
        // print runtime for x86_64
        cg_emit(cg, "    .globl %sprint", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%sprint:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    movq %%rdi, %%rsi");
        cg_emit(cg, "    leaq L_.fmt(%%rip), %%rdi");
        cg_emit(cg, "    xorl %%eax, %%eax");
        cg_emit(cg, "    callq %sprintf", pfx);
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        cg_emit(cg, "    .globl %sprint_str", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%sprint_str:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    movq %%rdi, %%rsi");
        cg_emit(cg, "    leaq L_.sfmt(%%rip), %%rdi");
        cg_emit(cg, "    xorl %%eax, %%eax");
        cg_emit(cg, "    callq %sprintf", pfx);
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        cg_emit(cg, "    .globl %sprint_float", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%sprint_float:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    movq %%rdi, %%xmm0");  // rdi contains float bits
        cg_emit(cg, "    leaq L_.ffmt(%%rip), %%rdi");
        cg_emit(cg, "    movl $1, %%eax");  // 1 float arg in xmm
        cg_emit(cg, "    callq %sprintf", pfx);
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        // print_char: prints a single character
        cg_emit(cg, "    .globl %sprint_char", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%sprint_char:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    callq %sputchar", pfx);
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        // print_newline: prints a newline character
        cg_emit(cg, "    .globl %sprint_newline", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%sprint_newline:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    movl $10, %%edi");
        cg_emit(cg, "    callq %sputchar", pfx);
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        // println: prints int with newline
        cg_emit(cg, "    .globl %sprintln", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%sprintln:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    movq %%rdi, %%rsi");
        cg_emit(cg, "    leaq L_.lnfmt(%%rip), %%rdi");
        cg_emit(cg, "    xorl %%eax, %%eax");
        cg_emit(cg, "    callq %sprintf", pfx);
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        // println_str: prints string with newline
        cg_emit(cg, "    .globl %sprintln_str", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%sprintln_str:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    callq %sputs", pfx);
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        // flush: flushes stdout
        cg_emit(cg, "    .globl %sflush", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%sflush:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    movq %s%s(%%rip), %%rdi", pfx, cg->os == OS_MACOS ? "__stdoutp" : "stdout");
        cg_emit(cg, "    callq %sfflush", pfx);
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        // __wyn_read_file: reads file to malloc'd string, returns ptr (or 0 on error)
        cg_emit(cg, "    .globl %s_wyn_read_file", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s_wyn_read_file:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    pushq %%rbx");
        cg_emit(cg, "    pushq %%r12");
        cg_emit(cg, "    leaq L_.rb(%%rip), %%rsi");  // "rb" mode
        cg_emit(cg, "    callq %sfopen", pfx);
        cg_emit(cg, "    testq %%rax, %%rax");
        cg_emit(cg, "    jz 1f");
        cg_emit(cg, "    movq %%rax, %%rbx");         // rbx = file handle
        cg_emit(cg, "    movq %%rax, %%rdi");
        cg_emit(cg, "    xorl %%esi, %%esi");
        cg_emit(cg, "    movl $2, %%edx");            // SEEK_END
        cg_emit(cg, "    callq %sfseek", pfx);
        cg_emit(cg, "    movq %%rbx, %%rdi");
        cg_emit(cg, "    callq %sftell", pfx);
        cg_emit(cg, "    movq %%rax, %%r12");         // r12 = file size
        cg_emit(cg, "    movq %%rbx, %%rdi");
        cg_emit(cg, "    xorl %%esi, %%esi");
        cg_emit(cg, "    xorl %%edx, %%edx");         // SEEK_SET
        cg_emit(cg, "    callq %sfseek", pfx);
        cg_emit(cg, "    leaq 1(%%r12), %%rdi");      // size + 1 for null
        cg_emit(cg, "    callq %smalloc", pfx);
        cg_emit(cg, "    testq %%rax, %%rax");
        cg_emit(cg, "    jz 2f");
        cg_emit(cg, "    pushq %%rax");               // save buffer ptr
        cg_emit(cg, "    movq %%rax, %%rdi");
        cg_emit(cg, "    movq $1, %%rsi");
        cg_emit(cg, "    movq %%r12, %%rdx");
        cg_emit(cg, "    movq %%rbx, %%rcx");
        cg_emit(cg, "    callq %sfread", pfx);
        cg_emit(cg, "    popq %%rax");                // restore buffer ptr
        cg_emit(cg, "    movb $0, (%%rax,%%r12)");    // null terminate
        cg_emit(cg, "    pushq %%rax");
        cg_emit(cg, "    movq %%rbx, %%rdi");
        cg_emit(cg, "    callq %sfclose", pfx);
        cg_emit(cg, "    popq %%rax");
        cg_emit(cg, "    jmp 3f");
        cg_emit(cg, "2:  movq %%rbx, %%rdi");         // malloc failed, close file
        cg_emit(cg, "    callq %sfclose", pfx);
        cg_emit(cg, "1:  xorl %%eax, %%eax");         // return 0 on error
        cg_emit(cg, "3:  popq %%r12");
        cg_emit(cg, "    popq %%rbx");
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        // __wyn_read_line: reads a line from stdin, returns ptr to static buffer
        cg_emit(cg, "    .globl %s_wyn_read_line", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s_wyn_read_line:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    leaq L_.linebuf(%%rip), %%rdi");
        cg_emit(cg, "    movq $1024, %%rsi");
        cg_emit(cg, "    movq %s%s(%%rip), %%rdx", pfx, cg->os == OS_MACOS ? "__stdinp" : "stdin");
        cg_emit(cg, "    callq %sfgets", pfx);
        cg_emit(cg, "    testq %%rax, %%rax");
        cg_emit(cg, "    jz 1f");
        // Strip trailing newline
        cg_emit(cg, "    leaq L_.linebuf(%%rip), %%rdi");
        cg_emit(cg, "    xorq %%rcx, %%rcx");
        cg_emit(cg, "2:  movb (%%rdi,%%rcx), %%al");
        cg_emit(cg, "    testb %%al, %%al");
        cg_emit(cg, "    jz 3f");
        cg_emit(cg, "    cmpb $10, %%al");
        cg_emit(cg, "    je 4f");
        cg_emit(cg, "    incq %%rcx");
        cg_emit(cg, "    jmp 2b");
        cg_emit(cg, "4:  movb $0, (%%rdi,%%rcx)");
        cg_emit(cg, "3:  leaq L_.linebuf(%%rip), %%rax");
        cg_emit(cg, "    jmp 5f");
        cg_emit(cg, "1:  leaq L_.linebuf(%%rip), %%rax");
        cg_emit(cg, "    movb $0, (%%rax)");
        cg_emit(cg, "5:  popq %%rbp");
        cg_emit(cg, "    retq");
        
        // __wyn_write_file: writes string to file, returns 0 on success, -1 on error
        cg_emit(cg, "    .globl %s_wyn_write_file", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s_wyn_write_file:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    pushq %%rbx");
        cg_emit(cg, "    pushq %%r12");
        cg_emit(cg, "    movq %%rsi, %%r12");         // r12 = content
        cg_emit(cg, "    leaq L_.wb(%%rip), %%rsi");  // "w" mode
        cg_emit(cg, "    callq %sfopen", pfx);
        cg_emit(cg, "    testq %%rax, %%rax");
        cg_emit(cg, "    jz 1f");
        cg_emit(cg, "    movq %%rax, %%rbx");         // rbx = file handle
        cg_emit(cg, "    movq %%r12, %%rdi");         // content
        cg_emit(cg, "    movq %%rbx, %%rsi");         // file
        cg_emit(cg, "    callq %sfputs", pfx);
        cg_emit(cg, "    movq %%rbx, %%rdi");
        cg_emit(cg, "    callq %sfclose", pfx);
        cg_emit(cg, "    xorl %%eax, %%eax");         // return 0 on success
        cg_emit(cg, "    jmp 2f");
        cg_emit(cg, "1:  movq $-1, %%rax");           // return -1 on error
        cg_emit(cg, "2:  popq %%r12");
        cg_emit(cg, "    popq %%rbx");
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        // __wyn_file_exists: check if file exists, returns 1 if exists, 0 otherwise
        cg_emit(cg, "    .globl %s_wyn_file_exists", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s_wyn_file_exists:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    xorl %%esi, %%esi");         // F_OK = 0
        cg_emit(cg, "    callq %saccess", pfx);
        cg_emit(cg, "    testl %%eax, %%eax");
        cg_emit(cg, "    setz %%al");
        cg_emit(cg, "    movzbl %%al, %%eax");
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        // __wyn_file_size: get file size, returns -1 on error
        cg_emit(cg, "    .globl %s_wyn_file_size", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s_wyn_file_size:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    subq $160, %%rsp");
        cg_emit(cg, "    leaq -160(%%rbp), %%rsi");    // stat buffer
        cg_emit(cg, "    callq %sstat", pfx);
        cg_emit(cg, "    testl %%eax, %%eax");
        cg_emit(cg, "    jnz 1f");
        cg_emit(cg, "    movq -112(%%rbp), %%rax");    // st_size at offset 48
        cg_emit(cg, "    jmp 2f");
        cg_emit(cg, "1:  movq $-1, %%rax");
        cg_emit(cg, "2:  leave");
        cg_emit(cg, "    retq");
        
        // __wyn_is_dir: check if path is directory, returns 1 if dir, 0 otherwise
        cg_emit(cg, "    .globl %s_wyn_is_dir", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s_wyn_is_dir:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    subq $160, %%rsp");
        cg_emit(cg, "    leaq -160(%%rbp), %%rsi");    // stat buffer
        cg_emit(cg, "    callq %sstat", pfx);
        cg_emit(cg, "    testl %%eax, %%eax");
        cg_emit(cg, "    jnz 1f");
        cg_emit(cg, "    movl -144(%%rbp), %%eax");    // st_mode at offset 16
        cg_emit(cg, "    andl $0xF000, %%eax");        // S_IFMT mask
        cg_emit(cg, "    cmpl $0x4000, %%eax");        // S_IFDIR
        cg_emit(cg, "    sete %%al");
        cg_emit(cg, "    movzbl %%al, %%eax");
        cg_emit(cg, "    jmp 2f");
        cg_emit(cg, "1:  xorl %%eax, %%eax");
        cg_emit(cg, "2:  leave");
        cg_emit(cg, "    retq");
        
        // __wyn_tcp_connect: connect to TCP socket, returns fd or -1
        cg_emit(cg, "    .globl %s_wyn_tcp_connect", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s_wyn_tcp_connect:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    subq $48, %%rsp");
        cg_emit(cg, "    movq %%rdi, -8(%%rbp)");      // save host
        cg_emit(cg, "    movq %%rsi, -16(%%rbp)");     // save port
        // socket(AF_INET=2, SOCK_STREAM=1, 0)
        cg_emit(cg, "    movl $2, %%edi");
        cg_emit(cg, "    movl $1, %%esi");
        cg_emit(cg, "    xorl %%edx, %%edx");
        cg_emit(cg, "    callq %ssocket", pfx);
        cg_emit(cg, "    testl %%eax, %%eax");
        cg_emit(cg, "    js 1f");
        cg_emit(cg, "    movl %%eax, -20(%%rbp)");     // save sockfd
        // gethostbyname(host)
        cg_emit(cg, "    movq -8(%%rbp), %%rdi");
        cg_emit(cg, "    callq %sgethostbyname", pfx);
        cg_emit(cg, "    testq %%rax, %%rax");
        cg_emit(cg, "    jz 2f");
        cg_emit(cg, "    movq %%rax, %%rbx");          // save hostent*
        // Build sockaddr_in: sin_family=AF_INET, sin_port=htons(port), sin_addr
        cg_emit(cg, "    movw $2, -32(%%rbp)");        // sin_family = AF_INET
        cg_emit(cg, "    movq -16(%%rbp), %%rdi");     // port
        cg_emit(cg, "    callq %shtons", pfx);
        cg_emit(cg, "    movw %%ax, -30(%%rbp)");      // sin_port
        cg_emit(cg, "    movq 16(%%rbx), %%rax");      // h_addr_list[0]
        cg_emit(cg, "    movl (%%rax), %%eax");
        cg_emit(cg, "    movl %%eax, -28(%%rbp)");     // sin_addr
        cg_emit(cg, "    xorq %%rax, %%rax");
        cg_emit(cg, "    movq %%rax, -24(%%rbp)");     // sin_zero
        // connect(sockfd, &addr, 16)
        cg_emit(cg, "    movl -20(%%rbp), %%edi");
        cg_emit(cg, "    leaq -32(%%rbp), %%rsi");
        cg_emit(cg, "    movl $16, %%edx");
        cg_emit(cg, "    callq %sconnect", pfx);
        cg_emit(cg, "    testl %%eax, %%eax");
        cg_emit(cg, "    jnz 2f");
        cg_emit(cg, "    movl -20(%%rbp), %%eax");     // return sockfd
        cg_emit(cg, "    jmp 3f");
        cg_emit(cg, "2:  movl -20(%%rbp), %%edi");     // close on error
        cg_emit(cg, "    callq %sclose", pfx);
        cg_emit(cg, "1:  movl $-1, %%eax");            // return -1
        cg_emit(cg, "3:  leave");
        cg_emit(cg, "    retq");
        
        // __wyn_tcp_listen: create listening socket, returns fd or -1
        cg_emit(cg, "    .globl %s_wyn_tcp_listen", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s_wyn_tcp_listen:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    subq $32, %%rsp");
        cg_emit(cg, "    movq %%rdi, -8(%%rbp)");      // save port
        // socket(AF_INET=2, SOCK_STREAM=1, 0)
        cg_emit(cg, "    movl $2, %%edi");
        cg_emit(cg, "    movl $1, %%esi");
        cg_emit(cg, "    xorl %%edx, %%edx");
        cg_emit(cg, "    callq %ssocket", pfx);
        cg_emit(cg, "    testl %%eax, %%eax");
        cg_emit(cg, "    js 1f");
        cg_emit(cg, "    movl %%eax, -12(%%rbp)");     // save sockfd
        // Build sockaddr_in: sin_family=AF_INET, sin_port=htons(port), sin_addr=INADDR_ANY
        cg_emit(cg, "    movw $2, -24(%%rbp)");        // sin_family = AF_INET
        cg_emit(cg, "    movq -8(%%rbp), %%rdi");      // port
        cg_emit(cg, "    callq %shtons", pfx);
        cg_emit(cg, "    movw %%ax, -22(%%rbp)");      // sin_port
        cg_emit(cg, "    movl $0, -20(%%rbp)");        // sin_addr = INADDR_ANY
        cg_emit(cg, "    xorq %%rax, %%rax");
        cg_emit(cg, "    movq %%rax, -16(%%rbp)");     // sin_zero
        // bind(sockfd, &addr, 16)
        cg_emit(cg, "    movl -12(%%rbp), %%edi");
        cg_emit(cg, "    leaq -24(%%rbp), %%rsi");
        cg_emit(cg, "    movl $16, %%edx");
        cg_emit(cg, "    callq %sbind", pfx);
        cg_emit(cg, "    testl %%eax, %%eax");
        cg_emit(cg, "    jnz 2f");
        // listen(sockfd, 5)
        cg_emit(cg, "    movl -12(%%rbp), %%edi");
        cg_emit(cg, "    movl $5, %%esi");
        cg_emit(cg, "    callq %slisten", pfx);
        cg_emit(cg, "    testl %%eax, %%eax");
        cg_emit(cg, "    jnz 2f");
        cg_emit(cg, "    movl -12(%%rbp), %%eax");     // return sockfd
        cg_emit(cg, "    jmp 3f");
        cg_emit(cg, "2:  movl -12(%%rbp), %%edi");     // close on error
        cg_emit(cg, "    callq %sclose", pfx);
        cg_emit(cg, "1:  movl $-1, %%eax");            // return -1
        cg_emit(cg, "3:  leave");
        cg_emit(cg, "    retq");
        
        // __wyn_char_at: get character at index, returns single-char string or ""
        cg_emit(cg, "    .globl %s_wyn_char_at", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s_wyn_char_at:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    pushq %%rdi");                // save string
        cg_emit(cg, "    pushq %%rsi");                // save index
        cg_emit(cg, "    callq %sstrlen", pfx);        // get string length
        cg_emit(cg, "    popq %%rsi");                 // restore index
        cg_emit(cg, "    popq %%rdi");                 // restore string
        cg_emit(cg, "    testq %%rsi, %%rsi");         // check i >= 0
        cg_emit(cg, "    js 1f");
        cg_emit(cg, "    cmpq %%rax, %%rsi");          // check i < len
        cg_emit(cg, "    jge 1f");
        cg_emit(cg, "    pushq %%rdi");                // save string
        cg_emit(cg, "    pushq %%rsi");                // save index
        cg_emit(cg, "    movq $2, %%rdi");             // malloc 2 bytes
        cg_emit(cg, "    callq %smalloc", pfx);
        cg_emit(cg, "    popq %%rsi");                 // restore index
        cg_emit(cg, "    popq %%rdi");                 // restore string
        cg_emit(cg, "    movb (%%rdi,%%rsi), %%cl");   // get s[i]
        cg_emit(cg, "    movb %%cl, (%%rax)");         // store char
        cg_emit(cg, "    movb $0, 1(%%rax)");          // null terminator
        cg_emit(cg, "    jmp 2f");
        cg_emit(cg, "1:  leaq L_.empty_str(%%rip), %%rax");  // return ""
        cg_emit(cg, "2:  popq %%rbp");
        cg_emit(cg, "    retq");
        
        // __wyn_delete_file: delete file, returns 1 on success, 0 on failure
        cg_emit(cg, "    .globl %s_wyn_delete_file", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s_wyn_delete_file:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    callq %sunlink", pfx);
        cg_emit(cg, "    testl %%eax, %%eax");
        cg_emit(cg, "    setz %%al");
        cg_emit(cg, "    movzbl %%al, %%eax");
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        // __wyn_mkdir: create directory, returns 1 on success, 0 on failure
        cg_emit(cg, "    .globl %s_wyn_mkdir", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s_wyn_mkdir:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    movl $0755, %%esi");         // mode 0755
        cg_emit(cg, "    callq %smkdir", pfx);
        cg_emit(cg, "    testl %%eax, %%eax");
        cg_emit(cg, "    setz %%al");
        cg_emit(cg, "    movzbl %%al, %%eax");
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        // __wyn_rmdir: remove directory, returns 1 on success, 0 on failure
        cg_emit(cg, "    .globl %s_wyn_rmdir", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s_wyn_rmdir:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    callq %srmdir", pfx);
        cg_emit(cg, "    testl %%eax, %%eax");
        cg_emit(cg, "    setz %%al");
        cg_emit(cg, "    movzbl %%al, %%eax");
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        // __wyn_append_file: append string to file, returns 1 on success, 0 on failure
        cg_emit(cg, "    .globl %s_wyn_append_file", pfx);
        cg_emit(cg, "    .p2align 4");
        cg_emit(cg, "%s_wyn_append_file:", pfx);
        cg_emit(cg, "    pushq %%rbp");
        cg_emit(cg, "    movq %%rsp, %%rbp");
        cg_emit(cg, "    pushq %%rbx");
        cg_emit(cg, "    pushq %%r12");
        cg_emit(cg, "    movq %%rsi, %%r12");         // r12 = content
        cg_emit(cg, "    leaq L_.ab(%%rip), %%rsi");  // "a" mode
        cg_emit(cg, "    callq %sfopen", pfx);
        cg_emit(cg, "    testq %%rax, %%rax");
        cg_emit(cg, "    jz 1f");
        cg_emit(cg, "    movq %%rax, %%rbx");         // rbx = file handle
        cg_emit(cg, "    movq %%r12, %%rdi");         // content
        cg_emit(cg, "    movq %%rbx, %%rsi");         // file
        cg_emit(cg, "    callq %sfputs", pfx);
        cg_emit(cg, "    movq %%rbx, %%rdi");
        cg_emit(cg, "    callq %sfclose", pfx);
        cg_emit(cg, "    movl $1, %%eax");            // return 1 on success
        cg_emit(cg, "    jmp 2f");
        cg_emit(cg, "1:  xorl %%eax, %%eax");         // return 0 on error
        cg_emit(cg, "2:  popq %%r12");
        cg_emit(cg, "    popq %%rbx");
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        
        // GUI functions (external, defined in runtime/gui_macos.c)
        if (cg->os == OS_MACOS) {
            cg_emit(cg, "    .globl %sgui_create_window", pfx);
            cg_emit(cg, "    .globl %sgui_show_window", pfx);
            cg_emit(cg, "    .globl %sgui_set_title", pfx);
            cg_emit(cg, "    .globl %sgui_run", pfx);
            cg_emit(cg, "    .globl %sgui_poll_events", pfx);
            cg_emit(cg, "    .globl %sgui_close_window", pfx);
            cg_emit(cg, "    .globl %sgui_draw_rect", pfx);
            cg_emit(cg, "    .globl %sgui_draw_text", pfx);
        }
        
        // Mobile functions (external, defined in runtime/mobile_ios.m)
        if (cg->os == OS_MACOS) {
            cg_emit(cg, "    .globl %smobile_init", pfx);
            cg_emit(cg, "    .globl %smobile_run", pfx);
            cg_emit(cg, "    .globl %smobile_create_view", pfx);
            cg_emit(cg, "    .globl %smobile_add_subview", pfx);
            cg_emit(cg, "    .globl %smobile_add_to_root", pfx);
            cg_emit(cg, "    .globl %smobile_create_button", pfx);
            cg_emit(cg, "    .globl %smobile_create_label", pfx);
            cg_emit(cg, "    .globl %smobile_create_textfield", pfx);
            cg_emit(cg, "    .globl %smobile_set_text", pfx);
            cg_emit(cg, "    .globl %smobile_get_text", pfx);
            cg_emit(cg, "    .globl %smobile_set_frame", pfx);
            cg_emit(cg, "    .globl %smobile_set_background_color", pfx);
            cg_emit(cg, "    .globl %smobile_set_text_color", pfx);
            cg_emit(cg, "    .globl %smobile_set_font_size", pfx);
            cg_emit(cg, "    .globl %smobile_on_tap", pfx);
            cg_emit(cg, "    .globl %smobile_on_text_change", pfx);
            cg_emit(cg, "    .globl %smobile_show_alert", pfx);
            cg_emit(cg, "    .globl %smobile_hide_view", pfx);
            cg_emit(cg, "    .globl %smobile_show_view", pfx);
            cg_emit(cg, "    .globl %smobile_remove_view", pfx);
        }
        
        if (cg->os == OS_MACOS) {
            cg_emit(cg, "    .section __DATA,__data");
        } else {
            cg_emit(cg, "    .data");
        }
        cg_emit(cg, "L_.fmt:");
        cg_emit(cg, "    .asciz \"%%lld\"");
        cg_emit(cg, "L_.lnfmt:");
        cg_emit(cg, "    .asciz \"%%lld\\n\"");
        cg_emit(cg, "L_.sfmt:");
        cg_emit(cg, "    .asciz \"%%s\"");
        cg_emit(cg, "L_.ffmt:");
        cg_emit(cg, "    .asciz \"%%g\"");
        cg_emit(cg, "L_.itsfmt:");
        cg_emit(cg, "    .asciz \"%%lld\"");
        cg_emit(cg, "L_.itsbuf:");
        cg_emit(cg, "    .space 24");
        cg_emit(cg, "L_.chrbuf:");
        cg_emit(cg, "    .space 2");
        cg_emit(cg, "L_.rb:");
        cg_emit(cg, "    .asciz \"rb\"");
        cg_emit(cg, "L_.wb:");
        cg_emit(cg, "    .asciz \"w\"");
        cg_emit(cg, "L_.ab:");
        cg_emit(cg, "    .asciz \"a\"");
        cg_emit(cg, "L_.linebuf:");
        cg_emit(cg, "    .space 1024");
        cg_emit(cg, "L_.empty_str:");
        cg_emit(cg, "    .asciz \"\"");
        cg_emit(cg, "L_.wyn_argc:");
        cg_emit(cg, "    .quad 0");
        cg_emit(cg, "L_.wyn_argv:");
        cg_emit(cg, "    .quad 0");
        
        for (int i = 0; i < cg->string_count; i++) {
            cg_emit(cg, "L_.str%d:", cg->strings[i].label);
            char escaped[4096];
            escape_string_for_asm(cg->strings[i].str, escaped, sizeof(escaped));
            cg_emit(cg, "    .asciz \"%s\"", escaped);
        }
        return;
    }
    
    // ARM64
    if (cg->os == OS_MACOS) {
        cg_emit(cg, "    .section __TEXT,__text,regular,pure_instructions");
        cg_emit(cg, "    .build_version macos, 14, 0");
    } else {
        cg_emit(cg, "    .text");
    }
    
    for (int i = 0; i < cg->module->function_count; i++) {
        cg_function(cg, &cg->module->functions[i]);
        cg_emit(cg, "");
    }
    
    // Emit struct methods
    for (int i = 0; i < cg->module->struct_count; i++) {
        StructDef* s = &cg->module->structs[i];
        for (int j = 0; j < s->method_count; j++) {
            cg_method(cg, s->name, &s->methods[j]);
            cg_emit(cg, "");
        }
    }
    
    // Emit lambdas
    cg_emit_lambdas(cg);
    
    const char* pfx = cg_sym_prefix(cg);
    
    // print runtime - ARM64 variadic calling convention
    cg_emit(cg, "    .globl %sprint", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%sprint:", pfx);
    cg_emit(cg, "    sub sp, sp, #32");
    cg_emit(cg, "    stp x29, x30, [sp, #16]");
    cg_emit(cg, "    add x29, sp, #16");
    cg_emit(cg, "    str x0, [sp, #8]");  // Save arg
    cg_emit(cg, "    ldr x8, [sp, #8]");  // Load arg
    cg_emit(cg, "    mov x9, sp");
    cg_emit(cg, "    str x8, [x9]");  // Store at sp for variadic
    cg_emit_adrp(cg, "x0", "L_.fmt");
    cg_emit_add_pageoff(cg, "x0", "x0", "L_.fmt");
    cg_emit(cg, "    bl %sprintf", pfx);
    cg_emit(cg, "    ldp x29, x30, [sp, #16]");
    cg_emit(cg, "    add sp, sp, #32");
    cg_emit(cg, "    ret");
    
    // print_str runtime for strings
    cg_emit(cg, "    .globl %sprint_str", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%sprint_str:", pfx);
    cg_emit(cg, "    sub sp, sp, #32");
    cg_emit(cg, "    stp x29, x30, [sp, #16]");
    cg_emit(cg, "    add x29, sp, #16");
    cg_emit(cg, "    str x0, [sp, #8]");
    cg_emit(cg, "    ldr x8, [sp, #8]");
    cg_emit(cg, "    mov x9, sp");
    cg_emit(cg, "    str x8, [x9]");
    cg_emit_adrp(cg, "x0", "L_.sfmt");
    cg_emit_add_pageoff(cg, "x0", "x0", "L_.sfmt");
    cg_emit(cg, "    bl %sprintf", pfx);
    cg_emit(cg, "    ldp x29, x30, [sp, #16]");
    cg_emit(cg, "    add sp, sp, #32");
    cg_emit(cg, "    ret");
    
    // print_float runtime for floats
    cg_emit(cg, "    .globl %sprint_float", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%sprint_float:", pfx);
    cg_emit(cg, "    sub sp, sp, #32");
    cg_emit(cg, "    stp x29, x30, [sp, #16]");
    cg_emit(cg, "    add x29, sp, #16");
    cg_emit(cg, "    fmov d0, x0");  // x0 contains float bits, move to d0
    cg_emit(cg, "    mov x8, sp");
    cg_emit(cg, "    str d0, [x8]");  // Store float on stack for variadic
    cg_emit_adrp(cg, "x0", "L_.ffmt");
    cg_emit_add_pageoff(cg, "x0", "x0", "L_.ffmt");
    cg_emit(cg, "    bl %sprintf", pfx);
    cg_emit(cg, "    ldp x29, x30, [sp, #16]");
    cg_emit(cg, "    add sp, sp, #32");
    cg_emit(cg, "    ret");
    
    // print_char: prints a single character
    cg_emit(cg, "    .globl %sprint_char", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%sprint_char:", pfx);
    cg_emit(cg, "    sub sp, sp, #16");
    cg_emit(cg, "    stp x29, x30, [sp]");
    cg_emit(cg, "    mov x29, sp");
    cg_emit(cg, "    bl %sputchar", pfx);
    cg_emit(cg, "    ldp x29, x30, [sp]");
    cg_emit(cg, "    add sp, sp, #16");
    cg_emit(cg, "    ret");
    
    // print_newline: prints a newline character
    cg_emit(cg, "    .globl %sprint_newline", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%sprint_newline:", pfx);
    cg_emit(cg, "    sub sp, sp, #16");
    cg_emit(cg, "    stp x29, x30, [sp]");
    cg_emit(cg, "    mov x29, sp");
    cg_emit(cg, "    mov w0, #10");
    cg_emit(cg, "    bl %sputchar", pfx);
    cg_emit(cg, "    ldp x29, x30, [sp]");
    cg_emit(cg, "    add sp, sp, #16");
    cg_emit(cg, "    ret");
    
    // println: prints int with newline
    cg_emit(cg, "    .globl %sprintln", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%sprintln:", pfx);
    cg_emit(cg, "    sub sp, sp, #32");
    cg_emit(cg, "    stp x29, x30, [sp, #16]");
    cg_emit(cg, "    add x29, sp, #16");
    cg_emit(cg, "    mov x8, x0");
    cg_emit(cg, "    add x9, sp, #0");
    cg_emit(cg, "    str x8, [x9]");
    cg_emit_adrp(cg, "x0", "L_.lnfmt");
    cg_emit_add_pageoff(cg, "x0", "x0", "L_.lnfmt");
    cg_emit(cg, "    bl %sprintf", pfx);
    cg_emit(cg, "    ldp x29, x30, [sp, #16]");
    cg_emit(cg, "    add sp, sp, #32");
    cg_emit(cg, "    ret");
    
    // println_str: prints string with newline
    cg_emit(cg, "    .globl %sprintln_str", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%sprintln_str:", pfx);
    cg_emit(cg, "    sub sp, sp, #16");
    cg_emit(cg, "    stp x29, x30, [sp]");
    cg_emit(cg, "    mov x29, sp");
    cg_emit(cg, "    bl %sputs", pfx);
    cg_emit(cg, "    ldp x29, x30, [sp]");
    cg_emit(cg, "    add sp, sp, #16");
    cg_emit(cg, "    ret");
    
    // flush: flushes stdout
    cg_emit(cg, "    .globl %sflush", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%sflush:", pfx);
    cg_emit(cg, "    sub sp, sp, #16");
    cg_emit(cg, "    stp x29, x30, [sp]");
    cg_emit(cg, "    mov x29, sp");
    if (cg->os == OS_MACOS) {
        const char* stdout_sym = cg->os == OS_MACOS ? "__stdoutp" : "stdout";
        cg_emit(cg, "    adrp x0, %s%s@GOTPAGE", pfx, stdout_sym);
        cg_emit(cg, "    ldr x0, [x0, %s%s@GOTPAGEOFF]", pfx, stdout_sym);
        cg_emit(cg, "    ldr x0, [x0]");
    } else {
        cg_emit(cg, "    adrp x0, stdout");
        cg_emit(cg, "    ldr x0, [x0, :lo12:stdout]");
    }
    cg_emit(cg, "    bl %sfflush", pfx);
    cg_emit(cg, "    ldp x29, x30, [sp]");
    cg_emit(cg, "    add sp, sp, #16");
    cg_emit(cg, "    ret");
    
    // __wyn_read_file: reads file to malloc'd string, returns ptr (or 0 on error)
    cg_emit(cg, "    .globl %s_wyn_read_file", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s_wyn_read_file:", pfx);
    cg_emit(cg, "    sub sp, sp, #64");
    cg_emit(cg, "    stp x29, x30, [sp, #48]");
    cg_emit(cg, "    stp x19, x20, [sp, #32]");
    cg_emit(cg, "    add x29, sp, #48");
    cg_emit_adrp(cg, "x1", "L_.rb");
    cg_emit_add_pageoff(cg, "x1", "x1", "L_.rb");
    cg_emit(cg, "    bl %sfopen", pfx);
    cg_emit(cg, "    cbz x0, 1f");
    cg_emit(cg, "    mov x19, x0");                   // x19 = file handle
    cg_emit(cg, "    mov x1, #0");
    cg_emit(cg, "    mov x2, #2");                    // SEEK_END
    cg_emit(cg, "    bl %sfseek", pfx);
    cg_emit(cg, "    mov x0, x19");
    cg_emit(cg, "    bl %sftell", pfx);
    cg_emit(cg, "    mov x20, x0");                   // x20 = file size
    cg_emit(cg, "    mov x0, x19");
    cg_emit(cg, "    mov x1, #0");
    cg_emit(cg, "    mov x2, #0");                    // SEEK_SET
    cg_emit(cg, "    bl %sfseek", pfx);
    cg_emit(cg, "    add x0, x20, #1");               // size + 1 for null
    cg_emit(cg, "    bl %smalloc", pfx);
    cg_emit(cg, "    cbz x0, 2f");
    cg_emit(cg, "    str x0, [sp, #16]");             // save buffer ptr
    cg_emit(cg, "    mov x1, #1");
    cg_emit(cg, "    mov x2, x20");
    cg_emit(cg, "    mov x3, x19");
    cg_emit(cg, "    bl %sfread", pfx);
    cg_emit(cg, "    ldr x0, [sp, #16]");             // restore buffer ptr
    cg_emit(cg, "    strb wzr, [x0, x20]");           // null terminate
    cg_emit(cg, "    str x0, [sp, #16]");
    cg_emit(cg, "    mov x0, x19");
    cg_emit(cg, "    bl %sfclose", pfx);
    cg_emit(cg, "    ldr x0, [sp, #16]");
    cg_emit(cg, "    b 3f");
    cg_emit(cg, "2:  mov x0, x19");                   // malloc failed, close file
    cg_emit(cg, "    bl %sfclose", pfx);
    cg_emit(cg, "1:  mov x0, #0");                    // return 0 on error
    cg_emit(cg, "3:  ldp x19, x20, [sp, #32]");
    cg_emit(cg, "    ldp x29, x30, [sp, #48]");
    cg_emit(cg, "    add sp, sp, #64");
    cg_emit(cg, "    ret");
    
    // __wyn_read_line: reads a line from stdin, returns ptr to static buffer
    cg_emit(cg, "    .globl %s_wyn_read_line", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s_wyn_read_line:", pfx);
    cg_emit(cg, "    sub sp, sp, #32");
    cg_emit(cg, "    stp x29, x30, [sp, #16]");
    cg_emit(cg, "    add x29, sp, #16");
    cg_emit_adrp(cg, "x0", "L_.linebuf");
    cg_emit_add_pageoff(cg, "x0", "x0", "L_.linebuf");
    cg_emit(cg, "    mov x1, #1024");
    // Load stdin pointer
    if (cg->os == OS_MACOS) {
        const char* stdin_sym = cg->os == OS_MACOS ? "__stdinp" : "stdin";
        cg_emit(cg, "    adrp x2, %s%s@GOTPAGE", pfx, stdin_sym);
        cg_emit(cg, "    ldr x2, [x2, %s%s@GOTPAGEOFF]", pfx, stdin_sym);
        cg_emit(cg, "    ldr x2, [x2]");
    } else {
        cg_emit(cg, "    adrp x2, stdin");
        cg_emit(cg, "    ldr x2, [x2, :lo12:stdin]");
    }
    cg_emit(cg, "    bl %sfgets", pfx);
    cg_emit(cg, "    cbz x0, 1f");
    // Strip trailing newline
    cg_emit_adrp(cg, "x0", "L_.linebuf");
    cg_emit_add_pageoff(cg, "x0", "x0", "L_.linebuf");
    cg_emit(cg, "    mov x1, #0");
    cg_emit(cg, "2:  ldrb w2, [x0, x1]");
    cg_emit(cg, "    cbz w2, 3f");
    cg_emit(cg, "    cmp w2, #10");
    cg_emit(cg, "    b.eq 4f");
    cg_emit(cg, "    add x1, x1, #1");
    cg_emit(cg, "    b 2b");
    cg_emit(cg, "4:  strb wzr, [x0, x1]");
    cg_emit(cg, "3:  b 5f");
    cg_emit(cg, "1:");
    cg_emit_adrp(cg, "x0", "L_.linebuf");
    cg_emit_add_pageoff(cg, "x0", "x0", "L_.linebuf");
    cg_emit(cg, "    strb wzr, [x0]");
    cg_emit(cg, "5:  ldp x29, x30, [sp, #16]");
    cg_emit(cg, "    add sp, sp, #32");
    cg_emit(cg, "    ret");
    
    // __wyn_write_file: writes string to file, returns 0 on success, -1 on error
    cg_emit(cg, "    .globl %s_wyn_write_file", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s_wyn_write_file:", pfx);
    cg_emit(cg, "    sub sp, sp, #48");
    cg_emit(cg, "    stp x29, x30, [sp, #32]");
    cg_emit(cg, "    stp x19, x20, [sp, #16]");
    cg_emit(cg, "    add x29, sp, #32");
    cg_emit(cg, "    mov x19, x1");                   // x19 = content
    cg_emit_adrp(cg, "x1", "L_.wb");
    cg_emit_add_pageoff(cg, "x1", "x1", "L_.wb");
    cg_emit(cg, "    bl %sfopen", pfx);
    cg_emit(cg, "    cbz x0, 1f");
    cg_emit(cg, "    mov x20, x0");                   // x20 = file handle
    cg_emit(cg, "    mov x0, x19");                   // content
    cg_emit(cg, "    mov x1, x20");                   // file
    cg_emit(cg, "    bl %sfputs", pfx);
    cg_emit(cg, "    mov x0, x20");
    cg_emit(cg, "    bl %sfclose", pfx);
    cg_emit(cg, "    mov x0, #0");                    // return 0 on success
    cg_emit(cg, "    b 2f");
    cg_emit(cg, "1:  mov x0, #-1");                   // return -1 on error
    cg_emit(cg, "2:  ldp x19, x20, [sp, #16]");
    cg_emit(cg, "    ldp x29, x30, [sp, #32]");
    cg_emit(cg, "    add sp, sp, #48");
    cg_emit(cg, "    ret");
    
    // __wyn_file_exists: check if file exists, returns 1 if exists, 0 otherwise
    cg_emit(cg, "    .globl %s_wyn_file_exists", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s_wyn_file_exists:", pfx);
    cg_emit(cg, "    sub sp, sp, #32");
    cg_emit(cg, "    stp x29, x30, [sp, #16]");
    cg_emit(cg, "    add x29, sp, #16");
    cg_emit(cg, "    mov w1, #0");                    // F_OK = 0
    cg_emit(cg, "    bl %saccess", pfx);
    cg_emit(cg, "    cmp w0, #0");
    cg_emit(cg, "    cset w0, eq");
    cg_emit(cg, "    ldp x29, x30, [sp, #16]");
    cg_emit(cg, "    add sp, sp, #32");
    cg_emit(cg, "    ret");
    
    // __wyn_delete_file: delete file, returns 1 on success, 0 on failure
    cg_emit(cg, "    .globl %s_wyn_delete_file", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s_wyn_delete_file:", pfx);
    cg_emit(cg, "    sub sp, sp, #32");
    cg_emit(cg, "    stp x29, x30, [sp, #16]");
    cg_emit(cg, "    add x29, sp, #16");
    cg_emit(cg, "    bl %sunlink", pfx);
    cg_emit(cg, "    cmp w0, #0");
    cg_emit(cg, "    cset w0, eq");
    cg_emit(cg, "    ldp x29, x30, [sp, #16]");
    cg_emit(cg, "    add sp, sp, #32");
    cg_emit(cg, "    ret");
    
    // __wyn_mkdir: create directory, returns 1 on success, 0 on failure
    cg_emit(cg, "    .globl %s_wyn_mkdir", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s_wyn_mkdir:", pfx);
    cg_emit(cg, "    sub sp, sp, #32");
    cg_emit(cg, "    stp x29, x30, [sp, #16]");
    cg_emit(cg, "    add x29, sp, #16");
    cg_emit(cg, "    mov w1, #0755");                 // mode 0755
    cg_emit(cg, "    bl %smkdir", pfx);
    cg_emit(cg, "    cmp w0, #0");
    cg_emit(cg, "    cset w0, eq");
    cg_emit(cg, "    ldp x29, x30, [sp, #16]");
    cg_emit(cg, "    add sp, sp, #32");
    cg_emit(cg, "    ret");
    
    // __wyn_rmdir: remove directory, returns 1 on success, 0 on failure
    cg_emit(cg, "    .globl %s_wyn_rmdir", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s_wyn_rmdir:", pfx);
    cg_emit(cg, "    sub sp, sp, #32");
    cg_emit(cg, "    stp x29, x30, [sp, #16]");
    cg_emit(cg, "    add x29, sp, #16");
    cg_emit(cg, "    bl %srmdir", pfx);
    cg_emit(cg, "    cmp w0, #0");
    cg_emit(cg, "    cset w0, eq");
    cg_emit(cg, "    ldp x29, x30, [sp, #16]");
    cg_emit(cg, "    add sp, sp, #32");
    cg_emit(cg, "    ret");
    
    // __wyn_file_size: get file size in bytes, returns -1 on error
    cg_emit(cg, "    .globl %s_wyn_file_size", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s_wyn_file_size:", pfx);
    cg_emit(cg, "    sub sp, sp, #176");
    cg_emit(cg, "    stp x29, x30, [sp, #160]");
    cg_emit(cg, "    add x29, sp, #160");
    cg_emit(cg, "    add x1, sp, #16");               // stat buffer
    cg_emit(cg, "    bl %sstat", pfx);
    cg_emit(cg, "    cmp w0, #0");
    cg_emit(cg, "    b.ne 1f");
    cg_emit(cg, "    ldr x0, [sp, #112]");            // st_size at offset 96
    cg_emit(cg, "    b 2f");
    cg_emit(cg, "1:  mov x0, #-1");
    cg_emit(cg, "2:  ldp x29, x30, [sp, #160]");
    cg_emit(cg, "    add sp, sp, #176");
    cg_emit(cg, "    ret");
    
    // __wyn_is_dir: check if path is directory, returns 1 if dir, 0 otherwise
    cg_emit(cg, "    .globl %s_wyn_is_dir", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s_wyn_is_dir:", pfx);
    cg_emit(cg, "    sub sp, sp, #176");
    cg_emit(cg, "    stp x29, x30, [sp, #160]");
    cg_emit(cg, "    add x29, sp, #160");
    cg_emit(cg, "    add x1, sp, #16");               // stat buffer
    cg_emit(cg, "    bl %sstat", pfx);
    cg_emit(cg, "    cmp w0, #0");
    cg_emit(cg, "    b.ne 1f");
    cg_emit(cg, "    ldr w0, [sp, #20]");             // st_mode at offset 4
    cg_emit(cg, "    and w0, w0, #0xF000");           // S_IFMT mask
    cg_emit(cg, "    cmp w0, #0x4000");               // S_IFDIR
    cg_emit(cg, "    cset w0, eq");
    cg_emit(cg, "    b 2f");
    cg_emit(cg, "1:  mov w0, #0");
    cg_emit(cg, "2:  ldp x29, x30, [sp, #160]");
    cg_emit(cg, "    add sp, sp, #176");
    cg_emit(cg, "    ret");
    
    // __wyn_append_file: append string to file, returns 1 on success, 0 on failure
    cg_emit(cg, "    .globl %s_wyn_append_file", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s_wyn_append_file:", pfx);
    cg_emit(cg, "    sub sp, sp, #48");
    cg_emit(cg, "    stp x29, x30, [sp, #32]");
    cg_emit(cg, "    stp x19, x20, [sp, #16]");
    cg_emit(cg, "    add x29, sp, #32");
    cg_emit(cg, "    mov x19, x1");                   // x19 = content
    cg_emit_adrp(cg, "x1", "L_.ab");
    cg_emit_add_pageoff(cg, "x1", "x1", "L_.ab");
    cg_emit(cg, "    bl %sfopen", pfx);
    cg_emit(cg, "    cbz x0, 1f");
    cg_emit(cg, "    mov x20, x0");                   // x20 = file handle
    cg_emit(cg, "    mov x0, x19");                   // content
    cg_emit(cg, "    mov x1, x20");                   // file
    cg_emit(cg, "    bl %sfputs", pfx);
    cg_emit(cg, "    mov x0, x20");
    cg_emit(cg, "    bl %sfclose", pfx);
    cg_emit(cg, "    mov x0, #1");                    // return 1 on success
    cg_emit(cg, "    b 2f");
    cg_emit(cg, "1:  mov x0, #0");                    // return 0 on error
    cg_emit(cg, "2:  ldp x19, x20, [sp, #16]");
    cg_emit(cg, "    ldp x29, x30, [sp, #32]");
    cg_emit(cg, "    add sp, sp, #48");
    cg_emit(cg, "    ret");
    
    // __wyn_tcp_connect: connect to TCP socket, returns fd or -1
    cg_emit(cg, "    .globl %s_wyn_tcp_connect", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s_wyn_tcp_connect:", pfx);
    cg_emit(cg, "    sub sp, sp, #80");
    cg_emit(cg, "    stp x29, x30, [sp, #64]");
    cg_emit(cg, "    stp x19, x20, [sp, #48]");
    cg_emit(cg, "    add x29, sp, #64");
    cg_emit(cg, "    str x0, [sp, #32]");             // save host
    cg_emit(cg, "    str x1, [sp, #24]");             // save port
    // socket(AF_INET=2, SOCK_STREAM=1, 0)
    cg_emit(cg, "    mov w0, #2");
    cg_emit(cg, "    mov w1, #1");
    cg_emit(cg, "    mov w2, #0");
    cg_emit(cg, "    bl %ssocket", pfx);
    cg_emit(cg, "    cmp w0, #0");
    cg_emit(cg, "    blt 1f");
    cg_emit(cg, "    str w0, [sp, #20]");             // save sockfd
    // gethostbyname(host)
    cg_emit(cg, "    ldr x0, [sp, #32]");
    cg_emit(cg, "    bl %sgethostbyname", pfx);
    cg_emit(cg, "    cbz x0, 2f");
    cg_emit(cg, "    mov x19, x0");                   // save hostent*
    // Build sockaddr_in: sin_family=AF_INET, sin_port=htons(port), sin_addr
    cg_emit(cg, "    mov w1, #2");
    cg_emit(cg, "    strh w1, [sp, #0]");             // sin_family = AF_INET
    cg_emit(cg, "    ldr x0, [sp, #24]");             // port
    cg_emit(cg, "    bl %shtons", pfx);
    cg_emit(cg, "    strh w0, [sp, #2]");             // sin_port
    cg_emit(cg, "    ldr x1, [x19, #16]");            // h_addr_list[0]
    cg_emit(cg, "    ldr w0, [x1]");
    cg_emit(cg, "    str w0, [sp, #4]");              // sin_addr
    cg_emit(cg, "    str xzr, [sp, #8]");             // sin_zero
    // connect(sockfd, &addr, 16)
    cg_emit(cg, "    ldr w0, [sp, #20]");
    cg_emit(cg, "    mov x1, sp");
    cg_emit(cg, "    mov w2, #16");
    cg_emit(cg, "    bl %sconnect", pfx);
    cg_emit(cg, "    cbnz w0, 2f");
    cg_emit(cg, "    ldr w0, [sp, #20]");             // return sockfd
    cg_emit(cg, "    b 3f");
    cg_emit(cg, "2:  ldr w0, [sp, #20]");             // close on error
    cg_emit(cg, "    bl %sclose", pfx);
    cg_emit(cg, "1:  mov w0, #-1");                   // return -1
    cg_emit(cg, "    sxtw x0, w0");                   // sign extend to 64-bit
    cg_emit(cg, "3:  ldp x19, x20, [sp, #48]");
    cg_emit(cg, "    ldp x29, x30, [sp, #64]");
    cg_emit(cg, "    add sp, sp, #80");
    cg_emit(cg, "    ret");
    
    // __wyn_tcp_listen: create listening socket, returns fd or -1
    cg_emit(cg, "    .globl %s_wyn_tcp_listen", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s_wyn_tcp_listen:", pfx);
    cg_emit(cg, "    sub sp, sp, #48");
    cg_emit(cg, "    stp x29, x30, [sp, #32]");
    cg_emit(cg, "    add x29, sp, #32");
    cg_emit(cg, "    str x0, [sp, #24]");             // save port
    // socket(AF_INET=2, SOCK_STREAM=1, 0)
    cg_emit(cg, "    mov w0, #2");
    cg_emit(cg, "    mov w1, #1");
    cg_emit(cg, "    mov w2, #0");
    cg_emit(cg, "    bl %ssocket", pfx);
    cg_emit(cg, "    cmp w0, #0");
    cg_emit(cg, "    blt 1f");
    cg_emit(cg, "    str w0, [sp, #20]");             // save sockfd
    // Build sockaddr_in: sin_family=AF_INET, sin_port=htons(port), sin_addr=INADDR_ANY
    cg_emit(cg, "    mov w1, #2");
    cg_emit(cg, "    strh w1, [sp, #0]");             // sin_family = AF_INET
    cg_emit(cg, "    ldr x0, [sp, #24]");             // port
    cg_emit(cg, "    bl %shtons", pfx);
    cg_emit(cg, "    strh w0, [sp, #2]");             // sin_port
    cg_emit(cg, "    str wzr, [sp, #4]");             // sin_addr = INADDR_ANY
    cg_emit(cg, "    str xzr, [sp, #8]");             // sin_zero
    // bind(sockfd, &addr, 16)
    cg_emit(cg, "    ldr w0, [sp, #20]");
    cg_emit(cg, "    mov x1, sp");
    cg_emit(cg, "    mov w2, #16");
    cg_emit(cg, "    bl %sbind", pfx);
    cg_emit(cg, "    cbnz w0, 2f");
    // listen(sockfd, 5)
    cg_emit(cg, "    ldr w0, [sp, #20]");
    cg_emit(cg, "    mov w1, #5");
    cg_emit(cg, "    bl %slisten", pfx);
    cg_emit(cg, "    cbnz w0, 2f");
    cg_emit(cg, "    ldr w0, [sp, #20]");             // return sockfd
    cg_emit(cg, "    b 3f");
    cg_emit(cg, "2:  ldr w0, [sp, #20]");             // close on error
    cg_emit(cg, "    bl %sclose", pfx);
    cg_emit(cg, "1:  mov w0, #-1");                   // return -1
    cg_emit(cg, "    sxtw x0, w0");                   // sign extend to 64-bit
    cg_emit(cg, "3:  ldp x29, x30, [sp, #32]");
    cg_emit(cg, "    add sp, sp, #48");
    cg_emit(cg, "    ret");
    
    // __wyn_char_at: get character at index, returns single-char string or ""
    cg_emit(cg, "    .globl %s_wyn_char_at", pfx);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s_wyn_char_at:", pfx);
    cg_emit(cg, "    stp x29, x30, [sp, #-32]!");
    cg_emit(cg, "    mov x29, sp");
    cg_emit(cg, "    stp x19, x20, [sp, #16]");
    cg_emit(cg, "    mov x19, x0");                   // save string
    cg_emit(cg, "    mov x20, x1");                   // save index
    cg_emit(cg, "    bl %sstrlen", pfx);              // get string length
    cg_emit(cg, "    cmp x20, #0");                   // check i >= 0
    cg_emit(cg, "    blt 1f");
    cg_emit(cg, "    cmp x20, x0");                   // check i < len
    cg_emit(cg, "    bge 1f");
    cg_emit(cg, "    mov x0, #2");                    // malloc 2 bytes
    cg_emit(cg, "    bl %smalloc", pfx);
    cg_emit(cg, "    ldrb w1, [x19, x20]");           // get s[i]
    cg_emit(cg, "    strb w1, [x0]");                 // store char
    cg_emit(cg, "    strb wzr, [x0, #1]");            // null terminator
    cg_emit(cg, "    b 2f");
    cg_emit(cg, "1:  adrp x0, L_.empty_str@PAGE");    // return ""
    cg_emit(cg, "    add x0, x0, L_.empty_str@PAGEOFF");
    cg_emit(cg, "2:  ldp x19, x20, [sp, #16]");
    cg_emit(cg, "    ldp x29, x30, [sp], #32");
    cg_emit(cg, "    ret");
    
    // GUI functions (external, defined in runtime/gui_macos.c)
    if (cg->os == OS_MACOS) {
        cg_emit(cg, "    .globl %sgui_create_window", pfx);
        cg_emit(cg, "    .globl %sgui_show_window", pfx);
        cg_emit(cg, "    .globl %sgui_set_title", pfx);
        cg_emit(cg, "    .globl %sgui_run", pfx);
        cg_emit(cg, "    .globl %sgui_poll_events", pfx);
        cg_emit(cg, "    .globl %sgui_close_window", pfx);
        cg_emit(cg, "    .globl %sgui_draw_rect", pfx);
        cg_emit(cg, "    .globl %sgui_draw_text", pfx);
    }
    
    // Mobile functions (external, defined in runtime/mobile_ios.m)
    if (cg->os == OS_MACOS) {
        cg_emit(cg, "    .globl %smobile_init", pfx);
        cg_emit(cg, "    .globl %smobile_run", pfx);
        cg_emit(cg, "    .globl %smobile_create_view", pfx);
        cg_emit(cg, "    .globl %smobile_add_subview", pfx);
        cg_emit(cg, "    .globl %smobile_add_to_root", pfx);
        cg_emit(cg, "    .globl %smobile_create_button", pfx);
        cg_emit(cg, "    .globl %smobile_create_label", pfx);
        cg_emit(cg, "    .globl %smobile_create_textfield", pfx);
        cg_emit(cg, "    .globl %smobile_set_text", pfx);
        cg_emit(cg, "    .globl %smobile_get_text", pfx);
        cg_emit(cg, "    .globl %smobile_set_frame", pfx);
        cg_emit(cg, "    .globl %smobile_set_background_color", pfx);
        cg_emit(cg, "    .globl %smobile_set_text_color", pfx);
        cg_emit(cg, "    .globl %smobile_set_font_size", pfx);
        cg_emit(cg, "    .globl %smobile_on_tap", pfx);
        cg_emit(cg, "    .globl %smobile_on_text_change", pfx);
        cg_emit(cg, "    .globl %smobile_show_alert", pfx);
        cg_emit(cg, "    .globl %smobile_hide_view", pfx);
        cg_emit(cg, "    .globl %smobile_show_view", pfx);
        cg_emit(cg, "    .globl %smobile_remove_view", pfx);
    }
    
    if (cg->os == OS_MACOS) {
        cg_emit(cg, "    .section __DATA,__data");
    } else {
        cg_emit(cg, "    .data");
    }
    cg_emit(cg, "L_.fmt:");
    cg_emit(cg, "    .asciz \"%%lld\"");
    cg_emit(cg, "L_.lnfmt:");
    cg_emit(cg, "    .asciz \"%%lld\\n\"");
    cg_emit(cg, "L_.sfmt:");
    cg_emit(cg, "    .asciz \"%%s\"");
    cg_emit(cg, "L_.ffmt:");
    cg_emit(cg, "    .asciz \"%%g\"");
    cg_emit(cg, "L_.itsfmt:");
    cg_emit(cg, "    .asciz \"%%lld\"");
    cg_emit(cg, "L_.itsbuf:");
    cg_emit(cg, "    .space 24");
    cg_emit(cg, "L_.chrbuf:");
    cg_emit(cg, "    .space 2");
    cg_emit(cg, "L_.rb:");
    cg_emit(cg, "    .asciz \"rb\"");
    cg_emit(cg, "L_.wb:");
    cg_emit(cg, "    .asciz \"w\"");
    cg_emit(cg, "L_.ab:");
    cg_emit(cg, "    .asciz \"a\"");
    cg_emit(cg, "    .asciz \"w\"");
    cg_emit(cg, "L_.linebuf:");
    cg_emit(cg, "    .space 1024");
    cg_emit(cg, "L_.empty_str:");
    cg_emit(cg, "    .asciz \"\"");
    cg_emit(cg, "L_.wyn_argc:");
    cg_emit(cg, "    .quad 0");
    cg_emit(cg, "L_.wyn_argv:");
    cg_emit(cg, "    .quad 0");
    
    // Emit string literals
    for (int i = 0; i < cg->string_count; i++) {
        cg_emit(cg, "L_.str%d:", cg->strings[i].label);
        char escaped[4096];
        escape_string_for_asm(cg->strings[i].str, escaped, sizeof(escaped));
        cg_emit(cg, "    .asciz \"%s\"", escaped);
    }
    
    // Emit float literals
    cg_emit(cg, "    .align 3");
    for (int i = 0; i < cg->float_count; i++) {
        cg_emit(cg, "L_.flt%d:", cg->floats[i].label);
        cg_emit(cg, "    .double %f", cg->floats[i].val);
    }
}

// ============================================================================
// Main Entry Point
// ============================================================================

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
        
        // Try relative path first
        if (imp->path[0] == '.' && imp->path[1] == '/') {
            snprintf(full_path, 512, "%s%s.wyn", dir, imp->path + 2);
            f = fopen(full_path, "rb");
        }
        
        // Try std/ directory for stdlib modules
        if (!f) {
            snprintf(full_path, 512, "std/%s.wyn", imp->path);
            f = fopen(full_path, "rb");
        }
        
        // Try root-level module
        if (!f && dir[0]) {
            char try_path[512];
            strcpy(try_path, dir);
            for (int depth = 0; depth < 5 && !f; depth++) {
                snprintf(full_path, 512, "%s%s.wyn", try_path, imp->path);
                f = fopen(full_path, "rb");
                if (!f) {
                    snprintf(full_path, 512, "%sstd/%s.wyn", try_path, imp->path);
                    f = fopen(full_path, "rb");
                }
                if (!f) strcat(try_path, "../");
            }
        }
        
        // Try std/ in current dir (legacy)
        if (!f) {
            snprintf(full_path, 512, "%sstd/%s.wyn", dir, imp->path);
            f = fopen(full_path, "rb");
        }
        
        // Try std/ in parent dir (legacy)
        if (!f) {
            snprintf(full_path, 512, "%s../std/%s.wyn", dir, imp->path);
            f = fopen(full_path, "rb");
        }
        
        // Try absolute std path (legacy - for project root)
        if (!f && dir[0]) {
            char try_path[512];
            strcpy(try_path, dir);
            for (int depth = 0; depth < 5 && !f; depth++) {
                snprintf(full_path, 512, "%sstd/%s.wyn", try_path, imp->path);
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
            // Merge functions
            for (int j = 0; j < imported->function_count; j++) {
                m->functions[m->function_count++] = imported->functions[j];
            }
            // Merge structs
            for (int j = 0; j < imported->struct_count; j++) {
                m->structs[m->struct_count++] = imported->structs[j];
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
} LLVMGen;

static void llvm_emit(LLVMGen* lg, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(lg->out, fmt, args);
    va_end(args);
    fprintf(lg->out, "\n");
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
    for (int i = lg->var_count - 1; i >= 0; i--) {
        if (strcmp(lg->vars[i], name) == 0) return lg->var_types[i];
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
            // Check if variable is float type using conservative heuristic
            return strstr(e->ident, "float") != NULL || 
                   (strlen(e->ident) > 0 && e->ident[strlen(e->ident)-1] == 'f');
        }
        case EXPR_BINARY:
            return llvm_is_float_expr(lg, e->binary.left) || llvm_is_float_expr(lg, e->binary.right);
        default: return false;
    }
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
            *result_reg = -(e->bool_val ? 1 : 0) - 2000000;
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
            bool is_float_op = llvm_is_float_expr(lg, e->binary.left) || llvm_is_float_expr(lg, e->binary.right);
            
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
            
            int t = llvm_new_temp(lg);
            if (is_float_op) {
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
                switch (e->binary.op) {
                    case TOK_PLUS:
                        llvm_emit(lg, "  %%%d = add i64 %s, %s", t, left_str, right_str);
                        break;
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
                    case TOK_EQEQ:
                        llvm_emit(lg, "  %%%d = icmp eq i64 %s, %s", t, left_str, right_str);
                        break;
                    case TOK_NOTEQ:
                        llvm_emit(lg, "  %%%d = icmp ne i64 %s, %s", t, left_str, right_str);
                        break;
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
            }
            *result_reg = t;
            break;
        }
        
        case EXPR_CALL: {
            // Check for module.function() calls
            if (e->call.func->kind == EXPR_FIELD && e->call.func->field.object->kind == EXPR_IDENT) {
                const char* module = e->call.func->field.object->ident;
                const char* function = e->call.func->field.field;
                const char* builtin_name = map_module_function(module, function);
                
                if (builtin_name) {
                    // Map to builtin function call with proper types
                    char args[1024] = "";
                    for (int i = 0; i < e->call.arg_count; i++) {
                        int arg_reg;
                        llvm_expr(lg, e->call.args[i], &arg_reg);
                        
                        char arg_str[128];
                        // Check if argument is a string (use i8* type)
                        bool is_string_arg = (e->call.args[i]->kind == EXPR_STRING) ||
                                            (e->call.args[i]->kind == EXPR_IDENT && 
                                             (strstr(e->call.args[i]->ident, "str") != NULL ||
                                              strstr(e->call.args[i]->ident, "s") != NULL));
                        
                        if (is_string_arg) {
                            snprintf(arg_str, 128, "i8* %%%d", arg_reg);
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
                                          strcmp(function, "int_to_str") == 0);
                    
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
            
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "print") == 0) {
                for (int i = 0; i < e->call.arg_count; i++) {
                    int arg_reg;
                    llvm_expr(lg, e->call.args[i], &arg_reg);
                    
                    int t = llvm_new_temp(lg);  // Capture printf return value
                    if (e->call.args[i]->kind == EXPR_STRING) {
                        llvm_emit(lg, "  %%%d = call i32 (i8*, ...) @printf(i8* %%%d)", t, arg_reg);
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
                // Print returns void, generate zero register for consistency
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
                
                // Check if it's a builtin function
                bool is_string_builtin = (strcmp(func_name, "chr") == 0 ||
                                         strcmp(func_name, "substring") == 0 ||
                                         strcmp(func_name, "str_concat") == 0 ||
                                         strcmp(func_name, "char_at") == 0 ||
                                         strcmp(func_name, "int_to_str") == 0 ||
                                         strcmp(func_name, "str_substr") == 0);
                
                bool is_int_builtin = (strcmp(func_name, "ord") == 0 ||
                                      strcmp(func_name, "str_find") == 0 ||
                                      strcmp(func_name, "str_cmp") == 0 ||
                                      strcmp(func_name, "clock") == 0 ||
                                      strcmp(func_name, "random") == 0 ||
                                      strcmp(func_name, "min") == 0 ||
                                      strcmp(func_name, "max") == 0 ||
                                      strcmp(func_name, "abs") == 0);
                
                // Build arguments
                char args[1024] = "";
                if (strcmp(func_name, "assert") != 0) {
                    // Normal argument processing for non-assert functions
                    for (int i = 0; i < e->call.arg_count; i++) {
                        int arg_reg;
                        llvm_expr(lg, e->call.args[i], &arg_reg);
                        
                        char arg_str[128];
                        // Check if argument should be i8* (string)
                        bool is_string_arg = (e->call.args[i]->kind == EXPR_STRING ||
                                             (e->call.args[i]->kind == EXPR_IDENT &&
                                              llvm_get_var_type(lg, e->call.args[i]->ident) &&
                                              llvm_get_var_type(lg, e->call.args[i]->ident)->kind == TYPE_STR));
                        
                        // Special case: ord() first arg is always i8*, str_find/str_cmp first two args are i8*
                        bool force_string_arg = false;
                        if (strcmp(func_name, "ord") == 0 && i == 0) force_string_arg = true;
                        if ((strcmp(func_name, "str_find") == 0 || strcmp(func_name, "str_cmp") == 0) && i < 2) force_string_arg = true;
                        
                        if (is_string_arg || is_string_builtin || force_string_arg) {
                            if (e->call.args[i]->kind == EXPR_STRING || is_string_arg || force_string_arg) {
                                snprintf(arg_str, 128, "i8* %%%d", arg_reg);
                            } else if (arg_reg <= -1000000) {
                                snprintf(arg_str, 128, "i64 %lld", (long long)(-arg_reg - 1000000));
                            } else {
                                snprintf(arg_str, 128, "i64 %%%d", arg_reg);
                            }
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
                    const char* runtime_name = func_name;
                    if (strcmp(func_name, "clock") == 0) runtime_name = "clock_wyn";
                    if (strcmp(func_name, "random") == 0) runtime_name = "random_wyn";
                    if (strcmp(func_name, "exit") == 0) runtime_name = "exit_wyn";
                    if (strcmp(func_name, "min") == 0) runtime_name = "min_int";
                    if (strcmp(func_name, "max") == 0) runtime_name = "max_int";
                    if (strcmp(func_name, "abs") == 0) runtime_name = "abs_int";
                    
                    if (is_string_builtin) {
                        llvm_emit(lg, "  %%%d = call i8* @%s(%s)", t, runtime_name, args);
                    } else {
                        llvm_emit(lg, "  %%%d = call i64 @%s(%s)", t, runtime_name, args);
                    }
                    *result_reg = t;
                } else {
                    // Special handling for assert - convert i1 arguments to i64
                    char converted_args[1024] = "";
                    for (int i = 0; i < e->call.arg_count; i++) {
                        int arg_reg;
                        llvm_expr(lg, e->call.args[i], &arg_reg);
                        
                        char arg_str[128];
                        if (arg_reg <= -1000000) {
                            snprintf(arg_str, 128, "i64 %lld", (long long)(-arg_reg - 1000000));
                        } else {
                            // Check if this is a comparison result (i1) and convert to i64
                            int conv_reg = llvm_new_temp(lg);
                            llvm_emit(lg, "  %%%d = zext i1 %%%d to i64", conv_reg, arg_reg);
                            snprintf(arg_str, 128, "i64 %%%d", conv_reg);
                        }
                        if (i > 0) strcat(converted_args, ", ");
                        strcat(converted_args, arg_str);
                    }
                    
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64 @%s(%s)", t, func_name, converted_args);
                    *result_reg = t;
                }
            } else if (e->call.func->kind == EXPR_FIELD) {
                // Method call: obj.method()
                const char* method_name = e->call.func->field.field;
                
                if (strcmp(method_name, "len") == 0) {
                    // Handle arr.len() - get array length
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    // Load length from array (first element)
                    int len_ptr = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = getelementptr i64, i64* %%%d, i64 0", len_ptr, obj_reg);
                    
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64, i64* %%%d", t, len_ptr);
                    *result_reg = t;
                } else {
                    // Unknown method - generate zero register
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = add i64 0, 0", t);
                    *result_reg = t;
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
            llvm_emit(lg, "  %%%d = getelementptr i64, i64* %%%d, i64 0", len_ptr, ptr_reg);
            llvm_emit(lg, "  store i64 %d, i64* %%%d", count, len_ptr);
            
            // Store capacity
            int cap_ptr = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = getelementptr i64, i64* %%%d, i64 1", cap_ptr, ptr_reg);
            llvm_emit(lg, "  store i64 %d, i64* %%%d", count, cap_ptr);
            
            // Store elements
            for (int i = 0; i < count; i++) {
                int elem_reg;
                llvm_expr(lg, e->array.elements[i], &elem_reg);
                
                int elem_ptr = llvm_new_temp(lg);
                llvm_emit(lg, "  %%%d = getelementptr i64, i64* %%%d, i64 %d", elem_ptr, ptr_reg, i + 2);
                
                if (elem_reg <= -1000000) {
                    llvm_emit(lg, "  store i64 %lld, i64* %%%d", (long long)(-elem_reg - 1000000), elem_ptr);
                } else {
                    llvm_emit(lg, "  store i64 %%%d, i64* %%%d", elem_reg, elem_ptr);
                }
            }
            
            *result_reg = ptr_reg;
            break;
        }
        
        case EXPR_INDEX: {
            // arr[idx]
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
                    llvm_emit(lg, "  %%%d = getelementptr i64, i64* %%%d, i64 0", len_ptr, arr_reg);
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
                llvm_emit(lg, "  %%%d = getelementptr i64, i64* %%%d, i64 0", len_ptr, arr_reg);
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
            llvm_emit(lg, "  %%%d = getelementptr i64, i64* %%%d, i64 %%%d", elem_ptr, arr_reg, adj_idx);
            
            // Load element
            int t = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = load i64, i64* %%%d", t, elem_ptr);
            *result_reg = t;
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
            // For now, generate a placeholder (full implementation would require runtime support)
            int obj_reg;
            llvm_expr(lg, e->slice.object, &obj_reg);
            
            // Just return the object for now (no actual slicing)
            *result_reg = obj_reg;
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
                               (s->let.value && s->let.value->kind == EXPR_ARRAY);
            
            // Check if this is a string variable
            bool is_string_var = (s->let.type && s->let.type->kind == TYPE_STR) ||
                                (s->let.value && s->let.value->kind == EXPR_STRING);
            
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
            llvm_add_var(lg, s->let.name, alloca_reg, s->let.type);
            
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
                } else if (!is_struct_var) {
                    // Regular variable assignment (only for non-struct variables)
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
                        llvm_emit(lg, "  store i8* %%%d, i8** %%%d", val_reg, alloca_reg);
                    } else {
                        if (val_reg <= -1000000) {
                            llvm_emit(lg, "  store i64 %lld, i64* %%%d", (long long)(-val_reg - 1000000), alloca_reg);
                        } else {
                            llvm_emit(lg, "  store i64 %%%d, i64* %%%d", val_reg, alloca_reg);
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
                            if (is_float_var) {
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
            int dummy;
            llvm_expr(lg, s->expr.expr, &dummy);
            break;
        }
        
        case STMT_RETURN:
            if (s->ret.value) {
                int val_reg;
                llvm_expr(lg, s->ret.value, &val_reg);
                if (val_reg <= -1000000) {
                    llvm_emit(lg, "  ret i64 %lld", (long long)(-val_reg - 1000000));
                } else {
                    llvm_emit(lg, "  ret i64 %%%d", val_reg);
                }
            } else {
                llvm_emit(lg, "  ret i32 0");
            }
            break;
            
        case STMT_IF: {
            int cond_reg;
            llvm_expr(lg, s->if_stmt.cond, &cond_reg);
            
            int then_label = llvm_new_label(lg);
            int else_label = llvm_new_label(lg);
            int end_label = llvm_new_label(lg);
            
            llvm_emit(lg, "  br i1 %%%d, label %%L%d, label %%L%d", cond_reg, then_label, 
                     s->if_stmt.else_count > 0 ? else_label : end_label);
            
            llvm_emit(lg, "L%d:", then_label);
            for (int i = 0; i < s->if_stmt.then_count; i++) {
                llvm_stmt(lg, s->if_stmt.then_block[i]);
            }
            bool then_returns = block_ends_with_return(s->if_stmt.then_block, s->if_stmt.then_count);
            if (!then_returns) {
                llvm_emit(lg, "  br label %%L%d", end_label);
            }
            
            if (s->if_stmt.else_count > 0) {
                llvm_emit(lg, "L%d:", else_label);
                for (int i = 0; i < s->if_stmt.else_count; i++) {
                    llvm_stmt(lg, s->if_stmt.else_block[i]);
                }
                bool else_returns = block_ends_with_return(s->if_stmt.else_block, s->if_stmt.else_count);
                if (!else_returns) {
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
            }
            // TODO: Array iteration
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
                
                if (!is_wildcard) {
                    // Generate pattern comparison
                    int pat_reg;
                    llvm_expr(lg, pat, &pat_reg);
                    
                    int cmp_reg = llvm_new_temp(lg);
                    int arm_label = llvm_new_label(lg);
                    
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
                    
                    llvm_emit(lg, "L%d:", arm_label);
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
                        int addr_t = llvm_new_temp(lg);
                        int cast_t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = getelementptr i8, i8* %%%d, i64 %d", addr_t, ctx_t, i * 8);
                        llvm_emit(lg, "  %%%d = bitcast i8* %%%d to i64**", cast_t, addr_t);
                        llvm_emit(lg, "  store i64* %%%d, i64** %%%d", var_idx, cast_t);
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
            snprintf(param, 128, "i64 %%param.%s", f->params[i].name);
            if (i > 0) strcat(params, ", ");
            strcat(params, param);
        }
        llvm_emit(lg, "define i64 @%s(%s) {", f->name, params);
        llvm_emit(lg, "entry:");
    }
    
    // Allocate and store parameters
    for (int i = 0; i < f->param_count; i++) {
        int alloca_reg = llvm_new_temp(lg);
        llvm_emit(lg, "  %%%d = alloca i64", alloca_reg);
        llvm_emit(lg, "  store i64 %%param.%s, i64* %%%d", f->params[i].name, alloca_reg);
        llvm_add_var(lg, f->params[i].name, alloca_reg, f->params[i].type);
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
        for (int j = 0; j < f->body_count; j++) {
            collect_strings_stmt(&strings, f->body[j]);
        }
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
             os == OS_MACOS ? "macosx" : "linux-gnu");
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
    llvm_emit(&lg, "declare void @__wyn_runtime_init()");
    llvm_emit(&lg, "declare void @__wyn_runtime_shutdown()");
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
    llvm_emit(&lg, "declare i64 @ord(i8*)");
    llvm_emit(&lg, "declare i8* @chr(i64)");
    llvm_emit(&lg, "declare i8* @substring(i8*, i64, i64)");
    llvm_emit(&lg, "declare i64 @str_find(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @str_concat(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @str_cmp(i8*, i8*)");
    llvm_emit(&lg, "declare i8** @str_split(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @char_at(i8*, i64)");
    llvm_emit(&lg, "declare i8* @int_to_str(i64)");
    llvm_emit(&lg, "declare i64 @str_to_int(i8*)");
    llvm_emit(&lg, "declare i64 @clock_wyn()");
    llvm_emit(&lg, "declare i64 @random_wyn()");
    llvm_emit(&lg, "declare void @sleep_ms(i64)");
    llvm_emit(&lg, "declare void @exit_wyn(i64)");
    llvm_emit(&lg, "declare i64 @min_int(i64, i64)");
    llvm_emit(&lg, "declare i64 @max_int(i64, i64)");
    llvm_emit(&lg, "declare i64 @abs_int(i64)");
    llvm_emit(&lg, "");
    
    // Generate functions
    for (int i = 0; i < m->function_count; i++) {
        llvm_function(&lg, &m->functions[i]);
        llvm_emit(&lg, "");
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
                // Add variable to spawn function scope
                strcpy(lg.vars[lg.var_count], s->spawn.captured_vars[j]);
                
                // Load variable address from context
                int addr_t = llvm_new_temp(&lg);
                int cast_t = llvm_new_temp(&lg);
                int var_t = llvm_new_temp(&lg);
                lg.var_regs[lg.var_count] = var_t;
                
                fprintf(lg.out, "  %%%d = getelementptr i8, i8* %%ctx, i64 %d\n", addr_t, j * 8);
                fprintf(lg.out, "  %%%d = bitcast i8* %%%d to i64**\n", cast_t, addr_t);
                fprintf(lg.out, "  %%%d = load i64*, i64** %%%d\n", var_t, cast_t);
                
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
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* input_file = NULL;
    const char* output_file = "a.out";
    bool compile_only = false;
    bool emit_asm = false;
    bool emit_ir = false;
    bool quiet = false;
    int opt_level = 0;
    bool stage1_opt = false;  // Enable Stage 1 optimizations
    bool use_stage1_tc = false;  // Use Stage 1 type checker
    bool no_bounds_check = false;  // Disable all bounds checks
    Arch target_arch = ARCH_ARM64;  // Default for macOS ARM
    TargetOS target_os = OS_MACOS;  // Default OS
    
    // Detect host architecture and OS
#if defined(__x86_64__) || defined(_M_X64)
    target_arch = ARCH_X86_64;
#endif
#if defined(__linux__)
    target_os = OS_LINUX;
#endif
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("wyn 0.1.0\n");
            return 0;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0) {
            compile_only = true;
        } else if (strcmp(argv[i], "-S") == 0) {
            emit_asm = true;
        } else if (strcmp(argv[i], "--emit-ir") == 0) {
            emit_ir = true;
        } else if (strcmp(argv[i], "-O0") == 0) {
            opt_level = 0;
        } else if (strcmp(argv[i], "-O1") == 0) {
            opt_level = 1;
        } else if (strcmp(argv[i], "-O2") == 0) {
            opt_level = 2;
        } else if (strcmp(argv[i], "--stage1") == 0) {
            stage1_opt = true;
        } else if (strcmp(argv[i], "--stage1-opt") == 0) {
            stage1_opt = true;
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            quiet = true;
        } else if (strcmp(argv[i], "--stage1-tc") == 0) {
            use_stage1_tc = true;
        } else if (strcmp(argv[i], "--stage1-opt") == 0) {
            stage1_opt = true;
        } else if (strcmp(argv[i], "--no-bounds-check") == 0) {
            no_bounds_check = true;
        } else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "arm64") == 0) target_arch = ARCH_ARM64;
            else if (strcmp(argv[i], "x86_64") == 0) target_arch = ARCH_X86_64;
            else { fprintf(stderr, "unknown target: %s\n", argv[i]); return 1; }
        } else if (strcmp(argv[i], "--target-os") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "macos") == 0 || strcmp(argv[i], "darwin") == 0) target_os = OS_MACOS;
            else if (strcmp(argv[i], "linux") == 0) target_os = OS_LINUX;
            else { fprintf(stderr, "unknown target OS: %s\n", argv[i]); return 1; }
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        } else {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            return 1;
        }
    }
    
    if (!input_file) {
        fprintf(stderr, "no input file\n");
        return 1;
    }
    
    if (!quiet) {
        printf("Wyn Bootstrap Compiler (Stage 0)\n");
        printf("Compiling: %s\n", input_file);
    }
    
    char* source = read_file(input_file);
    Lexer* lexer = lexer_new(source, input_file);
    
    // Parse the source
    Parser* parser = parser_new(lexer, input_file);
    Module* module = parse_module(parser);
    
    if (parser->had_error) {
        fprintf(stderr, "Parsing failed.\n");
        return 1;
    }
    
    // Resolve imports
    resolve_imports(module, input_file);
    
    // Print AST summary
    if (!quiet) print_module(module);
    
    // Type check
    TypeChecker* tc = NULL;
    if (use_stage1_tc) {
        if (!quiet) printf("Using Stage 1 type checker...\n");
        TypeChecker1* tc1 = tc1_new(module, input_file);
        if (!typecheck_module_stage1(tc1)) {
            fprintf(stderr, "Type checking failed.\n");
            return 1;
        }
        if (!quiet) printf("\nType checking passed.\n");
        
        // Stage 1 optimizations
        if (stage1_opt) {
            if (!quiet) printf("Running Stage 1 optimizations...\n");
            Optimizer* opt = opt_new(module);
            optimize_module(opt);
            optimize_dead_code(module);
            inline_pass(module);
            optimize_bounds_checks(module, no_bounds_check);
        } else if (no_bounds_check) {
            // Apply bounds check elimination even without other optimizations
            optimize_bounds_checks(module, true);
        }
        
        // Create dummy TypeChecker for codegen compatibility
        tc = typechecker_new(module, input_file);
    } else {
        tc = typechecker_new(module, input_file);
        if (!typecheck_module(tc)) {
            fprintf(stderr, "Type checking failed.\n");
            return 1;
        }
        if (!quiet) printf("\nType checking passed.\n");
        
        // Stage 1 optimizations (can run with standard type checker too)
        if (stage1_opt) {
            if (!quiet) printf("Running Stage 1 optimizations...\n");
            Optimizer* opt = opt_new(module);
            optimize_module(opt);
            optimize_dead_code(module);
            inline_pass(module);
            optimize_bounds_checks(module, no_bounds_check);
        } else if (no_bounds_check) {
            // Apply bounds check elimination even without other optimizations
            optimize_bounds_checks(module, true);
        }
    }
    
    // Analyze spawn variable captures
    analyze_spawn_captures(module);
    
    // Code generation - LLVM Backend
    char ir_file[256];
    snprintf(ir_file, 256, "/tmp/wyn_%d.ll", getpid());
    FILE* ir_out = fopen(ir_file, "w");
    if (!ir_out) {
        fprintf(stderr, "Cannot open %s for writing\n", ir_file);
        return 1;
    }
    
    // Generate LLVM IR
    llvm_generate(ir_out, module, target_arch, target_os);
    fclose(ir_out);
    
    if (!quiet) printf("LLVM IR written to: %s\n", ir_file);
    
    // Compile LLVM IR to object file
    char obj_file[256];
    snprintf(obj_file, 256, "/tmp/wyn_%d.o", getpid());
    char cmd[512];
    snprintf(cmd, 512, "/opt/homebrew/opt/llvm/bin/llc -filetype=obj %s -o %s", ir_file, obj_file);
    if (system(cmd) != 0) {
        fprintf(stderr, "LLVM compilation failed\n");
        return 1;
    }
    
    // Link
    if (!compile_only) {
        snprintf(cmd, 512, "clang %s build/builtins_runtime.o build/array_runtime.o build/spawn_runtime.o build/channels_runtime.o -lpthread -o %s", obj_file, output_file);
        if (system(cmd) != 0) {
            fprintf(stderr, "Linking failed\n");
            return 1;
        }
        if (!quiet) printf("Compiled to: %s\n", output_file);
    }
    
    // Cleanup
    // unlink(ir_file);
    if (!compile_only) unlink(obj_file);
    
    parser_free(parser);
    lexer_free(lexer);
    free(source);
    
    return 0;
}
