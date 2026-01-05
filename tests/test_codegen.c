#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "common.h"
#include "ast.h"

void init_lexer(const char* source);
void init_parser();
void init_codegen(FILE* output);
Expr* expression();
void codegen_expr(Expr* expr);

int tests_run = 0;
int tests_passed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    test_##name(); \
    tests_run++; \
    tests_passed++; \
    printf("✓ test_%s\n", #name); \
} while(0)

TEST(codegen_int) {
    FILE* f = fopen("temp/test_int.c", "w");
    init_codegen(f);
    init_lexer("42");
    init_parser();
    Expr* expr = expression();
    codegen_expr(expr);
    fclose(f);
    
    f = fopen("temp/test_int.c", "r");
    char buf[100];
    fgets(buf, 100, f);
    fclose(f);
    assert(strcmp(buf, "42") == 0);
}

TEST(codegen_binary) {
    FILE* f = fopen("temp/test_binary.c", "w");
    init_codegen(f);
    init_lexer("1 + 2");
    init_parser();
    Expr* expr = expression();
    codegen_expr(expr);
    fclose(f);
    
    f = fopen("temp/test_binary.c", "r");
    char buf[100];
    fgets(buf, 100, f);
    fclose(f);
    assert(strcmp(buf, "(1 + 2)") == 0);
}

TEST(codegen_call) {
    FILE* f = fopen("temp/test_call.c", "w");
    init_codegen(f);
    init_lexer("foo(1, 2)");
    init_parser();
    Expr* expr = expression();
    codegen_expr(expr);
    fclose(f);
    
    f = fopen("temp/test_call.c", "r");
    char buf[100];
    fgets(buf, 100, f);
    fclose(f);
    assert(strcmp(buf, "foo(1, 2)") == 0);
}

int main() {
    printf("=== Code Generator Tests ===\n");
    RUN_TEST(codegen_int);
    RUN_TEST(codegen_binary);
    RUN_TEST(codegen_call);
    
    printf("\n%d/%d tests passed!\n", tests_passed, tests_run);
    return tests_run != tests_passed;
}
