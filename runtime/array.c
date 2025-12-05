#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Array structure: [length, capacity, elem0, elem1, ...]
// First 8 bytes: length
// Second 8 bytes: capacity
// Rest: elements

static pthread_mutex_t array_mutex = PTHREAD_MUTEX_INITIALIZER;

// Append element to array (thread-safe, grows array)
// Returns new array pointer (may be reallocated)
void* _wyn_array_append(void* arr_ptr, long long element) {
    pthread_mutex_lock(&array_mutex);
    
    long long* arr = (long long*)arr_ptr;
    long long length = arr[0];
    long long capacity = arr[1];
    
    // If we need to grow, allocate new array on heap
    if (length >= capacity) {
        long long new_capacity = capacity == 0 ? 8 : capacity * 2;
        long long* new_arr = (long long*)malloc((new_capacity + 2) * sizeof(long long));
        if (new_arr) {
            // Copy existing elements
            new_arr[0] = length;
            new_arr[1] = new_capacity;
            for (long long i = 0; i < length; i++) {
                new_arr[i + 2] = arr[i + 2];
            }
            arr = new_arr;
        }
    }
    
    // Append element
    arr[length + 2] = element;
    arr[0] = length + 1;
    
    pthread_mutex_unlock(&array_mutex);
    return arr;
}

// Create empty array with capacity
void* _wyn_array_new(long long capacity) {
    long long* arr = (long long*)malloc((capacity + 2) * sizeof(long long));
    arr[0] = 0;  // length
    arr[1] = capacity;
    return arr;
}
