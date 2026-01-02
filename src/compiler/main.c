int main(int argc, char** argv) {
#endif
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* input_file = NULL;
    const char* output_file = "a.out";
    bool compile_only = false;
    bool emit_asm = false;
    bool emit_ir = false;
    bool quiet = true;  // Default to quiet mode for clean output
    int opt_level = 0;
    bool stage1_opt = false;  // Enable Stage 1 optimizations
    bool use_stage1_tc = false;  // Use Stage 1 type checker
    bool no_bounds_check = false;  // Disable all bounds checks
    Arch target_arch = ARCH_ARM64;  // Default for macOS ARM
    TargetOS target_os = OS_MACOS;  // Default OS
    
    // Detect host architecture and OS
#if defined(__x86_64__) || defined(_M_X64)
    target_arch = ARCH_X86_64;
#endif
#if defined(__linux__)
    target_os = OS_LINUX;
#endif
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("wyn 0.1.0\n");
            return 0;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0) {
            compile_only = true;
        } else if (strcmp(argv[i], "-S") == 0) {
            emit_asm = true;
        } else if (strcmp(argv[i], "--emit-ir") == 0) {
            emit_ir = true;
        } else if (strcmp(argv[i], "-O0") == 0) {
            opt_level = 0;
        } else if (strcmp(argv[i], "-O1") == 0) {
            opt_level = 1;
        } else if (strcmp(argv[i], "-O2") == 0) {
            opt_level = 2;
        } else if (strcmp(argv[i], "--stage1") == 0) {
            stage1_opt = true;
        } else if (strcmp(argv[i], "--stage1-opt") == 0) {
            stage1_opt = true;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            quiet = false;
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            quiet = true;
        } else if (strcmp(argv[i], "--stage1-tc") == 0) {
            use_stage1_tc = true;
        } else if (strcmp(argv[i], "--stage1-opt") == 0) {
            stage1_opt = true;
        } else if (strcmp(argv[i], "--no-bounds-check") == 0) {
            no_bounds_check = true;
        } else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "arm64") == 0) target_arch = ARCH_ARM64;
            else if (strcmp(argv[i], "x86_64") == 0) target_arch = ARCH_X86_64;
            else { fprintf(stderr, "unknown target: %s\n", argv[i]); return 1; }
        } else if (strcmp(argv[i], "--target-os") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "macos") == 0 || strcmp(argv[i], "darwin") == 0) target_os = OS_MACOS;
            else if (strcmp(argv[i], "linux") == 0) target_os = OS_LINUX;
            else { fprintf(stderr, "unknown target OS: %s\n", argv[i]); return 1; }
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        } else {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            return 1;
        }
    }
    
    if (!input_file) {
        fprintf(stderr, "no input file\n");
        return 1;
    }
    
    if (!quiet) {
        printf("Wyn Bootstrap Compiler (Stage 0)\n");
        printf("Compiling: %s\n", input_file);
    }
    
    char* source = read_file(input_file);
    Lexer* lexer = lexer_new(source, input_file);
    
    // Parse the source
    Parser* parser = parser_new(lexer, input_file);
    Module* module = parse_module(parser);
    
    if (parser->had_error) {
        fprintf(stderr, "Parsing failed.\n");
        return 1;
    }
    
    // Resolve imports
    resolve_imports(module, input_file);
    
    // Print AST summary
    if (!quiet) print_module(module);
    
    // Type check
    TypeChecker* tc = NULL;
    if (use_stage1_tc) {
        if (!quiet) printf("Using Stage 1 type checker...\n");
        TypeChecker1* tc1 = tc1_new(module, input_file);
        if (!typecheck_module_stage1(tc1)) {
            fprintf(stderr, "Type checking failed.\n");
            return 1;
        }
        if (!quiet) printf("\nType checking passed.\n");
        
        // Stage 1 optimizations
        if (stage1_opt) {
            if (!quiet) printf("Running Stage 1 optimizations...\n");
            Optimizer* opt = opt_new(module);
            optimize_module(opt);
            optimize_dead_code(module);
            inline_pass(module);
            optimize_bounds_checks(module, no_bounds_check);
        } else if (no_bounds_check) {
            // Apply bounds check elimination even without other optimizations
            optimize_bounds_checks(module, true);
        }
        
        // Create dummy TypeChecker for codegen compatibility
        tc = typechecker_new(module, input_file);
    } else {
        tc = typechecker_new(module, input_file);
        if (!typecheck_module(tc)) {
            fprintf(stderr, "Type checking failed.\n");
            return 1;
        }
        if (!quiet) printf("\nType checking passed.\n");
        
        // Stage 1 optimizations (can run with standard type checker too)
        if (stage1_opt) {
            if (!quiet) printf("Running Stage 1 optimizations...\n");
            Optimizer* opt = opt_new(module);
            optimize_module(opt);
            optimize_dead_code(module);
            inline_pass(module);
            optimize_bounds_checks(module, no_bounds_check);
        } else if (no_bounds_check) {
            // Apply bounds check elimination even without other optimizations
            optimize_bounds_checks(module, true);
        }
    }
    
    // Analyze spawn variable captures
    analyze_spawn_captures(module);
    
    // Code generation - LLVM Backend
    char ir_file[256];
    snprintf(ir_file, 256, "/tmp/wyn_%d_%d.ll", getpid(), rand());
    FILE* ir_out = fopen(ir_file, "w");
    if (!ir_out) {
        fprintf(stderr, "Cannot open %s for writing\n", ir_file);
        return 1;
    }
    
    // Generate LLVM IR
    llvm_generate(ir_out, module, target_arch, target_os);
    fclose(ir_out);
    
    if (!quiet) printf("LLVM IR written to: %s\n", ir_file);
    
    // Compile LLVM IR to object file
    char obj_file[256];
    snprintf(obj_file, 256, "/tmp/wyn_%d_%d.o", getpid(), rand());
    char cmd[512];
    
    // Try to find llc in various locations
    const char* llc_path = "llc";  // Default: use PATH
    if (access("/opt/homebrew/opt/llvm/bin/llc", X_OK) == 0) {
        llc_path = "/opt/homebrew/opt/llvm/bin/llc";
    } else if (getenv("LLVM_PATH")) {
        static char llc_from_env[512];
        snprintf(llc_from_env, 512, "%s/bin/llc", getenv("LLVM_PATH"));
        if (access(llc_from_env, X_OK) == 0) {
            llc_path = llc_from_env;
        }
    }
    
    snprintf(cmd, 512, "%s -relocation-model=pic -filetype=obj %s -o %s", llc_path, ir_file, obj_file);
    if (system(cmd) != 0) {
        fprintf(stderr, "LLVM compilation failed\n");
        return 1;
    }
    
    // Find runtime libraries
    char runtime_path[512] = "build";
    if (access("build/builtins_runtime.o", F_OK) != 0) {
        // Try installed location
        #ifdef __APPLE__
        strcpy(runtime_path, "/usr/local/share/wyn/runtime");
        #else
        strcpy(runtime_path, "/usr/local/share/wyn/runtime");
        #endif
        
        // Check if installed location exists
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s/builtins_runtime.o", runtime_path);
        if (access(test_path, F_OK) != 0) {
            // Fallback to build directory
            strcpy(runtime_path, "build");
        }
    }
    
    // Link
    if (!compile_only) {
        char cmd[2048];  // Increased buffer size for long paths
        snprintf(cmd, sizeof(cmd), "clang -fPIE %s %s/builtins_runtime.o %s/array_runtime.o %s/spawn_runtime.o %s/channels_runtime.o %s/task_runtime.o -lpthread -lm -o %s", 
                 obj_file, runtime_path, runtime_path, runtime_path, runtime_path, runtime_path, output_file);
        if (system(cmd) != 0) {
            fprintf(stderr, "Linking failed\n");
            return 1;
        }
        if (!quiet) printf("Compiled to: %s\n", output_file);
    }
    
    // Cleanup
    // unlink(ir_file);
    if (!compile_only) unlink(obj_file);
    
    parser_free(parser);
    lexer_free(lexer);
    free(source);
    
    return 0;
}
