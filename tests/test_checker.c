#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "common.h"
#include "ast.h"
#include "types.h"

void init_lexer(const char* source);
void init_parser();
void init_checker();
Expr* expression();
Type* check_expr(Expr* expr, SymbolTable* scope);
SymbolTable* get_global_scope();

int tests_run = 0;
int tests_passed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    test_##name(); \
    tests_run++; \
    tests_passed++; \
    printf("✓ test_%s\n", #name); \
} while(0)

TEST(check_int) {
    init_checker();
    init_lexer("42");
    init_parser();
    Expr* expr = expression();
    Type* type = check_expr(expr, get_global_scope());
    assert(type != NULL);
    assert(type->kind == TYPE_INT);
}

TEST(check_float) {
    init_checker();
    init_lexer("3.14");
    init_parser();
    Expr* expr = expression();
    Type* type = check_expr(expr, get_global_scope());
    assert(type != NULL);
    assert(type->kind == TYPE_FLOAT);
}

TEST(check_binary) {
    init_checker();
    init_lexer("1 + 2");
    init_parser();
    Expr* expr = expression();
    Type* type = check_expr(expr, get_global_scope());
    assert(type != NULL);
    assert(type->kind == TYPE_INT);
}

int main() {
    printf("=== Type Checker Tests ===\n");
    RUN_TEST(check_int);
    RUN_TEST(check_float);
    RUN_TEST(check_binary);
    
    printf("\n%d/%d tests passed!\n", tests_passed, tests_run);
    return tests_run != tests_passed;
}
