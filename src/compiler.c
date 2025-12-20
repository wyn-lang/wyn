/*
 * Wyn Language Compiler - LLVM Backend Only
 * 
 * Clean implementation: 8K lines (was 14K)
 * Removed: 6K lines of unused x86/ARM codegen
 */

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

#ifdef _WIN32
  #include <io.h>
  #define access _access
  #define X_OK 0
#else
  #include <unistd.h>
#endif

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
    TOK_LAMBDA,
    
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
    TOK_PIPEGT,
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
    "and", "or", "not", "test", "bench", "assert", "lambda"
};

static const TokenKind keyword_tokens[] = {
    TOK_FN, TOK_STRUCT, TOK_ENUM, TOK_INTERFACE, TOK_IMPL, TOK_LET, TOK_MUT, TOK_CONST, TOK_PUB,
    TOK_IF, TOK_ELSE, TOK_THEN, TOK_MATCH, TOK_FOR, TOK_WHILE, TOK_IN,
    TOK_BREAK, TOK_CONTINUE, TOK_RETURN, TOK_IMPORT, TOK_FROM, TOK_AS,
    TOK_TRUE, TOK_FALSE, TOK_NONE, TOK_SOME, TOK_OK, TOK_ERR, TOK_SELF, TOK_SELF_TYPE,
    TOK_TRY, TOK_CATCH, TOK_DEFER, TOK_SPAWN, TOK_PARALLEL, TOK_ASYNC, TOK_AWAIT, TOK_YIELD,
    TOK_AND, TOK_OR, TOK_NOT, TOK_TEST, TOK_BENCH, TOK_ASSERT, TOK_LAMBDA
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
        case '|': 
            lexer_advance(l); 
            if (lexer_char(l) == '>') { 
                lexer_advance(l); 
                t.kind = TOK_PIPEGT; 
            } else { 
                t.kind = TOK_PIPE; 
            }
            break;
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
    EXPR_UNWRAP, EXPR_TRY, EXPR_DEFAULT, EXPR_SLICE, EXPR_ADDR, EXPR_DEREF, EXPR_AWAIT, EXPR_TERNARY, EXPR_COMPREHENSION
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
        struct { Expr** elements; int count; bool is_empty_literal; } array;
        struct { char** keys; Expr** values; int count; } map;
        struct { char name[MAX_IDENT_LEN]; char fields[MAX_FIELDS][MAX_IDENT_LEN]; Expr* values[MAX_FIELDS]; int count; Type** type_args; int type_arg_count; } struct_lit;
        struct { Expr* cond; Expr* then_expr; Expr* else_expr; } if_expr;
        struct { Expr* value; Expr** patterns; Expr** arms; int arm_count; char bindings[64][MAX_IDENT_LEN]; Type** binding_types; } match_expr;
        struct { Expr* value; } some;  // also used for ok/err
        struct { Expr* value; Expr* default_val; } default_expr;
        struct { Expr* object; Expr* start; Expr* end; Expr* step; } slice;
        struct { char params[8][MAX_IDENT_LEN]; Type* param_types[8]; int param_count; Type* return_type; Stmt** body; int body_count; int id; } lambda;
        struct { Expr* future; } await_expr;
        struct { Expr* cond; Expr* true_val; Expr* false_val; } ternary;
        struct { Expr* expr; char var[MAX_IDENT_LEN]; Expr* iter; Expr* condition; } comprehension;
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
        struct { Stmt** body; int body_count; char** captured_vars; int captured_count; Type** captured_types; } spawn;
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
    bool is_extension;
    char extension_type[MAX_IDENT_LEN];
    char extension_method[MAX_IDENT_LEN];
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
        if (strcmp(function, "upper") == 0) return "str_upper";
        if (strcmp(function, "lower") == 0) return "str_lower";
        if (strcmp(function, "trim") == 0) return "str_trim";
    } else if (strcmp(module, "compress") == 0) {
        if (strcmp(function, "gzip_file") == 0) return "gzip_file";
        if (strcmp(function, "gunzip_file") == 0) return "gunzip_file";
        if (strcmp(function, "tar_create") == 0) return "tar_create";
        if (strcmp(function, "tar_extract") == 0) return "tar_extract";
        if (strcmp(function, "tar_list") == 0) return "tar_list";
        if (strcmp(function, "compress_string") == 0) return "compress_string";
        if (strcmp(function, "decompress_string") == 0) return "decompress_string";
    } else if (strcmp(module, "regex") == 0) {
        if (strcmp(function, "matches") == 0) return "regex_matches";
        if (strcmp(function, "find") == 0) return "regex_find";
        if (strcmp(function, "replace") == 0) return "regex_replace";
        if (strcmp(function, "count") == 0) return "regex_count";
    } else if (strcmp(module, "time") == 0) {
        if (strcmp(function, "now_unix") == 0) return "now_unix";
        if (strcmp(function, "now_millis") == 0) return "now_millis";
        if (strcmp(function, "now_micros") == 0) return "now_micros";
        if (strcmp(function, "sleep") == 0) return "sleep_seconds";
        if (strcmp(function, "format_timestamp") == 0) return "format_timestamp";
        if (strcmp(function, "format_iso8601") == 0) return "format_iso8601";
        if (strcmp(function, "parse_timestamp") == 0) return "parse_timestamp";
        // Legacy mappings
        if (strcmp(function, "sleep_ms") == 0) return "sleep_ms";
        if (strcmp(function, "now") == 0) return "time_now";
        if (strcmp(function, "clock") == 0) return "clock";
    } else if (strcmp(module, "os") == 0) {
        if (strcmp(function, "getenv") == 0) return "getenv_wyn";
        if (strcmp(function, "setenv") == 0) return "setenv_wyn";
        if (strcmp(function, "exec") == 0) return "exec_wyn";
        if (strcmp(function, "exec_output") == 0) return "exec_output_wyn";
        if (strcmp(function, "exec_args") == 0) return "exec_args_wyn";
        if (strcmp(function, "exit") == 0) return "exit_wyn";
        if (strcmp(function, "args") == 0) return "args_wyn";
        if (strcmp(function, "cwd") == 0) return "cwd_wyn";
        if (strcmp(function, "chdir") == 0) return "chdir_wyn";
        if (strcmp(function, "hostname") == 0) return "hostname_wyn";
        if (strcmp(function, "getpid") == 0) return "getpid_wyn";
        // Command builder
        if (strcmp(function, "command_start") == 0) return "command_start";
        if (strcmp(function, "command_arg") == 0) return "command_arg";
        if (strcmp(function, "command_run") == 0) return "command_run";
        if (strcmp(function, "command_output") == 0) return "command_output";
        // New chaining API
        if (strcmp(function, "command") == 0) return "command_new";
        if (strcmp(function, "arg") == 0) return "command_arg_chain";
        if (strcmp(function, "run") == 0) return "command_run_chain";
        if (strcmp(function, "output") == 0) return "command_output_chain";
        // Legacy mappings
        if (strcmp(function, "getpid") == 0) return "getpid";
        if (strcmp(function, "getcwd") == 0) return "getcwd";
        if (strcmp(function, "system") == 0) return "system";
    } else if (strcmp(module, "json") == 0) {
        if (strcmp(function, "parse") == 0) return "parse_json";
        if (strcmp(function, "stringify") == 0) return "stringify_json";
        if (strcmp(function, "get_string") == 0) return "get_string_json";
        if (strcmp(function, "get_int") == 0) return "get_int_json";
        if (strcmp(function, "get_bool") == 0) return "get_bool_json";
    } else if (strcmp(module, "config") == 0) {
        if (strcmp(function, "yaml_parse") == 0) return "yaml_parse";
        if (strcmp(function, "toml_parse") == 0) return "toml_parse";
    } else if (strcmp(module, "net") == 0) {
        if (strcmp(function, "http_get") == 0) return "http_get_wyn";
        if (strcmp(function, "http_post") == 0) return "http_post_wyn";
        if (strcmp(function, "tcp_connect") == 0) return "tcp_connect_wyn";
        if (strcmp(function, "tcp_send") == 0) return "tcp_send_wyn";
        if (strcmp(function, "tcp_recv") == 0) return "tcp_recv_wyn";
        if (strcmp(function, "tcp_close") == 0) return "tcp_close_wyn";
        if (strcmp(function, "http_put") == 0) return "http_put_wyn";
        if (strcmp(function, "http_delete") == 0) return "http_delete_wyn";
        if (strcmp(function, "http_head") == 0) return "http_head_wyn";
        if (strcmp(function, "http_get_headers") == 0) return "http_get_headers_wyn";
        if (strcmp(function, "udp_send") == 0) return "udp_send_wyn";
        if (strcmp(function, "udp_recv") == 0) return "udp_recv_wyn";
        if (strcmp(function, "resolve_host") == 0) return "resolve_host_wyn";
        if (strcmp(function, "get_local_ip") == 0) return "get_local_ip_wyn";
        if (strcmp(function, "server_create") == 0) return "http_server_create";
        if (strcmp(function, "server_accept") == 0) return "http_server_accept";
        if (strcmp(function, "server_read_request") == 0) return "http_server_read_request";
        if (strcmp(function, "parse_method") == 0) return "http_parse_method";
        if (strcmp(function, "parse_path") == 0) return "http_parse_path";
        if (strcmp(function, "parse_body") == 0) return "http_parse_body";
        if (strcmp(function, "server_send_response") == 0) return "http_server_send_response";
        if (strcmp(function, "server_send_json") == 0) return "http_server_send_json";
        if (strcmp(function, "server_close_client") == 0) return "http_server_close_client";
        if (strcmp(function, "server_close") == 0) return "http_server_close";
        if (strcmp(function, "args") == 0) return "args";
        if (strcmp(function, "args") == 0) return "args";
    } else if (strcmp(module, "io") == 0) {
        // Console I/O only
        if (strcmp(function, "print") == 0) return "print";
        if (strcmp(function, "println") == 0) return "println";
        if (strcmp(function, "print_str") == 0) return "print_str";
        if (strcmp(function, "print_int") == 0) return "print_int";
        if (strcmp(function, "print_float") == 0) return "print_float";
        if (strcmp(function, "print_newline") == 0) return "print_newline";
        if (strcmp(function, "read_line") == 0) return "read_line";
        if (strcmp(function, "input") == 0) return "input";
    } else if (strcmp(module, "fs") == 0) {
        // File operations
        if (strcmp(function, "read") == 0) return "read_file";
        if (strcmp(function, "write") == 0) return "write_file";
        if (strcmp(function, "append") == 0) return "append_file";
        if (strcmp(function, "delete") == 0) return "delete_file";
        if (strcmp(function, "exists") == 0) return "file_exists";
        if (strcmp(function, "size") == 0) return "file_size";
        if (strcmp(function, "copy") == 0) return "file_copy";
        if (strcmp(function, "move") == 0) return "file_move";
        if (strcmp(function, "chmod") == 0) return "file_chmod";
        if (strcmp(function, "glob") == 0) return "file_glob";
        // Directory operations
        if (strcmp(function, "mkdir") == 0) return "mkdir_wyn";
        if (strcmp(function, "rmdir") == 0) return "rmdir_wyn";
        if (strcmp(function, "list_dir") == 0) return "list_dir";
        if (strcmp(function, "is_dir") == 0) return "is_dir_wyn";
        if (strcmp(function, "is_file") == 0) return "is_file_wyn";
        // Legacy names for compatibility
        if (strcmp(function, "read_file") == 0) return "read_file";
        if (strcmp(function, "write_file") == 0) return "write_file";
        if (strcmp(function, "append_file") == 0) return "append_file";
        if (strcmp(function, "delete_file") == 0) return "delete_file";
        if (strcmp(function, "file_exists") == 0) return "file_exists";
        if (strcmp(function, "file_size") == 0) return "file_size";
    } else if (strcmp(module, "fs_old") == 0) {
        if (strcmp(function, "read_file") == 0) return "read_file";
        if (strcmp(function, "delete") == 0) return "delete_file";
        if (strcmp(function, "size") == 0) return "file_size";
        if (strcmp(function, "mkdir") == 0) return "mkdir_wyn";
        if (strcmp(function, "rmdir") == 0) return "rmdir_wyn";
        if (strcmp(function, "is_dir") == 0) return "is_dir_wyn";
        if (strcmp(function, "is_file") == 0) return "is_file_wyn";
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
    } else if (strcmp(module, "os") == 0) {
        if (strcmp(function, "getenv") == 0) return "getenv_wyn";
        if (strcmp(function, "setenv") == 0) return "setenv_wyn";
        if (strcmp(function, "exec") == 0) return "exec_wyn";
        if (strcmp(function, "exit") == 0) return "exit_wyn";
        if (strcmp(function, "args") == 0) return "args_wyn";
        if (strcmp(function, "cwd") == 0) return "cwd_wyn";
        if (strcmp(function, "chdir") == 0) return "chdir_wyn";
        if (strcmp(function, "hostname") == 0) return "hostname_wyn";
        if (strcmp(function, "getpid") == 0) return "getpid_wyn";
    } else if (strcmp(module, "json") == 0) {
        if (strcmp(function, "parse") == 0) return "parse_json";
        if (strcmp(function, "stringify") == 0) return "stringify_json";
        if (strcmp(function, "get_string") == 0) return "get_string_json";
        if (strcmp(function, "get_int") == 0) return "get_int_json";
        if (strcmp(function, "get_bool") == 0) return "get_bool_json";
    } else if (strcmp(module, "config") == 0) {
        if (strcmp(function, "yaml_parse") == 0) return "yaml_parse";
        if (strcmp(function, "toml_parse") == 0) return "toml_parse";
    } else if (strcmp(module, "net") == 0) {
        if (strcmp(function, "http_get") == 0) return "http_get_wyn";
        if (strcmp(function, "http_post") == 0) return "http_post_wyn";
        if (strcmp(function, "tcp_connect") == 0) return "tcp_connect_wyn";
        if (strcmp(function, "tcp_send") == 0) return "tcp_send_wyn";
        if (strcmp(function, "tcp_recv") == 0) return "tcp_recv_wyn";
        if (strcmp(function, "tcp_close") == 0) return "tcp_close_wyn";
        if (strcmp(function, "http_put") == 0) return "http_put_wyn";
        if (strcmp(function, "http_delete") == 0) return "http_delete_wyn";
        if (strcmp(function, "http_head") == 0) return "http_head_wyn";
        if (strcmp(function, "http_get_headers") == 0) return "http_get_headers_wyn";
        if (strcmp(function, "udp_send") == 0) return "udp_send_wyn";
        if (strcmp(function, "udp_recv") == 0) return "udp_recv_wyn";
        if (strcmp(function, "resolve_host") == 0) return "resolve_host_wyn";
        if (strcmp(function, "get_local_ip") == 0) return "get_local_ip_wyn";
        if (strcmp(function, "server_create") == 0) return "http_server_create";
        if (strcmp(function, "server_accept") == 0) return "http_server_accept";
        if (strcmp(function, "server_read_request") == 0) return "http_server_read_request";
        if (strcmp(function, "parse_method") == 0) return "http_parse_method";
        if (strcmp(function, "parse_path") == 0) return "http_parse_path";
        if (strcmp(function, "parse_body") == 0) return "http_parse_body";
        if (strcmp(function, "server_send_response") == 0) return "http_server_send_response";
        if (strcmp(function, "server_send_json") == 0) return "http_server_send_json";
        if (strcmp(function, "server_close_client") == 0) return "http_server_close_client";
        if (strcmp(function, "server_close") == 0) return "http_server_close";
    } else if (strcmp(module, "log") == 0) {
        if (strcmp(function, "info") == 0) return "log_info";
        if (strcmp(function, "warn") == 0) return "log_warn";
        if (strcmp(function, "error") == 0) return "log_error";
        if (strcmp(function, "debug") == 0) return "log_debug";
        if (strcmp(function, "log_with_level") == 0) return "log_with_level";
    } else if (strcmp(module, "encoding") == 0) {
        if (strcmp(function, "base64_encode") == 0) return "base64_encode";
        if (strcmp(function, "base64_decode") == 0) return "base64_decode";
        if (strcmp(function, "hex_encode") == 0) return "hex_encode";
        if (strcmp(function, "hex_decode") == 0) return "hex_decode";
        if (strcmp(function, "url_encode") == 0) return "url_encode";
        if (strcmp(function, "url_decode") == 0) return "url_decode";
    } else if (strcmp(module, "hash") == 0) {
        if (strcmp(function, "sha256") == 0) return "sha256_hash";
        if (strcmp(function, "md5") == 0) return "md5_hash";
        if (strcmp(function, "sha1") == 0) return "sha1_hash";
    } else if (strcmp(module, "collections") == 0) {
        if (strcmp(function, "hashmap_new") == 0) return "hashmap_new";
        if (strcmp(function, "hashmap_put") == 0) return "hashmap_put";
        if (strcmp(function, "hashmap_get") == 0) return "hashmap_get";
        if (strcmp(function, "hashmap_contains") == 0) return "hashmap_contains";
        if (strcmp(function, "hashmap_remove") == 0) return "hashmap_remove";
        if (strcmp(function, "hashmap_size") == 0) return "hashmap_size";
        if (strcmp(function, "hashmap_clear") == 0) return "hashmap_clear";
        if (strcmp(function, "set_new") == 0) return "set_new";
        if (strcmp(function, "set_add") == 0) return "set_add";
        if (strcmp(function, "set_contains") == 0) return "set_contains";
        if (strcmp(function, "set_remove") == 0) return "set_remove";
        if (strcmp(function, "set_size") == 0) return "set_size";
        if (strcmp(function, "set_clear") == 0) return "set_clear";
    } else if (strcmp(module, "crypto") == 0) {
        if (strcmp(function, "encrypt_aes256") == 0) return "encrypt_aes256";
        if (strcmp(function, "decrypt_aes256") == 0) return "decrypt_aes256";
        if (strcmp(function, "generate_random_bytes") == 0) return "generate_random_bytes";
        if (strcmp(function, "hmac_sha256") == 0) return "hmac_sha256";
        if (strcmp(function, "verify_signature") == 0) return "verify_signature";
    } else if (strcmp(module, "cli") == 0) {
        if (strcmp(function, "has_flag") == 0) return "cli_has_flag";
        if (strcmp(function, "get_arg") == 0) return "cli_get_arg";
        if (strcmp(function, "get_positional") == 0) return "cli_get_positional";
        if (strcmp(function, "arg_count") == 0) return "cli_arg_count";
    } else if (strcmp(module, "array") == 0) {
        if (strcmp(function, "reverse") == 0) return "array_reverse";
        if (strcmp(function, "append") == 0) return "array_append_elem";
        if (strcmp(function, "prepend") == 0) return "array_prepend_elem";
        if (strcmp(function, "contains") == 0) return "array_contains_elem";
        if (strcmp(function, "filter") == 0) return "array_filter";
        if (strcmp(function, "map") == 0) return "array_map";
        if (strcmp(function, "reduce") == 0) return "array_reduce";
    } else if (strcmp(module, "database") == 0) {
        if (strcmp(function, "sqlite_open") == 0) return "sqlite_open";
        if (strcmp(function, "sqlite_exec") == 0) return "sqlite_exec";
        if (strcmp(function, "sqlite_query") == 0) return "sqlite_query";
        if (strcmp(function, "sqlite_close") == 0) return "sqlite_close";
    } else if (strcmp(module, "http") == 0) {
        if (strcmp(function, "router_new") == 0) return "http_router_new";
        if (strcmp(function, "router_add") == 0) return "http_router_add";
        if (strcmp(function, "router_match") == 0) return "http_router_match";
        if (strcmp(function, "response_json") == 0) return "http_response_json";
        if (strcmp(function, "response_html") == 0) return "http_response_html";
        if (strcmp(function, "response_text") == 0) return "http_response_text";
    } else if (strcmp(module, "testing") == 0) {
        if (strcmp(function, "assert_eq") == 0) return "test_assert_eq";
        if (strcmp(function, "assert_ne") == 0) return "test_assert_ne";
        if (strcmp(function, "assert_true") == 0) return "test_assert_true";
        if (strcmp(function, "assert_false") == 0) return "test_assert_false";
        if (strcmp(function, "summary") == 0) return "test_summary";
    } else if (strcmp(module, "result") == 0) {
        if (strcmp(function, "make_ok") == 0) return "result_ok";
        if (strcmp(function, "make_err") == 0) return "result_err";
        if (strcmp(function, "is_ok") == 0) return "result_is_ok";
        if (strcmp(function, "is_err") == 0) return "result_is_err";
        if (strcmp(function, "unwrap") == 0) return "result_unwrap";
        if (strcmp(function, "make_some") == 0) return "option_some";
        if (strcmp(function, "make_none") == 0) return "option_none";
        if (strcmp(function, "is_some") == 0) return "option_is_some";
        if (strcmp(function, "is_none") == 0) return "option_is_none";
        if (strcmp(function, "unwrap_option") == 0) return "option_unwrap";
        if (strcmp(function, "generate_random_bytes") == 0) return "generate_random_bytes";
        if (strcmp(function, "hmac_sha256") == 0) return "hmac_sha256";
        if (strcmp(function, "verify_signature") == 0) return "verify_signature";
    } else if (strcmp(module, "atomic") == 0) {
        if (strcmp(function, "add") == 0) return "wyn_atomic_add";
        if (strcmp(function, "sub") == 0) return "wyn_atomic_sub";
        if (strcmp(function, "load") == 0) return "wyn_atomic_load";
        if (strcmp(function, "store") == 0) return "wyn_atomic_store";
        if (strcmp(function, "cas") == 0) return "wyn_atomic_cas";
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
    fprintf(stderr, "\n");
    fprintf(stderr, "╭─ Parse Error in %s:%d:%d\n", p->filename, p->current.line, p->current.column);
    fprintf(stderr, "│\n");
    fprintf(stderr, "│  ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fprintf(stderr, "│\n");
    fprintf(stderr, "╰─ Syntax error\n");
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
    
    // Add helpful suggestions
    if (kind == TOK_LBRACE) {
        fprintf(stderr, "   💡 Missing opening brace '{' - check if/while/for/fn syntax\n");
    } else if (kind == TOK_RBRACE) {
        fprintf(stderr, "   💡 Missing closing brace '}' - check block structure\n");
    } else if (kind == TOK_LPAREN) {
        fprintf(stderr, "   💡 Missing opening parenthesis '(' - check function call\n");
    } else if (kind == TOK_RPAREN) {
        fprintf(stderr, "   💡 Missing closing parenthesis ')' - check expression\n");
    } else if (kind == TOK_COLON) {
        fprintf(stderr, "   💡 Missing colon ':' - check type annotation\n");
    }
    
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

// Helper: Get return type for builtin functions
static Type* get_builtin_return_type(const char* builtin_name) {
    // Array-returning functions
    if (strcmp(builtin_name, "str_split") == 0) {
        Type* arr = new_type(TYPE_ARRAY);
        arr->inner = new_type(TYPE_STR);
        return arr;
    }
    
    // String-returning functions
    if (strcmp(builtin_name, "format_timestamp") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "format_iso8601") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "base64_encode") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "base64_decode") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "hex_encode") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "hex_decode") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "url_encode") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "url_decode") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "sha256_hash") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "md5_hash") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "sha1_hash") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "hashmap_new") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "hashmap_get") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "set_new") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "encrypt_aes256") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "decrypt_aes256") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "generate_random_bytes") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "hmac_sha256") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "http_server_read_request") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "http_parse_method") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "http_parse_path") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "http_parse_body") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "exec_output_wyn") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "str_replace") == 0) return new_type(TYPE_STR);
    
    // Integer-returning functions
    if (strcmp(builtin_name, "now_unix") == 0) return new_type(TYPE_INT);
    if (strcmp(builtin_name, "now_millis") == 0) return new_type(TYPE_INT);
    if (strcmp(builtin_name, "now_micros") == 0) return new_type(TYPE_INT);
    if (strcmp(builtin_name, "parse_timestamp") == 0) return new_type(TYPE_INT);
    
    // Void-returning functions
    if (strcmp(builtin_name, "log_info") == 0) return new_type(TYPE_VOID);
    if (strcmp(builtin_name, "log_warn") == 0) return new_type(TYPE_VOID);
    if (strcmp(builtin_name, "log_error") == 0) return new_type(TYPE_VOID);
    if (strcmp(builtin_name, "log_debug") == 0) return new_type(TYPE_VOID);
    if (strcmp(builtin_name, "log_with_level") == 0) return new_type(TYPE_VOID);
    if (strcmp(builtin_name, "sleep_seconds") == 0) return new_type(TYPE_VOID);
    
    // Default to ANY for unknown functions
    return new_type(TYPE_ANY);
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

// Helper to check if string has interpolation ${expr}
static bool has_interpolation(const char* s) {
    for (int i = 0; s[i]; i++) {
        if (s[i] == '$' && s[i+1] == '{') return true;
    }
    return false;
}

// Parse interpolated string into concatenation expression
static Expr* parse_interpolated_string(Parser* p, const char* str, int line, int col) {
    Expr* result = NULL;
    char buf[MAX_STRING_LEN];
    int bi = 0;
    
    for (int i = 0; str[i]; i++) {
        if (str[i] == '$' && str[i+1] == '{') {
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
            // Skip ${
            i += 2;
            // Extract variable name
            char var[256];
            int vi = 0;
            while (str[i] && str[i] != '}') {
                var[vi++] = str[i++];
            }
            var[vi] = '\0';
            
            // Create var.to_str() call for type conversion
            Expr* varexpr = new_expr(EXPR_IDENT, line, col);
            strcpy(varexpr->ident, var);
            
            Expr* field = new_expr(EXPR_FIELD, line, col);
            field->field.object = varexpr;
            strcpy(field->field.field, "to_str");
            
            Expr* call = new_expr(EXPR_CALL, line, col);
            call->call.func = field;
            call->call.args = NULL;
            call->call.arg_count = 0;
            
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
        TokenKind str_kind = p->current.kind;
        parser_advance(p);
        if (str_kind == TOK_STRING && has_interpolation(str)) {
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
    
    // Array literal or list comprehension
    if (parser_match(p, TOK_LBRACKET)) {
        // Check for empty array: []
        if (parser_check(p, TOK_RBRACKET)) {
            parser_advance(p);  // consume ]
            Expr* e = new_expr(EXPR_ARRAY, line, col);
            e->array.elements = malloc(sizeof(Expr*) * 256);
            e->array.count = 0;
            e->array.is_empty_literal = true;  // Mark as empty for type inference
            return e;
        }
        
        // Check if this is a list comprehension: [expr for var in iter]
        // We need to peek ahead to see if there's a 'for' keyword
        int saved_pos = p->lexer->pos;
        int saved_line = p->lexer->line;
        int saved_col = p->lexer->column;
        
        // Try to parse first element
        Expr* first_expr = parse_expr(p);
        
        // Check if 'for' follows
        if (parser_check(p, TOK_FOR)) {
            // This is a list comprehension
            parser_advance(p);  // consume 'for'
            
            Expr* comp = new_expr(EXPR_COMPREHENSION, line, col);
            comp->comprehension.expr = first_expr;
            
            Token var = parser_expect(p, TOK_IDENT, "variable name");
            strcpy(comp->comprehension.var, var.ident);
            
            parser_expect(p, TOK_IN, "in");
            comp->comprehension.iter = parse_expr(p);
            
            // Optional 'if' condition
            if (parser_match(p, TOK_IF)) {
                comp->comprehension.condition = parse_expr(p);
            } else {
                comp->comprehension.condition = NULL;
            }
            
            parser_expect(p, TOK_RBRACKET, "]");
            return comp;
        } else {
            // Regular array literal
            Expr* e = new_expr(EXPR_ARRAY, line, col);
            e->array.elements = malloc(sizeof(Expr*) * 256);
            e->array.count = 0;
            e->array.is_empty_literal = false;
            e->array.elements[e->array.count++] = first_expr;
            
            while (parser_match(p, TOK_COMMA)) {
                if (parser_check(p, TOK_RBRACKET)) break;
                e->array.elements[e->array.count++] = parse_expr(p);
            }
            parser_expect(p, TOK_RBRACKET, "]");
            return e;
        }
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
    
    // Python-style Lambda: lambda x, y: expr or lambda: expr
    if (parser_match(p, TOK_LAMBDA)) {
        Expr* e = new_expr(EXPR_LAMBDA, line, col);
        e->lambda.param_count = 0;
        e->lambda.return_type = NULL;
        e->lambda.id = p->lambda_count++;
        
        // Parse parameters (no parentheses, no types)
        if (!parser_check(p, TOK_COLON)) {
            do {
                Token name = parser_expect(p, TOK_IDENT, "parameter name");
                strcpy(e->lambda.params[e->lambda.param_count], name.ident);
                e->lambda.param_types[e->lambda.param_count] = NULL; // Type inference
                e->lambda.param_count++;
            } while (parser_match(p, TOK_COMMA));
        }
        
        parser_expect(p, TOK_COLON, ":");
        
        // Parse single expression body
        Expr* body_expr = parse_expr(p);
        e->lambda.body = malloc(sizeof(Stmt*) * 1);
        e->lambda.body_count = 1;
        Stmt* ret_stmt = new_stmt(STMT_RETURN, line, col);
        ret_stmt->ret.value = body_expr;
        e->lambda.body[0] = ret_stmt;
        
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
        
        // Try: expr? (only if not followed by expression - that would be ternary)
        if (parser_check(p, TOK_QUESTION)) {
            // Peek ahead - if next token is an expression start, this is ternary, not try
            Token next = lexer_peek(p->lexer);
            bool is_expr_start = (next.kind == TOK_INT || next.kind == TOK_STRING || 
                                 next.kind == TOK_IDENT || next.kind == TOK_LPAREN ||
                                 next.kind == TOK_LBRACKET || next.kind == TOK_TRUE ||
                                 next.kind == TOK_FALSE || next.kind == TOK_MINUS ||
                                 next.kind == TOK_NOT);
            
            if (!is_expr_start) {
                // This is try operator
                parser_advance(p);
                Expr* try_e = new_expr(EXPR_TRY, line, col);
                try_e->unary.operand = e;
                try_e->unary.op = TOK_QUESTION;
                e = try_e;
                continue;
            }
            // Otherwise, leave it for ternary parsing in parse_expr
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
        case TOK_PIPEGT: return 0;      // pipe operator (lowest precedence)
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
        
        if (op == TOK_PIPEGT) {
            // Transform x |> f into f(x)
            Expr* call = new_expr(EXPR_CALL, line, col);
            call->call.func = right;
            call->call.args = malloc(sizeof(Expr*));
            call->call.args[0] = left;
            call->call.arg_count = 1;
            left = call;
        } else if (op == TOK_QUESTIONQUESTION) {
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
    Expr* expr = parse_binary(p, 0);
    
    // Check for ternary operator: condition ? true_val : false_val
    if (parser_check(p, TOK_QUESTION)) {
        parser_advance(p);  // consume ?
        int line = p->current.line, col = p->current.column;
        
        // Parse true value
        Expr* true_val = parse_unary(p);
        
        if (!parser_check(p, TOK_COLON)) {
            parser_error(p, "expected ':' in ternary expression");
        }
        parser_advance(p);  // consume :
        
        // Parse false value (can be another ternary)
        Expr* false_val = parse_expr(p);
        
        Expr* ternary = new_expr(EXPR_TERNARY, line, col);
        ternary->ternary.cond = expr;
        ternary->ternary.true_val = true_val;
        ternary->ternary.false_val = false_val;
        return ternary;
    }
    
    return expr;
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
    
    // Check for extension method: fn TypeName.method_name
    if (parser_match(p, TOK_DOT)) {
        // This is an extension method
        char type_name[MAX_IDENT_LEN];
        strcpy(type_name, name.ident);
        
        Token method_name = parser_expect(p, TOK_IDENT, "method name");
        // Store as "TypeName__method_name" internally
        snprintf(fn->name, MAX_IDENT_LEN, "%s__%s", type_name, method_name.ident);
        fn->is_extension = true;
        strcpy(fn->extension_type, type_name);
        strcpy(fn->extension_method, method_name.ident);
    } else {
        strcpy(fn->name, name.ident);
    }
    
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
                if (strcmp(method_name, "reverse") == 0) return new_type(TYPE_ARRAY);
                if (strcmp(method_name, "append") == 0) return new_type(TYPE_ARRAY);
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
                if (strcmp(method_name, "reverse") == 0) return new_type(TYPE_ARRAY);
                if (strcmp(method_name, "append") == 0) return new_type(TYPE_ARRAY);
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
        
        // Try ~/.wyn/packages/ for installed packages
        if (!f) {
            const char* home = getenv("HOME");
            if (home) {
                snprintf(full_path, 512, "%s/.wyn/packages/%s/src/%s.wyn", home, imp->path, imp->path);
                f = fopen(full_path, "rb");
                
                // Also try without /src/
                if (!f) {
                    snprintf(full_path, 512, "%s/.wyn/packages/%s/%s.wyn", home, imp->path, imp->path);
                    f = fopen(full_path, "rb");
                }
            }
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
        // Check if this is indexing into a string array
        Type* obj_type = llvm_get_var_type(lg, e->index.object->ident);
        return obj_type && obj_type->kind == TYPE_ARRAY && obj_type->inner && obj_type->inner->kind == TYPE_STR;
    }
    if (e->kind == EXPR_BINARY && e->binary.op == TOK_PLUS) {
        return llvm_is_string_expr(lg, e->binary.left) || llvm_is_string_expr(lg, e->binary.right);
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
            // Check if this is a string-returning method (regardless of object)
            if (strcmp(func, "upper") == 0 || strcmp(func, "lower") == 0 ||
                strcmp(func, "trim") == 0 || strcmp(func, "read") == 0 ||
                strcmp(func, "sqlite_query") == 0 || strcmp(func, "exec_output") == 0 ||
                strcmp(func, "http_get") == 0 || strcmp(func, "server_read_request") == 0 ||
                strcmp(func, "parse_method") == 0 || strcmp(func, "parse_path") == 0 ||
                strcmp(func, "parse_body") == 0 || strcmp(func, "output") == 0 ||
                strcmp(func, "replace") == 0 || strcmp(func, "to_str") == 0 ||
                strcmp(func, "join") == 0) {
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
                // Could be: module.function() or result.method()
                const char* function = e->call.func->field.field;
                const char* builtin_name = NULL;
                
                // Check if it's a simple module.function call
                if (e->call.func->field.object->kind == EXPR_IDENT) {
                    const char* module = e->call.func->field.object->ident;
                    builtin_name = map_module_function(module, function);
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
                                          strcmp(function, "output") == 0);
                    
                    bool returns_array = (strcmp(function, "split") == 0 ||
                                         strcmp(function, "glob") == 0 ||
                                         strcmp(function, "reverse") == 0 ||
                                         strcmp(function, "append") == 0 ||
                                         strcmp(function, "prepend") == 0 ||
                                         strcmp(function, "make_ok") == 0 ||
                                         strcmp(function, "make_err") == 0 ||
                                         strcmp(function, "make_some") == 0 ||
                                         strcmp(function, "make_none") == 0);
                    
                    int t = llvm_new_temp(lg);
                    if (returns_string) {
                        llvm_emit(lg, "  %%%d = call i8* @%s(%s)", t, builtin_name, args);
                    } else if (returns_array) {
                        llvm_emit(lg, "  %%%d = call i64* @%s(%s)", t, builtin_name, args);
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
                    
                    if (is_string_builtin) {
                        int t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = call i8* @%s(%s)", t, runtime_name, args);
                        *result_reg = t;
                    } else if (is_array_builtin) {
                        int t = llvm_new_temp(lg);
                        llvm_emit(lg, "  %%%d = call i64* @%s(%s)", t, runtime_name, args);
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
                            
                            llvm_emit(lg, "  %%%d = call %s @%s(%s)", t, ret_type, runtime_name, args);
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
                        
                        llvm_emit(lg, "  %%%d = call %s @%s(%s)", t, ret_type, func_name, converted_args);
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
                        llvm_emit(lg, "  %%%d = getelementptr i64, i64* %%%d, i64 0", len_ptr, obj_reg);
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
                    
                    // Get the argument
                    int arg_reg;
                    if (e->call.arg_count > 0) {
                        llvm_expr(lg, e->call.args[0], &arg_reg);
                    }
                    
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i64 @str_contains(i8* %%%d, i8* %%%d)", t, obj_reg, arg_reg);
                    *result_reg = t;
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
                    llvm_emit(lg, "  %%%d = getelementptr i64, i64* %%%d, i64 %%%d", elem_ptr, obj_reg, offset_reg);
                    
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = load i64, i64* %%%d", t, elem_ptr);
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
                    // Handle arr.append(item)
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int item_reg;
                    if (e->call.arg_count > 0) {
                        llvm_expr(lg, e->call.args[0], &item_reg);
                    }
                    
                    int t = llvm_new_temp(lg);
                    if (item_reg <= -1000000) {
                        long long const_val = -item_reg - 1000000;
                        llvm_emit(lg, "  %%%d = call i64* @array_append_elem(i64* %%%d, i64 %lld)", t, obj_reg, const_val);
                    } else {
                        llvm_emit(lg, "  %%%d = call i64* @array_append_elem(i64* %%%d, i64 %%%d)", t, obj_reg, item_reg);
                    }
                    *result_reg = t;
                } else if (strcmp(method_name, "join") == 0) {
                    // Handle arr.join(sep)
                    int obj_reg;
                    llvm_expr(lg, e->call.func->field.object, &obj_reg);
                    
                    int sep_reg;
                    if (e->call.arg_count > 0) {
                        llvm_expr(lg, e->call.args[0], &sep_reg);
                    }
                    
                    int t = llvm_new_temp(lg);
                    llvm_emit(lg, "  %%%d = call i8* @array_join(i64* %%%d, i8* %%%d)", t, obj_reg, sep_reg);
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
                end_reg = -1000001;  // -1 constant (means end of string/array)
            }
            
            // Determine if this is string or array slicing
            Type* obj_type = NULL;
            if (e->slice.object->kind == EXPR_IDENT) {
                obj_type = llvm_get_var_type(lg, e->slice.object->ident);
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
            
            // Get iterator
            int iter_reg;
            llvm_expr(lg, e->comprehension.iter, &iter_reg);
            
            // Get length
            int len_ptr = llvm_new_temp(lg);
            llvm_emit(lg, "  %%%d = getelementptr i64, i64* %%%d, i64 0", len_ptr, iter_reg);
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
            llvm_emit(lg, "  %%%d = getelementptr i64, i64* %%%d, i64 %%%d", elem_ptr, iter_reg, idx_plus_2);
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
                        // Check if value is from array indexing (i64 that needs cast to i8*)
                        if (s->let.value->kind == EXPR_INDEX) {
                            // Cast i64 to i8* for string array elements
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
            int dummy;
            llvm_expr(lg, s->expr.expr, &dummy);
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
                llvm_emit(lg, "  %%%d = getelementptr i64, i64* %%%d, i64 0", len_ptr, arr_reg);
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
                llvm_emit(lg, "  %%%d = getelementptr i64, i64* %%%d, i64 %%%d", elem_ptr, arr_reg, adj_idx);
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
        
        llvm_emit(lg, "define %s @%s(%s) {", ret_type, f->name, params);
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
    llvm_emit(&lg, "declare i64 @file_exists(i8*)");
    llvm_emit(&lg, "declare i64 @append_file(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @file_copy(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @file_move(i8*, i8*)");
    llvm_emit(&lg, "declare i64 @file_chmod(i8*, i64)");
    llvm_emit(&lg, "declare i64* @file_glob(i8*)");
    llvm_emit(&lg, "declare i64 @ord(i8*)");
    llvm_emit(&lg, "declare i8* @chr(i64)");
    llvm_emit(&lg, "declare i8* @substring(i8*, i64, i64)");
    llvm_emit(&lg, "declare i64 @str_find(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @str_concat(i8*, i8*)");
    llvm_emit(&lg, "declare i8* @str_replace(i8*, i8*, i8*)");
    llvm_emit(&lg, "declare i64 @str_cmp(i8*, i8*)");
    llvm_emit(&lg, "declare i64* @str_split(i8*, i8*)");
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
    llvm_emit(&lg, "declare i64* @array_append_elem(i64*, i64)");
    llvm_emit(&lg, "declare i64* @array_prepend_elem(i64*, i64)");
    llvm_emit(&lg, "declare i64 @array_contains_elem(i64*, i64)");
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
int main(int argc, char** argv) {
#endif
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* input_file = NULL;
    const char* output_file = "a.out";
    bool compile_only = false;
    bool emit_asm = false;
    bool emit_ir = false;
    bool quiet = true;  // Default to quiet mode for clean output
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
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            quiet = false;
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
    snprintf(ir_file, 256, "/tmp/wyn_%d_%d.ll", getpid(), rand());
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
    snprintf(obj_file, 256, "/tmp/wyn_%d_%d.o", getpid(), rand());
    char cmd[512];
    
    // Try to find llc in various locations
    const char* llc_path = "llc";  // Default: use PATH
    if (access("/opt/homebrew/opt/llvm/bin/llc", X_OK) == 0) {
        llc_path = "/opt/homebrew/opt/llvm/bin/llc";
    } else if (getenv("LLVM_PATH")) {
        static char llc_from_env[512];
        snprintf(llc_from_env, 512, "%s/bin/llc", getenv("LLVM_PATH"));
        if (access(llc_from_env, X_OK) == 0) {
            llc_path = llc_from_env;
        }
    }
    
    snprintf(cmd, 512, "%s -relocation-model=pic -filetype=obj %s -o %s", llc_path, ir_file, obj_file);
    if (system(cmd) != 0) {
        fprintf(stderr, "LLVM compilation failed\n");
        return 1;
    }
    
    // Find runtime libraries
    char runtime_path[512] = "build";
    if (access("build/builtins_runtime.o", F_OK) != 0) {
        // Try installed location
        #ifdef __APPLE__
        strcpy(runtime_path, "/usr/local/share/wyn/runtime");
        #else
        strcpy(runtime_path, "/usr/local/share/wyn/runtime");
        #endif
        
        // Check if installed location exists
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s/builtins_runtime.o", runtime_path);
        if (access(test_path, F_OK) != 0) {
            // Fallback to build directory
            strcpy(runtime_path, "build");
        }
    }
    
    // Link
    if (!compile_only) {
        char cmd[2048];  // Increased buffer size for long paths
        snprintf(cmd, sizeof(cmd), "clang -fPIE %s %s/builtins_runtime.o %s/array_runtime.o %s/spawn_runtime.o %s/channels_runtime.o %s/task_runtime.o -lpthread -lm -o %s", 
                 obj_file, runtime_path, runtime_path, runtime_path, runtime_path, runtime_path, output_file);
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
