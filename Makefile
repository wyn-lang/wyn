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

# Unified Wyn binary
WYN_CLI_SRC = $(SRC_DIR)/wyn.c
WYN_COMPILER_SRC = $(BOOTSTRAP_DIR)/stage0.c
WYN_BIN = $(BUILD_DIR)/wyn

# Runtime libraries
SPAWN_RUNTIME_SRC = $(RUNTIME_DIR)/spawn.c
SPAWN_RUNTIME_OBJ = $(BUILD_DIR)/spawn_runtime.o
ARRAY_RUNTIME_SRC = $(RUNTIME_DIR)/array.c
ARRAY_RUNTIME_OBJ = $(BUILD_DIR)/array_runtime.o
BUILTINS_RUNTIME_SRC = $(RUNTIME_DIR)/builtins.c
BUILTINS_RUNTIME_OBJ = $(BUILD_DIR)/builtins_runtime.o
CHANNELS_RUNTIME_SRC = $(RUNTIME_DIR)/channels.c
CHANNELS_RUNTIME_OBJ = $(BUILD_DIR)/channels_runtime.o

# Default target - build unified wyn binary
.PHONY: all
all: wyn runtime

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build unified Wyn binary
.PHONY: wyn
wyn: $(BUILD_DIR) $(WYN_BIN)

$(WYN_BIN): $(WYN_CLI_SRC) $(WYN_COMPILER_SRC)
	@echo "Building unified wyn binary..."
	$(CC) $(CFLAGS) -DCOMPILER_MAIN -o $@ $(WYN_CLI_SRC) $(WYN_COMPILER_SRC)
	@echo "✓ Built: $(WYN_BIN)"

# Legacy targets (for compatibility during transition)
.PHONY: wync repl
wync: wyn
	@echo "Note: 'wync' is now 'wyn compile'"
repl: wyn
	@echo "Note: 'wyn-repl' is now 'wyn repl'"

$(REPL_BIN): $(REPL_SRC)
	$(CC) $(CFLAGS) -o $@ $<

# Build runtime libraries
.PHONY: runtime
runtime: $(BUILD_DIR) $(SPAWN_RUNTIME_OBJ) $(ARRAY_RUNTIME_OBJ) $(BUILTINS_RUNTIME_OBJ) $(CHANNELS_RUNTIME_OBJ)

$(SPAWN_RUNTIME_OBJ): $(SPAWN_RUNTIME_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(ARRAY_RUNTIME_OBJ): $(ARRAY_RUNTIME_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILTINS_RUNTIME_OBJ): $(BUILTINS_RUNTIME_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(CHANNELS_RUNTIME_OBJ): $(CHANNELS_RUNTIME_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

# Debug build
.PHONY: wyn-debug
wyn-debug: $(BUILD_DIR)
	$(CC) $(DEBUG_FLAGS) -DCOMPILER_MAIN -o $(WYN_BIN) $(WYN_CLI_SRC) $(WYN_COMPILER_SRC)

# Test all examples
.PHONY: test-examples
test-examples: wyn
	@echo "Testing examples..."
	@passed=0; failed=0; \
	for f in examples/*.wyn; do \
		echo "  Testing $$f..."; \
		if ./$(WYN_BIN) compile $$f -o /tmp/wyn_test 2>&1 | grep -q "Compiled to:" && timeout 2 /tmp/wyn_test > /dev/null 2>&1; then \
			echo "    ✅ PASS"; \
			passed=$$((passed + 1)); \
		else \
			echo "    ❌ FAIL"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo "Examples: $$passed passed, $$failed failed"

# Test all test files
.PHONY: test-all-tests
test-all-tests: wyn
	@echo "Testing test files..."
	@passed=0; failed=0; \
	for f in tests/*_test.wyn; do \
		[ -f "$$f" ] || continue; \
		echo "  Testing $$f..."; \
		if ./$(WYN_BIN) compile $$f -o /tmp/wyn_test 2>&1 | grep -q "Compiled to:" && timeout 5 /tmp/wyn_test > /dev/null 2>&1; then \
			echo "    ✅ PASS"; \
			passed=$$((passed + 1)); \
		else \
			echo "    ❌ FAIL"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo "Tests: $$passed passed, $$failed failed"

# Run all tests (sequential, reliable)
.PHONY: test
test: test-examples test-all-tests
	@echo "✅ Test suite complete!"

# Run tests in parallel (8x faster, uses bash)
.PHONY: test-parallel
test-parallel: wyn
	@./scripts/parallel_test.sh

# Install to system
.PHONY: install
install: all
	@echo "Installing Wyn to $(PREFIX)/bin..."
	install -m 755 $(WYN_BIN) $(PREFIX)/bin/wyn
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
	@echo "Wyn $(VERSION) - Production Compiler"

# Help
.PHONY: help
help:
	@echo "Wyn Language Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all                 - Build unified wyn binary and runtime (default)"
	@echo "  wyn                 - Build unified wyn binary"
	@echo "  runtime             - Build runtime libraries"
	@echo "  test                - Run all tests"
	@echo "  test-examples       - Test examples"
	@echo "  test-parallel       - Run tests in parallel (faster)"
	@echo "  install             - Install to system"
	@echo "  uninstall           - Uninstall from system"
	@echo "  clean               - Remove build artifacts"
	@echo "  rebuild             - Clean and rebuild"
	@echo "  version             - Show version"
	@echo "  help                - Show this help"
	@echo ""
	@echo "Wyn commands:"
	@echo "  ./build/wyn compile <file>   - Compile a file"
	@echo "  ./build/wyn run <file>       - Compile and run"
	@echo "  ./build/wyn repl             - Start REPL"
	@echo "  ./build/wyn doc [module]     - Show docs"
	@echo "  ./build/wyn fmt <file>       - Format/validate"
	@echo "  ./build/wyn lsp              - Start LSP server"
	@echo "  ./build/wyn pkg <cmd>        - Package manager"

.DEFAULT_GOAL := all
