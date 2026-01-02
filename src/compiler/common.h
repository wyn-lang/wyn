/*
 * Wyn Language Compiler - LLVM Backend Only
 * 
 * Clean implementation: 8K lines (was 14K)
 * Removed: 6K lines of unused x86/ARM codegen
 */

/*
 * Wyn Language Bootstrap Compiler (Stage 0)
 * 
 * This is a minimal C compiler that can compile a subset of Wyn
 * sufficient to build the self-hosting compiler.
 * 
 * Supported subset:
 * - Basic types: int, str, bool
 * - Functions with parameters and return types
 * - Structs and enums
 * - Control flow: if/else, match, for, while
 * - Basic expressions and operators
 * - Simple generics (monomorphization)
 * 
 * Output: x86_64 assembly (AT&T syntax) -> assembled with system assembler
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

#ifdef _WIN32
  #include <io.h>
  #define access _access
  #define X_OK 0
#else
  #include <unistd.h>
#endif

#include "windows_compat.h"
#include "utils/arena.h"

#define MAX_TOKEN_LEN 256
#define MAX_IDENT_LEN 128
#define MAX_STRING_LEN 4096
#define MAX_PARAMS 32
#define MAX_THREADS 1024

// Thread pool for spawn
typedef struct {
    pthread_t threads[MAX_THREADS];
    int count;
} ThreadPool;

static ThreadPool g_thread_pool = {0};
#define MAX_FIELDS 64
#define MAX_VARIANTS 128
#define MAX_GENERICS 8

// Forward declarations
typedef struct Token Token;
typedef struct Lexer Lexer;
typedef struct Type Type;
typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct FnDef FnDef;
typedef struct StructDef StructDef;
typedef struct EnumDef EnumDef;
typedef struct Module Module;
