CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -g
OPTFLAGS=-O2

all: wyn test

wyn: src/main.c src/lexer.c src/parser.c src/checker.c src/codegen.c
	$(CC) $(CFLAGS) -I src -o $@ $^

wyn-release: src/main.c src/lexer.c src/parser.c src/checker.c src/codegen.c
	$(CC) $(CFLAGS) $(OPTFLAGS) -I src -o wyn $^
	strip wyn

test: test_lexer test_parser test_checker test_codegen

test_lexer: tests/test_lexer
	@echo "=== Running Lexer Tests ==="
	@./tests/test_lexer

test_parser: tests/test_parser
	@echo "=== Running Parser Tests ==="
	@./tests/test_parser

test_checker: tests/test_checker
	@echo "=== Running Type Checker Tests ==="
	@./tests/test_checker

test_codegen: tests/test_codegen
	@echo "=== Running Code Generator Tests ==="
	@mkdir -p temp
	@./tests/test_codegen

tests/test_lexer: tests/test_lexer.c src/lexer.c
	$(CC) $(CFLAGS) -I src -o $@ $^

tests/test_parser: tests/test_parser.c src/parser.c src/lexer.c
	$(CC) $(CFLAGS) -I src -o $@ $^

tests/test_checker: tests/test_checker.c src/checker.c src/parser.c src/lexer.c
	$(CC) $(CFLAGS) -I src -o $@ $^

tests/test_codegen: tests/test_codegen.c src/codegen.c src/parser.c src/lexer.c
	$(CC) $(CFLAGS) -I src -o $@ $^

clean:
	rm -f wyn tests/test_lexer tests/test_parser tests/test_checker tests/test_codegen
	rm -rf temp

.PHONY: all test test_lexer test_parser test_checker test_codegen clean
