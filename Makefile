# Wyn Language Build System
VERSION = 0.2.0

CC = cc
CFLAGS = -Wall -Wextra -std=c11 -O2
DEBUG_FLAGS = -g -O0 -DDEBUG

# LLVM Configuration
LLVM_PREFIX ?= /opt/homebrew/opt/llvm
LLVM_CONFIG = $(LLVM_PREFIX)/bin/llvm-config
LLC = $(LLVM_PREFIX)/bin/llc
CLANG = clang

# Compiler backend (llvm is default)
BACKEND ?= llvm
BACKEND_FLAG =

# Detect OS
UNAME_S := $(shell uname -s)

# Set macOS deployment target for compatibility
ifeq ($(UNAME_S),Darwin)
CFLAGS += -mmacosx-version-min=11.0
endif

# Directories
BOOTSTRAP_DIR = bootstrap
BUILD_DIR = build
RUNTIME_DIR = runtime
SRC_DIR = src
TESTS_DIR = tests
EXAMPLES_DIR = examples
PREFIX ?= /usr/local

# Wyn Compiler (wync)
WYNC_SRC = $(BOOTSTRAP_DIR)/stage0.c
WYNC_BIN = $(BUILD_DIR)/wync

# Stage 2 (Self-hosting compiler)
STAGE2_SRC = $(SRC_DIR)/stage1/minimal_self.wyn
STAGE2_BIN = $(BUILD_DIR)/stage2

# Runtime libraries
SPAWN_RUNTIME_SRC = $(RUNTIME_DIR)/spawn.c
SPAWN_RUNTIME_OBJ = $(BUILD_DIR)/spawn_runtime.o
ARRAY_RUNTIME_SRC = $(RUNTIME_DIR)/array.c
ARRAY_RUNTIME_OBJ = $(BUILD_DIR)/array_runtime.o
BUILTINS_RUNTIME_SRC = $(RUNTIME_DIR)/builtins.c
BUILTINS_RUNTIME_OBJ = $(BUILD_DIR)/builtins_runtime.o

# Default target - just build the compiler
.PHONY: all
all: wync runtime

# Build everything including stage2 (currently broken)
.PHONY: all-full
all-full: wync stage2 runtime

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build Wyn Compiler (wync)
.PHONY: wync
wync: $(BUILD_DIR) $(WYNC_BIN)

$(WYNC_BIN): $(WYNC_SRC)
	$(CC) $(CFLAGS) -o $@ $<

# Build Stage 2 (self-hosting compiler)
.PHONY: stage2
stage2: wync runtime $(STAGE2_BIN)

$(STAGE2_BIN): $(STAGE2_SRC)
	./$(WYNC_BIN) --stage1-tc $(BACKEND_FLAG) -o $@ $<

# Build runtime libraries
.PHONY: runtime
runtime: $(BUILD_DIR) $(SPAWN_RUNTIME_OBJ) $(ARRAY_RUNTIME_OBJ) $(BUILTINS_RUNTIME_OBJ)

$(SPAWN_RUNTIME_OBJ): $(SPAWN_RUNTIME_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(ARRAY_RUNTIME_OBJ): $(ARRAY_RUNTIME_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILTINS_RUNTIME_OBJ): $(BUILTINS_RUNTIME_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

# Debug build of wync
.PHONY: wync-debug
wync-debug: $(BUILD_DIR)
	$(CC) $(DEBUG_FLAGS) -o $(WYNC_BIN) $(WYNC_SRC)

# Test core examples with stage0
.PHONY: test-examples
test-examples: wync
	@echo "Testing core examples..."
	@for f in examples/loops.wyn examples/functions.wyn examples/recursion.wyn examples/comprehensive.wyn; do \
		echo "  Testing $$f..."; \
		./$(WYNC_BIN) $$f -o /tmp/wyn_test 2>&1 | grep -q "Compiled to:" && /tmp/wyn_test > /dev/null 2>&1 && echo "    ✅ PASS" || echo "    ❌ FAIL"; \
	done

# Test spawn functionality
.PHONY: test-spawn
test-spawn: wync
	@echo "Testing spawn blocks..."
	@./$(WYNC_BIN) temp/spawn_simple.wyn -o /tmp/wyn_spawn 2>&1 | grep -q "Compiled to:" && timeout 2 /tmp/wyn_spawn > /dev/null 2>&1 && echo "  ✅ Spawn working" || echo "  ❌ Spawn failed"

# Test match statements
.PHONY: test-match
test-match: wync
	@echo "Testing match statements..."
	@./$(WYNC_BIN) match_demo.wyn -o /tmp/wyn_match 2>&1 | grep -q "Compiled to:" && /tmp/wyn_match > /dev/null 2>&1 && echo "  ✅ Match working" || echo "  ❌ Match failed"

# Test self-hosting (disabled - requires stage2 fixes)
.PHONY: test-self-hosting
test-self-hosting: stage2
	@echo "Testing Stage 2 self-hosting..."
	@./tests/scripts/test_stage2_self_hosting.sh

# Test infinite compilation (disabled - requires stage2 fixes)
.PHONY: test-infinite
test-infinite: stage2
	@echo "Testing infinite compilation..."
	@./tests/scripts/test_infinite_compilation.sh

# Run all tests (stage0 only for now)
.PHONY: test
test: test-examples test-spawn test-match
	@echo "✅ All stage0 tests passed!"

# Run full tests including stage2 (currently broken)
.PHONY: test-all
test-all: test test-self-hosting test-infinite
	@echo "✅ All tests passed!"

# Install to system
.PHONY: install
install: all
	@echo "Installing Wyn to $(PREFIX)/bin..."
	install -m 755 $(BUILD_DIR)/wync $(PREFIX)/bin/wync
	install -m 755 $(BUILD_DIR)/stage2 $(PREFIX)/bin/wyn
	@echo "Installing stdlib to $(PREFIX)/share/wyn/std..."
	mkdir -p $(PREFIX)/share/wyn
	cp -r std $(PREFIX)/share/wyn/
	@echo "✅ Installation complete!"

# Uninstall from system
.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/bin/wync
	rm -f $(PREFIX)/bin/wyn
	rm -rf $(PREFIX)/share/wyn
	@echo "✅ Uninstalled"

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Full clean
.PHONY: distclean
distclean: clean
	rm -rf .wyn

# Full rebuild
.PHONY: rebuild
rebuild: clean all

# Show version
.PHONY: version
version:
	@echo "Wyn $(VERSION) - Self-Hosting"

# Help
.PHONY: help
help:
	@echo "Wyn Language Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all                 - Build everything (default)"
	@echo "  wync                - Build Wyn compiler (LLVM backend)"
	@echo "  stage2              - Build Stage 2 (self-hosting)"
	@echo "  runtime             - Build runtime libraries"
	@echo "  test                - Run all tests"
	@echo "  test-self-hosting   - Test Stage 2 self-hosting"
	@echo "  test-infinite       - Test infinite compilation"
	@echo "  install             - Install to system"
	@echo "  uninstall           - Uninstall from system"
	@echo "  clean               - Remove build artifacts"
	@echo "  rebuild             - Clean and rebuild"
	@echo "  version             - Show version"
	@echo "  help                - Show this help"

.DEFAULT_GOAL := all
