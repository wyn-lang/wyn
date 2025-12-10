// Temporary stub for M:N scheduler runtime
// Will be replaced by agent implementation

#include <stdio.h>

long long __wyn_runtime_init() {
    printf("[Runtime] Initializing M:N scheduler...\n");
    return 0;
}

long long __wyn_runtime_shutdown() {
    printf("[Runtime] Shutting down scheduler...\n");
    return 0;
}

long long __wyn_spawn(void* func_ptr, void* context) {
    printf("[Runtime] Spawn called (stub - not executing)\n");
    return 0;
}

void __wyn_yield() {
    // No-op for now
}

long long __wyn_await(void* future) {
    return 0;
}
