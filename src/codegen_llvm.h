#ifndef CODEGEN_LLVM_H
#define CODEGEN_LLVM_H

#include <stdio.h>

// Forward declarations
typedef struct Module Module;

// Architecture and OS enums
typedef enum {
    ARCH_X86_64,
    ARCH_ARM64
} Arch;

typedef enum {
    OS_MACOS,
    OS_LINUX
} TargetOS;

// Main LLVM code generation function
void llvm_generate(FILE* out, Module* m, Arch arch, TargetOS os);

#endif // CODEGEN_LLVM_H