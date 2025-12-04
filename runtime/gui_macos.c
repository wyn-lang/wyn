// Wyn GUI Runtime - macOS (Cocoa/AppKit)
#include <objc/runtime.h>
#include <objc/message.h>
#include <CoreGraphics/CoreGraphics.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Constants
#define NSWindowStyleMaskTitled (1 << 0)
#define NSWindowStyleMaskClosable (1 << 1)
#define NSWindowStyleMaskMiniaturizable (1 << 2)
#define NSWindowStyleMaskResizable (1 << 3)
#define NSBackingStoreBuffered 2
#define NSApplicationActivationPolicyRegular 0

// Global app instance
static id g_app = NULL;
static Class g_view_class = NULL;

// Window handle (opaque pointer)
typedef struct {
    id window;
    id view;
} WynWindow;

// Initialize NSApplication
void gui_init() {
    if (g_app) return;
    
    Class NSApplication = objc_getClass("NSApplication");
    g_app = ((id (*)(id, SEL))objc_msgSend)((id)NSApplication, sel_registerName("sharedApplication"));
    ((void (*)(id, SEL, long))objc_msgSend)(g_app, sel_registerName("setActivationPolicy:"), NSApplicationActivationPolicyRegular);
}

// Custom view for drawing
static void view_drawRect(id self, SEL _cmd, CGRect rect) {
    (void)self; (void)_cmd;
    Class NSColor = objc_getClass("NSColor");
    id white = ((id (*)(id, SEL))objc_msgSend)((id)NSColor, sel_registerName("whiteColor"));
    ((void (*)(id, SEL))objc_msgSend)(white, sel_registerName("set"));
    
    Class NSBezierPath = objc_getClass("NSBezierPath");
    id path = ((id (*)(id, SEL, CGRect))objc_msgSend)((id)NSBezierPath, sel_registerName("bezierPathWithRect:"), rect);
    ((void (*)(id, SEL))objc_msgSend)(path, sel_registerName("fill"));
}

void gui_setup_view_class() {
    if (g_view_class) return;
    
    Class NSView = objc_getClass("NSView");
    g_view_class = objc_allocateClassPair(NSView, "WynView", 0);
    class_addMethod(g_view_class, sel_registerName("drawRect:"), (IMP)view_drawRect, "v@:{CGRect={CGPoint=dd}{CGSize=dd}}");
    objc_registerClassPair(g_view_class);
}

// Create window
int64_t gui_create_window(const char* title, int64_t width, int64_t height) {
    gui_init();
    gui_setup_view_class();
    
    WynWindow* win = malloc(sizeof(WynWindow));
    
    Class NSWindow = objc_getClass("NSWindow");
    Class NSString = objc_getClass("NSString");
    
    CGRect frame = {{100, 100}, {width, height}};
    uint64_t style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | 
                     NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    
    id alloc = ((id (*)(id, SEL))objc_msgSend)((id)NSWindow, sel_registerName("alloc"));
    win->window = ((id (*)(id, SEL, CGRect, uint64_t, uint64_t, int))objc_msgSend)(
        alloc, sel_registerName("initWithContentRect:styleMask:backing:defer:"),
        frame, style, NSBackingStoreBuffered, 0);
    
    id titleStr = ((id (*)(id, SEL, const char*))objc_msgSend)((id)NSString, sel_registerName("stringWithUTF8String:"), title);
    ((void (*)(id, SEL, id))objc_msgSend)(win->window, sel_registerName("setTitle:"), titleStr);
    
    id viewAlloc = ((id (*)(id, SEL))objc_msgSend)((id)g_view_class, sel_registerName("alloc"));
    win->view = ((id (*)(id, SEL, CGRect))objc_msgSend)(viewAlloc, sel_registerName("initWithFrame:"), frame);
    ((void (*)(id, SEL, id))objc_msgSend)(win->window, sel_registerName("setContentView:"), win->view);
    
    return (int64_t)win;
}

// Show window
void gui_show_window(int64_t handle) {
    WynWindow* win = (WynWindow*)handle;
    ((void (*)(id, SEL, id))objc_msgSend)(win->window, sel_registerName("makeKeyAndOrderFront:"), NULL);
    ((void (*)(id, SEL, int))objc_msgSend)(g_app, sel_registerName("activateIgnoringOtherApps:"), 1);
}

// Set window title
void gui_set_title(int64_t handle, const char* title) {
    WynWindow* win = (WynWindow*)handle;
    Class NSString = objc_getClass("NSString");
    id titleStr = ((id (*)(id, SEL, const char*))objc_msgSend)((id)NSString, sel_registerName("stringWithUTF8String:"), title);
    ((void (*)(id, SEL, id))objc_msgSend)(win->window, sel_registerName("setTitle:"), titleStr);
}

// Run event loop (blocking)
void gui_run() {
    ((void (*)(id, SEL))objc_msgSend)(g_app, sel_registerName("run"));
}

// Process events (non-blocking)
int64_t gui_poll_events() {
    Class NSDate = objc_getClass("NSDate");
    id untilDate = ((id (*)(id, SEL))objc_msgSend)((id)NSDate, sel_registerName("distantPast"));
    
    Class NSString = objc_getClass("NSString");
    id mode = ((id (*)(id, SEL, const char*))objc_msgSend)((id)NSString, sel_registerName("stringWithUTF8String:"), "kCFRunLoopDefaultMode");
    
    id event = ((id (*)(id, SEL, uint64_t, id, id, int))objc_msgSend)(
        g_app, sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"),
        UINT64_MAX, untilDate, mode, 1);
    
    if (event) {
        ((void (*)(id, SEL, id))objc_msgSend)(g_app, sel_registerName("sendEvent:"), event);
        ((void (*)(id, SEL))objc_msgSend)(g_app, sel_registerName("updateWindows"));
        return 1;
    }
    return 0;
}

// Close window
void gui_close_window(int64_t handle) {
    WynWindow* win = (WynWindow*)handle;
    ((void (*)(id, SEL))objc_msgSend)(win->window, sel_registerName("close"));
    free(win);
}

// Draw rectangle
void gui_draw_rect(int64_t handle, int64_t x, int64_t y, int64_t w, int64_t h, int64_t color) {
    WynWindow* win = (WynWindow*)handle;
    
    ((void (*)(id, SEL))objc_msgSend)(win->view, sel_registerName("lockFocus"));
    
    double r = ((color >> 16) & 0xFF) / 255.0;
    double g = ((color >> 8) & 0xFF) / 255.0;
    double b = (color & 0xFF) / 255.0;
    
    Class NSColor = objc_getClass("NSColor");
    id colorObj = ((id (*)(id, SEL, double, double, double, double))objc_msgSend)(
        (id)NSColor, sel_registerName("colorWithRed:green:blue:alpha:"), r, g, b, 1.0);
    ((void (*)(id, SEL))objc_msgSend)(colorObj, sel_registerName("set"));
    
    Class NSBezierPath = objc_getClass("NSBezierPath");
    CGRect rect = {{x, y}, {w, h}};
    id path = ((id (*)(id, SEL, CGRect))objc_msgSend)((id)NSBezierPath, sel_registerName("bezierPathWithRect:"), rect);
    ((void (*)(id, SEL))objc_msgSend)(path, sel_registerName("fill"));
    
    ((void (*)(id, SEL))objc_msgSend)(win->view, sel_registerName("unlockFocus"));
    ((void (*)(id, SEL))objc_msgSend)(win->window, sel_registerName("display"));
}

// Draw text
void gui_draw_text(int64_t handle, int64_t x, int64_t y, const char* text) {
    WynWindow* win = (WynWindow*)handle;
    
    ((void (*)(id, SEL))objc_msgSend)(win->view, sel_registerName("lockFocus"));
    
    Class NSString = objc_getClass("NSString");
    id str = ((id (*)(id, SEL, const char*))objc_msgSend)((id)NSString, sel_registerName("stringWithUTF8String:"), text);
    
    CGPoint point = {x, y};
    ((void (*)(id, SEL, CGPoint, id))objc_msgSend)(str, sel_registerName("drawAtPoint:withAttributes:"), point, NULL);
    
    ((void (*)(id, SEL))objc_msgSend)(win->view, sel_registerName("unlockFocus"));
    ((void (*)(id, SEL))objc_msgSend)(win->window, sel_registerName("display"));
}
