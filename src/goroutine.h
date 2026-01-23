// Goroutine-style runtime for Wyn
// Lightweight tasks (not OS threads) with work-stealing scheduler
#ifndef WYN_GOROUTINE_H
#define WYN_GOROUTINE_H

#include <stdint.h>
#include <pthread.h>

// Forward declarations
typedef struct WynTask WynTask;
typedef struct WynChannel WynChannel;
typedef struct WynScheduler WynScheduler;

// Task (goroutine equivalent)
typedef void (*WynTaskFunc)(void*);

struct WynTask {
    WynTaskFunc func;
    void* arg;
    WynTask* next;
    int worker_id;
};

// Channel (Go-style channel)
struct WynChannel {
    void** buffer;
    int capacity;
    int size;
    int read_pos;
    int write_pos;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    int closed;
};

// Work-stealing scheduler
struct WynScheduler {
    WynTask** queues;        // Per-worker queues
    pthread_mutex_t* locks;  // Per-worker locks
    pthread_t* workers;      // Worker threads
    int num_workers;
    int running;
    pthread_mutex_t global_lock;
};

// Global scheduler
extern WynScheduler* global_scheduler;

// Scheduler functions
WynScheduler* wyn_scheduler_init(int num_workers);
void wyn_scheduler_shutdown(WynScheduler* sched);
void wyn_scheduler_spawn(WynScheduler* sched, WynTaskFunc func, void* arg);

// Task functions (user-facing)
void wyn_go(WynTaskFunc func, void* arg);  // spawn task
void wyn_yield();                           // yield to scheduler

// Channel functions
WynChannel* wyn_channel_new(int capacity);
void wyn_channel_send(WynChannel* ch, void* value);
void* wyn_channel_recv(WynChannel* ch);
void wyn_channel_close(WynChannel* ch);
void wyn_channel_free(WynChannel* ch);

#endif // WYN_GOROUTINE_H
