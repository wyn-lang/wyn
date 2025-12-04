// Wyn spawn runtime - link this with compiled programs
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_THREADS 1024

static pthread_t threads[MAX_THREADS];
static int thread_count = 0;
static pthread_mutex_t spawn_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef void (*thread_func_t)(void);

static void* thread_wrapper(void* arg) {
    thread_func_t func = (thread_func_t)arg;
    func();
    return NULL;
}

void __wyn_spawn(thread_func_t func) {
    pthread_mutex_lock(&spawn_mutex);
    if (thread_count < MAX_THREADS) {
        pthread_create(&threads[thread_count++], NULL, thread_wrapper, (void*)func);
    } else {
        fprintf(stderr, "Error: Maximum thread limit reached (%d)\n", MAX_THREADS);
    }
    pthread_mutex_unlock(&spawn_mutex);
}

void __wyn_join_all(void) {
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    thread_count = 0;
}
