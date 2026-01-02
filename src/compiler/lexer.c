
// ============================================================================
// LEXER - Tokenization (Lines 71-624)
// ============================================================================

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
    
    // Keywords
    TOK_FN,
    TOK_STRUCT,
    TOK_ENUM,
    TOK_VAR,
    TOK_MUT,
    TOK_CONST,
    TOK_PUB,
    TOK_IF,
    TOK_ELSE,
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
    TOK_SPAWN,
    TOK_AND,
    TOK_OR,
    TOK_NOT,
    TOK_TEST,
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
    "fn", "struct", "enum", "var", "mut", "const", "pub",
    "if", "else", "match", "for", "while", "in",
    "break", "continue", "return", "import", "from", "as",
    "true", "false", "none", "some", "ok", "err", "self",
    "spawn",
    "and", "or", "not",
    "test", "assert"
};

static const TokenKind keyword_tokens[] = {
    TOK_FN, TOK_STRUCT, TOK_ENUM, TOK_VAR, TOK_MUT, TOK_CONST, TOK_PUB,
    TOK_IF, TOK_ELSE, TOK_MATCH, TOK_FOR, TOK_WHILE, TOK_IN,
    TOK_BREAK, TOK_CONTINUE, TOK_RETURN, TOK_IMPORT, TOK_FROM, TOK_AS,
    TOK_TRUE, TOK_FALSE, TOK_NONE, TOK_SOME, TOK_OK, TOK_ERR, TOK_SELF,
    TOK_SPAWN,
    TOK_AND, TOK_OR, TOK_NOT,
    TOK_TEST, TOK_ASSERT
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
            t.kind = TOK_INT;
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
            t.kind = TOK_INT;
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
// PARSER - AST Generation (Lines 624-2862)
// ============================================================================

// ============================================================================
