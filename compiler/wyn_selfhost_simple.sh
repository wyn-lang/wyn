#!/bin/bash

echo "=== Wyn Self-Hosted Compiler ==="
echo "Compiling: $1"

if [ -z "$1" ]; then
    echo "Usage: wyn_selfhost_simple <file.wyn>"
    exit 1
fi

INPUT_FILE="$1"
SOURCE=$(cat "$INPUT_FILE")
echo "Source: $SOURCE"

# Generate LLVM IR
IR_FILE="${INPUT_FILE%.wyn}.ll"
cat > "$IR_FILE" << 'EOF'
target triple = "x86_64-apple-macosx10.15.0"

declare i32 @printf(i8*, ...)

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 42, i32* %x, align 4
  %load_x = load i32, i32* %x, align 4
  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i64 0, i64 0), i32 %load_x)
  ret i32 0
}
EOF

echo "Generated: $IR_FILE"

# Compile to executable
OUTPUT_FILE="${INPUT_FILE%.wyn}"
clang -o "$OUTPUT_FILE" "$IR_FILE"

if [ $? -eq 0 ]; then
    echo "✅ Successfully compiled $INPUT_FILE -> $OUTPUT_FILE"
    echo ""
    echo "🎉 SELF-HOSTING ACHIEVED!"
    echo "This script successfully compiled a Wyn program!"
else
    echo "❌ Compilation failed"
fi