// Wyn spawn runtime with proper task synchronization
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sched.h>

#define MAX_THREADS 1024

typedef struct {
    pthread_t thread;
    bool completed;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Task;

static Task tasks[MAX_THREADS];
static int task_count = 0;
static pthread_mutex_t spawn_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef void (*thread_func_t)(long);

struct thread_arg {
    thread_func_t func;
    long arg;
    int task_id;
};

static void* thread_wrapper(void* arg) {
    struct thread_arg* ta = (struct thread_arg*)arg;
    ta->func(ta->arg);
    
    // Mark task as completed
    pthread_mutex_lock(&tasks[ta->task_id].mutex);
    tasks[ta->task_id].completed = true;
    pthread_cond_broadcast(&tasks[ta->task_id].cond);
    pthread_mutex_unlock(&tasks[ta->task_id].mutex);
    
    free(ta);
    return NULL;
}

// Returns task ID that can be awaited
long __wyn_spawn(thread_func_t func, long arg) {
    pthread_mutex_lock(&spawn_mutex);
    if (task_count >= MAX_THREADS) {
        fprintf(stderr, "Error: Maximum thread limit reached (%d)\n", MAX_THREADS);
        pthread_mutex_unlock(&spawn_mutex);
        return -1;
    }
    
    int task_id = task_count++;
    
    // Initialize task
    tasks[task_id].completed = false;
    pthread_mutex_init(&tasks[task_id].mutex, NULL);
    pthread_cond_init(&tasks[task_id].cond, NULL);
    
    struct thread_arg* ta = malloc(sizeof(struct thread_arg));
    ta->func = func;
    ta->arg = arg;
    ta->task_id = task_id;
    
    pthread_create(&tasks[task_id].thread, NULL, thread_wrapper, ta);
    pthread_mutex_unlock(&spawn_mutex);
    
    return task_id;
}

// Wait for specific task to complete
void __wyn_await(long task_id) {
    if (task_id < 0 || task_id >= task_count) return;
    
    pthread_mutex_lock(&tasks[task_id].mutex);
    while (!tasks[task_id].completed) {
        pthread_cond_wait(&tasks[task_id].cond, &tasks[task_id].mutex);
    }
    pthread_mutex_unlock(&tasks[task_id].mutex);
    
    pthread_join(tasks[task_id].thread, NULL);
}

// Wait for all spawned tasks to complete
void __wyn_join_all(void) {
    for (int i = 0; i < task_count; i++) {
        __wyn_await(i);
    }
    task_count = 0;
}

// Runtime initialization
void __wyn_runtime_init(void) {
    // Initialize runtime state
    task_count = 0;
}

// Runtime shutdown - automatically join all tasks
void __wyn_runtime_shutdown(void) {
    __wyn_join_all();
}

// Yield current task (placeholder for cooperative scheduling)
void __wyn_yield(void) {
    #ifdef __linux__
    pthread_yield();
    #else
    sched_yield();
    #endif
}
