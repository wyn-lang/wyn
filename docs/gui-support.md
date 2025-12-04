# GUI Support in Wyn

Wyn includes native GUI support using platform-specific APIs for high performance and native look-and-feel.

## Platform Support

- **macOS**: Cocoa/AppKit (implemented)
- **Linux**: GTK (planned)
- **Windows**: Win32 (planned)

## Quick Start

```wyn
fn main() {
    let win: int = gui.create_window("Hello Wyn", 800, 600)
    gui.show_window(win)
    gui.draw_rect(win, 100, 100, 200, 150, 16711680)  # Red rectangle
    gui.draw_text(win, 120, 180, "Hello, GUI!")
    gui.run()
}
```

## Building

The GUI runtime is automatically built and linked on macOS:

```bash
make all
./build/stage0 -o myapp examples/gui_window.wyn
./myapp
```

## API Reference

### Window Management

#### `gui.create_window(title: str, width: int, height: int) -> int`
Creates a new window and returns a handle.

```wyn
let win: int = gui.create_window("My App", 800, 600)
```

#### `gui.show_window(window: int)`
Shows the window on screen.

```wyn
gui.show_window(win)
```

#### `gui.set_title(window: int, title: str)`
Changes the window title.

```wyn
gui.set_title(win, "New Title")
```

#### `gui.close_window(window: int)`
Closes the window and frees resources.

```wyn
gui.close_window(win)
```

### Event Loop

#### `gui.run()`
Runs the main event loop (blocking). Call this after setting up your window.

```wyn
gui.run()
```

#### `gui.poll_events() -> int`
Processes one event without blocking. Returns 1 if an event was processed, 0 otherwise.

```wyn
while gui.poll_events() == 1 {
    # Handle events
}
```

### Drawing

#### `gui.draw_rect(window: int, x: int, y: int, width: int, height: int, color: int)`
Draws a filled rectangle. Color is RGB as decimal (0xRRGGBB).

```wyn
gui.draw_rect(win, 50, 50, 200, 100, 16711680)  # Red
gui.draw_rect(win, 50, 200, 200, 100, 65280)    # Green
gui.draw_rect(win, 50, 350, 200, 100, 255)      # Blue
```

#### `gui.draw_text(window: int, x: int, y: int, text: str)`
Draws text at the specified position.

```wyn
gui.draw_text(win, 100, 100, "Hello, World!")
```

## Color Values

Colors are specified as RGB integers (0xRRGGBB format in decimal):

- Red: `16711680` (0xFF0000)
- Green: `65280` (0x00FF00)
- Blue: `255` (0x0000FF)
- White: `16777215` (0xFFFFFF)
- Black: `0` (0x000000)

Calculate custom colors: `(R * 65536) + (G * 256) + B`

## Examples

### Simple Window

```wyn
fn main() {
    let win: int = gui.create_window("Simple Window", 400, 300)
    gui.show_window(win)
    gui.run()
}
```

### Drawing Shapes

```wyn
fn main() {
    let win: int = gui.create_window("Shapes", 800, 600)
    gui.show_window(win)
    
    # Draw colored rectangles
    gui.draw_rect(win, 50, 50, 200, 100, 16711680)   # Red
    gui.draw_rect(win, 300, 50, 200, 100, 65280)     # Green
    gui.draw_rect(win, 550, 50, 200, 100, 255)       # Blue
    
    # Add labels
    gui.draw_text(win, 50, 200, "Red")
    gui.draw_text(win, 300, 200, "Green")
    gui.draw_text(win, 550, 200, "Blue")
    
    gui.run()
}
```

## Implementation Details

### macOS (Cocoa/AppKit)

The macOS implementation uses the Objective-C runtime to call Cocoa/AppKit APIs:

- `NSApplication` for app lifecycle
- `NSWindow` for windows
- `NSView` for custom drawing
- `NSBezierPath` for shapes
- `NSString` for text rendering

The runtime is compiled as `build/gui_runtime.o` and automatically linked with:
- `-framework Cocoa`
- `-framework CoreGraphics`

### Runtime Files

- `runtime/gui_macos.c` - macOS Cocoa implementation (~180 lines)
- `bootstrap/stage0.c` - Compiler integration
- `std/gui.wyn` - High-level API (planned)

## Limitations

Current implementation is minimal (~180 lines) and supports:
- Basic window creation and display
- Simple shape drawing (rectangles)
- Text rendering
- Event loop management

Not yet implemented:
- Event callbacks
- Mouse/keyboard input
- Images
- Advanced shapes (circles, lines, etc.)
- Widgets (buttons, text fields, etc.)

## Future Enhancements

- Event handling with callbacks
- Mouse and keyboard input
- Image loading and display
- More drawing primitives
- Widget library
- Layout system
- Linux (GTK) support
- Windows (Win32) support
