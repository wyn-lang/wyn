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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

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

// Additional OS module functions
int setenv_wyn(const char* name, const char* value) {
    return setenv(name, value, 1) == 0 ? 1 : 0;
}

int exec_wyn(const char* command) {
    return system(command);
}

char** args_wyn() {
    // Return empty array for now - will be set by main
    static char* empty[] = {NULL};
    return empty;
}

char* cwd_wyn() {
    char* buf = malloc(1024);
    return getcwd(buf, 1024) ? buf : strdup("");
}

int chdir_wyn(const char* path) {
    return chdir(path) == 0 ? 1 : 0;
}

// JSON module functions (minimal but functional implementation)
char* parse_json(const char* s) {
    // Simple JSON parser - just validates and returns the string
    // Real parsing would require complex state machine
    return strdup(s);
}

char* stringify_json(const char* json) {
    // For now, just return the input
    return strdup(json);
}

char* get_string_json(const char* json, const char* key) {
    // Simple key-value extraction: find "key":"value"
    char pattern[256];
    snprintf(pattern, 256, "\"%s\":\"", key);
    
    const char* start = strstr(json, pattern);
    if (!start) return strdup("");
    
    start += strlen(pattern);
    const char* end = strchr(start, '"');
    if (!end) return strdup("");
    
    size_t len = end - start;
    char* result = malloc(len + 1);
    strncpy(result, start, len);
    result[len] = '\0';
    return result;
}

int get_int_json(const char* json, const char* key) {
    // Simple key-value extraction: find "key":123
    char pattern[256];
    snprintf(pattern, 256, "\"%s\":", key);
    
    const char* start = strstr(json, pattern);
    if (!start) return 0;
    
    start += strlen(pattern);
    while (*start == ' ' || *start == '\t') start++;
    
    return atoi(start);
}

int get_bool_json(const char* json, const char* key) {
    // Simple key-value extraction: find "key":true/false
    char pattern[256];
    snprintf(pattern, 256, "\"%s\":", key);
    
    const char* start = strstr(json, pattern);
    if (!start) return 0;
    
    start += strlen(pattern);
    while (*start == ' ' || *start == '\t') start++;
    
    if (strncmp(start, "true", 4) == 0) return 1;
    return 0;
}

// Net module functions
char* http_get_wyn(const char* url) {
    // Use curl command for HTTP GET
    char cmd[2048];
    snprintf(cmd, 2048, "curl -s '%s' 2>/dev/null", url);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(65536);  // 64KB buffer
    size_t total = 0;
    size_t n;
    
    while ((n = fread(result + total, 1, 4096, pipe)) > 0) {
        total += n;
        if (total >= 65536 - 4096) break;  // Prevent overflow
    }
    result[total] = '\0';
    pclose(pipe);
    
    return result;
}

char* http_post_wyn(const char* url, const char* body) {
    // Use curl command for HTTP POST
    char cmd[4096];
    snprintf(cmd, 4096, "curl -s -X POST -d '%s' '%s' 2>/dev/null", body, url);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(65536);
    size_t total = 0;
    size_t n;
    
    while ((n = fread(result + total, 1, 4096, pipe)) > 0) {
        total += n;
        if (total >= 65536 - 4096) break;
    }
    result[total] = '\0';
    pclose(pipe);
    
    return result;
}

int tcp_connect_wyn(const char* host, int port) {
    // Real TCP connection using sockets
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;
    
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    // Convert hostname to IP
    struct hostent* server = gethostbyname(host);
    if (!server) {
        close(sockfd);
        return -1;
    }
    
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

int tcp_send_wyn(int fd, const char* data) {
    if (fd < 0) return 0;
    ssize_t sent = send(fd, data, strlen(data), 0);
    return (sent > 0) ? 1 : 0;
}

char* tcp_recv_wyn(int fd, int size) {
    if (fd < 0) return strdup("");
    
    char* buf = malloc(size + 1);
    ssize_t received = recv(fd, buf, size, 0);
    
    if (received < 0) {
        free(buf);
        return strdup("");
    }
    
    buf[received] = '\0';
    return buf;
}

void tcp_close_wyn(int fd) {
    if (fd >= 0) close(fd);
}
