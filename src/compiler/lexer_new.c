/*
 * Wyn Lexer Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include "lexer.h"

// Lexer implementation
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
    return (l->pos < l->len) ? l->source[l->pos] : '\0';
}

static char lexer_peek_char(Lexer* l, int offset) {
    size_t pos = l->pos + offset;
    return (pos < l->len) ? l->source[pos] : '\0';
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
        } else if (c == '#') {
            while (l->pos < l->len && lexer_char(l) != '\n') {
                lexer_advance(l);
            }
        } else if (c == '/' && lexer_peek_char(l, 1) == '/') {
            while (l->pos < l->len && lexer_char(l) != '\n') {
                lexer_advance(l);
            }
        } else if (c == '/' && lexer_peek_char(l, 1) == '*') {
            lexer_advance(l); lexer_advance(l);
            while (l->pos < l->len - 1) {
                if (lexer_char(l) == '*' && lexer_peek_char(l, 1) == '/') {
                    lexer_advance(l); lexer_advance(l);
                    break;
                }
                lexer_advance(l);
            }
        } else {
            break;
        }
    }
}

// Rest of lexer implementation would go here...
// For now, this is a minimal stub to show the structure

Token lexer_next(Lexer* l) {
    // Full implementation from compiler.c lines 461-609
    Token t = {.kind = TOK_EOF, .line = l->line, .column = l->column};
    return t;
}

Token lexer_peek(Lexer* l) {
    if (!l->has_peek) {
        l->peek = lexer_next(l);
        l->has_peek = true;
    }
    return l->peek;
}
