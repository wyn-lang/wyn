// Wyn Runtime Library - Builtin Functions
// This file provides the runtime implementations of builtin functions
// that are called from compiled Wyn programs.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdint.h>

// Assert function
int64_t assert(int64_t condition) {
    if (!condition) {
        fprintf(stderr, "Assertion failed\n");
        exit(1);
    }
    return 0;
}

// Assert function with message
int64_t assert_msg(int64_t condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "Assertion failed: %s\n", message);
        exit(1);
    }
    return 0;
}

// Note: print_str, print_newline, print_int, print_float are generated inline
// by the compiler's codegen, so we don't define them here.

// File I/O functions
char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return strdup("");
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* buf = malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    
    return buf;
}

int write_file(const char* path, const char* content) {
    FILE* f = fopen(path, "wb");
    if (!f) return 0;
    
    fwrite(content, 1, strlen(content), f);
    fclose(f);
    return 1;
}

int append_file(const char* path, const char* content) {
    FILE* f = fopen(path, "ab");
    if (!f) return 0;
    
    fwrite(content, 1, strlen(content), f);
    fclose(f);
    return 1;
}

int delete_file(const char* path) {
    return remove(path) == 0 ? 1 : 0;
}

int file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 ? 1 : 0;
}

long long file_size(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return st.st_size;
}

int is_dir(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode) ? 1 : 0;
}

int mkdir_wyn(const char* path) {
    return mkdir(path, 0755) == 0 ? 1 : 0;
}

int rmdir_wyn(const char* path) {
    return rmdir(path) == 0 ? 1 : 0;
}

// List directory contents
char** list_dir(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) {
        char** empty = malloc(sizeof(char*));
        empty[0] = NULL;
        return empty;
    }
    
    // Count entries first
    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }
    
    // Allocate array
    char** result = malloc(sizeof(char*) * (count + 1));
    
    // Read entries again
    rewinddir(dir);
    int i = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            result[i++] = strdup(entry->d_name);
        }
    }
    result[i] = NULL;
    
    closedir(dir);
    return result;
}

// Time functions
long long time_now() {
    return (long long)time(NULL);
}

void sleep_ms(long long ms) {
    usleep(ms * 1000);
}

long long clock_wyn() {
    return (long long)clock();
}

long long random_wyn() {
    return (long long)rand();
}

long long clock_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// String functions
long long str_len(const char* s) {
    return strlen(s);
}

// Stub implementations - these need proper implementation
char** str_split(const char* s, const char* delim) {
    // TODO: Implement properly
    char** result = malloc(sizeof(char*) * 2);
    result[0] = strdup(s);
    result[1] = NULL;
    return result;
}

long long str_find(const char* haystack, const char* needle) {
    const char* pos = strstr(haystack, needle);
    if (!pos) return -1;
    return pos - haystack;
}

char* str_concat(const char* a, const char* b) {
    size_t len = strlen(a) + strlen(b) + 1;
    char* result = malloc(len);
    strcpy(result, a);
    strcat(result, b);
    return result;
}

long long str_cmp(const char* a, const char* b) {
    return strcmp(a, b);
}

char* substring(const char* s, long long start, long long end) {
    long long len = end - start;
    if (len < 0) len = 0;
    char* result = malloc(len + 1);
    strncpy(result, s + start, len);
    result[len] = '\0';
    return result;
}

char* char_at(const char* s, long long index) {
    char* result = malloc(2);
    result[0] = s[index];
    result[1] = '\0';
    return result;
}

char* chr(long long code) {
    char* result = malloc(2);
    result[0] = (char)code;
    result[1] = '\0';
    return result;
}

long long ord(const char* s) {
    return (long long)(unsigned char)s[0];
}

char* int_to_str(long long n) {
    char* result = malloc(32);
    snprintf(result, 32, "%lld", n);
    return result;
}

long long str_to_int(const char* s) {
    return atoll(s);
}

// OS functions
char* getenv_wyn(const char* name) {
    char* val = getenv(name);
    return val ? strdup(val) : strdup("");
}

// Read a line from stdin
char* read_line() {
    char* line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, stdin);
    if (read == -1) {
        if (line) free(line);
        return strdup("");
    }
    // Remove trailing newline
    if (read > 0 && line[read-1] == '\n') {
        line[read-1] = '\0';
    }
    return line;
}

long long getpid_wyn() {
    return (long long)getpid();
}

void exit_wyn(long long code) {
    exit((int)code);
}

char* getcwd_wyn() {
    char buf[1024];
    return getcwd(buf, sizeof(buf)) ? strdup(buf) : strdup("");
}

// Math functions (these are provided by libm, just declare them)
extern double sinf(double);
extern double cosf(double);
extern double tanf(double);
extern double asinf(double);
extern double acosf(double);
extern double atanf(double);
extern double atan2f(double, double);
extern double floorf(double);
extern double ceilf(double);
extern double roundf(double);
extern double logf(double);
extern double log10f(double);
extern double expf(double);
extern double sqrtf(double);
extern double powf(double, double);

long long abs_int(long long n) {
    return n < 0 ? -n : n;
}

long long min_int(long long a, long long b) {
    return a < b ? a : b;
}

long long max_int(long long a, long long b) {
    return a > b ? a : b;
}

// Array contains - check if element is in array
int64_t array_contains(int64_t* arr, int64_t elem) {
    if (!arr) return 0;
    int64_t len = arr[0];  // Length is at index 0
    for (int64_t i = 0; i < len; i++) {
        if (arr[i + 2] == elem) return 1;  // Elements start at index 2
    }
    return 0;
}
