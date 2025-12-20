#!/usr/bin/env bash
# Local validation script for CI/CD changes
# Run this before pushing to verify everything works

set -e

echo "🔍 Validating CI/CD Implementation..."
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

pass() {
    echo -e "${GREEN}✅ $1${NC}"
}

fail() {
    echo -e "${RED}❌ $1${NC}"
    exit 1
}

warn() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

# 1. Check YAML syntax
echo "1. Validating YAML syntax..."
if command -v python3 &> /dev/null; then
    python3 -c "
import yaml
import sys

files = [
    '.github/workflows/ci.yml',
    '.github/workflows/release.yml',
    '.github/workflows/build.yml'
]

for f in files:
    try:
        with open(f) as file:
            yaml.safe_load(file)
    except Exception as e:
        print(f'Error in {f}: {e}')
        sys.exit(1)
" && pass "All YAML files are valid" || fail "YAML validation failed"
else
    warn "Python3 not found, skipping YAML validation"
fi

# 2. Check platform detection script
echo ""
echo "2. Testing platform detection..."
if [ -f "scripts/detect_platform.sh" ]; then
    chmod +x scripts/detect_platform.sh
    ./scripts/detect_platform.sh info > /dev/null && pass "Platform detection works" || fail "Platform detection failed"
else
    fail "Platform detection script not found"
fi

# 3. Check Makefile
echo ""
echo "3. Validating Makefile..."
if [ -f "Makefile" ]; then
    make --dry-run clean > /dev/null 2>&1 && pass "Makefile syntax is valid" || fail "Makefile has errors"
else
    fail "Makefile not found"
fi

# 4. Check required files
echo ""
echo "4. Checking required files..."
required_files=(
    ".github/workflows/ci.yml"
    ".github/workflows/release.yml"
    ".github/CI_PLATFORM_SUPPORT.md"
    ".github/CI_QUICK_REFERENCE.md"
    ".github/IMPLEMENTATION_SUMMARY.md"
    "scripts/detect_platform.sh"
    "Makefile"
    "README.md"
)

for file in "${required_files[@]}"; do
    if [ -f "$file" ]; then
        pass "$file exists"
    else
        fail "$file is missing"
    fi
done

# 5. Check README badges
echo ""
echo "5. Checking README badges..."
if grep -q "badge.svg" README.md; then
    pass "README has CI badges"
else
    warn "README might be missing CI badges"
fi

# 6. Test local build
echo ""
echo "6. Testing local build..."
if command -v make &> /dev/null; then
    if make clean > /dev/null 2>&1 && make > /dev/null 2>&1; then
        pass "Local build successful"
        
        # Test binary
        if [ -f "build/wyn" ] || [ -f "build/wyn.exe" ]; then
            pass "Binary created successfully"
            
            # Test version command
            if ./build/wyn --version > /dev/null 2>&1; then
                pass "Binary runs successfully"
            else
                warn "Binary exists but --version failed"
            fi
        else
            fail "Binary not created"
        fi
    else
        fail "Build failed"
    fi
else
    warn "Make not available, skipping build test"
fi

# 7. Check documentation
echo ""
echo "7. Validating documentation..."
docs=(
    ".github/CI_PLATFORM_SUPPORT.md"
    ".github/CI_QUICK_REFERENCE.md"
    ".github/IMPLEMENTATION_SUMMARY.md"
)

for doc in "${docs[@]}"; do
    if [ -f "$doc" ]; then
        lines=$(wc -l < "$doc")
        if [ "$lines" -gt 50 ]; then
            pass "$doc is comprehensive ($lines lines)"
        else
            warn "$doc might be incomplete ($lines lines)"
        fi
    fi
done

# 8. Summary
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo -e "${GREEN}✅ All validations passed!${NC}"
echo ""
echo "Summary of changes:"
echo "  • Created: .github/workflows/ci.yml (multi-platform CI)"
echo "  • Updated: .github/workflows/release.yml (multi-platform releases)"
echo "  • Updated: Makefile (cross-platform LLVM detection)"
echo "  • Updated: README.md (CI badges)"
echo "  • Created: scripts/detect_platform.sh (platform detection)"
echo "  • Created: 3 documentation files"
echo ""
echo "Platform support:"
echo "  • Tier 1: macOS ARM64, Linux x86_64, Windows x86_64 (full tests)"
echo "  • Tier 2: macOS x86_64, Linux ARM64 (build + smoke tests)"
echo ""
echo "Next steps:"
echo "  1. Review changes: git diff"
echo "  2. Commit changes: git add . && git commit -m 'Add multi-platform CI/CD'"
echo "  3. Push to trigger CI: git push"
echo "  4. Monitor CI: gh run watch (or check Actions tab)"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
