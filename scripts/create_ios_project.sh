#!/bin/bash
# Create minimal Xcode project for Wyn iOS app

set -e

if [ $# -lt 1 ]; then
    echo "Usage: $0 <AppName>"
    echo "Example: $0 MyWynApp"
    exit 1
fi

APP_NAME=$1
PROJECT_DIR="${APP_NAME}.xcodeproj"

echo "Creating Xcode project: $APP_NAME"

mkdir -p "$APP_NAME"
cd "$APP_NAME"

# Create Info.plist
cat > Info.plist << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>$(EXECUTABLE_NAME)</string>
    <key>CFBundleIdentifier</key>
    <string>com.wyn.$(PRODUCT_NAME:rfc1034identifier)</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>$(PRODUCT_NAME)</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>LSRequiresIPhoneOS</key>
    <true/>
    <key>UIRequiredDeviceCapabilities</key>
    <array>
        <string>arm64</string>
    </array>
    <key>UISupportedInterfaceOrientations</key>
    <array>
        <string>UIInterfaceOrientationPortrait</string>
    </array>
</dict>
</plist>
EOF

# Create main.wyn
cat > main.wyn << 'EOF'
fn on_button_tap() {
    mobile.show_alert("Wyn", "Hello from Wyn iOS!")
}

fn main() {
    mobile.init()
    
    let title: i64 = mobile.create_label("Wyn iOS App", 20, 100, 350, 50)
    mobile.set_font_size(title, 28)
    mobile.set_text_color(title, 0, 0, 0)
    mobile.add_to_root(title)
    
    let subtitle: i64 = mobile.create_label("20-30x faster than Python", 20, 160, 350, 30)
    mobile.set_font_size(subtitle, 16)
    mobile.set_text_color(subtitle, 100, 100, 100)
    mobile.add_to_root(subtitle)
    
    let button: i64 = mobile.create_button("Tap Me", 20, 220, 350, 50)
    mobile.set_background_color(button, 0, 122, 255)
    mobile.add_to_root(button)
    mobile.on_tap(button, on_button_tap)
    
    mobile.run()
}
EOF

# Create build script
cat > build.sh << 'EOF'
#!/bin/bash
set -e

WYN_ROOT="../.."
BUILD_DIR="$WYN_ROOT/build"

echo "Building Wyn iOS app..."

# Build compiler if needed
if [ ! -f "$BUILD_DIR/stage0" ]; then
    cd "$WYN_ROOT"
    make stage0
    cd -
fi

# Build mobile runtime
xcrun -sdk iphonesimulator clang -Wall -Wextra -std=c11 -O2 -fobjc-arc \
    -c -o mobile_runtime.o "$WYN_ROOT/runtime/mobile_ios.m"

# Compile Wyn source
"$BUILD_DIR/stage0" -S -o app.s main.wyn

# Assemble
xcrun -sdk iphonesimulator clang -c -o app.o app.s

# Link
xcrun -sdk iphonesimulator clang -o app app.o mobile_runtime.o \
    -framework UIKit -framework Foundation

echo "✓ Build complete: ./app"
EOF

chmod +x build.sh

echo "✓ Created iOS project: $APP_NAME/"
echo ""
echo "Next steps:"
echo "  cd $APP_NAME"
echo "  ./build.sh"
echo ""
echo "To run in simulator, you'll need to:"
echo "  1. Create app bundle structure"
echo "  2. Copy executable and Info.plist"
echo "  3. Use 'xcrun simctl install' and 'xcrun simctl launch'"
