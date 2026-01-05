#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "ast.h"

extern Token next_token();

typedef struct {
    Token current;
    Token previous;
    bool had_error;
} Parser;

static Parser parser;

Expr* expression();

static void advance() {
    parser.previous = parser.current;
    parser.current = next_token();
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

static void expect(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    fprintf(stderr, "Error at line %d: %s\n", parser.current.line, message);
    parser.had_error = true;
    // Skip to next statement to avoid cascading errors
    while (!check(TOKEN_SEMI) && !check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        advance();
    }
    if (check(TOKEN_SEMI)) advance();
}

static Expr* alloc_expr() {
    return (Expr*)calloc(1, sizeof(Expr));
}

static Stmt* alloc_stmt() {
    return (Stmt*)calloc(1, sizeof(Stmt));
}

static Expr* primary() {
    if (match(TOKEN_NOT) || match(TOKEN_MINUS) || match(TOKEN_BANG) || match(TOKEN_TILDE)) {
        Token op = parser.previous;
        Expr* operand = primary();
        Expr* unary = alloc_expr();
        unary->type = EXPR_UNARY;
        unary->unary.op = op;
        unary->unary.operand = operand;
        return unary;
    }
    
    if (match(TOKEN_INT)) {
        Expr* expr = alloc_expr();
        expr->type = EXPR_INT;
        expr->token = parser.previous;
        return expr;
    }
    
    if (match(TOKEN_FLOAT)) {
        Expr* expr = alloc_expr();
        expr->type = EXPR_FLOAT;
        expr->token = parser.previous;
        return expr;
    }
    
    if (match(TOKEN_STRING)) {
        Expr* expr = alloc_expr();
        expr->type = EXPR_STRING;
        expr->token = parser.previous;
        return expr;
    }
    
    if (match(TOKEN_IDENT)) {
        Expr* expr = alloc_expr();
        expr->type = EXPR_IDENT;
        expr->token = parser.previous;
        return expr;
    }
    
    if (match(TOKEN_TRUE) || match(TOKEN_FALSE)) {
        Expr* expr = alloc_expr();
        expr->type = EXPR_BOOL;
        expr->token = parser.previous;
        return expr;
    }
    
    if (match(TOKEN_NONE)) {
        Expr* expr = alloc_expr();
        expr->type = EXPR_NONE;
        return expr;
    }
    
    if (match(TOKEN_SOME)) {
        Expr* expr = alloc_expr();
        expr->type = EXPR_SOME;
        expect(TOKEN_LPAREN, "Expected '(' after some");
        expr->option.value = expression();
        expect(TOKEN_RPAREN, "Expected ')' after value");
        return expr;
    }
    
    if (match(TOKEN_OK)) {
        Expr* expr = alloc_expr();
        expr->type = EXPR_OK;
        expect(TOKEN_LPAREN, "Expected '(' after ok");
        expr->option.value = expression();
        expect(TOKEN_RPAREN, "Expected ')' after value");
        return expr;
    }
    
    if (match(TOKEN_ERR)) {
        Expr* expr = alloc_expr();
        expr->type = EXPR_ERR;
        expect(TOKEN_LPAREN, "Expected '(' after err");
        expr->option.value = expression();
        expect(TOKEN_RPAREN, "Expected ')' after value");
        return expr;
    }
    
    if (match(TOKEN_MATCH)) {
        Expr* expr = alloc_expr();
        expr->type = EXPR_MATCH;
        expr->match.value = expression();
        expect(TOKEN_LBRACE, "Expected '{' after match value");
        
        expr->match.arm_count = 0;
        expr->match.arms = malloc(sizeof(MatchArm) * 32);
        
        while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
            expr->match.arms[expr->match.arm_count].pattern = parser.current;
            expect(TOKEN_IDENT, "Expected pattern");
            expect(TOKEN_FATARROW, "Expected '=>' after pattern");
            expr->match.arms[expr->match.arm_count].result = expression();
            expr->match.arm_count++;
            if (!check(TOKEN_RBRACE)) {
                match(TOKEN_COMMA);
            }
        }
        
        expect(TOKEN_RBRACE, "Expected '}' after match arms");
        return expr;
    }
    
    if (match(TOKEN_LBRACKET)) {
        Expr* expr = alloc_expr();
        expr->type = EXPR_ARRAY;
        expr->array.elements = NULL;
        expr->array.count = 0;
        
        if (!check(TOKEN_RBRACKET)) {
            int capacity = 8;
            expr->array.elements = malloc(sizeof(Expr*) * capacity);
            do {
                if (expr->array.count >= capacity) {
                    capacity *= 2;
                    expr->array.elements = realloc(expr->array.elements, sizeof(Expr*) * capacity);
                }
                expr->array.elements[expr->array.count++] = expression();
            } while (match(TOKEN_COMMA));
        }
        
        expect(TOKEN_RBRACKET, "Expected ']' after array elements");
        return expr;
    }
    
    if (match(TOKEN_LPAREN)) {
        Expr* expr = expression();
        expect(TOKEN_RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    return NULL;
}

static Expr* call() {
    Expr* expr = primary();
    
    while (true) {
        if (match(TOKEN_LPAREN)) {
            Expr* call_expr = alloc_expr();
            call_expr->type = EXPR_CALL;
            call_expr->call.callee = expr;
            call_expr->call.args = NULL;
            call_expr->call.arg_count = 0;
            
            if (!check(TOKEN_RPAREN)) {
                int capacity = 8;
                call_expr->call.args = malloc(sizeof(Expr*) * capacity);
                do {
                    if (call_expr->call.arg_count >= capacity) {
                        capacity *= 2;
                        call_expr->call.args = realloc(call_expr->call.args, sizeof(Expr*) * capacity);
                    }
                    call_expr->call.args[call_expr->call.arg_count++] = expression();
                } while (match(TOKEN_COMMA));
            }
            
            expect(TOKEN_RPAREN, "Expected ')' after arguments");
            expr = call_expr;
        } else if (match(TOKEN_LBRACKET)) {
            Expr* index_expr = alloc_expr();
            index_expr->type = EXPR_INDEX;
            index_expr->index.array = expr;
            index_expr->index.index = expression();
            expect(TOKEN_RBRACKET, "Expected ']' after index");
            expr = index_expr;
        } else if (match(TOKEN_DOT)) {
            Token field_or_method = parser.current;
            expect(TOKEN_IDENT, "Expected field or method name after '.'");
            
            if (match(TOKEN_LPAREN)) {
                Expr* method_expr = alloc_expr();
                method_expr->type = EXPR_METHOD_CALL;
                method_expr->method_call.object = expr;
                method_expr->method_call.method = field_or_method;
                method_expr->method_call.args = NULL;
                method_expr->method_call.arg_count = 0;
                
                if (!check(TOKEN_RPAREN)) {
                    int capacity = 8;
                    method_expr->method_call.args = malloc(sizeof(Expr*) * capacity);
                    do {
                        if (method_expr->method_call.arg_count >= capacity) {
                            capacity *= 2;
                            method_expr->method_call.args = realloc(method_expr->method_call.args, sizeof(Expr*) * capacity);
                        }
                        method_expr->method_call.args[method_expr->method_call.arg_count++] = expression();
                    } while (match(TOKEN_COMMA));
                }
                
                expect(TOKEN_RPAREN, "Expected ')' after arguments");
                expr = method_expr;
            } else {
                Expr* field_expr = alloc_expr();
                field_expr->type = EXPR_FIELD_ACCESS;
                field_expr->field_access.object = expr;
                field_expr->field_access.field = field_or_method;
                expr = field_expr;
            }
        } else {
            break;
        }
    }
    
    return expr;
}

static Expr* multiplication() {
    Expr* expr = call();
    
    while (match(TOKEN_STAR) || match(TOKEN_SLASH) || match(TOKEN_PERCENT)) {
        Token op = parser.previous;
        Expr* right = call();
        Expr* binary = alloc_expr();
        binary->type = EXPR_BINARY;
        binary->binary.left = expr;
        binary->binary.op = op;
        binary->binary.right = right;
        expr = binary;
    }
    
    return expr;
}

static Expr* bitwise() {
    Expr* expr = multiplication();
    
    while (match(TOKEN_AMP) || match(TOKEN_PIPE) || match(TOKEN_CARET)) {
        Token op = parser.previous;
        Expr* right = multiplication();
        Expr* binary = alloc_expr();
        binary->type = EXPR_BINARY;
        binary->binary.left = expr;
        binary->binary.op = op;
        binary->binary.right = right;
        expr = binary;
    }
    
    return expr;
}

static Expr* addition() {
    Expr* expr = bitwise();
    
    while (match(TOKEN_PLUS) || match(TOKEN_MINUS)) {
        Token op = parser.previous;
        Expr* right = bitwise();
        Expr* binary = alloc_expr();
        binary->type = EXPR_BINARY;
        binary->binary.left = expr;
        binary->binary.op = op;
        binary->binary.right = right;
        expr = binary;
    }
    
    return expr;
}

static Expr* comparison() {
    Expr* expr = addition();
    
    while (match(TOKEN_LT) || match(TOKEN_GT) || match(TOKEN_LTEQ) || match(TOKEN_GTEQ) ||
           match(TOKEN_EQEQ) || match(TOKEN_BANGEQ)) {
        Token op = parser.previous;
        Expr* right = addition();
        Expr* binary = alloc_expr();
        binary->type = EXPR_BINARY;
        binary->binary.left = expr;
        binary->binary.op = op;
        binary->binary.right = right;
        expr = binary;
    }
    
    return expr;
}

static Expr* logical_and() {
    Expr* expr = comparison();
    
    while (match(TOKEN_AND)) {
        Token op = parser.previous;
        Expr* right = comparison();
        Expr* binary = alloc_expr();
        binary->type = EXPR_BINARY;
        binary->binary.left = expr;
        binary->binary.op = op;
        binary->binary.right = right;
        expr = binary;
    }
    
    return expr;
}

static Expr* logical_or() {
    Expr* expr = logical_and();
    
    while (match(TOKEN_OR)) {
        Token op = parser.previous;
        Expr* right = logical_and();
        Expr* binary = alloc_expr();
        binary->type = EXPR_BINARY;
        binary->binary.left = expr;
        binary->binary.op = op;
        binary->binary.right = right;
        expr = binary;
    }
    
    return expr;
}

static Expr* assignment() {
    Expr* expr = logical_or();
    
    if (match(TOKEN_EQ) || match(TOKEN_PLUSEQ) || match(TOKEN_MINUSEQ) || 
        match(TOKEN_STAREQ) || match(TOKEN_SLASHEQ)) {
        if (expr->type == EXPR_IDENT) {
            Token op = parser.previous;
            Expr* assign = alloc_expr();
            assign->type = EXPR_ASSIGN;
            assign->assign.name = expr->token;
            
            if (op.type == TOKEN_EQ) {
                assign->assign.value = assignment();
            } else {
                Expr* right = assignment();
                Expr* binary = alloc_expr();
                binary->type = EXPR_BINARY;
                binary->binary.left = expr;
                binary->binary.right = right;
                
                Token bin_op;
                if (op.type == TOKEN_PLUSEQ) { bin_op.type = TOKEN_PLUS; bin_op.start = "+"; bin_op.length = 1; }
                else if (op.type == TOKEN_MINUSEQ) { bin_op.type = TOKEN_MINUS; bin_op.start = "-"; bin_op.length = 1; }
                else if (op.type == TOKEN_STAREQ) { bin_op.type = TOKEN_STAR; bin_op.start = "*"; bin_op.length = 1; }
                else if (op.type == TOKEN_SLASHEQ) { bin_op.type = TOKEN_SLASH; bin_op.start = "/"; bin_op.length = 1; }
                
                binary->binary.op = bin_op;
                assign->assign.value = binary;
            }
            
            return assign;
        }
    }
    
    return expr;
}

Expr* expression() {
    return assignment();
}

Stmt* statement();

Stmt* statement() {
    if (match(TOKEN_RETURN)) {
        Stmt* stmt = alloc_stmt();
        stmt->type = STMT_RETURN;
        stmt->ret.value = expression();
        return stmt;
    }
    
    if (match(TOKEN_BREAK)) {
        Stmt* stmt = alloc_stmt();
        stmt->type = STMT_BREAK;
        return stmt;
    }
    
    if (match(TOKEN_CONTINUE)) {
        Stmt* stmt = alloc_stmt();
        stmt->type = STMT_CONTINUE;
        return stmt;
    }
    
    if (match(TOKEN_VAR) || match(TOKEN_CONST)) {
        Stmt* stmt = alloc_stmt();
        stmt->type = STMT_VAR;
        stmt->var.is_const = (parser.previous.type == TOKEN_CONST);
        stmt->var.name = parser.current;
        expect(TOKEN_IDENT, "Expected variable name");
        expect(TOKEN_EQ, "Expected '=' after variable name");
        stmt->var.init = expression();
        return stmt;
    }
    
    if (match(TOKEN_IF)) {
        Stmt* stmt = alloc_stmt();
        stmt->type = STMT_IF;
        if (match(TOKEN_LPAREN)) {
            stmt->if_stmt.condition = expression();
            expect(TOKEN_RPAREN, "Expected ')' after if condition");
        } else {
            stmt->if_stmt.condition = expression();
        }
        expect(TOKEN_LBRACE, "Expected '{' after if condition");
        stmt->if_stmt.then_branch = alloc_stmt();
        stmt->if_stmt.then_branch->type = STMT_BLOCK;
        stmt->if_stmt.then_branch->block.stmts = malloc(sizeof(Stmt*) * 32);
        stmt->if_stmt.then_branch->block.count = 0;
        while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
            stmt->if_stmt.then_branch->block.stmts[stmt->if_stmt.then_branch->block.count++] = statement();
        }
        expect(TOKEN_RBRACE, "Expected '}' after if body");
        
        if (match(TOKEN_ELSE)) {
            if (check(TOKEN_IF)) {
                stmt->if_stmt.else_branch = statement();
            } else {
                expect(TOKEN_LBRACE, "Expected '{' after else");
                stmt->if_stmt.else_branch = alloc_stmt();
                stmt->if_stmt.else_branch->type = STMT_BLOCK;
                stmt->if_stmt.else_branch->block.stmts = malloc(sizeof(Stmt*) * 32);
                stmt->if_stmt.else_branch->block.count = 0;
                while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
                    stmt->if_stmt.else_branch->block.stmts[stmt->if_stmt.else_branch->block.count++] = statement();
                }
                expect(TOKEN_RBRACE, "Expected '}' after else body");
            }
        } else {
            stmt->if_stmt.else_branch = NULL;
        }
        return stmt;
    }
    
    if (match(TOKEN_WHILE)) {
        Stmt* stmt = alloc_stmt();
        stmt->type = STMT_WHILE;
        if (match(TOKEN_LPAREN)) {
            stmt->while_stmt.condition = expression();
            expect(TOKEN_RPAREN, "Expected ')' after while condition");
        } else {
            stmt->while_stmt.condition = expression();
        }
        expect(TOKEN_LBRACE, "Expected '{' after while condition");
        stmt->while_stmt.body = alloc_stmt();
        stmt->while_stmt.body->type = STMT_BLOCK;
        stmt->while_stmt.body->block.stmts = malloc(sizeof(Stmt*) * 32);
        stmt->while_stmt.body->block.count = 0;
        while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
            stmt->while_stmt.body->block.stmts[stmt->while_stmt.body->block.count++] = statement();
        }
        expect(TOKEN_RBRACE, "Expected '}' after while body");
        return stmt;
    }
    
    if (match(TOKEN_FOR)) {
        Stmt* stmt = alloc_stmt();
        stmt->type = STMT_FOR;
        
        if (match(TOKEN_VAR)) {
            stmt->for_stmt.init = alloc_stmt();
            stmt->for_stmt.init->type = STMT_VAR;
            stmt->for_stmt.init->var.name = parser.current;
            expect(TOKEN_IDENT, "Expected variable name");
            expect(TOKEN_EQ, "Expected '=' after variable name");
            stmt->for_stmt.init->var.init = expression();
        } else {
            stmt->for_stmt.init = NULL;
        }
        
        expect(TOKEN_SEMI, "Expected ';' after for init");
        stmt->for_stmt.condition = expression();
        expect(TOKEN_SEMI, "Expected ';' after for condition");
        stmt->for_stmt.increment = expression();
        
        expect(TOKEN_LBRACE, "Expected '{' after for header");
        stmt->for_stmt.body = alloc_stmt();
        stmt->for_stmt.body->type = STMT_BLOCK;
        stmt->for_stmt.body->block.stmts = malloc(sizeof(Stmt*) * 32);
        stmt->for_stmt.body->block.count = 0;
        while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
            stmt->for_stmt.body->block.stmts[stmt->for_stmt.body->block.count++] = statement();
        }
        expect(TOKEN_RBRACE, "Expected '}' after for body");
        return stmt;
    }
    
    Stmt* stmt = alloc_stmt();
    stmt->type = STMT_EXPR;
    stmt->expr = expression();
    return stmt;
}

Stmt* function() {
    bool is_public = match(TOKEN_PUB);
    expect(TOKEN_FN, "Expected 'fn'");
    Stmt* stmt = alloc_stmt();
    stmt->type = STMT_FN;
    stmt->fn.is_public = is_public;
    stmt->fn.name = parser.current;
    expect(TOKEN_IDENT, "Expected function name");
    
    stmt->fn.type_param_count = 0;
    stmt->fn.type_params = NULL;
    if (match(TOKEN_LBRACKET)) {
        stmt->fn.type_params = malloc(sizeof(Token) * 8);
        do {
            stmt->fn.type_params[stmt->fn.type_param_count++] = parser.current;
            expect(TOKEN_IDENT, "Expected type parameter");
        } while (match(TOKEN_COMMA));
        expect(TOKEN_RBRACKET, "Expected ']' after type parameters");
    }
    
    expect(TOKEN_LPAREN, "Expected '(' after function name");
    
    stmt->fn.param_count = 0;
    stmt->fn.params = NULL;
    stmt->fn.param_types = NULL;
    stmt->fn.param_mutable = NULL;
    
    if (!check(TOKEN_RPAREN)) {
        int capacity = 8;
        stmt->fn.params = malloc(sizeof(Token) * capacity);
        stmt->fn.param_types = malloc(sizeof(Expr*) * capacity);
        stmt->fn.param_mutable = malloc(sizeof(bool) * capacity);
        
        do {
            if (stmt->fn.param_count >= capacity) {
                capacity *= 2;
                stmt->fn.params = realloc(stmt->fn.params, sizeof(Token) * capacity);
                stmt->fn.param_types = realloc(stmt->fn.param_types, sizeof(Expr*) * capacity);
                stmt->fn.param_mutable = realloc(stmt->fn.param_mutable, sizeof(bool) * capacity);
            }
            stmt->fn.param_mutable[stmt->fn.param_count] = match(TOKEN_MUT);
            stmt->fn.params[stmt->fn.param_count] = parser.current;
            expect(TOKEN_IDENT, "Expected parameter name");
            expect(TOKEN_COLON, "Expected ':' after parameter name");
            stmt->fn.param_types[stmt->fn.param_count] = primary();
            stmt->fn.param_count++;
        } while (match(TOKEN_COMMA));
    }
    
    expect(TOKEN_RPAREN, "Expected ')' after parameters");
    
    if (match(TOKEN_ARROW)) {
        stmt->fn.return_type = primary();
    } else {
        stmt->fn.return_type = NULL;
    }
    
    expect(TOKEN_LBRACE, "Expected '{' before function body");
    Stmt* body = alloc_stmt();
    body->type = STMT_BLOCK;
    body->block.count = 0;
    body->block.stmts = malloc(sizeof(Stmt*) * 32);
    
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        body->block.stmts[body->block.count++] = statement();
    }
    
    expect(TOKEN_RBRACE, "Expected '}' after function body");
    stmt->fn.body = body;
    
    return stmt;
}

Stmt* struct_decl() {
    expect(TOKEN_STRUCT, "Expected 'struct'");
    Stmt* stmt = alloc_stmt();
    stmt->type = STMT_STRUCT;
    stmt->struct_decl.name = parser.current;
    expect(TOKEN_IDENT, "Expected struct name");
    expect(TOKEN_LBRACE, "Expected '{' after struct name");
    
    stmt->struct_decl.field_count = 0;
    stmt->struct_decl.fields = malloc(sizeof(Token) * 32);
    stmt->struct_decl.field_types = malloc(sizeof(Expr*) * 32);
    
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        stmt->struct_decl.fields[stmt->struct_decl.field_count] = parser.current;
        expect(TOKEN_IDENT, "Expected field name");
        expect(TOKEN_COLON, "Expected ':' after field name");
        stmt->struct_decl.field_types[stmt->struct_decl.field_count] = primary();
        stmt->struct_decl.field_count++;
    }
    
    expect(TOKEN_RBRACE, "Expected '}' after struct fields");
    return stmt;
}

Stmt* enum_decl() {
    expect(TOKEN_ENUM, "Expected 'enum'");
    Stmt* stmt = alloc_stmt();
    stmt->type = STMT_ENUM;
    stmt->enum_decl.name = parser.current;
    expect(TOKEN_IDENT, "Expected enum name");
    expect(TOKEN_LBRACE, "Expected '{' after enum name");
    
    stmt->enum_decl.variant_count = 0;
    stmt->enum_decl.variants = malloc(sizeof(Token) * 32);
    
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        stmt->enum_decl.variants[stmt->enum_decl.variant_count] = parser.current;
        expect(TOKEN_IDENT, "Expected variant name");
        stmt->enum_decl.variant_count++;
        if (!check(TOKEN_RBRACE)) {
            match(TOKEN_COMMA);
        }
    }
    
    expect(TOKEN_RBRACE, "Expected '}' after enum variants");
    return stmt;
}

Stmt* type_alias() {
    expect(TOKEN_TYPE, "Expected 'type'");
    Stmt* stmt = alloc_stmt();
    stmt->type = STMT_TYPE_ALIAS;
    stmt->type_alias.name = parser.current;
    expect(TOKEN_IDENT, "Expected type alias name");
    expect(TOKEN_EQ, "Expected '=' after type name");
    stmt->type_alias.target = parser.current;
    expect(TOKEN_IDENT, "Expected target type");
    return stmt;
}

Stmt* impl_block() {
    expect(TOKEN_IMPL, "Expected 'impl'");
    Stmt* stmt = alloc_stmt();
    stmt->type = STMT_IMPL;
    stmt->impl.type_name = parser.current;
    expect(TOKEN_IDENT, "Expected type name");
    expect(TOKEN_LBRACE, "Expected '{' after type name");
    
    stmt->impl.method_count = 0;
    stmt->impl.methods = malloc(sizeof(FnStmt*) * 32);
    
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        Stmt* method = function();
        stmt->impl.methods[stmt->impl.method_count] = malloc(sizeof(FnStmt));
        *stmt->impl.methods[stmt->impl.method_count] = method->fn;
        stmt->impl.method_count++;
    }
    
    expect(TOKEN_RBRACE, "Expected '}' after impl block");
    return stmt;
}

Program* parse_program() {
    Program* prog = calloc(1, sizeof(Program));
    prog->stmts = malloc(sizeof(Stmt*) * 32);
    prog->count = 0;
    
    while (!check(TOKEN_EOF)) {
        if (check(TOKEN_FN) || check(TOKEN_PUB)) {
            prog->stmts[prog->count++] = function();
        } else if (check(TOKEN_STRUCT)) {
            prog->stmts[prog->count++] = struct_decl();
        } else if (check(TOKEN_ENUM)) {
            prog->stmts[prog->count++] = enum_decl();
        } else if (check(TOKEN_TYPE)) {
            prog->stmts[prog->count++] = type_alias();
        } else if (check(TOKEN_IMPL)) {
            prog->stmts[prog->count++] = impl_block();
        } else {
            prog->stmts[prog->count++] = statement();
        }
    }
    
    return prog;
}

void init_parser() {
    parser.had_error = false;
    advance();
}

bool parser_had_error() {
    return parser.had_error;
}
