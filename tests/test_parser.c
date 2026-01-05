#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "common.h"
#include "ast.h"

void init_lexer(const char* source);
void init_parser();
Expr* expression();
bool parser_had_error();

int tests_run = 0;
int tests_passed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    test_##name(); \
    tests_run++; \
    tests_passed++; \
    printf("✓ test_%s\n", #name); \
} while(0)

TEST(parse_int) {
    init_lexer("42");
    init_parser();
    Expr* expr = expression();
    assert(expr != NULL);
    assert(expr->type == EXPR_INT);
    assert(!parser_had_error());
}

TEST(parse_ident) {
    init_lexer("x");
    init_parser();
    Expr* expr = expression();
    assert(expr != NULL);
    assert(expr->type == EXPR_IDENT);
}

TEST(parse_binary) {
    init_lexer("1 + 2");
    init_parser();
    Expr* expr = expression();
    assert(expr != NULL);
    assert(expr->type == EXPR_BINARY);
    assert(expr->binary.left->type == EXPR_INT);
    assert(expr->binary.right->type == EXPR_INT);
    assert(expr->binary.op.type == TOKEN_PLUS);
}

TEST(parse_precedence) {
    init_lexer("1 + 2 * 3");
    init_parser();
    Expr* expr = expression();
    assert(expr != NULL);
    assert(expr->type == EXPR_BINARY);
    assert(expr->binary.op.type == TOKEN_PLUS);
    assert(expr->binary.right->type == EXPR_BINARY);
    assert(expr->binary.right->binary.op.type == TOKEN_STAR);
}

TEST(parse_call) {
    init_lexer("foo()");
    init_parser();
    Expr* expr = expression();
    assert(expr != NULL);
    assert(expr->type == EXPR_CALL);
    assert(expr->call.callee->type == EXPR_IDENT);
    assert(expr->call.arg_count == 0);
}

TEST(parse_call_with_args) {
    init_lexer("add(1, 2)");
    init_parser();
    Expr* expr = expression();
    assert(expr != NULL);
    assert(expr->type == EXPR_CALL);
    assert(expr->call.arg_count == 2);
}

TEST(parse_method_call) {
    init_lexer("obj.method()");
    init_parser();
    Expr* expr = expression();
    assert(expr != NULL);
    assert(expr->type == EXPR_METHOD_CALL);
    assert(expr->method_call.object->type == EXPR_IDENT);
    assert(expr->method_call.arg_count == 0);
}

TEST(parse_method_chain) {
    init_lexer("obj.method1().method2()");
    init_parser();
    Expr* expr = expression();
    assert(expr != NULL);
    assert(expr->type == EXPR_METHOD_CALL);
    assert(expr->method_call.object->type == EXPR_METHOD_CALL);
}

int main() {
    printf("=== Parser Tests ===\n");
    RUN_TEST(parse_int);
    RUN_TEST(parse_ident);
    RUN_TEST(parse_binary);
    RUN_TEST(parse_precedence);
    RUN_TEST(parse_call);
    RUN_TEST(parse_call_with_args);
    RUN_TEST(parse_method_call);
    RUN_TEST(parse_method_chain);
    
    printf("\n%d/%d tests passed!\n", tests_passed, tests_run);
    return tests_run != tests_passed;
}
