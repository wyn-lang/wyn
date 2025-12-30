#include <stdlib.h>
#include <string.h>

// StringBuilder structure: [chunk_count, capacity, chunk0_ptr, chunk1_ptr, ...]
// First 8 bytes: chunk_count
// Second 8 bytes: capacity
// Rest: pointers to string chunks

typedef struct {
    long long chunk_count;
    long long capacity;
    char** chunks;
} StringBuilder;

// Create new StringBuilder
StringBuilder* sb_new() {
    StringBuilder* sb = (StringBuilder*)malloc(sizeof(StringBuilder));
    sb->chunk_count = 0;
    sb->capacity = 8;
    sb->chunks = (char**)malloc(sb->capacity * sizeof(char*));
    return sb;
}

// Append string to StringBuilder
void sb_append(StringBuilder* sb, const char* str) {
    if (!sb || !str) return;
    
    // Grow if needed
    if (sb->chunk_count >= sb->capacity) {
        sb->capacity *= 2;
        sb->chunks = (char**)realloc(sb->chunks, sb->capacity * sizeof(char*));
    }
    
    // Store copy of string
    sb->chunks[sb->chunk_count] = strdup(str);
    sb->chunk_count++;
}

// Convert StringBuilder to string
char* sb_to_str(StringBuilder* sb) {
    if (!sb || sb->chunk_count == 0) return strdup("");
    
    // Calculate total length
    long long total_len = 0;
    for (long long i = 0; i < sb->chunk_count; i++) {
        total_len += strlen(sb->chunks[i]);
    }
    
    // Allocate result string
    char* result = (char*)malloc(total_len + 1);
    result[0] = '\0';
    
    // Concatenate all chunks
    for (long long i = 0; i < sb->chunk_count; i++) {
        strcat(result, sb->chunks[i]);
    }
    
    return result;
}

// Free StringBuilder
void sb_free(StringBuilder* sb) {
    if (!sb) return;
    
    for (long long i = 0; i < sb->chunk_count; i++) {
        free(sb->chunks[i]);
    }
    free(sb->chunks);
    free(sb);
}