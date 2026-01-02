/*
 * Wyn Lexer - Tokenization
 * Converts source code into tokens
 */

#ifndef WYN_LEXER_H
#define WYN_LEXER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_TOKEN_LEN 256
#define MAX_IDENT_LEN 128
#define MAX_STRING_LEN 4096

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

// Token structure
typedef struct {
    TokenKind kind;
    char ident[MAX_IDENT_LEN];
    int64_t int_val;
    double float_val;
    char str_val[MAX_STRING_LEN];
    int line, column;
} Token;

// Lexer structure
typedef struct {
    const char* source;
    const char* filename;
    size_t pos;
    size_t len;
    int line, column;
    Token current, peek;
    bool has_peek;
} Lexer;

// Lexer functions
Lexer* lexer_new(const char* source, const char* filename);
Token lexer_next(Lexer* lexer);
Token lexer_peek(Lexer* lexer);
void lexer_free(Lexer* lexer);

#endif // WYN_LEXER_H
