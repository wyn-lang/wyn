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
#include <unistd.h>

#define MAX_TOKEN_LEN 256
#define MAX_IDENT_LEN 128
#define MAX_STRING_LEN 4096
#define MAX_PARAMS 32
#define MAX_FIELDS 64
#define MAX_VARIANTS 64
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
    "fn", "struct", "enum", "interface", "impl", "let", "mut", "pub",
    "if", "else", "then", "match", "for", "while", "in",
    "break", "continue", "return", "import", "from", "as",
    "true", "false", "none", "some", "ok", "err", "self", "Self",
    "try", "catch", "defer", "spawn", "parallel", "async", "await", "yield",
    "and", "or", "not", "test", "bench", "assert"
};

static const TokenKind keyword_tokens[] = {
    TOK_FN, TOK_STRUCT, TOK_ENUM, TOK_INTERFACE, TOK_IMPL, TOK_LET, TOK_MUT, TOK_PUB,
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
    TYPE_FUNCTION, TYPE_NAMED, TYPE_GENERIC
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
    EXPR_UNWRAP, EXPR_TRY, EXPR_DEFAULT, EXPR_SLICE
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
        struct { Expr* object; Expr* index; } index;
        struct { Expr* object; char field[MAX_IDENT_LEN]; } field;
        struct { Expr** elements; int count; } array;
        struct { char** keys; Expr** values; int count; } map;
        struct { char name[MAX_IDENT_LEN]; char fields[MAX_FIELDS][MAX_IDENT_LEN]; Expr* values[MAX_FIELDS]; int count; Type** type_args; int type_arg_count; } struct_lit;
        struct { Expr* cond; Expr* then_expr; Expr* else_expr; } if_expr;
        struct { Expr* value; } some;  // also used for ok/err
        struct { Expr* value; Expr* default_val; } default_expr;
        struct { Expr* object; Expr* start; Expr* end; Expr* step; } slice;
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
        struct { Expr* value; Expr** patterns; Stmt*** arms; int* arm_counts; int arm_count; } match_stmt;
        struct { Stmt** stmts; int count; } block;
        struct { Expr* expr; } defer;
        struct { Stmt** body; int body_count; } spawn;
    };
};

typedef struct {
    char name[MAX_IDENT_LEN];
    Type* type;
    Expr* default_val;
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
} Parser;

static Parser* parser_new(Lexer* lexer, const char* filename) {
    Parser* p = malloc(sizeof(Parser));
    p->lexer = lexer;
    p->filename = filename;
    p->had_error = false;
    p->current = lexer_next(lexer);
    return p;
}

static void parser_free(Parser* p) { free(p); }

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
    // Skip newlines in most contexts
    while (p->current.kind == TOK_NEWLINE) {
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
        Expr* e = new_expr(EXPR_STRING, line, col);
        strcpy(e->str_val, p->current.str_val);
        parser_advance(p);
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
        if (parser_check(p, TOK_LBRACE) && name[0] >= 'A' && name[0] <= 'Z') {
            parser_advance(p);
            Expr* e = new_expr(EXPR_STRUCT, line, col);
            strcpy(e->struct_lit.name, name);
            e->struct_lit.count = 0;
            e->struct_lit.type_args = type_args;
            e->struct_lit.type_arg_count = type_arg_count;
            if (!parser_check(p, TOK_RBRACE)) {
                do {
                    Token fld = parser_expect(p, TOK_IDENT, "field name");
                    parser_expect(p, TOK_COLON, ":");
                    strcpy(e->struct_lit.fields[e->struct_lit.count], fld.ident);
                    e->struct_lit.values[e->struct_lit.count] = parse_expr(p);
                    e->struct_lit.count++;
                } while (parser_match(p, TOK_COMMA) && !parser_check(p, TOK_RBRACE));
            }
            parser_expect(p, TOK_RBRACE, "}");
            return e;
        }
        
        Expr* e = new_expr(EXPR_IDENT, line, col);
        strcpy(e->ident, name);
        return e;
    }
    
    // Parenthesized expr or tuple
    if (parser_match(p, TOK_LPAREN)) {
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
    
    // Lambda: fn(params) { body } or fn(params) -> Type { body }
    if (parser_match(p, TOK_FN)) {
        Expr* e = new_expr(EXPR_LAMBDA, line, col);
        // Simplified: just parse as call-like for now
        parser_expect(p, TOK_LPAREN, "(");
        // Skip params for now
        while (!parser_check(p, TOK_RPAREN) && !parser_check(p, TOK_EOF)) {
            parser_advance(p);
        }
        parser_expect(p, TOK_RPAREN, ")");
        if (parser_match(p, TOK_ARROW)) {
            parse_type(p); // skip return type
        }
        parser_expect(p, TOK_LBRACE, "{");
        while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
            parser_advance(p);
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
        case TOK_EQEQ: case TOK_NOTEQ: return 6;
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
    
    // Let statement: let x, let mut x, pub let x, pub let mut x
    if (parser_check(p, TOK_LET) || 
        (parser_check(p, TOK_PUB) && lexer_peek(p->lexer).kind == TOK_LET)) {
        Stmt* s = new_stmt(STMT_LET, line, col);
        s->let.is_pub = parser_match(p, TOK_PUB);
        parser_expect(p, TOK_LET, "let");
        s->let.is_mut = parser_match(p, TOK_MUT);
        
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
        s->match_stmt.arm_count = 0;
        parser_expect(p, TOK_LBRACE, "{");
        while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
            // Parse pattern (expression or _ for wildcard)
            Expr* pattern = parse_expr(p);
            s->match_stmt.patterns[s->match_stmt.arm_count] = pattern;
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
            bool is_mut = parser_match(p, TOK_MUT);
            // Handle 'self' as a special parameter
            if (parser_check(p, TOK_SELF)) {
                parser_advance(p);
                strcpy(fn->params[fn->param_count].name, "self");
            } else {
                Token pname = parser_expect(p, TOK_IDENT, "parameter name");
                strcpy(fn->params[fn->param_count].name, pname.ident);
            }
            (void)is_mut; // TODO: store mutability
            if (parser_match(p, TOK_COLON)) {
                fn->params[fn->param_count].type = parse_type(p);
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
            bool is_mut = parser_match(p, TOK_MUT);
            if (parser_check(p, TOK_SELF)) {
                parser_advance(p);
                strcpy(fn->params[fn->param_count].name, "self");
            } else {
                Token pname = parser_expect(p, TOK_IDENT, "parameter name");
                strcpy(fn->params[fn->param_count].name, pname.ident);
            }
            (void)is_mut;
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
    if (a->kind == TYPE_ARRAY || a->kind == TYPE_OPTIONAL) {
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
                // Built-in functions
                if (strcmp(e->ident, "print") == 0 || strcmp(e->ident, "print_str") == 0 ||
                    strcmp(e->ident, "assert") == 0 || strcmp(e->ident, "sqrt") == 0 || 
                    strcmp(e->ident, "len") == 0 || strcmp(e->ident, "exit") == 0 ||
                    strcmp(e->ident, "abs") == 0 || strcmp(e->ident, "min") == 0 ||
                    strcmp(e->ident, "max") == 0 || strcmp(e->ident, "int_to_str") == 0 ||
                    strcmp(e->ident, "str_to_int") == 0 ||
                    strcmp(e->ident, "read_file") == 0 || strcmp(e->ident, "write_file") == 0 ||
                    strcmp(e->ident, "ord") == 0 || strcmp(e->ident, "chr") == 0) {
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
        
        case EXPR_CALL: {
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
        
        case EXPR_FIELD: {
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
            tc_check_expr(tc, s->match_stmt.value);
            for (int i = 0; i < s->match_stmt.arm_count; i++) {
                tc_check_expr(tc, s->match_stmt.patterns[i]);
                tc_enter_scope(tc);
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
    struct { char name[MAX_IDENT_LEN]; int offset; Type* type; } locals[256];
    int local_count;
    struct { char* str; int label; } strings[256];
    int string_count;
    struct { double val; int label; } floats[256];
    int float_count;
    int loop_start_label;  // For continue
    int loop_end_label;    // For break
    TypeChecker* tc;       // For type checking during codegen
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

static int cg_struct_field_offset(StructDef* s, const char* field) {
    for (int i = 0; i < s->field_count; i++) {
        if (strcmp(s->fields[i].name, field) == 0) return i * 8;
    }
    return -1;
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
    cg->stack_offset += 8;
    strcpy(cg->locals[cg->local_count].name, name);
    cg->locals[cg->local_count].offset = cg->stack_offset;
    cg->locals[cg->local_count].type = type;
    cg->local_count++;
}

static Type* cg_local_type(CodeGen* cg, const char* name) {
    for (int i = cg->local_count - 1; i >= 0; i--) {
        if (strcmp(cg->locals[i].name, name) == 0) return cg->locals[i].type;
    }
    return NULL;
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
                    if (off) cg_emit(cg, "    movq -%d(%%rbp), %%rax", off);
                }
                break;
            }
            case EXPR_BINARY: {
                // Check for string concatenation - at least one side must be a string literal
                // or a binary+ that contains a string literal (for chained concat)
                if (e->binary.op == TOK_PLUS) {
                    bool left_is_str = (e->binary.left->kind == EXPR_STRING);
                    bool right_is_str = (e->binary.right->kind == EXPR_STRING);
                    // Check for chained concat: (str + str) + str
                    if (e->binary.left->kind == EXPR_BINARY && e->binary.left->binary.op == TOK_PLUS) {
                        // Recursively check if left subtree contains a string
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
                cg_expr(cg, e->binary.right);
                cg_emit(cg, "    pushq %%rax");
                cg_expr(cg, e->binary.left);
                cg_emit(cg, "    popq %%rcx");
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
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "abs") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rcx");
                    cg_emit(cg, "    negq %%rax");
                    cg_emit(cg, "    cmovlq %%rcx, %%rax");
                    break;
                }
                // Handle built-in min()
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "min") == 0) {
                    cg_expr(cg, e->call.args[1]);
                    cg_emit(cg, "    pushq %%rax");
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    popq %%rcx");
                    cg_emit(cg, "    cmpq %%rcx, %%rax");
                    cg_emit(cg, "    cmovgq %%rcx, %%rax");
                    break;
                }
                // Handle built-in max()
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "max") == 0) {
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
                // Handle built-in read_file() - reads entire file to string
                if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "read_file") == 0) {
                    cg_expr(cg, e->call.args[0]);
                    cg_emit(cg, "    movq %%rax, %%rdi");
                    cg_emit(cg, "    callq %s_wyn_read_file", cg_sym_prefix(cg));
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
                // x86_64 SysV ABI: rdi, rsi, rdx, rcx, r8, r9
                const char* regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                for (int i = e->call.arg_count - 1; i >= 0; i--) {
                    cg_expr(cg, e->call.args[i]);
                    cg_emit(cg, "    pushq %%rax");
                }
                for (int i = 0; i < e->call.arg_count && i < 6; i++) {
                    cg_emit(cg, "    popq %%%s", regs[i]);
                }
                if (e->call.func->kind == EXPR_IDENT) {
                    cg_emit(cg, "    xorl %%eax, %%eax");  // Clear AL for varargs
                    cg_emit(cg, "    callq %s%s", cg_sym_prefix(cg), e->call.func->ident);
                }
                break;
            }
            case EXPR_ARRAY: {
                int count = e->array.count;
                int base_off = cg->stack_offset + 8;
                cg->stack_offset += (count + 1) * 8;
                cg_emit(cg, "    movq $%d, -%d(%%rbp)", count, base_off);
                for (int i = 0; i < count; i++) {
                    cg_expr(cg, e->array.elements[i]);
                    cg_emit(cg, "    movq %%rax, -%d(%%rbp)", base_off + (i + 1) * 8);
                }
                cg_emit(cg, "    leaq -%d(%%rbp), %%rax", base_off);
                break;
            }
            case EXPR_INDEX: {
                cg_expr(cg, e->index.index);
                cg_emit(cg, "    pushq %%rax");
                cg_expr(cg, e->index.object);
                cg_emit(cg, "    popq %%rcx");
                // Handle negative index: if idx < 0, idx += len
                cg_emit(cg, "    testq %%rcx, %%rcx");
                cg_emit(cg, "    jns 1f");
                cg_emit(cg, "    addq (%%rax), %%rcx");  // rcx += len
                cg_emit(cg, "1:");
                cg_emit(cg, "    addq $1, %%rcx");
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
                    cg_emit(cg, "    ldr x0, [x29, #%d]", 16 + off);  // +16 for saved x29/x30
                }
            }
            break;
        }
        
        case EXPR_BINARY: {
            // Check for string concatenation - at least one side must be a string literal
            if (e->binary.op == TOK_PLUS) {
                bool left_is_str = (e->binary.left->kind == EXPR_STRING);
                bool right_is_str = (e->binary.right->kind == EXPR_STRING);
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
                    cg_emit(cg, "    add x0, x29, #%d", 16 + buf_off);  // dest buffer
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
                    cg_emit(cg, "    add x0, x29, #%d", 16 + buf_off);  // return buffer ptr
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
                    char label1[32], label2[32];
                    snprintf(label1, sizeof(label1), "L_.str%d", lbl1);
                    snprintf(label2, sizeof(label2), "L_.str%d", lbl2);
                    cg_emit_adrp(cg, "x1", label1);
                    cg_emit_add_pageoff(cg, "x1", "x1", label1);
                    cg_emit_adrp(cg, "x2", label2);
                    cg_emit_add_pageoff(cg, "x2", "x2", label2);
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
            cg_expr(cg, e->binary.right);
            cg_emit(cg, "    str x0, [sp, #-16]!");
            cg_expr(cg, e->binary.left);
            cg_emit(cg, "    ldr x1, [sp], #16");
            
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
            // Handle built-in abs()
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "abs") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    cmp x0, #0");
                cg_emit(cg, "    cneg x0, x0, lt");
                break;
            }
            // Handle built-in min()
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "min") == 0) {
                cg_expr(cg, e->call.args[1]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    ldr x1, [sp], #16");
                cg_emit(cg, "    cmp x0, x1");
                cg_emit(cg, "    csel x0, x0, x1, lt");
                break;
            }
            // Handle built-in max()
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "max") == 0) {
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
            // Handle built-in read_file() - reads entire file to string
            if (e->call.func->kind == EXPR_IDENT && strcmp(e->call.func->ident, "read_file") == 0) {
                cg_expr(cg, e->call.args[0]);
                cg_emit(cg, "    bl %s_wyn_read_file", cg_sym_prefix(cg));
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
                    cg_expr(cg, obj);
                    int loop_lbl = cg_new_label(cg), end_lbl = cg_new_label(cg);
                    cg_emit(cg, "    mov x1, x0");
                    cg_emit(cg, "    mov x0, #0");
                    cg_emit(cg, "L%d:", loop_lbl);
                    cg_emit(cg, "    ldrb w2, [x1], #1");
                    cg_emit(cg, "    cbz w2, L%d", end_lbl);
                    cg_emit(cg, "    add x0, x0, #1");
                    cg_emit(cg, "    b L%d", loop_lbl);
                    cg_emit(cg, "L%d:", end_lbl);
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
            }
            // Method call: obj.method(args)
            if (e->call.func->kind == EXPR_FIELD) {
                // Push args in reverse
                for (int i = e->call.arg_count - 1; i >= 0; i--) {
                    cg_expr(cg, e->call.args[i]);
                    cg_emit(cg, "    str x0, [sp, #-16]!");
                }
                // Push self (object)
                cg_expr(cg, e->call.func->field.object);
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
            // Push all args to stack first
            for (int i = e->call.arg_count - 1; i >= 0; i--) {
                cg_expr(cg, e->call.args[i]);
                cg_emit(cg, "    str x0, [sp, #-16]!");
            }
            // Pop into argument registers
            for (int i = 0; i < e->call.arg_count && i < 8; i++) {
                cg_emit(cg, "    ldr x%d, [sp], #16", i);
            }
            if (e->call.func->kind == EXPR_IDENT) {
                cg_emit(cg, "    bl %s%s", cg_sym_prefix(cg), e->call.func->ident);
            }
            break;
        }
        
        case EXPR_ARRAY: {
            // Allocate array in local variable area: [count, elem0, elem1, ...]
            int count = e->array.count;
            // Reserve space for length + elements
            int base_off = cg->stack_offset + 8;  // Start after current locals
            cg->stack_offset += (count + 1) * 8;
            // Store length
            cg_emit(cg, "    mov x0, #%d", count);
            cg_emit(cg, "    str x0, [x29, #%d]", 16 + base_off);
            // Store elements
            for (int i = 0; i < count; i++) {
                cg_expr(cg, e->array.elements[i]);
                cg_emit(cg, "    str x0, [x29, #%d]", 16 + base_off + (i + 1) * 8);
            }
            // Return pointer to array
            cg_emit(cg, "    add x0, x29, #%d", 16 + base_off);
            break;
        }
        
        case EXPR_INDEX: {
            // arr[idx] - array is pointer, first element is length
            cg_expr(cg, e->index.index);
            cg_emit(cg, "    str x0, [sp, #-16]!");  // Save index
            cg_expr(cg, e->index.object);
            cg_emit(cg, "    ldr x1, [sp], #16");    // Restore index
            // Handle negative index: if idx < 0, idx += len
            cg_emit(cg, "    cmp x1, #0");
            cg_emit(cg, "    b.ge 1f");
            cg_emit(cg, "    ldr x2, [x0]");         // x2 = len
            cg_emit(cg, "    add x1, x1, x2");       // idx += len
            cg_emit(cg, "1:");
            cg_emit(cg, "    add x1, x1, #1");       // Skip length field
            cg_emit(cg, "    ldr x0, [x0, x1, lsl #3]");  // Load arr[idx+1]
            break;
        }
        
        case EXPR_SLICE: {
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
                cg_emit(cg, "    str x0, [x29, #%d]", 16 + base_off + foff);
            }
            cg_emit(cg, "    add x0, x29, #%d", 16 + base_off);
            break;
        }
        
        case EXPR_FIELD: {
            cg_expr(cg, e->field.object);
            for (int i = 0; i < cg->module->struct_count; i++) {
                int off = cg_struct_field_offset(&cg->module->structs[i], e->field.field);
                if (off >= 0) {
                    cg_emit(cg, "    ldr x0, [x0, #%d]", off);
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
            cg_emit(cg, "    str x0, [x29, #%d]", 16 + base_off);  // has_value = 1
            cg_expr(cg, e->some.value);
            cg_emit(cg, "    str x0, [x29, #%d]", 16 + base_off + 8);  // value
            cg_emit(cg, "    add x0, x29, #%d", 16 + base_off);
            break;
        }
        
        case EXPR_NONE: {
            // Optional without value: store [0, 0] on stack
            int base_off = cg->stack_offset + 8;
            cg->stack_offset += 16;
            cg_emit(cg, "    mov x0, #0");
            cg_emit(cg, "    str x0, [x29, #%d]", 16 + base_off);  // has_value = 0
            cg_emit(cg, "    str x0, [x29, #%d]", 16 + base_off + 8);  // value = 0
            cg_emit(cg, "    add x0, x29, #%d", 16 + base_off);
            break;
        }
        
        case EXPR_OK: {
            // Result ok: store [1, value] on stack
            int base_off = cg->stack_offset + 8;
            cg->stack_offset += 16;
            cg_emit(cg, "    mov x0, #1");
            cg_emit(cg, "    str x0, [x29, #%d]", 16 + base_off);  // is_ok = 1
            cg_expr(cg, e->some.value);
            cg_emit(cg, "    str x0, [x29, #%d]", 16 + base_off + 8);  // value
            cg_emit(cg, "    add x0, x29, #%d", 16 + base_off);
            break;
        }
        
        case EXPR_ERR: {
            // Result err: store [0, error] on stack
            int base_off = cg->stack_offset + 8;
            cg->stack_offset += 16;
            cg_emit(cg, "    mov x0, #0");
            cg_emit(cg, "    str x0, [x29, #%d]", 16 + base_off);  // is_ok = 0
            cg_expr(cg, e->some.value);
            cg_emit(cg, "    str x0, [x29, #%d]", 16 + base_off + 8);  // error
            cg_emit(cg, "    add x0, x29, #%d", 16 + base_off);
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
        
        default:
            cg_emit(cg, "    ; unsupported expr");
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
                cg_emit(cg, "    movq %%rax, -%d(%%rbp)", cg->locals[cg->local_count - 1].offset);
                break;
            case STMT_ASSIGN:
                if (s->assign.op == TOK_EQ) {
                    cg_expr(cg, s->assign.value);
                } else {
                    // Compound assignment: load current value, apply op, store
                    if (s->assign.target->kind == EXPR_IDENT) {
                        int off = cg_local_offset(cg, s->assign.target->ident);
                        if (off) cg_emit(cg, "    movq -%d(%%rbp), %%rax", off);
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
                    if (off) cg_emit(cg, "    movq %%rax, -%d(%%rbp)", off);
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
            case STMT_EXPR: cg_expr(cg, s->expr.expr); break;
            case STMT_RETURN:
                if (s->ret.value) cg_expr(cg, s->ret.value);
                cg_emit(cg, "    movq %%rbp, %%rsp");
                cg_emit(cg, "    popq %%rbp");
                cg_emit(cg, "    retq");
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
            case STMT_MATCH: {
                int end_lbl = cg_new_label(cg);
                // Evaluate match value once and save
                cg_expr(cg, s->match_stmt.value);
                cg_emit(cg, "    pushq %%rax");
                for (int i = 0; i < s->match_stmt.arm_count; i++) {
                    int next_lbl = cg_new_label(cg);
                    // Check if pattern is wildcard (_)
                    bool is_wildcard = s->match_stmt.patterns[i]->kind == EXPR_IDENT &&
                                       strcmp(s->match_stmt.patterns[i]->ident, "_") == 0;
                    if (!is_wildcard) {
                        cg_emit(cg, "    movq (%%rsp), %%rax");  // Load match value
                        cg_emit(cg, "    pushq %%rax");
                        cg_expr(cg, s->match_stmt.patterns[i]);
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
        case STMT_LET:
            cg_add_local(cg, s->let.name, s->let.type);
            if (s->let.value) {
                cg_expr(cg, s->let.value);
            } else {
                cg_emit(cg, "    mov x0, #0");
            }
            cg_emit(cg, "    str x0, [x29, #%d]", 16 + cg->locals[cg->local_count - 1].offset);
            break;
            
        case STMT_ASSIGN: {
            if (s->assign.op == TOK_EQ) {
                cg_expr(cg, s->assign.value);
            } else {
                // Compound assignment: load current value, apply op, store
                if (s->assign.target->kind == EXPR_IDENT) {
                    int off = cg_local_offset(cg, s->assign.target->ident);
                    if (off) cg_emit(cg, "    ldr x0, [x29, #%d]", 16 + off);
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
                if (off) cg_emit(cg, "    str x0, [x29, #%d]", 16 + off);
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
        
        case STMT_EXPR:
            cg_expr(cg, s->expr.expr);
            break;
            
        case STMT_RETURN:
            if (s->ret.value) cg_expr(cg, s->ret.value);
            cg_emit(cg, "    ldp x29, x30, [sp], #256");
            cg_emit(cg, "    ret");
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
            cg->loop_start_label = cont_lbl;
            cg->loop_end_label = end_lbl;
            
            // Check if iterating over array or range
            bool is_array = s->for_stmt.iter && s->for_stmt.iter->kind != EXPR_BINARY;
            
            if (is_array) {
                // Array iteration: for item in arr
                cg_add_local(cg, "__arr", NULL);
                int arr_off = 16 + cg->locals[cg->local_count - 1].offset;
                cg_expr(cg, s->for_stmt.iter);
                cg_emit(cg, "    str x0, [x29, #%d]", arr_off);
                
                cg_add_local(cg, "__idx", NULL);
                int idx_off = 16 + cg->locals[cg->local_count - 1].offset;
                cg_emit(cg, "    mov x0, #0");
                cg_emit(cg, "    str x0, [x29, #%d]", idx_off);
                
                cg_add_local(cg, s->for_stmt.var, NULL);
                int var_off = 16 + cg->locals[cg->local_count - 1].offset;
                
                cg_emit(cg, "L%d:", start_lbl);
                cg_emit(cg, "    ldr x0, [x29, #%d]", arr_off);
                cg_emit(cg, "    ldr x1, [x0]");  // length
                cg_emit(cg, "    ldr x2, [x29, #%d]", idx_off);
                cg_emit(cg, "    cmp x2, x1");
                cg_emit(cg, "    b.ge L%d", end_lbl);
                
                // Load element: arr[idx]
                cg_emit(cg, "    ldr x0, [x29, #%d]", arr_off);
                cg_emit(cg, "    ldr x1, [x29, #%d]", idx_off);
                cg_emit(cg, "    add x1, x1, #1");
                cg_emit(cg, "    ldr x0, [x0, x1, lsl #3]");
                cg_emit(cg, "    str x0, [x29, #%d]", var_off);
                
                for (int i = 0; i < s->for_stmt.body_count; i++) cg_stmt(cg, s->for_stmt.body[i]);
                cg_emit(cg, "L%d:", cont_lbl);
                cg_emit(cg, "    ldr x0, [x29, #%d]", idx_off);
                cg_emit(cg, "    add x0, x0, #1");
                cg_emit(cg, "    str x0, [x29, #%d]", idx_off);
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
                    cg_emit(cg, "    str x0, [x29, #%d]", var_off);
                    // Initialize end value
                    cg_expr(cg, s->for_stmt.iter->binary.right);
                    cg_emit(cg, "    str x0, [x29, #%d]", end_off);
                } else {
                    cg_emit(cg, "    mov x0, #0");
                    cg_emit(cg, "    str x0, [x29, #%d]", var_off);
                    cg_emit(cg, "    str x0, [x29, #%d]", end_off);
                }
                
                cg_emit(cg, "L%d:", start_lbl);
                cg_emit(cg, "    ldr x0, [x29, #%d]", var_off);
                cg_emit(cg, "    ldr x1, [x29, #%d]", end_off);
                cg_emit(cg, "    cmp x0, x1");
                cg_emit(cg, "    b.%s L%d", inclusive ? "gt" : "ge", end_lbl);
                
                for (int i = 0; i < s->for_stmt.body_count; i++) {
                    cg_stmt(cg, s->for_stmt.body[i]);
                }
                
                cg_emit(cg, "L%d:", cont_lbl);
                cg_emit(cg, "    ldr x0, [x29, #%d]", var_off);
                cg_emit(cg, "    add x0, x0, #1");
                cg_emit(cg, "    str x0, [x29, #%d]", var_off);
                cg_emit(cg, "    b L%d", start_lbl);
            }
            
            cg_emit(cg, "L%d:", end_lbl);
            cg->loop_start_label = saved_start;
            cg->loop_end_label = saved_end;
            break;
        }
        
        case STMT_BREAK:
            cg_emit(cg, "    b L%d", cg->loop_end_label);
            break;
        
        case STMT_CONTINUE:
            cg_emit(cg, "    b L%d", cg->loop_start_label);
            break;
        
        case STMT_MATCH: {
            int end_lbl = cg_new_label(cg);
            // Evaluate match value once and save
            cg_expr(cg, s->match_stmt.value);
            cg_emit(cg, "    str x0, [sp, #-16]!");
            for (int i = 0; i < s->match_stmt.arm_count; i++) {
                int next_lbl = cg_new_label(cg);
                // Check if pattern is wildcard (_)
                bool is_wildcard = s->match_stmt.patterns[i]->kind == EXPR_IDENT &&
                                   strcmp(s->match_stmt.patterns[i]->ident, "_") == 0;
                if (!is_wildcard) {
                    cg_emit(cg, "    ldr x0, [sp]");  // Load match value
                    cg_emit(cg, "    str x0, [sp, #-16]!");
                    cg_expr(cg, s->match_stmt.patterns[i]);
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
    
    // ARM64
    cg_emit(cg, "    .globl %s%s", pfx, mangled);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s%s:", pfx, mangled);
    cg_emit(cg, "    stp x29, x30, [sp, #-256]!");
    cg_emit(cg, "    mov x29, sp");
    
    for (int i = 0; i < fn->param_count && i < 8; i++) {
        cg_add_local(cg, fn->params[i].name, fn->params[i].type);
        cg_emit(cg, "    str x%d, [x29, #%d]", i, 16 + cg->locals[cg->local_count - 1].offset);
    }
    
    for (int i = 0; i < fn->body_count; i++) cg_stmt(cg, fn->body[i]);
    
    cg_emit(cg, "    mov x0, #0");
    cg_emit(cg, "    ldp x29, x30, [sp], #256");
    cg_emit(cg, "    ret");
}

static void cg_function(CodeGen* cg, FnDef* fn) {
    cg->stack_offset = 0;
    cg->local_count = 0;
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
        
        for (int i = 0; i < fn->body_count; i++) cg_stmt(cg, fn->body[i]);
        
        cg_emit(cg, "    xorl %%eax, %%eax");
        cg_emit(cg, "    movq %%rbp, %%rsp");
        cg_emit(cg, "    popq %%rbp");
        cg_emit(cg, "    retq");
        return;
    }
    
    // ARM64
    cg_emit(cg, "    .globl %s%s", pfx, fn->name);
    cg_emit(cg, "    .p2align 2");
    cg_emit(cg, "%s%s:", pfx, fn->name);
    cg_emit(cg, "    stp x29, x30, [sp, #-256]!");
    cg_emit(cg, "    mov x29, sp");
    
    for (int i = 0; i < fn->param_count && i < 8; i++) {
        cg_add_local(cg, fn->params[i].name, fn->params[i].type);
        cg_emit(cg, "    str x%d, [x29, #%d]", i, 16 + cg->locals[cg->local_count - 1].offset);
    }
    
    for (int i = 0; i < fn->body_count; i++) {
        cg_stmt(cg, fn->body[i]);
    }
    
    cg_emit(cg, "    mov x0, #0");
    cg_emit(cg, "    ldp x29, x30, [sp], #256");
    cg_emit(cg, "    ret");
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
        
        if (cg->os == OS_MACOS) {
            cg_emit(cg, "    .section __DATA,__data");
        } else {
            cg_emit(cg, "    .data");
        }
        cg_emit(cg, "L_.fmt:");
        cg_emit(cg, "    .asciz \"%%lld\\n\"");
        cg_emit(cg, "L_.sfmt:");
        cg_emit(cg, "    .asciz \"%%s\\n\"");
        cg_emit(cg, "L_.itsfmt:");
        cg_emit(cg, "    .asciz \"%%lld\"");
        cg_emit(cg, "L_.itsbuf:");
        cg_emit(cg, "    .space 24");
        cg_emit(cg, "L_.chrbuf:");
        cg_emit(cg, "    .space 2");
        cg_emit(cg, "L_.rb:");
        cg_emit(cg, "    .asciz \"rb\"");
        
        for (int i = 0; i < cg->string_count; i++) {
            cg_emit(cg, "L_.str%d:", cg->strings[i].label);
            cg_emit(cg, "    .asciz \"%s\"", cg->strings[i].str);
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
    
    if (cg->os == OS_MACOS) {
        cg_emit(cg, "    .section __DATA,__data");
    } else {
        cg_emit(cg, "    .data");
    }
    cg_emit(cg, "L_.fmt:");
    cg_emit(cg, "    .asciz \"%%lld\\n\"");
    cg_emit(cg, "L_.sfmt:");
    cg_emit(cg, "    .asciz \"%%s\\n\"");
    cg_emit(cg, "L_.itsfmt:");
    cg_emit(cg, "    .asciz \"%%lld\"");
    cg_emit(cg, "L_.itsbuf:");
    cg_emit(cg, "    .space 24");
    cg_emit(cg, "L_.chrbuf:");
    cg_emit(cg, "    .space 2");
    cg_emit(cg, "L_.rb:");
    cg_emit(cg, "    .asciz \"rb\"");
    
    // Emit string literals
    for (int i = 0; i < cg->string_count; i++) {
        cg_emit(cg, "L_.str%d:", cg->strings[i].label);
        cg_emit(cg, "    .asciz \"%s\"", cg->strings[i].str);
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
        
        // Try std/ in current dir
        if (!f) {
            snprintf(full_path, 512, "%sstd/%s.wyn", dir, imp->path);
            f = fopen(full_path, "rb");
        }
        
        // Try std/ in parent dir
        if (!f) {
            snprintf(full_path, 512, "%s../std/%s.wyn", dir, imp->path);
            f = fopen(full_path, "rb");
        }
        
        // Try absolute std path (for project root)
        if (!f && dir[0]) {
            // Go up directories looking for std/
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
    int opt_level = 0;
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
    
    printf("Wyn Bootstrap Compiler (Stage 0)\n");
    printf("Compiling: %s\n", input_file);
    
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
    print_module(module);
    
    // Type check
    TypeChecker* tc = typechecker_new(module, input_file);\
    if (!typecheck_module(tc)) {
        fprintf(stderr, "Type checking failed.\n");
        return 1;
    }
    printf("\nType checking passed.\n");
    
    // Code generation
    if (emit_asm) {
        // Output assembly to file
        char asm_file[256];
        snprintf(asm_file, 256, "%s.s", output_file);
        FILE* asm_out = fopen(asm_file, "w");
        if (!asm_out) {
            fprintf(stderr, "Cannot open %s for writing\n", asm_file);
            return 1;
        }
        CodeGen* cg = codegen_new(asm_out, module, target_arch, target_os, tc);
        codegen_module(cg);
        fclose(asm_out);
        printf("Assembly written to: %s\n", asm_file);
    } else {
        // Generate assembly to temp file, assemble and link
        char asm_file[256], obj_file[256];
        snprintf(asm_file, 256, "/tmp/wyn_%d.s", getpid());
        snprintf(obj_file, 256, "/tmp/wyn_%d.o", getpid());
        
        FILE* asm_out = fopen(asm_file, "w");
        CodeGen* cg = codegen_new(asm_out, module, target_arch, target_os, tc);
        codegen_module(cg);
        fclose(asm_out);
        
        // Assemble and link in one step using cc
        char cmd[512];
        if (!compile_only) {
            snprintf(cmd, 512, "cc -o %s %s", output_file, asm_file);
            if (system(cmd) != 0) {
                fprintf(stderr, "Compilation failed.\n");
                return 1;
            }
            printf("Compiled to: %s\n", output_file);
        } else {
            snprintf(cmd, 512, "cc -c -o %s %s", obj_file, asm_file);
            if (system(cmd) != 0) {
                fprintf(stderr, "Assembly failed.\n");
                return 1;
            }
            printf("Object file: %s\n", obj_file);
        }
        
        // Cleanup
        remove(asm_file);
    }
    
    parser_free(parser);
    lexer_free(lexer);
    free(source);
    
    return 0;
}
