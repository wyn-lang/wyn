// Wyn GUI Runtime - Windows (Win32 API)
// Requires: Windows SDK for compilation

#include <stdint.h>
#include <stdlib.h>

// Stub implementations - will be implemented with Win32 API when Windows SDK is available

void gui_init() {
}

int64_t gui_create_window(const char* title, int64_t width, int64_t height) {
    return 0;
}

void gui_show_window(int64_t handle) {
}

void gui_set_title(int64_t handle, const char* title) {
}

void gui_run() {
}

int64_t gui_poll_events() {
    return 0;
}

void gui_close_window(int64_t handle) {
}

void gui_draw_rect(int64_t handle, int64_t x, int64_t y, int64_t w, int64_t h, int64_t color) {
}

void gui_draw_text(int64_t handle, int64_t x, int64_t y, const char* text) {
}
