# Wyn Language Build System
VERSION = 0.1.0

CC = cc
CFLAGS = -Wall -Wextra -std=c11 -O2
DEBUG_FLAGS = -g -O0 -DDEBUG

# Directories
BOOTSTRAP_DIR = bootstrap
BUILD_DIR = build
SRC_DIR = src
TESTS_DIR = tests
EXAMPLES_DIR = examples
PREFIX ?= /usr/local

# Bootstrap compiler (Stage 0)
STAGE0_SRC = $(BOOTSTRAP_DIR)/stage0.c
STAGE0_BIN = $(BUILD_DIR)/stage0

# Default target
.PHONY: all
all: stage0

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build Stage 0 (bootstrap compiler in C)
.PHONY: stage0
stage0: $(BUILD_DIR) $(STAGE0_BIN)

$(STAGE0_BIN): $(STAGE0_SRC)
	$(CC) $(CFLAGS) -o $@ $<

# Debug build of Stage 0
.PHONY: stage0-debug
stage0-debug: $(BUILD_DIR)
	$(CC) $(DEBUG_FLAGS) -o $(STAGE0_BIN) $(STAGE0_SRC)

# Run all tests
.PHONY: test
test: stage0
	@./$(TESTS_DIR)/run_tests.sh

# Run demo
.PHONY: demo
demo: stage0
	$(STAGE0_BIN) -o $(BUILD_DIR)/demo $(EXAMPLES_DIR)/demo.wyn
	./$(BUILD_DIR)/demo

# Install
.PHONY: install
install: stage0
	install -d $(PREFIX)/bin
	install -m 755 $(STAGE0_BIN) $(PREFIX)/bin/wyn

# Uninstall
.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/bin/wyn

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Full rebuild
.PHONY: rebuild
rebuild: clean all

# Show version
.PHONY: version
version:
	@echo "Wyn $(VERSION)"

# Help
.PHONY: help
help:
	@echo "Wyn Language Build System v$(VERSION)"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build Stage 0 compiler (default)"
	@echo "  stage0       - Build Stage 0 (C bootstrap compiler)"
	@echo "  stage0-debug - Build Stage 0 with debug symbols"
	@echo "  test         - Run all tests"
	@echo "  demo         - Build and run demo"
	@echo "  install      - Install to $(PREFIX)/bin"
	@echo "  uninstall    - Remove installed binary"
	@echo "  clean        - Remove build artifacts"
	@echo "  rebuild      - Clean and rebuild"
	@echo "  version      - Show version"
	@echo "  help         - Show this help"
