# Wyn Language Build System
VERSION = 0.1.0

CC = cc
CFLAGS = -Wall -Wextra -std=c11 -O2
DEBUG_FLAGS = -g -O0 -DDEBUG
GUI_CFLAGS = -framework Cocoa -framework CoreGraphics

# Detect OS
UNAME_S := $(shell uname -s)

# Directories
BOOTSTRAP_DIR = bootstrap
BUILD_DIR = build
RUNTIME_DIR = runtime
SRC_DIR = src
TESTS_DIR = tests
EXAMPLES_DIR = examples
PREFIX ?= /usr/local

# Bootstrap compiler (Stage 0)
STAGE0_SRC = $(BOOTSTRAP_DIR)/stage0.c
STAGE0_BIN = $(BUILD_DIR)/stage0

# Runtime libraries
GUI_RUNTIME_SRC = $(RUNTIME_DIR)/gui_macos.c
GUI_RUNTIME_OBJ = $(BUILD_DIR)/gui_runtime.o
GPU_RUNTIME_SRC = $(RUNTIME_DIR)/gpu_metal.m
GPU_RUNTIME_OBJ = $(BUILD_DIR)/gpu_runtime.o
VULKAN_RUNTIME_SRC = $(RUNTIME_DIR)/gpu_vulkan.c
VULKAN_RUNTIME_OBJ = $(BUILD_DIR)/gpu_vulkan.o

MOBILE_RUNTIME_SRC = $(RUNTIME_DIR)/mobile_ios.m
MOBILE_RUNTIME_OBJ = $(BUILD_DIR)/mobile_runtime.o

# Default target - platform specific
.PHONY: all
ifeq ($(UNAME_S),Darwin)
all: stage0 gui-runtime gpu-runtime vulkan-runtime wyn-cli
else
all: stage0 vulkan-runtime wyn-cli
endif

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build Stage 0 (bootstrap compiler in C)
.PHONY: stage0
stage0: $(BUILD_DIR) $(STAGE0_BIN)

$(STAGE0_BIN): $(STAGE0_SRC)
	$(CC) $(CFLAGS) -o $@ $<

# Build GUI runtime (macOS only)
.PHONY: gui-runtime
gui-runtime: $(BUILD_DIR) $(GUI_RUNTIME_OBJ)

$(GUI_RUNTIME_OBJ): $(GUI_RUNTIME_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

# Build GPU runtime (macOS only)
.PHONY: gpu-runtime
gpu-runtime: $(BUILD_DIR) $(GPU_RUNTIME_OBJ)

$(GPU_RUNTIME_OBJ): $(GPU_RUNTIME_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

# Build Vulkan runtime
.PHONY: vulkan-runtime
vulkan-runtime: $(BUILD_DIR) $(VULKAN_RUNTIME_OBJ)

$(VULKAN_RUNTIME_OBJ): $(VULKAN_RUNTIME_SRC)
	$(CC) $(CFLAGS) -I/opt/homebrew/include -c -o $@ $<

# Build Mobile runtime (iOS simulator only)
.PHONY: mobile-runtime
mobile-runtime: $(BUILD_DIR)
	@echo "Note: Mobile runtime requires iOS SDK. Use 'make mobile-runtime-ios' with iOS toolchain."

.PHONY: mobile-runtime-ios
mobile-runtime-ios: $(BUILD_DIR) $(MOBILE_RUNTIME_OBJ)

$(MOBILE_RUNTIME_OBJ): $(MOBILE_RUNTIME_SRC)
	xcrun -sdk iphonesimulator clang $(CFLAGS) -fobjc-arc -c -o $@ $@ -framework UIKit -framework Foundation

# Debug build of Stage 0
.PHONY: stage0-debug
stage0-debug: $(BUILD_DIR)
	$(CC) $(DEBUG_FLAGS) -o $(STAGE0_BIN) $(STAGE0_SRC)

# Build wyn CLI tool
.PHONY: wyn-cli
wyn-cli: stage0
	./$(STAGE0_BIN) --stage1-tc --stage1-opt -o $(BUILD_DIR)/wyn src/wyn_cli.wyn

# Install to system
.PHONY: install
install: all
	@echo "Installing Wyn to $(PREFIX)/bin..."
	install -m 755 $(BUILD_DIR)/stage0 $(PREFIX)/bin/wyn-compiler
	install -m 755 $(BUILD_DIR)/wyn $(PREFIX)/bin/wyn
	@echo "✓ Installation complete!"
	@echo ""
	@echo "Try it:"
	@echo "  wyn version"
	@echo "  wyn new myapp"
	@echo "  cd myapp && wyn run ."

# Uninstall from system
.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/bin/wyn-compiler
	rm -f $(PREFIX)/bin/wyn
	@echo "✓ Uninstalled"

# Run all tests
.PHONY: test
test: stage0
	@./$(STAGE0_BIN) --stage1-opt -o $(BUILD_DIR)/run_tests $(TESTS_DIR)/_run_tests.wyn
	@./$(BUILD_DIR)/run_tests || true

# Run benchmarks
.PHONY: bench
bench: stage0
	@./$(STAGE0_BIN) --stage1-opt -o $(BUILD_DIR)/run_bench benchmarks/_run_bench.wyn
	@./$(BUILD_DIR)/run_bench

# Fast test - suppress compiler output
.PHONY: fast-test
fast-test: stage0
	@STAGE0_QUIET=1 ./$(TESTS_DIR)/run_tests.sh

# Run demo
.PHONY: demo
demo: stage0
	$(STAGE0_BIN) -o $(BUILD_DIR)/demo $(EXAMPLES_DIR)/demo.wyn
	./$(BUILD_DIR)/demo

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Clean test cache
.PHONY: clean-cache
clean-cache:
	rm -rf .wyn/test-cache

# Full clean
.PHONY: distclean
distclean: clean clean-cache
	rm -rf .wyn

# Full rebuild
.PHONY: rebuild
rebuild: clean all

# Show version
.PHONY: version
version:
	@echo "Wyn $(VERSION)"

# Package manager commands (wrappers for future wyn pkg tool)
.PHONY: new init add
new:
	@echo "Usage: make new NAME=<project-name>"
	@test -n "$(NAME)" || (echo "Error: NAME required" && exit 1)
	@mkdir -p $(NAME)/src
	@echo '[package]\nname = "$(NAME)"\nversion = "0.1.0"\nlicense = "MIT"\n\n[build]\nentry = "src/main.wyn"' > $(NAME)/wyn.toml
	@echo 'fn main() {\n    print_str("Hello, $(NAME)!")\n}' > $(NAME)/src/main.wyn
	@echo 'build/\n.wyn/' > $(NAME)/.gitignore
	@echo "Created project: $(NAME)"

init:
	@test ! -f wyn.toml || (echo "Error: wyn.toml already exists" && exit 1)
	@echo '[package]\nname = "project"\nversion = "0.1.0"\nlicense = "MIT"\n\n[build]\nentry = "src/main.wyn"' > wyn.toml
	@mkdir -p src
	@echo "Initialized Wyn project"

# Help
.PHONY: help
help:
	@echo "Wyn Language Build System v$(VERSION)"
	@echo ""
	@echo "Build Targets:"
	@echo "  all          - Build Stage 0 compiler (default)"
	@echo "  stage0       - Build Stage 0 (C bootstrap compiler)"
	@echo "  stage0-debug - Build Stage 0 with debug symbols"
	@echo "  test         - Run all tests"
	@echo "  demo         - Build and run demo"
	@echo ""
	@echo "Project Management:"
	@echo "  new NAME=x   - Create new project named x"
	@echo "  init         - Initialize project in current dir"
	@echo ""
	@echo "Installation:"
	@echo "  install      - Install to $(PREFIX)/bin"
	@echo "  uninstall    - Remove installed binary"
	@echo "  clean        - Remove build artifacts"
	@echo "  rebuild      - Clean and rebuild"
	@echo ""
	@echo "Info:"
	@echo "  version      - Show version"
	@echo "  help         - Show this help"
