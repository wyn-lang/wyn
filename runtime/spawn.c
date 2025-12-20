#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

// Platform-specific includes
#ifdef __APPLE__
#include <sys/sysctl.h>
#endif

// M:N Scheduler Configuration
#define TASK_STACK_SIZE 8192
#define MAX_TASKS 1000000
#define QUEUE_SIZE 4096

// Task states
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_DONE
} TaskState;

// Task structure (green thread)
typedef struct Task {
    void (*func)(void*);
    void* context;
    TaskState state;
    struct Task* next;
} Task;

// Lock-free queue for work stealing
typedef struct {
    Task* tasks[QUEUE_SIZE];
    _Atomic int head;
    _Atomic int tail;
    pthread_mutex_t mutex; // Fallback to mutex for simplicity
} WorkQueue;

// Worker thread
typedef struct {
    pthread_t thread;
    WorkQueue local_queue;
    int worker_id;
    _Atomic bool running;
} Worker;

// Global runtime state
static struct {
    Worker* workers;
    int worker_count;
    WorkQueue global_queue;
    _Atomic int task_count;
    _Atomic bool initialized;
    pthread_mutex_t init_mutex;
} runtime = {0};

// Queue operations with mutex for reliability
static bool queue_push(WorkQueue* q, Task* task) {
    pthread_mutex_lock(&q->mutex);
    
    int tail = atomic_load(&q->tail);
    int next_tail = (tail + 1) % QUEUE_SIZE;
    
    if (next_tail == atomic_load(&q->head)) {
        pthread_mutex_unlock(&q->mutex);
        return false; // Queue full
    }
    
    q->tasks[tail] = task;
    atomic_store(&q->tail, next_tail);
    
    pthread_mutex_unlock(&q->mutex);
    return true;
}

static Task* queue_pop(WorkQueue* q) {
    pthread_mutex_lock(&q->mutex);
    
    int head = atomic_load(&q->head);
    if (head == atomic_load(&q->tail)) {
        pthread_mutex_unlock(&q->mutex);
        return NULL; // Queue empty
    }
    
    Task* task = q->tasks[head];
    atomic_store(&q->head, (head + 1) % QUEUE_SIZE);
    
    pthread_mutex_unlock(&q->mutex);
    return task;
}

// Work stealing - try to steal from another worker
static Task* steal_task(int worker_id) {
    for (int i = 0; i < runtime.worker_count; i++) {
        if (i == worker_id) continue;
        
        Task* task = queue_pop(&runtime.workers[i].local_queue);
        if (task) return task;
    }
    return queue_pop(&runtime.global_queue);
}

// Worker thread main loop
static void* worker_main(void* arg) {
    Worker* worker = (Worker*)arg;
    
    while (atomic_load(&worker->running)) {
        // Try local queue first
        Task* task = queue_pop(&worker->local_queue);
        
        // Try work stealing if local queue empty
        if (!task) {
            task = steal_task(worker->worker_id);
        }
        
        if (task) {
            task->state = TASK_RUNNING;
            task->func(task->context);
            task->state = TASK_DONE;
            atomic_fetch_sub(&runtime.task_count, 1);
            free(task);
        } else {
            // No work available, yield CPU briefly
            usleep(1000); // 1ms
        }
    }
    
    return NULL;
}

// Get CPU count
static int get_cpu_count() {
#ifdef __APPLE__
    int count = 1;
    size_t size = sizeof(count);
    if (sysctlbyname("hw.ncpu", &count, &size, NULL, 0) != 0) {
        count = 4; // Default fallback
    }
    return count;
#elif defined(__linux__)
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    return (count > 0) ? (int)count : 4;
#else
    return 4; // Default for other platforms
#endif
}

// Initialize queue
static void init_queue(WorkQueue* q) {
    atomic_store(&q->head, 0);
    atomic_store(&q->tail, 0);
    pthread_mutex_init(&q->mutex, NULL);
    memset(q->tasks, 0, sizeof(q->tasks));
}

// Initialize runtime
int64_t __wyn_runtime_init() {
    if (atomic_load(&runtime.initialized)) return 0;
    
    pthread_mutex_lock(&runtime.init_mutex);
    if (atomic_load(&runtime.initialized)) {
        pthread_mutex_unlock(&runtime.init_mutex);
        return 0;
    }
    
    // printf("[Runtime] Initializing M:N scheduler...\n");
    
    runtime.worker_count = get_cpu_count();
    // printf("[Runtime] Using %d worker threads\n", runtime.worker_count);
    
    runtime.workers = calloc(runtime.worker_count, sizeof(Worker));
    if (!runtime.workers) {
        fprintf(stderr, "[Runtime] Failed to allocate workers\n");
        pthread_mutex_unlock(&runtime.init_mutex);
        return 1;
    }
    
    // Initialize global queue
    init_queue(&runtime.global_queue);
    
    // Start worker threads
    for (int i = 0; i < runtime.worker_count; i++) {
        Worker* worker = &runtime.workers[i];
        worker->worker_id = i;
        atomic_store(&worker->running, true);
        
        // Initialize local queue
        init_queue(&worker->local_queue);
        
        if (pthread_create(&worker->thread, NULL, worker_main, worker) != 0) {
            fprintf(stderr, "[Runtime] Failed to create worker %d\n", i);
        }
    }
    
    atomic_store(&runtime.initialized, true);
    // printf("[Runtime] M:N scheduler initialized\n");
    pthread_mutex_unlock(&runtime.init_mutex);
    return 0;
}

// Shutdown runtime
int64_t __wyn_runtime_shutdown() {
    if (!atomic_load(&runtime.initialized)) return 0;
    
    // printf("[Runtime] Shutting down scheduler...\n");
    
    // Wait for all tasks to complete
    while (atomic_load(&runtime.task_count) > 0) {
        usleep(10000); // 10ms
    }
    
    // Stop workers
    for (int i = 0; i < runtime.worker_count; i++) {
        atomic_store(&runtime.workers[i].running, false);
        pthread_join(runtime.workers[i].thread, NULL);
        pthread_mutex_destroy(&runtime.workers[i].local_queue.mutex);
    }
    
    pthread_mutex_destroy(&runtime.global_queue.mutex);
    free(runtime.workers);
    runtime.workers = NULL;
    atomic_store(&runtime.initialized, false);
    return 0;
}

// Spawn a new task
void __wyn_spawn(void (*func)(void*), void* context) {
    if (!func) return;
    
    __wyn_runtime_init();
    
    Task* task = malloc(sizeof(Task));
    if (!task) return;
    
    task->func = func;
    task->context = context;
    task->state = TASK_READY;
    task->next = NULL;
    
    atomic_fetch_add(&runtime.task_count, 1);
    
    // Try to add to a worker's local queue (round-robin)
    static _Atomic int next_worker = 0;
    int worker_id = atomic_fetch_add(&next_worker, 1) % runtime.worker_count;
    
    if (!queue_push(&runtime.workers[worker_id].local_queue, task)) {
        // Local queue full, try global queue
        if (!queue_push(&runtime.global_queue, task)) {
            // Both queues full, execute immediately (fallback)
            task->func(task->context);
            atomic_fetch_sub(&runtime.task_count, 1);
            free(task);
        }
    }
}

// Yield current task (placeholder for future cooperative scheduling)
void __wyn_yield() {
    usleep(1);
}

// Wait for async result
// For now, just yield to allow other tasks to run
long long __wyn_await(long long future_id) {
    // Simple implementation: yield and return the value
    // Full implementation would track futures and wait for completion
    __wyn_yield();
    return future_id;  // Return the value (simplified)
}

// Join all tasks (compatibility with existing API)
void __wyn_join_all() {
    while (atomic_load(&runtime.task_count) > 0) {
        usleep(10000); // 10ms
    }
}

// Wait for all spawned tasks to complete (public API)
long long task_join_all() {
    if (!runtime.initialized) {
        return 0;
    }
    
    // Wait for all tasks to complete
    while (atomic_load(&runtime.task_count) > 0) {
        usleep(1000); // Sleep 1ms
    }
    
    return 0;
}

// Atomic operations (existing API compatibility)
static pthread_mutex_t atomic_mutex = PTHREAD_MUTEX_INITIALIZER;

long long __wyn_atomic_add(long long* ptr, long long value) {
    pthread_mutex_lock(&atomic_mutex);
    long long old = *ptr;
    *ptr = old + value;
    pthread_mutex_unlock(&atomic_mutex);
    return old;
}

long long __wyn_atomic_load(long long* ptr) {
    pthread_mutex_lock(&atomic_mutex);
    long long value = *ptr;
    pthread_mutex_unlock(&atomic_mutex);
    return value;
}
