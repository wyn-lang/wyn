#!/bin/bash
# Simple Wyn code formatter

if [ $# -eq 0 ]; then
    echo "Usage: wyn_fmt.sh <file.wyn>"
    exit 1
fi

FILE="$1"

if [ ! -f "$FILE" ]; then
    echo "Error: File not found: $FILE"
    exit 1
fi

# Validate the file compiles
./build/wync "$FILE" -o /tmp/wyn_fmt_test >/dev/null 2>&1

if [ $? -eq 0 ]; then
    echo "✅ $FILE is valid Wyn code"
    rm -f /tmp/wyn_fmt_test
else
    echo "❌ $FILE has syntax errors:"
    ./build/wync "$FILE" 2>&1 | grep "Error\|error:" | head -5
    exit 1
fi

echo ""
echo "Note: Auto-formatting coming soon!"
echo "Current: Validates syntax only"
