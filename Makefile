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

# Runtime libraries
SPAWN_RUNTIME_SRC = $(RUNTIME_DIR)/spawn.c
SPAWN_RUNTIME_OBJ = $(BUILD_DIR)/spawn_runtime.o
ARRAY_RUNTIME_SRC = $(RUNTIME_DIR)/array.c
ARRAY_RUNTIME_OBJ = $(BUILD_DIR)/array_runtime.o
BUILTINS_RUNTIME_SRC = $(RUNTIME_DIR)/builtins.c
BUILTINS_RUNTIME_OBJ = $(BUILD_DIR)/builtins_runtime.o
CHANNELS_RUNTIME_SRC = $(RUNTIME_DIR)/channels.c
CHANNELS_RUNTIME_OBJ = $(BUILD_DIR)/channels_runtime.o

# Default target - build compiler and runtime
.PHONY: all
all: wync runtime

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build Wyn Compiler (wync)
.PHONY: wync
wync: $(BUILD_DIR) $(WYNC_BIN)

$(WYNC_BIN): $(WYNC_SRC)
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

# Debug build of wync
.PHONY: wync-debug
wync-debug: $(BUILD_DIR)
	$(CC) $(DEBUG_FLAGS) -o $(WYNC_BIN) $(WYNC_SRC)

# Test all examples
.PHONY: test-examples
test-examples: wync
	@echo "Testing examples..."
	@passed=0; failed=0; \
	for f in examples/*.wyn; do \
		echo "  Testing $$f..."; \
		if ./$(WYNC_BIN) $$f -o /tmp/wyn_test 2>&1 | grep -q "Compiled to:" && timeout 2 /tmp/wyn_test > /dev/null 2>&1; then \
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
test-all-tests: wync
	@echo "Testing test files..."
	@passed=0; failed=0; \
	for f in tests/*_test.wyn; do \
		[ -f "$$f" ] || continue; \
		echo "  Testing $$f..."; \
		if ./$(WYNC_BIN) $$f -o /tmp/wyn_test 2>&1 | grep -q "Compiled to:" && timeout 5 /tmp/wyn_test > /dev/null 2>&1; then \
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
test-parallel: wync
	@./scripts/parallel_test.sh

# Install to system
.PHONY: install
install: all
	@echo "Installing Wyn to $(PREFIX)/bin..."
	install -m 755 $(BUILD_DIR)/wync $(PREFIX)/bin/wync
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
	@echo "  all                 - Build compiler and runtime (default)"
	@echo "  wync                - Build Wyn compiler (LLVM backend)"
	@echo "  runtime             - Build runtime libraries"
	@echo "  test                - Run all tests"
	@echo "  test-examples       - Test core examples"
	@echo "  test-spawn          - Test spawn blocks"
	@echo "  test-match          - Test match statements"
	@echo "  install             - Install to system"
	@echo "  uninstall           - Uninstall from system"
	@echo "  clean               - Remove build artifacts"
	@echo "  rebuild             - Clean and rebuild"
	@echo "  version             - Show version"
	@echo "  help                - Show this help"

.DEFAULT_GOAL := all
