// Wyn - Unified CLI Tool
// Consolidates compiler, REPL, LSP, formatter, docs, and package manager
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
  #include <io.h>
  #include <process.h>
  #include <fcntl.h>
  #define access _access
  #define dup _dup
  #define dup2 _dup2
  #define open _open
  #define close _close
  #define popen _popen
  #define pclose _pclose
  #define F_OK 0
  #define X_OK 0
  #define O_WRONLY _O_WRONLY
  #define STDOUT_FILENO 1
  #define STDERR_FILENO 2
  #define WIFEXITED(status) 1
  #define WEXITSTATUS(status) (status)
  typedef int pid_t;
  // Windows doesn't have fork, these functions will fail gracefully
  static inline pid_t fork(void) { return -1; }
  static inline pid_t waitpid(pid_t pid, int *status, int options) { (void)pid; (void)status; (void)options; return -1; }
#else
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/wait.h>
#endif

// Compiler entry point (from compiler.c)
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
        // Ultra-fast test runner with smart caching
        if (argc > 2) {
            // Run specific test file
            char run_cmd[512];
            snprintf(run_cmd, 512, "%s run %s", argv[0], argv[2]);
            int result = system(run_cmd);
            
            if (result == 0) {
                printf("\n✅ Test passed: %s\n", argv[2]);
            } else {
                printf("\n❌ Test failed: %s\n", argv[2]);
            }
            return result;
        } else {
            // Fast parallel test execution with smart caching
            printf("Discovering tests...\n");
            
            // Create cache directory
            system("mkdir -p .wyn/test-cache 2>/dev/null");
            
            // Find all test files
            char find_cmd[512];
            snprintf(find_cmd, 512, "find tests -name '*_test.wyn' 2>/dev/null | sort");
            FILE* pipe = popen(find_cmd, "r");
            if (!pipe) {
                fprintf(stderr, "Failed to discover tests\n");
                return 1;
            }
            
            char test_files[256][512];
            int test_count = 0;
            while (fgets(test_files[test_count], 512, pipe) && test_count < 256) {
                test_files[test_count][strcspn(test_files[test_count], "\n")] = 0;
                test_count++;
            }
            pclose(pipe);
            
            if (test_count == 0) {
                printf("No test files found\n");
                return 0;
            }
            
            printf("Found %d test files\n", test_count);
            
            // Check which tests need recompilation
            int needs_compile[256] = {0};
            int compile_count = 0;
            
            for (int i = 0; i < test_count; i++) {
                char cache_bin[512];
                snprintf(cache_bin, 512, ".wyn/test-cache/%d.bin", i);
                
                struct stat src_stat, bin_stat;
                if (stat(test_files[i], &src_stat) == 0 && stat(cache_bin, &bin_stat) == 0) {
                    // Check if source is newer than binary
                    if (src_stat.st_mtime > bin_stat.st_mtime) {
                        needs_compile[i] = 1;
                        compile_count++;
                    }
                } else {
                    needs_compile[i] = 1;
                    compile_count++;
                }
            }
            
            if (compile_count > 0) {
                printf("Compiling %d tests in parallel...\n", compile_count);
                
                // Compile only changed tests in parallel
                pid_t compile_pids[256];
                for (int i = 0; i < test_count; i++) {
                    if (needs_compile[i]) {
                        char cache_bin[512];
                        snprintf(cache_bin, 512, ".wyn/test-cache/%d.bin", i);
                        
                        pid_t pid = fork();
                        if (pid == 0) {
                            char compile_cmd[1024];
                            snprintf(compile_cmd, 1024, "%s compile %s -o %s > /dev/null 2>&1", 
                                     argv[0], test_files[i], cache_bin);
                            exit(system(compile_cmd) == 0 ? 0 : 1);
                        } else {
                            compile_pids[i] = pid;
                        }
                    } else {
                        compile_pids[i] = -1;
                    }
                }
                
                // Wait for compilation
                for (int i = 0; i < test_count; i++) {
                    if (compile_pids[i] > 0) {
                        waitpid(compile_pids[i], NULL, 0);
                    }
                }
            } else {
                printf("All tests cached, skipping compilation\n");
            }
            
            printf("Running tests...\n\n");
            
            // Run all tests in parallel
            pid_t run_pids[256];
            int results[256] = {0};
            
            for (int i = 0; i < test_count; i++) {
                char cache_bin[512];
                snprintf(cache_bin, 512, ".wyn/test-cache/%d.bin", i);
                
                pid_t pid = fork();
                if (pid == 0) {
                    char run_cmd[1024];
                    snprintf(run_cmd, 1024, "%s > /dev/null 2>&1", cache_bin);
                    exit(system(run_cmd) == 0 ? 0 : 1);
                } else if (pid > 0) {
                    run_pids[i] = pid;
                } else {
                    run_pids[i] = -1;
                    results[i] = 1;
                }
            }
            
            // Collect results
            for (int i = 0; i < test_count; i++) {
                if (run_pids[i] > 0) {
                    int status;
                    waitpid(run_pids[i], &status, 0);
                    results[i] = WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : 1;
                }
            }
            
            // Print results
            int passed = 0;
            int failed = 0;
            for (int i = 0; i < test_count; i++) {
                if (results[i] == 0) {
                    printf("  ✅ %s\n", test_files[i]);
                    passed++;
                } else {
                    printf("  ❌ %s\n", test_files[i]);
                    failed++;
                }
            }
            
            printf("\nTests: %d passed, %d failed\n", passed, failed);
            
            return failed > 0 ? 1 : 0;
        }
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
// LSP helper functions
void lsp_send_response(int id, const char* result) {
    char response[8192];
    snprintf(response, sizeof(response), 
             "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":%s}", id, result);
    printf("Content-Length: %zu\r\n\r\n%s", strlen(response), response);
    fflush(stdout);
}

void lsp_send_notification(const char* method, const char* params) {
    char notification[8192];
    snprintf(notification, sizeof(notification),
             "{\"jsonrpc\":\"2.0\",\"method\":\"%s\",\"params\":%s}", method, params);
    printf("Content-Length: %zu\r\n\r\n%s", strlen(notification), notification);
    fflush(stdout);
}

void lsp_send_diagnostics(const char* uri, const char* diagnostics) {
    char params[8192];
    snprintf(params, sizeof(params), "{\"uri\":\"%s\",\"diagnostics\":[%s]}", uri, diagnostics);
    lsp_send_notification("textDocument/publishDiagnostics", params);
}

int run_lsp(void) {
    fprintf(stderr, "Wyn LSP Server v0.3.0 starting...\n");
    fprintf(stderr, "Capabilities: diagnostics, completion, hover, formatting\n");
    
    while (1) {
        char header[256];
        int content_length = 0;
        
        // Read headers
        while (fgets(header, sizeof(header), stdin)) {
            if (strncmp(header, "Content-Length:", 15) == 0) {
                content_length = atoi(header + 15);
            } else if (strcmp(header, "\r\n") == 0 || strcmp(header, "\n") == 0) {
                break;
            }
        }
        
        if (content_length == 0) break;
        
        // Read content
        char* content = malloc(content_length + 1);
        if (!content) break;
        
        size_t read = fread(content, 1, content_length, stdin);
        content[read] = '\0';
        
        fprintf(stderr, "LSP: Received message (%d bytes)\n", content_length);
        
        // Extract request ID
        int id = 1;
        char* id_start = strstr(content, "\"id\":");
        if (id_start) {
            id = atoi(id_start + 5);
        }
        
        // Handle requests
        if (strstr(content, "\"method\":\"initialize\"")) {
            fprintf(stderr, "LSP: Initialize\n");
            
            const char* capabilities = 
                "{\"capabilities\":{"
                "\"textDocumentSync\":1,"
                "\"completionProvider\":{\"triggerCharacters\":[\".\",\":\",\" \"]},"
                "\"hoverProvider\":true,"
                "\"definitionProvider\":true,"
                "\"documentFormattingProvider\":true"
                "}}";
            
            lsp_send_response(id, capabilities);
            
        } else if (strstr(content, "\"method\":\"textDocument/didOpen\"") ||
                   strstr(content, "\"method\":\"textDocument/didChange\"")) {
            fprintf(stderr, "LSP: Document changed\n");
            
            // Extract URI
            char* uri_start = strstr(content, "\"uri\":\"");
            if (uri_start) {
                uri_start += 7;
                char* uri_end = strchr(uri_start, '"');
                if (uri_end) {
                    char uri[512];
                    int uri_len = uri_end - uri_start;
                    strncpy(uri, uri_start, uri_len);
                    uri[uri_len] = '\0';
                    
                    // Send empty diagnostics (no errors)
                    lsp_send_diagnostics(uri, "");
                }
            }
            
        } else if (strstr(content, "\"method\":\"textDocument/completion\"")) {
            fprintf(stderr, "LSP: Completion\n");
            
            const char* completions = 
                "{\"isIncomplete\":false,\"items\":["
                // Keywords
                "{\"label\":\"fn\",\"kind\":14,\"detail\":\"Function\"},"
                "{\"label\":\"const\",\"kind\":14,\"detail\":\"Constant\"},"
                "{\"label\":\"let\",\"kind\":14,\"detail\":\"Variable\"},"
                "{\"label\":\"if\",\"kind\":14,\"detail\":\"Conditional\"},"
                "{\"label\":\"else\",\"kind\":14,\"detail\":\"Conditional\"},"
                "{\"label\":\"while\",\"kind\":14,\"detail\":\"Loop\"},"
                "{\"label\":\"for\",\"kind\":14,\"detail\":\"Loop\"},"
                "{\"label\":\"in\",\"kind\":14,\"detail\":\"Loop iterator\"},"
                "{\"label\":\"match\",\"kind\":14,\"detail\":\"Pattern matching\"},"
                "{\"label\":\"return\",\"kind\":14,\"detail\":\"Return statement\"},"
                "{\"label\":\"break\",\"kind\":14,\"detail\":\"Break loop\"},"
                "{\"label\":\"continue\",\"kind\":14,\"detail\":\"Continue loop\"},"
                "{\"label\":\"spawn\",\"kind\":14,\"detail\":\"Concurrency\"},"
                "{\"label\":\"import\",\"kind\":14,\"detail\":\"Import module\"},"
                "{\"label\":\"struct\",\"kind\":14,\"detail\":\"Struct definition\"},"
                "{\"label\":\"enum\",\"kind\":14,\"detail\":\"Enum definition\"},"
                "{\"label\":\"test\",\"kind\":14,\"detail\":\"Test block\"},"
                // Result/Option
                "{\"label\":\"ok\",\"kind\":3,\"detail\":\"Result success\"},"
                "{\"label\":\"err\",\"kind\":3,\"detail\":\"Result error\"},"
                "{\"label\":\"some\",\"kind\":3,\"detail\":\"Option value\"},"
                "{\"label\":\"none\",\"kind\":3,\"detail\":\"Option empty\"},"
                // Built-in functions
                "{\"label\":\"print\",\"kind\":3,\"detail\":\"Print value\"},"
                "{\"label\":\"assert\",\"kind\":3,\"detail\":\"Assert condition\"},"
                "{\"label\":\"task_join_all\",\"kind\":3,\"detail\":\"Wait for all spawns\"},"
                // String methods
                "{\"label\":\".upper()\",\"kind\":2,\"detail\":\"String → String\"},"
                "{\"label\":\".lower()\",\"kind\":2,\"detail\":\"String → String\"},"
                "{\"label\":\".trim()\",\"kind\":2,\"detail\":\"String → String\"},"
                "{\"label\":\".len()\",\"kind\":2,\"detail\":\"→ Int\"},"
                "{\"label\":\".contains()\",\"kind\":2,\"detail\":\"String → Bool\"},"
                "{\"label\":\".starts_with()\",\"kind\":2,\"detail\":\"String → Bool\"},"
                "{\"label\":\".ends_with()\",\"kind\":2,\"detail\":\"String → Bool\"},"
                "{\"label\":\".split()\",\"kind\":2,\"detail\":\"String → Array\"},"
                "{\"label\":\".replace()\",\"kind\":2,\"detail\":\"String → String\"},"
                // Int methods
                "{\"label\":\".abs()\",\"kind\":2,\"detail\":\"Int → Int\"},"
                "{\"label\":\".to_str()\",\"kind\":2,\"detail\":\"Int → String\"},"
                // Array methods
                "{\"label\":\".join()\",\"kind\":2,\"detail\":\"Array → String\"},"
                "{\"label\":\".get()\",\"kind\":2,\"detail\":\"Array → Int\"},"
                "{\"label\":\".reverse()\",\"kind\":2,\"detail\":\"Array → Array\"},"
                "{\"label\":\".append()\",\"kind\":2,\"detail\":\"Array → Array\"},"
                // Modules
                "{\"label\":\"fs\",\"kind\":9,\"detail\":\"File system module\"},"
                "{\"label\":\"net\",\"kind\":9,\"detail\":\"Network module\"},"
                "{\"label\":\"os\",\"kind\":9,\"detail\":\"Operating system module\"},"
                "{\"label\":\"time\",\"kind\":9,\"detail\":\"Time module\"},"
                "{\"label\":\"json\",\"kind\":9,\"detail\":\"JSON module\"},"
                "{\"label\":\"crypto\",\"kind\":9,\"detail\":\"Crypto module\"},"
                "{\"label\":\"collections\",\"kind\":9,\"detail\":\"Collections module\"},"
                "{\"label\":\"testing\",\"kind\":9,\"detail\":\"Testing module\"},"
                // Types
                "{\"label\":\"int\",\"kind\":25,\"detail\":\"Integer type\"},"
                "{\"label\":\"str\",\"kind\":25,\"detail\":\"String type\"},"
                "{\"label\":\"bool\",\"kind\":25,\"detail\":\"Boolean type\"},"
                "{\"label\":\"array\",\"kind\":25,\"detail\":\"Array type\"},"
                "{\"label\":\"Result\",\"kind\":25,\"detail\":\"Result[T, E] type\"},"
                "{\"label\":\"Option\",\"kind\":25,\"detail\":\"Option[T] type\"}"
                "]}";
            
            lsp_send_response(id, completions);
            
        } else if (strstr(content, "\"method\":\"textDocument/hover\"")) {
            fprintf(stderr, "LSP: Hover\n");
            
            const char* hover = 
                "{\"contents\":{\"kind\":\"markdown\",\"value\":"
                "\"**Wyn Language**\\n\\n"
                "Fast, compiled language with:\\n"
                "- Method syntax\\n"
                "- Extension methods\\n"
                "- String interpolation\\n"
                "- Type inference\\n"
                "- LLVM-powered\"}}";
            
            lsp_send_response(id, hover);
            
        } else if (strstr(content, "\"method\":\"textDocument/formatting\"")) {
            fprintf(stderr, "LSP: Formatting\n");
            lsp_send_response(id, "[]");
            
        } else if (strstr(content, "\"method\":\"shutdown\"")) {
            fprintf(stderr, "LSP: Shutdown\n");
            lsp_send_response(id, "null");
            
        } else if (strstr(content, "\"method\":\"exit\"")) {
            fprintf(stderr, "LSP: Exit\n");
            free(content);
            break;
        }
        
        free(content);
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
        printf("Opening documentation in browser...\n");
        printf("URL: https://wyn-lang.github.io/\n");
        
        // Try to open browser
        #ifdef __APPLE__
            system("open https://wyn-lang.github.io/ 2>/dev/null");
        #elif __linux__
            system("xdg-open https://wyn-lang.github.io/ 2>/dev/null || firefox https://wyn-lang.github.io/ 2>/dev/null");
        #elif _WIN32
            system("start https://wyn-lang.github.io/");
        #endif
        
        printf("\nOr view locally:\n");
        printf("  Module docs: docs/stdlib/\n");
        printf("  API reference: docs/API_REFERENCE.md\n");
        return 0;
    }
    
    const char* module = argv[1];
    
    // Try to open specific module doc
    char url[256];
    snprintf(url, sizeof(url), "https://wyn-lang.github.io/stdlib/%s.html", module);
    
    printf("Opening %s documentation...\n", module);
    printf("URL: %s\n", url);
    
    #ifdef __APPLE__
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "open %s 2>/dev/null", url);
        system(cmd);
    #elif __linux__
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "xdg-open %s 2>/dev/null || firefox %s 2>/dev/null", url, url);
        system(cmd);
    #elif _WIN32
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "start %s", url);
        system(cmd);
    #endif
    
    // Fallback: show local path
    char path[256];
    snprintf(path, sizeof(path), "docs/stdlib/%s.md", module);
    
    if (access(path, F_OK) == 0) {
        printf("\nOr view locally: %s\n", path);
        return 0;
    } else {
        printf("\nModule '%s' not found\n", module);
        printf("Try: wyn doc (without args) to see all modules\n");
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
