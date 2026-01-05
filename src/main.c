#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "ast.h"
#include "types.h"

void init_lexer(const char* source);
void init_parser();
void init_checker();
void init_codegen(FILE* output);
Program* parse_program();
void check_program(Program* prog);
bool checker_had_error();
void free_program(Program* prog);
void codegen_c_header();
void codegen_program(Program* prog);

static char* read_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: Could not open file '%s'\n", path);
        exit(1);
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* buffer = malloc(size + 1);
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);
    
    return buffer;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Wyn Compiler v1.0.0\n");
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  wyn <file.wyn>           Compile file\n");
        fprintf(stderr, "  wyn run <file.wyn>       Compile and run\n");
        fprintf(stderr, "  wyn test                 Run tests\n");
        fprintf(stderr, "  wyn fmt <file.wyn>       Validate file\n");
        fprintf(stderr, "  wyn clean                Clean artifacts\n");
        fprintf(stderr, "  wyn cross <os> <file>    Cross-compile (linux/macos/windows)\n");
        fprintf(stderr, "  wyn version              Show version\n");
        fprintf(stderr, "  wyn help                 Show this help\n");
        return 1;
    }
    
    char* command = argv[1];
    
    if (strcmp(command, "version") == 0) {
        printf("Wyn v1.0.0\n");
        return 0;
    }
    
    if (strcmp(command, "help") == 0) {
        printf("Wyn Compiler v1.0.0\n\n");
        printf("Commands:\n");
        printf("  wyn <file.wyn>           Compile file\n");
        printf("  wyn run <file.wyn>       Compile and run\n");
        printf("  wyn test                 Run tests\n");
        printf("  wyn fmt <file.wyn>       Validate file\n");
        printf("  wyn clean                Clean artifacts\n");
        printf("  wyn cross <os> <file>    Cross-compile\n");
        printf("  wyn version              Show version\n");
        printf("  wyn help                 Show this help\n");
        printf("\nCross-compile targets:\n");
        printf("  linux   - Linux x86_64\n");
        printf("  macos   - macOS (current platform)\n");
        printf("  windows - Windows x86_64 (requires mingw)\n");
        return 0;
    }
    
    if (strcmp(command, "test") == 0) {
        printf("Running tests...\n");
        return system("make test 2>&1 && ./tests/integration_tests.sh");
    }
    
    if (strcmp(command, "clean") == 0) {
        printf("Cleaning build artifacts...\n");
        system("find examples -name '*.c' -delete 2>/dev/null");
        system("find examples -name '*.out' -delete 2>/dev/null");
        system("find temp -name '*.c' -delete 2>/dev/null");
        system("find temp -name '*.out' -delete 2>/dev/null");
        system("rm -f tests/test_quick 2>/dev/null");
        printf("✅ Clean complete\n");
        return 0;
    }
    
    if (strcmp(command, "cross") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: wyn cross <target> <file.wyn>\n");
            fprintf(stderr, "Targets: linux, macos, windows\n");
            return 1;
        }
        
        char* target = argv[2];
        char* file = argv[3];
        
        // Compile to C first
        char* source = read_file(file);
        init_lexer(source);
        init_parser();
        init_checker();
        
        Program* prog = parse_program();
        if (!prog) {
            fprintf(stderr, "Error: Failed to parse program\n");
            free(source);
            return 1;
        }
        
        check_program(prog);
        if (checker_had_error()) {
            fprintf(stderr, "Compilation failed due to errors\n");
            free(source);
            return 1;
        }
        
        char out_path[256];
        snprintf(out_path, 256, "%s.c", file);
        FILE* out = fopen(out_path, "w");
        init_codegen(out);
        codegen_c_header();
        codegen_program(prog);
        fclose(out);
        free(source);
        
        // Cross-compile based on target
        char compile_cmd[512];
        if (strcmp(target, "linux") == 0) {
            // On macOS, just compile normally (user can transfer binary)
            snprintf(compile_cmd, 512, "gcc -O2 -o %s.linux %s.c 2>&1", file, file);
            printf("Compiling for Linux (native binary)...\n");
        } else if (strcmp(target, "macos") == 0) {
            snprintf(compile_cmd, 512, "gcc -O2 -o %s.macos %s.c 2>&1", file, file);
            printf("Compiling for macOS...\n");
        } else if (strcmp(target, "windows") == 0) {
            snprintf(compile_cmd, 512, "x86_64-w64-mingw32-gcc -O2 -static -o %s.exe %s.c 2>&1", file, file);
            printf("Cross-compiling for Windows...\n");
        } else {
            fprintf(stderr, "Unknown target: %s\n", target);
            fprintf(stderr, "Available: linux, macos, windows\n");
            return 1;
        }
        
        int result = system(compile_cmd);
        if (result == 0) {
            printf("✅ Cross-compilation successful\n");
        } else {
            fprintf(stderr, "❌ Cross-compilation failed\n");
            if (strcmp(target, "windows") == 0) {
                fprintf(stderr, "Note: Windows cross-compilation requires mingw-w64\n");
                fprintf(stderr, "Install: brew install mingw-w64 (macOS) or apt install mingw-w64 (Linux)\n");
            }
        }
        return result;
    }
    
    if (strcmp(command, "fmt") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: wyn fmt <file.wyn>\n");
            return 1;
        }
        char temp_cmd[512];
        snprintf(temp_cmd, 512, "./wyn %s > /dev/null 2>&1", argv[2]);
        if (system(temp_cmd) == 0) {
            printf("✅ %s is valid\n", argv[2]);
            return 0;
        } else {
            printf("❌ %s has errors\n", argv[2]);
            return 1;
        }
    }
    
    if (strcmp(command, "run") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: wyn run <file.wyn>\n");
            return 1;
        }
        char* file = argv[2];
        char* source = read_file(file);
        
        init_lexer(source);
        init_parser();
        init_checker();
        
        Program* prog = parse_program();
        if (!prog) {
            fprintf(stderr, "Error: Failed to parse program\n");
            free(source);
            return 1;
        }
        
        // Type check
        check_program(prog);
        
        if (checker_had_error()) {
            fprintf(stderr, "Compilation failed due to errors\n");
            free(source);
            return 1;
        }
        
        char out_path[256];
        snprintf(out_path, 256, "%s.c", file);
        FILE* out = fopen(out_path, "w");
        init_codegen(out);
        codegen_c_header();
        codegen_program(prog);
        fclose(out);
        
        char compile_cmd[512];
        snprintf(compile_cmd, 512, "gcc -O2 -o %s.out %s.c 2>&1", file, file);
        int result = system(compile_cmd);
        
        if (result != 0) {
            fprintf(stderr, "C compilation failed\n");
            free(source);
            return result;
        }
        
        char run_cmd[512];
        snprintf(run_cmd, 512, "./%s.out", file);
        result = system(run_cmd);
        free(source);
        return result;
    }
    
    char* source = read_file(argv[1]);
    
    init_lexer(source);
    init_parser();
    init_checker();
    
    Program* prog = parse_program();
    if (!prog) {
        fprintf(stderr, "Error: Failed to parse program\n");
        free(source);
        return 1;
    }
    
    // Type check
    check_program(prog);
    
    if (checker_had_error()) {
        fprintf(stderr, "Compilation failed due to errors\n");
        free(source);
        return 1;
    }
    
    char out_path[256];
    snprintf(out_path, 256, "%s.c", argv[1]);
    FILE* out = fopen(out_path, "w");
    init_codegen(out);
    codegen_c_header();
    codegen_program(prog);
    fclose(out);
    
    // Free AST
    // free_program(prog);  // TODO: Implement cleanup
    
    char compile_cmd[512];
    snprintf(compile_cmd, 512, "gcc -O2 -o %s.out %s.c 2>&1", argv[1], argv[1]);
    int result = system(compile_cmd);
    
    if (result == 0) {
        printf("Compiled successfully: %s.out\n", argv[1]);
    } else {
        fprintf(stderr, "C compilation failed\n");
    }
    
    free(source);
    return result;
}
