#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// String module - essential string manipulation functions

// Length: get string length
int wyn_string_len(const char* str) {
    return strlen(str);
}

// Contains: check if string contains substring
int wyn_string_contains(const char* str, const char* substr) {
    return strstr(str, substr) != NULL;
}

// Starts with: check if string starts with prefix
int wyn_string_starts_with(const char* str, const char* prefix) {
    size_t len = strlen(prefix);
    return strncmp(str, prefix, len) == 0;
}

// Ends with: check if string ends with suffix
int wyn_string_ends_with(const char* str, const char* suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return 0;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

// To upper: convert string to uppercase
char* wyn_string_to_upper(const char* str) {
    char* result = malloc(strlen(str) + 1);
    for (int i = 0; str[i]; i++) {
        result[i] = toupper(str[i]);
    }
    result[strlen(str)] = '\0';
    return result;
}

// To lower: convert string to lowercase
char* wyn_string_to_lower(const char* str) {
    char* result = malloc(strlen(str) + 1);
    for (int i = 0; str[i]; i++) {
        result[i] = tolower(str[i]);
    }
    result[strlen(str)] = '\0';
    return result;
}

// Trim: remove leading/trailing whitespace
char* wyn_string_trim(const char* str) {
    while (isspace(*str)) str++;
    if (*str == 0) return strdup("");
    
    const char* end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    
    size_t len = end - str + 1;
    char* result = malloc(len + 1);
    memcpy(result, str, len);
    result[len] = '\0';
    return result;
}
