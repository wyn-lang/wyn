#include <stdio.h>
#include <assert.h>
#include "common.h"

void init_lexer(const char* source);
Token next_token();

int main() {
    printf("=== Quick Lexer Test ===\n");
    
    init_lexer("42");
    Token t = next_token();
    assert(t.type == TOKEN_INT);
    printf("✓ Lexer works\n");
    
    printf("\n1/1 tests passed!\n");
    return 0;
}
