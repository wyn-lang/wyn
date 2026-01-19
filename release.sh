#!/bin/bash
# Create a new release

if [ -z "$1" ]; then
    echo "Usage: ./release.sh <version>"
    echo "Example: ./release.sh 1.0.1"
    exit 1
fi

VERSION=$1

echo "Creating release v$VERSION"

# Update VERSION file
echo "$VERSION" > VERSION

# Build main compiler
echo "Building compiler..."
make clean >/dev/null 2>&1
make wyn || exit 1

# Run module tests
echo "Running module tests..."
./tests/module_tests/test_modules.sh || exit 1
./tests/module_tests/test_module_system.sh || exit 1

# Test all examples compile
echo "Testing examples..."
FAIL=0
for f in examples/*.wyn; do
    ./wyn "$f" >/dev/null 2>&1 || FAIL=1
done
if [ $FAIL -eq 1 ]; then
    echo "❌ Some examples failed to compile"
    exit 1
fi
echo "✅ All examples compile"

# Test basic execution
echo "Testing basic execution..."
cat > /tmp/test_release.wyn << 'EOF'
fn main() -> int {
    return 42
}
EOF
./wyn /tmp/test_release.wyn >/dev/null 2>&1 || exit 1
/tmp/test_release.wyn.out >/dev/null 2>&1
if [ $? -eq 42 ]; then
    echo "✅ Basic execution works"
else
    echo "❌ Basic execution failed"
    exit 1
fi
rm -f /tmp/test_release.wyn /tmp/test_release.wyn.out /tmp/test_release.wyn.c

echo "✅ All tests passed!"

# Commit version bump
git add VERSION
git commit -m "Bump version to $VERSION"

# Create tag
git tag -a "v$VERSION" -m "Release v$VERSION"

echo "Release v$VERSION created"
echo "Push with: git push && git push --tags"
echo "GitHub Actions will build binaries for linux, macos, windows"
