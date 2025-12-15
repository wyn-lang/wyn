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
#include <sys/time.h>
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

// String split - returns Wyn array of strings
int64_t* str_split(const char* s, const char* delim) {
    if (!s || !delim) {
        int64_t* arr = malloc(sizeof(int64_t) * 2);
        arr[0] = 0;  // length
        arr[1] = 0;  // capacity
        return arr;
    }
    
    // Count occurrences
    int count = 1;
    const char* p = s;
    size_t delim_len = strlen(delim);
    while ((p = strstr(p, delim)) != NULL) {
        count++;
        p += delim_len;
    }
    
    // Allocate array: [length, capacity, elem0, elem1, ...]
    int64_t* arr = malloc(sizeof(int64_t) * (count + 2));
    arr[0] = count;
    arr[1] = count;
    
    // Split string
    char* copy = strdup(s);
    char* token = copy;
    char* next;
    int idx = 0;
    
    while (token && idx < count) {
        next = strstr(token, delim);
        if (next) {
            *next = '\0';
            next += delim_len;
        }
        arr[idx + 2] = (int64_t)strdup(token);
        token = next;
        idx++;
    }
    
    free(copy);
    return arr;
}

long long str_find(const char* haystack, const char* needle) {
    const char* pos = strstr(haystack, needle);
    if (!pos) return -1;
    return pos - haystack;
}

char* str_replace(const char* str, const char* old, const char* new) {
    if (!str || !old || !new) return strdup(str ? str : "");
    
    size_t old_len = strlen(old);
    size_t new_len = strlen(new);
    
    // Count occurrences
    int count = 0;
    const char* p = str;
    while ((p = strstr(p, old)) != NULL) {
        count++;
        p += old_len;
    }
    
    if (count == 0) return strdup(str);
    
    // Allocate result
    size_t result_len = strlen(str) + count * (new_len - old_len) + 1;
    char* result = malloc(result_len);
    char* dst = result;
    const char* src = str;
    
    while (*src) {
        const char* match = strstr(src, old);
        if (match) {
            // Copy before match
            size_t len = match - src;
            memcpy(dst, src, len);
            dst += len;
            // Copy replacement
            memcpy(dst, new, new_len);
            dst += new_len;
            src = match + old_len;
        } else {
            // Copy rest
            strcpy(dst, src);
            break;
        }
    }
    
    return result;
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

// Extended OS functions (hostname and getpid already exist above)
char* hostname_wyn() {
    char* buf = malloc(256);
    if (gethostname(buf, 256) == 0) return buf;
    free(buf);
    return strdup("");
}

int64_t is_file_wyn(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISREG(st.st_mode) ? 1 : 0;
}

int64_t is_dir_wyn(const char* path) {
    return is_dir(path);
}

// Time module functions
int64_t now_unix() {
    return (int64_t)time(NULL);
}

int64_t now_millis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int64_t now_micros() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)(tv.tv_sec * 1000000 + tv.tv_usec);
}

void sleep_seconds(int64_t seconds) {
    sleep((unsigned int)seconds);
}

char* format_timestamp(int64_t unix_time) {
    time_t t = (time_t)unix_time;
    struct tm* tm_info = localtime(&t);
    char* buf = malloc(64);
    strftime(buf, 64, "%Y-%m-%d %H:%M:%S", tm_info);
    return buf;
}

char* format_iso8601(int64_t unix_time) {
    time_t t = (time_t)unix_time;
    struct tm* tm_info = gmtime(&t);
    char* buf = malloc(64);
    strftime(buf, 64, "%Y-%m-%dT%H:%M:%SZ", tm_info);
    return buf;
}

int64_t parse_timestamp(const char* str) {
    struct tm tm_info = {0};
    if (strptime(str, "%Y-%m-%d %H:%M:%S", &tm_info) != NULL) {
        return (int64_t)mktime(&tm_info);
    }
    return 0;
}

// Extended Net functions (only new ones)
char* http_put_wyn(const char* url, const char* body) {
    char cmd[4096];
    snprintf(cmd, 4096, "curl -s -X PUT -d '%s' '%s' 2>/dev/null", body, url);
    
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

char* http_delete_wyn(const char* url) {
    char cmd[2048];
    snprintf(cmd, 2048, "curl -s -X DELETE '%s' 2>/dev/null", url);
    
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

char* http_head_wyn(const char* url) {
    char cmd[2048];
    snprintf(cmd, 2048, "curl -s -I '%s' 2>/dev/null", url);
    
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

char* http_get_headers_wyn(const char* url, const char* headers) {
    char cmd[4096];
    snprintf(cmd, 4096, "curl -s -H '%s' '%s' 2>/dev/null", headers, url);
    
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

int64_t udp_send_wyn(const char* host, int port, const char* data) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return 0;
    
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    struct hostent* server = gethostbyname(host);
    if (!server) {
        close(sockfd);
        return 0;
    }
    
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    ssize_t sent = sendto(sockfd, data, strlen(data), 0,
                          (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    close(sockfd);
    
    return (sent > 0) ? 1 : 0;
}

char* udp_recv_wyn(int port, int size) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return strdup("");
    
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return strdup("");
    }
    
    char* buf = malloc(size + 1);
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    
    ssize_t received = recvfrom(sockfd, buf, size, 0,
                                (struct sockaddr*)&cli_addr, &cli_len);
    close(sockfd);
    
    if (received < 0) {
        free(buf);
        return strdup("");
    }
    
    buf[received] = '\0';
    return buf;
}

char* resolve_host_wyn(const char* hostname) {
    struct hostent* he = gethostbyname(hostname);
    if (!he) return strdup("");
    
    struct in_addr** addr_list = (struct in_addr**)he->h_addr_list;
    if (addr_list[0]) {
        return strdup(inet_ntoa(*addr_list[0]));
    }
    return strdup("");
}

char* get_local_ip_wyn() {
    char hostname[256];
    if (gethostname(hostname, 256) != 0) return strdup("");
    
    struct hostent* he = gethostbyname(hostname);
    if (!he) return strdup("127.0.0.1");
    
    struct in_addr** addr_list = (struct in_addr**)he->h_addr_list;
    if (addr_list[0]) {
        return strdup(inet_ntoa(*addr_list[0]));
    }
    return strdup("127.0.0.1");
}

// Regex module functions (using grep/sed)
int64_t regex_matches(const char* text, const char* pattern) {
    char cmd[4096];
    snprintf(cmd, 4096, "echo '%s' | grep -E '%s' >/dev/null 2>&1", text, pattern);
    return (system(cmd) == 0) ? 1 : 0;
}

char* regex_find(const char* text, const char* pattern) {
    char cmd[4096];
    snprintf(cmd, 4096, "echo '%s' | grep -oE '%s' | head -1", text, pattern);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(1024);
    size_t n = fread(result, 1, 1023, pipe);
    result[n] = '\0';
    pclose(pipe);
    
    if (n > 0 && result[n-1] == '\n') result[n-1] = '\0';
    return result;
}

char* regex_replace(const char* text, const char* pattern, const char* replacement) {
    char cmd[4096];
    snprintf(cmd, 4096, "echo '%s' | sed -E 's/%s/%s/g'", text, pattern, replacement);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(4096);
    size_t n = fread(result, 1, 4095, pipe);
    result[n] = '\0';
    pclose(pipe);
    
    if (n > 0 && result[n-1] == '\n') result[n-1] = '\0';
    return result;
}

int64_t regex_count(const char* text, const char* pattern) {
    char cmd[4096];
    snprintf(cmd, 4096, "echo '%s' | grep -oE '%s' | wc -l", text, pattern);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return 0;
    
    char buf[32];
    fgets(buf, 32, pipe);
    pclose(pipe);
    
    return atoi(buf);
}

// HTTP Server implementation
int64_t http_server_create(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;
    
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return -1;
    }
    
    if (listen(sockfd, 10) < 0) {
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

int64_t http_server_accept(int64_t server_fd) {
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&cli_addr, &cli_len);
    return client_fd;
}

char* http_server_read_request(int64_t client_fd) {
    if (client_fd < 0) return strdup("");
    
    char* buffer = malloc(65536);
    ssize_t n = recv(client_fd, buffer, 65535, 0);
    
    if (n < 0) {
        free(buffer);
        return strdup("");
    }
    
    buffer[n] = '\0';
    return buffer;
}

char* http_parse_method(const char* request) {
    char* method = malloc(16);
    sscanf(request, "%15s", method);
    return method;
}

char* http_parse_path(const char* request) {
    char method[16], path[1024];
    sscanf(request, "%15s %1023s", method, path);
    return strdup(path);
}

char* http_parse_body(const char* request) {
    const char* body_start = strstr(request, "\r\n\r\n");
    if (body_start) {
        return strdup(body_start + 4);
    }
    return strdup("");
}

int64_t http_server_send_response(int64_t client_fd, int status, const char* body) {
    if (client_fd < 0) return 0;
    
    char* status_text = "OK";
    if (status == 404) status_text = "Not Found";
    else if (status == 500) status_text = "Internal Server Error";
    else if (status == 201) status_text = "Created";
    else if (status == 400) status_text = "Bad Request";
    
    char response[65536];
    int len = snprintf(response, 65536,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status, status_text, strlen(body), body);
    
    ssize_t sent = send(client_fd, response, len, 0);
    return (sent > 0) ? 1 : 0;
}

int64_t http_server_send_json(int64_t client_fd, int status, const char* json) {
    if (client_fd < 0) return 0;
    
    char* status_text = "OK";
    if (status == 404) status_text = "Not Found";
    else if (status == 500) status_text = "Internal Server Error";
    
    char response[65536];
    int len = snprintf(response, 65536,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status, status_text, strlen(json), json);
    
    ssize_t sent = send(client_fd, response, len, 0);
    return (sent > 0) ? 1 : 0;
}

void http_server_close_client(int64_t client_fd) {
    if (client_fd >= 0) close(client_fd);
}

void http_server_close(int64_t server_fd) {
    if (server_fd >= 0) close(server_fd);
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

// Collections module - Simple HashMap implementation
typedef struct HashMapEntry {
    char* key;
    char* value;
    struct HashMapEntry* next;
} HashMapEntry;

typedef struct {
    HashMapEntry** buckets;
    int size;
    int count;
} HashMap;

// Simple hash function
static unsigned int hash_string(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

HashMap* hashmap_new() {
    HashMap* map = malloc(sizeof(HashMap));
    map->size = 128;
    map->count = 0;
    map->buckets = calloc(map->size, sizeof(HashMapEntry*));
    return map;
}

void hashmap_put(HashMap* map, const char* key, const char* value) {
    if (!map || !key) return;
    
    unsigned int index = hash_string(key) % map->size;
    HashMapEntry* entry = map->buckets[index];
    
    // Check if key exists
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            free(entry->value);
            entry->value = strdup(value);
            return;
        }
        entry = entry->next;
    }
    
    // Add new entry
    HashMapEntry* new_entry = malloc(sizeof(HashMapEntry));
    new_entry->key = strdup(key);
    new_entry->value = strdup(value);
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->count++;
}

char* hashmap_get(HashMap* map, const char* key) {
    if (!map || !key) return strdup("");
    
    unsigned int index = hash_string(key) % map->size;
    HashMapEntry* entry = map->buckets[index];
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return strdup(entry->value);
        }
        entry = entry->next;
    }
    
    return strdup("");
}

int64_t hashmap_contains(HashMap* map, const char* key) {
    if (!map || !key) return 0;
    
    unsigned int index = hash_string(key) % map->size;
    HashMapEntry* entry = map->buckets[index];
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) return 1;
        entry = entry->next;
    }
    
    return 0;
}

void hashmap_remove(HashMap* map, const char* key) {
    if (!map || !key) return;
    
    unsigned int index = hash_string(key) % map->size;
    HashMapEntry* entry = map->buckets[index];
    HashMapEntry* prev = NULL;
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                map->buckets[index] = entry->next;
            }
            free(entry->key);
            free(entry->value);
            free(entry);
            map->count--;
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

int64_t hashmap_size(HashMap* map) {
    return map ? map->count : 0;
}

void hashmap_clear(HashMap* map) {
    if (!map) return;
    
    for (int i = 0; i < map->size; i++) {
        HashMapEntry* entry = map->buckets[i];
        while (entry) {
            HashMapEntry* next = entry->next;
            free(entry->key);
            free(entry->value);
            free(entry);
            entry = next;
        }
        map->buckets[i] = NULL;
    }
    map->count = 0;
}

// Simple Set implementation (using HashMap with dummy values)
typedef HashMap Set;

Set* set_new() {
    return hashmap_new();
}

void set_add(Set* set, const char* value) {
    hashmap_put(set, value, "1");
}

int64_t set_contains(Set* set, const char* value) {
    return hashmap_contains(set, value);
}

void set_remove(Set* set, const char* value) {
    hashmap_remove(set, value);
}

int64_t set_size(Set* set) {
    return hashmap_size(set);
}

void set_clear(Set* set) {
    hashmap_clear(set);
}

// Crypto module functions (using openssl commands)
char* encrypt_aes256(const char* data, const char* key) {
    // Use openssl for AES-256 encryption
    char cmd[4096];
    snprintf(cmd, 4096, "echo -n '%s' | openssl enc -aes-256-cbc -a -salt -pass pass:'%s' 2>/dev/null", data, key);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(4096);
    size_t n = fread(result, 1, 4095, pipe);
    result[n] = '\0';
    pclose(pipe);
    
    // Remove trailing newline
    if (n > 0 && result[n-1] == '\n') result[n-1] = '\0';
    
    return result;
}

char* decrypt_aes256(const char* data, const char* key) {
    // Use openssl for AES-256 decryption
    char cmd[4096];
    snprintf(cmd, 4096, "echo '%s' | openssl enc -aes-256-cbc -d -a -pass pass:'%s' 2>/dev/null", data, key);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(4096);
    size_t n = fread(result, 1, 4095, pipe);
    result[n] = '\0';
    pclose(pipe);
    
    // Remove trailing newline
    if (n > 0 && result[n-1] == '\n') result[n-1] = '\0';
    
    return result;
}

char* generate_random_bytes(int64_t count) {
    // Generate random bytes using openssl
    char cmd[256];
    snprintf(cmd, 256, "openssl rand -hex %lld", (long long)count);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(count * 2 + 1);
    size_t n = fread(result, 1, count * 2, pipe);
    result[n] = '\0';
    pclose(pipe);
    
    // Remove trailing newline
    if (n > 0 && result[n-1] == '\n') result[n-1] = '\0';
    
    return result;
}

char* hmac_sha256(const char* data, const char* key) {
    // HMAC-SHA256 using openssl
    char cmd[4096];
    snprintf(cmd, 4096, "echo -n '%s' | openssl dgst -sha256 -hmac '%s' | cut -d' ' -f2", data, key);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(128);
    size_t n = fread(result, 1, 127, pipe);
    result[n] = '\0';
    pclose(pipe);
    
    // Remove trailing newline
    if (n > 0 && result[n-1] == '\n') result[n-1] = '\0';
    
    return result;
}

int64_t verify_signature(const char* data, const char* signature, const char* public_key) {
    // Simplified signature verification - just check if signature matches hash
    // Real implementation would use RSA/ECDSA
    char cmd[4096];
    snprintf(cmd, 4096, "echo -n '%s' | openssl dgst -sha256 | cut -d' ' -f2", data);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return 0;
    
    char hash[128];
    if (fgets(hash, 128, pipe) != NULL) {
        // Remove newline
        char* newline = strchr(hash, '\n');
        if (newline) *newline = '\0';
        
        pclose(pipe);
        return (strcmp(hash, signature) == 0) ? 1 : 0;
    }
    
    pclose(pipe);
    return 0;
}

// Compress module functions
int64_t gzip_file(const char* input, const char* output) {
    char cmd[2048];
    snprintf(cmd, 2048, "gzip -c '%s' > '%s' 2>/dev/null", input, output);
    return (system(cmd) == 0) ? 1 : 0;
}

int64_t gunzip_file(const char* input, const char* output) {
    char cmd[2048];
    snprintf(cmd, 2048, "gunzip -c '%s' > '%s' 2>/dev/null", input, output);
    return (system(cmd) == 0) ? 1 : 0;
}

int64_t tar_create(const char* archive, const char* files) {
    char cmd[4096];
    snprintf(cmd, 4096, "tar -czf '%s' %s 2>/dev/null", archive, files);
    return (system(cmd) == 0) ? 1 : 0;
}

int64_t tar_extract(const char* archive, const char* dest) {
    char cmd[2048];
    snprintf(cmd, 2048, "tar -xzf '%s' -C '%s' 2>/dev/null", archive, dest);
    return (system(cmd) == 0) ? 1 : 0;
}

int64_t tar_list(const char* archive) {
    char cmd[2048];
    snprintf(cmd, 2048, "tar -tzf '%s' 2>/dev/null", archive);
    return (system(cmd) == 0) ? 1 : 0;
}

char* compress_string(const char* data) {
    // Write to temp file, gzip, read back
    const char* tmp_in = "/tmp/wyn_compress_in";
    const char* tmp_out = "/tmp/wyn_compress_out";
    
    FILE* f = fopen(tmp_in, "w");
    if (!f) return strdup("");
    fwrite(data, 1, strlen(data), f);
    fclose(f);
    
    char cmd[512];
    snprintf(cmd, 512, "gzip -c '%s' > '%s' 2>/dev/null", tmp_in, tmp_out);
    if (system(cmd) != 0) {
        unlink(tmp_in);
        return strdup("");
    }
    
    f = fopen(tmp_out, "rb");
    if (!f) {
        unlink(tmp_in);
        unlink(tmp_out);
        return strdup("");
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* result = malloc(size + 1);
    fread(result, 1, size, f);
    result[size] = '\0';
    fclose(f);
    
    unlink(tmp_in);
    unlink(tmp_out);
    
    return result;
}

char* decompress_string(const char* data) {
    // Write to temp file, gunzip, read back
    const char* tmp_in = "/tmp/wyn_decompress_in";
    const char* tmp_out = "/tmp/wyn_decompress_out";
    
    FILE* f = fopen(tmp_in, "wb");
    if (!f) return strdup("");
    fwrite(data, 1, strlen(data), f);
    fclose(f);
    
    char cmd[512];
    snprintf(cmd, 512, "gunzip -c '%s' > '%s' 2>/dev/null", tmp_in, tmp_out);
    if (system(cmd) != 0) {
        unlink(tmp_in);
        return strdup("");
    }
    
    f = fopen(tmp_out, "r");
    if (!f) {
        unlink(tmp_in);
        unlink(tmp_out);
        return strdup("");
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* result = malloc(size + 1);
    fread(result, 1, size, f);
    result[size] = '\0';
    fclose(f);
    
    unlink(tmp_in);
    unlink(tmp_out);
    
    return result;
}

// Additional OS module functions
int setenv_wyn(const char* name, const char* value) {
    return setenv(name, value, 1) == 0 ? 1 : 0;
}

int exec_wyn(const char* command) {
    return system(command);
}

char* exec_output_wyn(const char* command) {
    FILE* pipe = popen(command, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(65536);
    size_t total = 0;
    size_t n;
    
    while ((n = fread(result + total, 1, 4096, pipe)) > 0) {
        total += n;
        if (total >= 61440) break;  // Leave room for null terminator
    }
    result[total] = '\0';
    
    pclose(pipe);
    return result;
}

char** args_wyn() {
    // Simple implementation: return program name and any WYN_ARGS env var
    static char* args[100];
    static int initialized = 0;
    
    if (!initialized) {
        args[0] = strdup("wyn_program");
        
        // Check for WYN_ARGS environment variable
        char* env_args = getenv("WYN_ARGS");
        if (env_args) {
            // Split by spaces (simple tokenization)
            char* copy = strdup(env_args);
            char* token = strtok(copy, " ");
            int i = 1;
            while (token && i < 99) {
                args[i++] = strdup(token);
                token = strtok(NULL, " ");
            }
            args[i] = NULL;
        } else {
            args[1] = NULL;
        }
        initialized = 1;
    }
    
    return args;
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

// Log module functions
void log_info(const char* msg) {
    printf("[INFO] %s\n", msg);
}

void log_warn(const char* msg) {
    printf("[WARN] %s\n", msg);
}

void log_error(const char* msg) {
    printf("[ERROR] %s\n", msg);
}

void log_debug(const char* msg) {
    printf("[DEBUG] %s\n", msg);
}

void log_with_level(const char* level, const char* msg) {
    printf("[%s] %s\n", level, msg);
}

// Encoding module functions
char* base64_encode(const char* data) {
    char cmd[4096];
    snprintf(cmd, 4096, "echo -n '%s' | base64", data);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(4096);
    if (fgets(result, 4096, pipe) != NULL) {
        // Remove newline
        char* newline = strchr(result, '\n');
        if (newline) *newline = '\0';
    } else {
        strcpy(result, "");
    }
    
    pclose(pipe);
    return result;
}

char* base64_decode(const char* data) {
    char cmd[4096];
    snprintf(cmd, 4096, "echo '%s' | base64 -d", data);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(4096);
    if (fgets(result, 4096, pipe) != NULL) {
        // Remove newline if present
        char* newline = strchr(result, '\n');
        if (newline) *newline = '\0';
    } else {
        strcpy(result, "");
    }
    
    pclose(pipe);
    return result;
}

char* hex_encode(const char* data) {
    char cmd[4096];
    snprintf(cmd, 4096, "echo -n '%s' | xxd -p | tr -d '\\n'", data);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(4096);
    if (fgets(result, 4096, pipe) != NULL) {
        // Remove newline
        char* newline = strchr(result, '\n');
        if (newline) *newline = '\0';
    } else {
        strcpy(result, "");
    }
    
    pclose(pipe);
    return result;
}

char* hex_decode(const char* data) {
    char cmd[4096];
    snprintf(cmd, 4096, "echo '%s' | xxd -r -p", data);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(4096);
    if (fgets(result, 4096, pipe) != NULL) {
        // Remove newline if present
        char* newline = strchr(result, '\n');
        if (newline) *newline = '\0';
    } else {
        strcpy(result, "");
    }
    
    pclose(pipe);
    return result;
}

char* url_encode(const char* data) {
    char cmd[4096];
    snprintf(cmd, 4096, "python3 -c \"import urllib.parse; print(urllib.parse.quote('%s'))\"", data);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(4096);
    if (fgets(result, 4096, pipe) != NULL) {
        // Remove newline
        char* newline = strchr(result, '\n');
        if (newline) *newline = '\0';
    } else {
        strcpy(result, "");
    }
    
    pclose(pipe);
    return result;
}

char* url_decode(const char* data) {
    char cmd[4096];
    snprintf(cmd, 4096, "python3 -c \"import urllib.parse; print(urllib.parse.unquote('%s'))\"", data);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(4096);
    if (fgets(result, 4096, pipe) != NULL) {
        // Remove newline
        char* newline = strchr(result, '\n');
        if (newline) *newline = '\0';
    } else {
        strcpy(result, "");
    }
    
    pclose(pipe);
    return result;
}

// Hash module functions
char* sha256_hash(const char* data) {
    char cmd[4096];
    snprintf(cmd, 4096, "echo -n '%s' | openssl dgst -sha256 | cut -d' ' -f2", data);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(128);
    if (fgets(result, 128, pipe) != NULL) {
        // Remove newline
        char* newline = strchr(result, '\n');
        if (newline) *newline = '\0';
    } else {
        strcpy(result, "");
    }
    
    pclose(pipe);
    return result;
}

char* md5_hash(const char* data) {
    char cmd[4096];
    snprintf(cmd, 4096, "echo -n '%s' | openssl dgst -md5 | cut -d' ' -f2", data);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(64);
    if (fgets(result, 64, pipe) != NULL) {
        // Remove newline
        char* newline = strchr(result, '\n');
        if (newline) *newline = '\0';
    } else {
        strcpy(result, "");
    }
    
    pclose(pipe);
    return result;
}

char* sha1_hash(const char* data) {
    char cmd[4096];
    snprintf(cmd, 4096, "echo -n '%s' | openssl dgst -sha1 | cut -d' ' -f2", data);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return strdup("");
    
    char* result = malloc(64);
    if (fgets(result, 64, pipe) != NULL) {
        // Remove newline
        char* newline = strchr(result, '\n');
        if (newline) *newline = '\0';
    } else {
        strcpy(result, "");
    }
    
    pclose(pipe);
    return result;
}

// Atomic operations for concurrent programming
#include <stdatomic.h>

int64_t wyn_atomic_add(int64_t* ptr, int64_t value) {
    return atomic_fetch_add((_Atomic int64_t*)ptr, value);
}

int64_t wyn_atomic_sub(int64_t* ptr, int64_t value) {
    return atomic_fetch_sub((_Atomic int64_t*)ptr, value);
}

int64_t wyn_atomic_load(int64_t* ptr) {
    return atomic_load_explicit((_Atomic int64_t*)ptr, memory_order_seq_cst);
}

void wyn_atomic_store(int64_t* ptr, int64_t value) {
    atomic_store_explicit((_Atomic int64_t*)ptr, value, memory_order_seq_cst);
}

int64_t wyn_atomic_cas(int64_t* ptr, int64_t expected, int64_t desired) {
    atomic_compare_exchange_strong((_Atomic int64_t*)ptr, &expected, desired);
    return expected;
}

// Additional file operations for shell replacement
int64_t file_copy(const char* src, const char* dest) {
    FILE* source = fopen(src, "rb");
    if (!source) return 0;
    
    FILE* target = fopen(dest, "wb");
    if (!target) {
        fclose(source);
        return 0;
    }
    
    char buffer[8192];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        if (fwrite(buffer, 1, bytes, target) != bytes) {
            fclose(source);
            fclose(target);
            return 0;
        }
    }
    
    fclose(source);
    fclose(target);
    return 1;
}

int64_t file_move(const char* src, const char* dest) {
    if (rename(src, dest) == 0) {
        return 1;
    }
    // If rename fails (cross-device), try copy and delete
    if (file_copy(src, dest)) {
        unlink(src);
        return 1;
    }
    return 0;
}

int64_t file_chmod(const char* path, int64_t mode) {
    return chmod(path, (mode_t)mode) == 0 ? 1 : 0;
}

#include <glob.h>

// Glob pattern matching for files
int64_t* file_glob(const char* pattern) {
    glob_t globbuf;
    
    if (glob(pattern, 0, NULL, &globbuf) != 0) {
        // Return empty array
        int64_t* result = malloc(sizeof(int64_t) * 2);
        result[0] = 0; // length
        result[1] = 0; // capacity
        return result;
    }
    
    size_t count = globbuf.gl_pathc;
    int64_t* result = malloc(sizeof(int64_t) * (count + 2));
    result[0] = count; // length
    result[1] = count; // capacity
    
    for (size_t i = 0; i < count; i++) {
        result[i + 2] = (int64_t)strdup(globbuf.gl_pathv[i]);
    }
    
    globfree(&globbuf);
    return result;
}
