# iOS Mobile Runtime

Wyn supports building native iOS applications using UIKit.

## Features

- Native UIKit bindings
- App lifecycle management
- UI components (buttons, labels, text fields)
- Event handling (tap, text change)
- Layout management
- Alerts and dialogs

## Building for iOS

The mobile runtime requires the iOS SDK and can be built for iOS simulator or device:

```bash
# Build for iOS simulator
make mobile-runtime-ios

# Compile Wyn app for iOS
./build/stage0 -o myapp examples/mobile_ios_app.wyn

# Link with mobile runtime
xcrun -sdk iphonesimulator clang -o myapp.app myapp build/mobile_runtime.o \
  -framework UIKit -framework Foundation
```

## API Reference

### App Lifecycle

```wyn
mobile.init()           // Initialize UIApplication
mobile.run()            // Start event loop (blocking)
```

### Views

```wyn
mobile.create_view(x: i64, y: i64, w: i64, h: i64) -> i64
mobile.add_subview(parent: i64, child: i64)
mobile.add_to_root(view: i64)  // Add to root view controller
```

### UI Components

```wyn
mobile.create_button(title: str, x: i64, y: i64, w: i64, h: i64) -> i64
mobile.create_label(text: str, x: i64, y: i64, w: i64, h: i64) -> i64
mobile.create_textfield(x: i64, y: i64, w: i64, h: i64) -> i64
```

### Text Operations

```wyn
mobile.set_text(view: i64, text: str)
mobile.get_text(view: i64) -> str
```

### Layout

```wyn
mobile.set_frame(view: i64, x: i64, y: i64, w: i64, h: i64)
mobile.set_background_color(view: i64, r: i64, g: i64, b: i64)
mobile.set_text_color(view: i64, r: i64, g: i64, b: i64)
mobile.set_font_size(view: i64, size: i64)
```

### Events

```wyn
mobile.on_tap(view: i64, callback: fn)
mobile.on_text_change(textfield: i64, callback: fn)
```

### Utilities

```wyn
mobile.show_alert(title: str, message: str)
mobile.hide_view(view: i64)
mobile.show_view(view: i64)
mobile.remove_view(view: i64)
```

## Example

```wyn
fn on_button_click() {
    mobile.show_alert("Hello", "Button clicked!")
}

fn main() {
    mobile.init()
    
    const label: i64 = mobile.create_label("Hello iOS", 20, 100, 350, 40)
    mobile.set_font_size(label, 24)
    mobile.add_to_root(label)
    
    const button: i64 = mobile.create_button("Click Me", 20, 160, 350, 50)
    mobile.set_background_color(button, 0, 122, 255)
    mobile.add_to_root(button)
    mobile.on_tap(button, on_button_click)
    
    mobile.run()
}
```

## Implementation Details

- Uses Objective-C runtime for UIKit bindings
- ARC (Automatic Reference Counting) enabled
- View handles are opaque pointers (i64)
- Callbacks stored with associated objects
- Main thread execution for UI operations

## Limitations

- iOS simulator or device required (not available on macOS desktop)
- Requires Xcode and iOS SDK
- Single window application model
- Callbacks must be top-level functions

## Future Enhancements

- SwiftUI bindings
- Auto Layout support
- Navigation controllers
- Table views and collection views
- Gesture recognizers
- Animation support
- Camera and photo library access
