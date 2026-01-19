#include <stdlib.h>
#include <string.h>

// Array module - essential array manipulation functions

// Map: apply function to each element
int* wyn_array_map(int* arr, int len, int (*fn)(int)) {
    int* result = malloc(sizeof(int) * len);
    for (int i = 0; i < len; i++) {
        result[i] = fn(arr[i]);
    }
    return result;
}

// Filter: keep elements that match predicate
int* wyn_array_filter(int* arr, int len, int (*pred)(int), int* out_len) {
    int* temp = malloc(sizeof(int) * len);
    int count = 0;
    for (int i = 0; i < len; i++) {
        if (pred(arr[i])) {
            temp[count++] = arr[i];
        }
    }
    *out_len = count;
    return temp;
}

// Reduce: fold array into single value
int wyn_array_reduce(int* arr, int len, int (*fn)(int, int), int initial) {
    int result = initial;
    for (int i = 0; i < len; i++) {
        result = fn(result, arr[i]);
    }
    return result;
}

// Find: return first element matching predicate
int wyn_array_find(int* arr, int len, int (*pred)(int), int* found) {
    for (int i = 0; i < len; i++) {
        if (pred(arr[i])) {
            *found = 1;
            return arr[i];
        }
    }
    *found = 0;
    return 0;
}

// Any: check if any element matches predicate
int wyn_array_any(int* arr, int len, int (*pred)(int)) {
    for (int i = 0; i < len; i++) {
        if (pred(arr[i])) return 1;
    }
    return 0;
}

// All: check if all elements match predicate
int wyn_array_all(int* arr, int len, int (*pred)(int)) {
    for (int i = 0; i < len; i++) {
        if (!pred(arr[i])) return 0;
    }
    return 1;
}

// Reverse: reverse array in place
void wyn_array_reverse(int* arr, int len) {
    for (int i = 0; i < len / 2; i++) {
        int temp = arr[i];
        arr[i] = arr[len - 1 - i];
        arr[len - 1 - i] = temp;
    }
}

// Sort: simple bubble sort
void wyn_array_sort(int* arr, int len) {
    for (int i = 0; i < len - 1; i++) {
        for (int j = 0; j < len - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

// Contains: check if array contains value
int wyn_array_contains(int* arr, int len, int value) {
    for (int i = 0; i < len; i++) {
        if (arr[i] == value) return 1;
    }
    return 0;
}

// Index of: find index of value
int wyn_array_index_of(int* arr, int len, int value) {
    for (int i = 0; i < len; i++) {
        if (arr[i] == value) return i;
    }
    return -1;
}
