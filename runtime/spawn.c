#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

// Thread pool for spawn
#define MAX_THREADS 1024

static pthread_t threads[MAX_THREADS];
static int thread_count = 0;

// Global mutex for atomic operations
static pthread_mutex_t atomic_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef void (*thread_func_t)(void*);

static void* thread_wrapper(void* arg) {
    thread_func_t func = (thread_func_t)arg;
    func(NULL);
    return NULL;
}

typedef struct {
    thread_func_t func;
    void* context;
} thread_args_t;

static void* thread_wrapper_with_context(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    args->func(args->context);
    free(args);
    return NULL;
}

void __wyn_spawn(thread_func_t func, void* context) {
    
    if (thread_count < MAX_THREADS) {
        if (context) {
            thread_args_t* args = malloc(sizeof(thread_args_t));
            args->func = func;
            args->context = context;
            int result = pthread_create(&threads[thread_count], NULL, thread_wrapper_with_context, args);
            if (result == 0) {
                thread_count++;
                // Delay to ensure thread starts
                usleep(100);
            }
        } else {
            int result = pthread_create(&threads[thread_count], NULL, thread_wrapper, (void*)func);
            if (result == 0) {
                thread_count++;
                // Delay to ensure thread starts
                usleep(100);
            }
        }
    } else {
    }
}

void __wyn_join_all() {
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    thread_count = 0;
}

// Mutex-based atomic add (reliable)
long long __wyn_atomic_add(long long* ptr, long long value) {
    pthread_mutex_lock(&atomic_mutex);
    long long old = *ptr;
    *ptr = old + value;
    long long new_val = *ptr;
    pthread_mutex_unlock(&atomic_mutex);
    return old;
}

// Mutex-based atomic load (reliable)
long long __wyn_atomic_load(long long* ptr) {
    pthread_mutex_lock(&atomic_mutex);
    long long value = *ptr;
    pthread_mutex_unlock(&atomic_mutex);
    return value;
}
