#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

// Task channel for communication between spawn blocks
// Simple unbuffered channel implementation

typedef struct {
    long long value;
    _Atomic bool has_value;
    _Atomic bool closed;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Channel;

// Create new channel
Channel* __wyn_channel_new() {
    Channel* ch = malloc(sizeof(Channel));
    ch->value = 0;
    atomic_store(&ch->has_value, false);
    atomic_store(&ch->closed, false);
    pthread_mutex_init(&ch->mutex, NULL);
    pthread_cond_init(&ch->cond, NULL);
    return ch;
}

// Send value to channel (blocks until received)
void __wyn_channel_send(Channel* ch, long long value) {
    if (!ch) return;
    
    pthread_mutex_lock(&ch->mutex);
    
    // Wait until previous value is consumed
    while (atomic_load(&ch->has_value) && !atomic_load(&ch->closed)) {
        pthread_cond_wait(&ch->cond, &ch->mutex);
    }
    
    if (atomic_load(&ch->closed)) {
        pthread_mutex_unlock(&ch->mutex);
        return;
    }
    
    ch->value = value;
    atomic_store(&ch->has_value, true);
    pthread_cond_signal(&ch->cond);
    
    pthread_mutex_unlock(&ch->mutex);
}

// Receive value from channel (blocks until available)
long long __wyn_channel_recv(Channel* ch) {
    if (!ch) return 0;
    
    pthread_mutex_lock(&ch->mutex);
    
    // Wait for value
    while (!atomic_load(&ch->has_value) && !atomic_load(&ch->closed)) {
        pthread_cond_wait(&ch->cond, &ch->mutex);
    }
    
    if (atomic_load(&ch->closed) && !atomic_load(&ch->has_value)) {
        pthread_mutex_unlock(&ch->mutex);
        return 0;
    }
    
    long long val = ch->value;
    atomic_store(&ch->has_value, false);
    pthread_cond_signal(&ch->cond);
    
    pthread_mutex_unlock(&ch->mutex);
    return val;
}

// Close channel
void __wyn_channel_close(Channel* ch) {
    if (!ch) return;
    
    pthread_mutex_lock(&ch->mutex);
    atomic_store(&ch->closed, true);
    pthread_cond_broadcast(&ch->cond);
    pthread_mutex_unlock(&ch->mutex);
}

// Free channel
void __wyn_channel_free(Channel* ch) {
    if (!ch) return;
    
    pthread_mutex_destroy(&ch->mutex);
    pthread_cond_destroy(&ch->cond);
    free(ch);
}
