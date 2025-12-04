#include <pthread.h>
#include <stdlib.h>

// Thread pool for spawn
#define MAX_THREADS 1024

static pthread_t threads[MAX_THREADS];
static int thread_count = 0;

typedef void (*thread_func_t)(void);

static void* thread_wrapper(void* arg) {
    thread_func_t func = (thread_func_t)arg;
    func();
    return NULL;
}

void __wyn_spawn(thread_func_t func) {
    if (thread_count < MAX_THREADS) {
        pthread_create(&threads[thread_count++], NULL, thread_wrapper, (void*)func);
    }
}

void __wyn_join_all() {
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    thread_count = 0;
}
