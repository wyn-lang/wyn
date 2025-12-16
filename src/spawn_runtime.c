// Wyn spawn runtime
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_THREADS 1024

static pthread_t threads[MAX_THREADS];
static int thread_count = 0;
static pthread_mutex_t spawn_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef void (*thread_func_t)(long);

struct thread_arg {
    thread_func_t func;
    long arg;
};

static void* thread_wrapper(void* arg) {
    struct thread_arg* ta = (struct thread_arg*)arg;
    ta->func(ta->arg);
    free(ta);
    return NULL;
}

void __wyn_spawn(thread_func_t func, long arg) {
    pthread_mutex_lock(&spawn_mutex);
    if (thread_count < MAX_THREADS) {
        struct thread_arg* ta = malloc(sizeof(struct thread_arg));
        ta->func = func;
        ta->arg = arg;
        pthread_create(&threads[thread_count++], NULL, thread_wrapper, ta);
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
