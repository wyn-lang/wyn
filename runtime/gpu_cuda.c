// Wyn GPU Runtime - CUDA Backend
// Requires: CUDA toolkit for compilation

#include <stdint.h>
#include <stdlib.h>

// Stub implementations - will be implemented with CUDA when toolkit is available

void __wyn_gpu_init() {
}

int __wyn_gpu_device_count() {
    return 0;
}

const char* __wyn_gpu_device_name(int device_id) {
    return "CUDA Device (stub)";
}

void* __wyn_gpu_malloc(size_t size) {
    return malloc(size);
}

void __wyn_gpu_free(void* ptr) {
    free(ptr);
}

void __wyn_gpu_memcpy_to_device(void* dst, const void* src, size_t size) {
}

void __wyn_gpu_memcpy_from_device(void* dst, const void* src, size_t size) {
}

void __wyn_gpu_launch_kernel(const char* kernel_source, void** buffers, int buffer_count, int grid_size) {
}

void __wyn_gpu_sync() {
}
