# iOS Mobile API Quick Reference

## Setup
```wyn
mobile.init()  // Initialize app
mobile.run()   // Start event loop (blocking)
```

## Views
```wyn
let view: i64 = mobile.create_view(x, y, w, h)
mobile.add_subview(parent, child)
mobile.add_to_root(view)
```

## Components
```wyn
let btn: i64 = mobile.create_button("Title", x, y, w, h)
let lbl: i64 = mobile.create_label("Text", x, y, w, h)
let txt: i64 = mobile.create_textfield(x, y, w, h)
```

## Text
```wyn
mobile.set_text(view, "New text")
let text: str = mobile.get_text(view)
```

## Layout
```wyn
mobile.set_frame(view, x, y, w, h)
mobile.set_background_color(view, r, g, b)  // 0-255
mobile.set_text_color(view, r, g, b)
mobile.set_font_size(view, size)
```

## Events
```wyn
fn my_callback() { /* ... */ }

mobile.on_tap(button, my_callback)
mobile.on_text_change(textfield, my_callback)
```

## Utilities
```wyn
mobile.show_alert("Title", "Message")
mobile.hide_view(view)
mobile.show_view(view)
mobile.remove_view(view)
```

## Complete Example
```wyn
fn on_click() {
    mobile.show_alert("Hi", "Button clicked!")
}

fn main() {
    mobile.init()
    
    let label: i64 = mobile.create_label("Hello iOS", 20, 100, 350, 40)
    mobile.set_font_size(label, 24)
    mobile.add_to_root(label)
    
    let button: i64 = mobile.create_button("Tap Me", 20, 160, 350, 50)
    mobile.set_background_color(button, 0, 122, 255)
    mobile.add_to_root(button)
    mobile.on_tap(button, on_click)
    
    mobile.run()
}
```

## Build
```bash
./scripts/build_ios.sh examples/mobile_ios_app.wyn MyApp
```

## Coordinates
- Origin: Top-left (0, 0)
- X: Left to right
- Y: Top to bottom
- Units: Points (not pixels)
- Safe area: Start at y=100 for iPhone with notch
