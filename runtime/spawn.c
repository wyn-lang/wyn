#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

// Dynamic thread pool for spawn (unlimited)
static pthread_t* threads = NULL;
static int thread_count = 0;
static int thread_capacity = 0;

// Synchronization
static pthread_mutex_t sync_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sync_cond = PTHREAD_COND_INITIALIZER;
static int active_threads = 0;

typedef void (*thread_func_t)(void*);

static void* thread_wrapper(void* arg) {
    thread_func_t func = (thread_func_t)arg;
    func(NULL);
    
    // Signal completion
    pthread_mutex_lock(&sync_mutex);
    active_threads--;
    pthread_cond_signal(&sync_cond);
    pthread_mutex_unlock(&sync_mutex);
    
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
    
    // Signal completion
    pthread_mutex_lock(&sync_mutex);
    active_threads--;
    pthread_cond_signal(&sync_cond);
    pthread_mutex_unlock(&sync_mutex);
    
    return NULL;
}

void __wyn_spawn(thread_func_t func, void* context) {
    // Grow array if needed
    if (thread_count >= thread_capacity) {
        int new_capacity = thread_capacity == 0 ? 1024 : thread_capacity * 2;
        pthread_t* new_threads = realloc(threads, new_capacity * sizeof(pthread_t));
        if (new_threads) {
            threads = new_threads;
            thread_capacity = new_capacity;
        } else {
            return;
        }
    }
    
    // Increment active count
    pthread_mutex_lock(&sync_mutex);
    active_threads++;
    pthread_mutex_unlock(&sync_mutex);
    
    int result;
    if (context) {
        thread_args_t* args = malloc(sizeof(thread_args_t));
        if (!args) {
            pthread_mutex_lock(&sync_mutex);
            active_threads--;
            pthread_mutex_unlock(&sync_mutex);
            return;
        }
        args->func = func;
        args->context = context;
        result = pthread_create(&threads[thread_count], NULL, thread_wrapper_with_context, args);
    } else {
        result = pthread_create(&threads[thread_count], NULL, thread_wrapper, (void*)func);
    }
    
    if (result == 0) {
        thread_count++;
    } else {
        pthread_mutex_lock(&sync_mutex);
        active_threads--;
        pthread_mutex_unlock(&sync_mutex);
    }
}

void __wyn_join_all() {
    // Wait for all threads to signal completion
    pthread_mutex_lock(&sync_mutex);
    while (active_threads > 0) {
        pthread_cond_wait(&sync_cond, &sync_mutex);
    }
    pthread_mutex_unlock(&sync_mutex);
    
    // Now join all threads (they're already done)
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    thread_count = 0;
    
    // Full memory barrier
    __sync_synchronize();
}
