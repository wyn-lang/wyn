#!/bin/bash
set -e
cd /Users/aoaws/src/ao/wyn-lang/wyn
[ ! -f "src/lsp.c" ] || exit 1
[ ! -f "src/lsp_server.c" ] || exit 1
if grep -q "lsp\.c\|lsp_server\.c\|lsp_basic\.c" Makefile; then
    echo "ERROR: LSP files still referenced in Makefile"
    exit 1
fi
make clean && make wyn || exit 1
echo "✓ Task 1.7 Complete"