// Wyn - Unified CLI Tool
// Consolidates compiler, REPL, LSP, formatter, docs, and package manager
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// Compiler entry point (from stage0.c)
extern int compiler_main(int argc, char** argv);

// Forward declarations for subcommands
int run_compiler(int argc, char** argv);
int run_repl(void);
int run_lsp(void);
int run_fmt(const char* file);
int run_doc(int argc, char** argv);
int run_pkg(int argc, char** argv);

void print_version(void) {
    printf("Wyn v0.2.0\n");
    printf("Fast, compiled systems language powered by LLVM\n");
}

void print_help(void) {
    printf("Wyn - Production-ready infrastructure language\n\n");
    printf("Usage: wyn <command> [options]\n\n");
    printf("Commands:\n");
    printf("  compile <file>       Compile a .wyn file\n");
    printf("  run <file>           Compile and run a file\n");
    printf("  -e <code>            Execute inline code\n");
    printf("  repl                 Start interactive REPL\n");
    printf("  fmt <file>           Format code (validate syntax)\n");
    printf("  doc [module]         Show documentation\n");
    printf("  lsp                  Start LSP server\n");
    printf("  pkg <cmd>            Package manager (install|list|update)\n");
    printf("  test                 Run test suite\n");
    printf("  version              Show version\n");
    printf("  help                 Show this help\n\n");
    printf("Examples:\n");
    printf("  wyn compile hello.wyn          # Compile to a.out\n");
    printf("  wyn compile -o app hello.wyn   # Compile to 'app'\n");
    printf("  wyn run hello.wyn              # Compile and run\n");
    printf("  wyn -e 'print(42)'             # Execute inline code\n");
    printf("  wyn repl                       # Interactive shell\n");
    printf("  wyn doc net                    # Show net module docs\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    const char* cmd = argv[1];

    if (strcmp(cmd, "compile") == 0) {
        // Shift args and call compiler
        return run_compiler(argc - 1, argv + 1);
    }
    else if (strcmp(cmd, "-e") == 0 || strcmp(cmd, "--eval") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: '-e' requires code argument\n");
            return 1;
        }
        // Execute inline code
        char temp_file[64];
        snprintf(temp_file, sizeof(temp_file), "/tmp/wyn_inline_%d.wyn", getpid());
        
        FILE* f = fopen(temp_file, "w");
        fprintf(f, "fn main() {\n    %s\n}\n", argv[2]);
        fclose(f);
        
        char temp_out[64];
        snprintf(temp_out, sizeof(temp_out), "/tmp/wyn_inline_out_%d", getpid());
        
        // Compile
        char* compile_argv[] = {"wyn", "compile", temp_file, "-o", temp_out, NULL};
        
        fflush(stdout);
        fflush(stderr);
        int saved_stdout = dup(STDOUT_FILENO);
        int saved_stderr = dup(STDERR_FILENO);
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        
        int result = compiler_main(5, compile_argv);
        
        fflush(stdout);
        fflush(stderr);
        dup2(saved_stdout, STDOUT_FILENO);
        dup2(saved_stderr, STDERR_FILENO);
        close(devnull);
        close(saved_stdout);
        close(saved_stderr);
        
        if (result == 0) {
            char run_cmd[256];
            snprintf(run_cmd, sizeof(run_cmd), "%s 2>&1 | grep -v '\\[Runtime\\]'", temp_out);
            system(run_cmd);
        }
        
        unlink(temp_file);
        unlink(temp_out);
        return result;
    }
    else if (strcmp(cmd, "run") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: 'run' requires a file argument\n");
            return 1;
        }
        // Compile to temp file and execute
        char temp_out[256];
        snprintf(temp_out, sizeof(temp_out), "/tmp/wyn_run_%d", getpid());
        
        // Build compile command
        char compile_cmd[1024];
        snprintf(compile_cmd, sizeof(compile_cmd), "%s compile -o %s %s 2>&1", 
                 argv[0], temp_out, argv[2]);
        
        int result = system(compile_cmd);
        if (result != 0) {
            return 1;
        }
        
        // Execute
        result = system(temp_out);
        unlink(temp_out);
        return result;
    }
    else if (strcmp(cmd, "repl") == 0) {
        return run_repl();
    }
    else if (strcmp(cmd, "lsp") == 0) {
        return run_lsp();
    }
    else if (strcmp(cmd, "fmt") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: 'fmt' requires a file argument\n");
            return 1;
        }
        return run_fmt(argv[2]);
    }
    else if (strcmp(cmd, "doc") == 0) {
        return run_doc(argc - 1, argv + 1);
    }
    else if (strcmp(cmd, "pkg") == 0) {
        return run_pkg(argc - 1, argv + 1);
    }
    else if (strcmp(cmd, "test") == 0) {
        printf("Running test suite...\n");
        return system("make test");
    }
    else if (strcmp(cmd, "version") == 0 || strcmp(cmd, "-v") == 0 || strcmp(cmd, "--version") == 0) {
        print_version();
        return 0;
    }
    else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0) {
        print_help();
        return 0;
    }
    else {
        fprintf(stderr, "Error: Unknown command '%s'\n\n", cmd);
        print_help();
        return 1;
    }
}

int run_compiler(int argc, char** argv) {
    return compiler_main(argc, argv);
}

// REPL implementation
int run_repl(void) {
    printf("Wyn REPL v0.2.0\n");
    printf("Type expressions to evaluate, 'exit' to quit\n\n");
    
    char line[1024];
    int stmt_count = 0;
    
    while (1) {
        printf("wyn> ");
        fflush(stdout);
        
        if (!fgets(line, sizeof(line), stdin)) break;
        
        line[strcspn(line, "\n")] = 0;
        
        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
            break;
        }
        
        if (strlen(line) == 0) continue;
        
        char filename[64];
        char outfile[64];
        snprintf(filename, sizeof(filename), "/tmp/wyn_repl_%d.wyn", stmt_count);
        snprintf(outfile, sizeof(outfile), "/tmp/wyn_repl_out_%d", stmt_count);
        stmt_count++;
        
        FILE* f = fopen(filename, "w");
        fprintf(f, "fn main() {\n    print(%s)\n}\n", line);
        fclose(f);
        
        // Call compiler directly (suppress all output)
        char* compile_argv[] = {"wyn", "compile", filename, "-o", outfile, NULL};
        
        // Redirect stdout/stderr to suppress compiler output
        fflush(stdout);
        fflush(stderr);
        int saved_stdout = dup(STDOUT_FILENO);
        int saved_stderr = dup(STDERR_FILENO);
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        
        int result = compiler_main(5, compile_argv);
        
        // Restore stdout/stderr
        fflush(stdout);
        fflush(stderr);
        dup2(saved_stdout, STDOUT_FILENO);
        dup2(saved_stderr, STDERR_FILENO);
        close(devnull);
        close(saved_stdout);
        close(saved_stderr);
        
        if (result == 0) {
            // Run and show only actual output
            char run_cmd[256];
            snprintf(run_cmd, sizeof(run_cmd), "%s 2>&1 | grep -v '\\[Runtime\\]' | grep -v '^0$'", outfile);
            system(run_cmd);
        }
        
        unlink(filename);
        unlink(outfile);
    }
    
    printf("\nGoodbye!\n");
    return 0;
}

// LSP server implementation
int run_lsp(void) {
    fprintf(stderr, "Wyn LSP Server v0.2.0 starting...\n");
    
    char line[8192];
    while (fgets(line, sizeof(line), stdin)) {
        if (strstr(line, "initialize")) {
            printf("Content-Length: 60\r\n\r\n");
            printf("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"capabilities\":{\"textDocumentSync\":1}}}\n");
            fflush(stdout);
        } else if (strstr(line, "shutdown")) {
            printf("Content-Length: 35\r\n\r\n");
            printf("{\"jsonrpc\":\"2.0\",\"id\":2,\"result\":null}\n");
            fflush(stdout);
        } else if (strstr(line, "exit")) {
            break;
        }
    }
    
    fprintf(stderr, "Wyn LSP Server stopped\n");
    return 0;
}

// Formatter (validates syntax)
int run_fmt(const char* file) {
    printf("Formatting %s...\n", file);
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "wyn compile %s -o /tmp/wyn_fmt_check 2>&1", file);
    
    int result = system(cmd);
    unlink("/tmp/wyn_fmt_check");
    
    if (result == 0) {
        printf("✓ %s is valid\n", file);
        return 0;
    } else {
        printf("✗ %s has syntax errors\n", file);
        return 1;
    }
}

// Documentation viewer
int run_doc(int argc, char** argv) {
    if (argc < 2) {
        printf("Wyn Standard Library\n\n");
        printf("Modules (99 functions):\n");
        printf("  io (13)          - File operations, directories\n");
        printf("  os (8)           - Process, environment, system\n");
        printf("  net (23)         - HTTP client/server, TCP, UDP\n");
        printf("  json (5)         - Parsing, stringify\n");
        printf("  time (7)         - Timestamps, formatting, sleep\n");
        printf("  log (5)          - Structured logging\n");
        printf("  encoding (6)     - Base64, hex, URL\n");
        printf("  hash (3)         - SHA256, MD5, SHA1\n");
        printf("  compress (7)     - Gzip, tar\n");
        printf("  regex (4)        - Pattern matching\n");
        printf("  collections (13) - HashMap, Set\n");
        printf("  crypto (5)       - AES-256, HMAC, random\n\n");
        printf("Usage: wyn doc <module>\n");
        printf("Full docs: docs/API_REFERENCE.md\n");
        return 0;
    }
    
    const char* module = argv[1];
    char path[256];
    snprintf(path, sizeof(path), "docs/stdlib/%s.md", module);
    
    if (access(path, F_OK) == 0) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "cat %s", path);
        return system(cmd);
    } else {
        printf("Module '%s' not found\n", module);
        printf("Try: wyn doc (without args) to see available modules\n");
        return 1;
    }
}

// Package manager (stub)
int run_pkg(int argc, char** argv) {
    if (argc < 2) {
        printf("Wyn Package Manager\n\n");
        printf("Commands:\n");
        printf("  wyn pkg install <package>  - Install a package\n");
        printf("  wyn pkg list               - List installed packages\n");
        printf("  wyn pkg update             - Update all packages\n");
        printf("  wyn pkg search <query>     - Search for packages\n\n");
        return 0;
    }
    
    const char* subcmd = argv[1];
    
    if (strcmp(subcmd, "list") == 0) {
        printf("Installed packages:\n");
        
        // Check if ~/.wyn/packages exists
        char pkg_dir[512];
        snprintf(pkg_dir, sizeof(pkg_dir), "%s/.wyn/packages", getenv("HOME"));
        
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "ls -1 %s 2>/dev/null || echo '  (none installed)'", pkg_dir);
        system(cmd);
        return 0;
    }
    else if (strcmp(subcmd, "install") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: 'install' requires a package name\n");
            fprintf(stderr, "Example: wyn pkg install github.com/user/package\n");
            return 1;
        }
        
        const char* package = argv[2];
        printf("Installing %s...\n", package);
        
        // Create ~/.wyn/packages directory
        char pkg_dir[512];
        snprintf(pkg_dir, sizeof(pkg_dir), "%s/.wyn/packages", getenv("HOME"));
        
        char mkdir_cmd[1024];
        snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", pkg_dir);
        system(mkdir_cmd);
        
        // Parse package URL (github.com/user/repo format)
        if (strncmp(package, "github.com/", 11) == 0) {
            // Extract user/repo
            const char* user_repo = package + 11;
            
            // Clone from GitHub
            char clone_cmd[2048];
            snprintf(clone_cmd, sizeof(clone_cmd), 
                    "cd %s && git clone --depth 1 https://%s.git 2>&1", 
                    pkg_dir, package);
            
            int result = system(clone_cmd);
            if (result == 0) {
                printf("✓ Installed %s\n", package);
                printf("  Location: %s/%s\n", pkg_dir, strrchr(user_repo, '/') + 1);
                return 0;
            } else {
                fprintf(stderr, "✗ Failed to install %s\n", package);
                return 1;
            }
        } else {
            fprintf(stderr, "Error: Package must be in format github.com/user/repo\n");
            return 1;
        }
    }
    else if (strcmp(subcmd, "update") == 0) {
        printf("Updating packages...\n");
        
        char pkg_dir[512];
        snprintf(pkg_dir, sizeof(pkg_dir), "%s/.wyn/packages", getenv("HOME"));
        
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), 
                "for dir in %s/*/; do echo \"Updating $(basename $dir)...\"; cd \"$dir\" && git pull; done",
                pkg_dir);
        system(cmd);
        return 0;
    }
    else if (strcmp(subcmd, "search") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: 'search' requires a query\n");
            return 1;
        }
        printf("Searching for '%s'...\n", argv[2]);
        printf("(Search not yet implemented - check github.com/topics/wyn-package)\n");
        return 0;
    }
    else {
        fprintf(stderr, "Error: Unknown pkg command '%s'\n", subcmd);
        return 1;
    }
}
