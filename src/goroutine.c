// Goroutine-style runtime implementation
#include "goroutine.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

WynScheduler* global_scheduler = NULL;

// Initialize scheduler with N worker threads
WynScheduler* wyn_scheduler_init(int num_workers) {
    WynScheduler* sched = malloc(sizeof(WynScheduler));
    sched->num_workers = num_workers;
    sched->running = 1;
    
    // Allocate per-worker queues and locks
    sched->queues = calloc(num_workers, sizeof(WynTask*));
    sched->locks = malloc(num_workers * sizeof(pthread_mutex_t));
    sched->workers = malloc(num_workers * sizeof(pthread_t));
    
    pthread_mutex_init(&sched->global_lock, NULL);
    
    for (int i = 0; i < num_workers; i++) {
        pthread_mutex_init(&sched->locks[i], NULL);
    }
    
    return sched;
}

// Worker thread function
static void* worker_thread(void* arg) {
    WynScheduler* sched = (WynScheduler*)arg;
    int worker_id = 0;
    
    // Find our worker ID
    for (int i = 0; i < sched->num_workers; i++) {
        if (pthread_equal(sched->workers[i], pthread_self())) {
            worker_id = i;
            break;
        }
    }
    
    while (sched->running) {
        WynTask* task = NULL;
        
        // Try to get task from our queue
        pthread_mutex_lock(&sched->locks[worker_id]);
        if (sched->queues[worker_id]) {
            task = sched->queues[worker_id];
            sched->queues[worker_id] = task->next;
        }
        pthread_mutex_unlock(&sched->locks[worker_id]);
        
        // Work stealing: try other queues
        if (!task) {
            for (int i = 0; i < sched->num_workers; i++) {
                if (i == worker_id) continue;
                
                pthread_mutex_lock(&sched->locks[i]);
                if (sched->queues[i]) {
                    task = sched->queues[i];
                    sched->queues[i] = task->next;
                    pthread_mutex_unlock(&sched->locks[i]);
                    break;
                }
                pthread_mutex_unlock(&sched->locks[i]);
            }
        }
        
        // Execute task
        if (task) {
            task->func(task->arg);
            free(task);
        } else {
            // No work, sleep briefly
            usleep(1000); // 1ms
        }
    }
    
    return NULL;
}

// Start worker threads
void wyn_scheduler_start(WynScheduler* sched) {
    for (int i = 0; i < sched->num_workers; i++) {
        pthread_create(&sched->workers[i], NULL, worker_thread, sched);
    }
}

// Spawn a task (goroutine)
void wyn_scheduler_spawn(WynScheduler* sched, WynTaskFunc func, void* arg) {
    WynTask* task = malloc(sizeof(WynTask));
    task->func = func;
    task->arg = arg;
    task->next = NULL;
    
    // Round-robin assignment to workers
    static int next_worker = 0;
    int worker_id = __sync_fetch_and_add(&next_worker, 1) % sched->num_workers;
    
    pthread_mutex_lock(&sched->locks[worker_id]);
    task->next = sched->queues[worker_id];
    sched->queues[worker_id] = task;
    pthread_mutex_unlock(&sched->locks[worker_id]);
}

// Shutdown scheduler
void wyn_scheduler_shutdown(WynScheduler* sched) {
    sched->running = 0;
    
    // Wait for workers
    for (int i = 0; i < sched->num_workers; i++) {
        pthread_join(sched->workers[i], NULL);
    }
    
    // Cleanup
    for (int i = 0; i < sched->num_workers; i++) {
        pthread_mutex_destroy(&sched->locks[i]);
    }
    pthread_mutex_destroy(&sched->global_lock);
    
    free(sched->queues);
    free(sched->locks);
    free(sched->workers);
    free(sched);
}

// User-facing API
void wyn_go(WynTaskFunc func, void* arg) {
    if (!global_scheduler) {
        // Lazy init with CPU count workers
        int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
        if (num_cpus < 1) num_cpus = 4;
        global_scheduler = wyn_scheduler_init(num_cpus);
        wyn_scheduler_start(global_scheduler);
    }
    
    wyn_scheduler_spawn(global_scheduler, func, arg);
}

void wyn_yield() {
    sched_yield();
}

// Channel implementation
WynChannel* wyn_channel_new(int capacity) {
    WynChannel* ch = malloc(sizeof(WynChannel));
    ch->capacity = capacity;
    ch->size = 0;
    ch->read_pos = 0;
    ch->write_pos = 0;
    ch->closed = 0;
    ch->buffer = malloc(capacity * sizeof(void*));
    
    pthread_mutex_init(&ch->mutex, NULL);
    pthread_cond_init(&ch->not_empty, NULL);
    pthread_cond_init(&ch->not_full, NULL);
    
    return ch;
}

void wyn_channel_send(WynChannel* ch, void* value) {
    pthread_mutex_lock(&ch->mutex);
    
    // Wait if full
    while (ch->size == ch->capacity && !ch->closed) {
        pthread_cond_wait(&ch->not_full, &ch->mutex);
    }
    
    if (ch->closed) {
        pthread_mutex_unlock(&ch->mutex);
        return; // Can't send to closed channel
    }
    
    // Add to buffer
    ch->buffer[ch->write_pos] = value;
    ch->write_pos = (ch->write_pos + 1) % ch->capacity;
    ch->size++;
    
    pthread_cond_signal(&ch->not_empty);
    pthread_mutex_unlock(&ch->mutex);
}

void* wyn_channel_recv(WynChannel* ch) {
    pthread_mutex_lock(&ch->mutex);
    
    // Wait if empty
    while (ch->size == 0 && !ch->closed) {
        pthread_cond_wait(&ch->not_empty, &ch->mutex);
    }
    
    if (ch->size == 0 && ch->closed) {
        pthread_mutex_unlock(&ch->mutex);
        return NULL; // Channel closed and empty
    }
    
    // Get from buffer
    void* value = ch->buffer[ch->read_pos];
    ch->read_pos = (ch->read_pos + 1) % ch->capacity;
    ch->size--;
    
    pthread_cond_signal(&ch->not_full);
    pthread_mutex_unlock(&ch->mutex);
    
    return value;
}

void wyn_channel_close(WynChannel* ch) {
    pthread_mutex_lock(&ch->mutex);
    ch->closed = 1;
    pthread_cond_broadcast(&ch->not_empty);
    pthread_cond_broadcast(&ch->not_full);
    pthread_mutex_unlock(&ch->mutex);
}

void wyn_channel_free(WynChannel* ch) {
    pthread_mutex_destroy(&ch->mutex);
    pthread_cond_destroy(&ch->not_empty);
    pthread_cond_destroy(&ch->not_full);
    free(ch->buffer);
    free(ch);
}
