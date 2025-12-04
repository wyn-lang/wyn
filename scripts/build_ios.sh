#!/bin/bash
# Build Wyn app for iOS

set -e

if [ $# -lt 2 ]; then
    echo "Usage: $0 <input.wyn> <output_name>"
    echo "Example: $0 examples/mobile_ios_app.wyn MyApp"
    exit 1
fi

INPUT=$1
OUTPUT=$2
BUILD_DIR="build"
RUNTIME_DIR="runtime"

echo "Building Wyn iOS app: $OUTPUT"

# Step 1: Build stage0 compiler if needed
if [ ! -f "$BUILD_DIR/stage0" ]; then
    echo "Building stage0 compiler..."
    make stage0
fi

# Step 2: Build mobile runtime for iOS simulator
echo "Building mobile runtime..."
mkdir -p "$BUILD_DIR"
xcrun -sdk iphonesimulator clang -Wall -Wextra -std=c11 -O2 -fobjc-arc \
    -c -o "$BUILD_DIR/mobile_runtime.o" "$RUNTIME_DIR/mobile_ios.m" \
    2>&1 | grep -v "warning:"

# Step 3: Compile Wyn source to assembly
echo "Compiling $INPUT..."
./"$BUILD_DIR/stage0" -S -o "$BUILD_DIR/${OUTPUT}.s" "$INPUT"

# Step 4: Assemble to object file
echo "Assembling..."
xcrun -sdk iphonesimulator clang -c -o "$BUILD_DIR/${OUTPUT}.o" "$BUILD_DIR/${OUTPUT}.s"

# Step 5: Link with mobile runtime
echo "Linking..."
xcrun -sdk iphonesimulator clang -o "$BUILD_DIR/${OUTPUT}" \
    "$BUILD_DIR/${OUTPUT}.o" "$BUILD_DIR/mobile_runtime.o" \
    -framework UIKit -framework Foundation

echo "✓ Built: $BUILD_DIR/${OUTPUT}"
echo ""
echo "To run in iOS simulator:"
echo "  1. Create Xcode project or use simctl"
echo "  2. Copy $BUILD_DIR/${OUTPUT} to app bundle"
echo "  3. Run: xcrun simctl launch booted com.yourapp.${OUTPUT}"
